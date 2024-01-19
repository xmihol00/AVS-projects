/**
 * @file    tree_mesh_builder.cpp
 *
 * @author  David Mihola <xmihol00@stud.fit.vutbr.cz>
 *
 * @brief   Parallel Marching Cubes implementation using OpenMP tasks + octree early elimination
 *
 * @date    15. 12. 2023
 **/

#include <iostream>
#include <math.h>
#include <limits>
#include <omp.h>
#include <cstring>

#include "tree_mesh_builder.h"

using namespace std;

#if 1 // override compiler defines
    #undef PRINT_DEFINES
    #undef CAPTURE_AND_ADD
    #undef SEPARATE_MEMORY
    #undef VECTORIZED_SIMD
    #undef CUT_OFF_VALUE

    #define SEPARATE_MEMORY
    #define VECTORIZED_SIMD
    #define CUT_OFF_VALUE 8
#endif

TreeMeshBuilder::TreeMeshBuilder(unsigned gridEdgeSize): BaseMeshBuilder(gridEdgeSize, "Octree")
{
#ifdef PRINT_DEFINES    
    #ifdef SEPARATE_MEMORY
        cerr << "SEPARATE_MEMORY defined" << endl;
    #else
        cerr << "SEPARATE_MEMORY not defined" << endl;
    #endif
    #ifdef CAPTURE_AND_ADD
        cerr << "CAPTURE_AND_ADD defined" << endl;
    #else
        cerr << "CAPTURE_AND_ADD not defined" << endl;
    #endif
    #ifdef VECTORIZED_SIMD
        cerr << "VECTORIZED_SIMD defined" << endl;
    #else
        cerr << "VECTORIZED_SIMD not defined" << endl;
    #endif
    #ifdef CUT_OFF_VALUE
        cerr << "CUT_OFF_VALUE defined: " << CUT_OFF_VALUE << endl;
    #else
        cerr << "CUT_OFF_VALUE not defined" << endl;
    #endif
#endif

    _numThreads = omp_get_max_threads();
    _numThreads = _numThreads == 0 ? 1 : _numThreads;

#ifdef CUT_OFF_VALUE 
    CUT_OFF = CUT_OFF_VALUE;
#else
    CUT_OFF = 1;
#endif

    unsigned maxNumberOfTriangles = gridEdgeSize * gridEdgeSize * gridEdgeSize * 5;
#if !defined(CAPTURE_AND_ADD) and !defined(SEPARATE_MEMORY) // default is to use critical section
    _sharedTriangles.reserve(maxNumberOfTriangles);
#else
    #ifdef SEPARATE_MEMORY
        _perThreadTriangles = new vector<Triangle_t>[_numThreads];
        for (int i = 0; i < _numThreads; i++)
        {
            _perThreadTriangles[i].reserve(maxNumberOfTriangles / _numThreads); // only reserve, so each thread can allocate to its closest RAM memory
        }
    #else
        #ifdef CAPTURE_AND_ADD
            _sharedTriangles.resize(maxNumberOfTriangles); // allocate vector, so it can be indexed
        #endif
    #endif
#endif
}

TreeMeshBuilder::~TreeMeshBuilder()
{
#ifdef VECTORIZED_SIMD
    delete[] _field_x;
    delete[] _field_y;
    delete[] _field_z;
#endif
#ifdef SEPARATE_MEMORY
    delete[] _perThreadTriangles;
    delete[] _trianglesMerged;
#endif
}

unsigned TreeMeshBuilder::traverseOctree(unsigned x, unsigned y, unsigned z, unsigned cubeSize)
{
    if (cubeSize == 1)
    {
        return buildCube(Vec3_t<float>(x, y, z), *_field);
    }
    else if (cubeSize <= CUT_OFF) // sequential octree traversal
    {
        unsigned numberOfTriangles = 0;
        float F = evaluateFieldAt(Vec3_t<float>(x * mGridResolution, y * mGridResolution, z * mGridResolution), *_field);
        if (F <= mIsoLevel + cubeSize * SQRT3DIV2)
        {
            cubeSize >>= 1;
            unsigned numberOfTriangles = 0;

            numberOfTriangles += traverseOctree(x, y, z, cubeSize);
            numberOfTriangles += traverseOctree(x + cubeSize, y, z, cubeSize);
            numberOfTriangles += traverseOctree(x, y + cubeSize, z, cubeSize);
            numberOfTriangles += traverseOctree(x, y, z + cubeSize, cubeSize);
            numberOfTriangles += traverseOctree(x + cubeSize, y + cubeSize, z, cubeSize);
            numberOfTriangles += traverseOctree(x, y + cubeSize, z + cubeSize, cubeSize);
            numberOfTriangles += traverseOctree(x + cubeSize, y, z + cubeSize, cubeSize);
            numberOfTriangles += traverseOctree(x + cubeSize, y + cubeSize, z + cubeSize, cubeSize);

            return numberOfTriangles;
        }
    }
    else // parallel octree traversal
    {
        float F = evaluateFieldAt(Vec3_t<float>(x * mGridResolution, y * mGridResolution, z * mGridResolution), *_field);
        if (F <= mIsoLevel + cubeSize * SQRT3DIV2)
        {
            cubeSize >>= 1;
            unsigned numberOfTriangles = 0;

            #pragma omp task shared(numberOfTriangles)
            #pragma omp atomic
            numberOfTriangles += traverseOctree(x, y, z, cubeSize);
            
            #pragma omp task shared(numberOfTriangles)
            #pragma omp atomic
            numberOfTriangles += traverseOctree(x + cubeSize, y, z, cubeSize);

            #pragma omp task shared(numberOfTriangles)
            #pragma omp atomic
            numberOfTriangles += traverseOctree(x, y + cubeSize, z, cubeSize);

            #pragma omp task shared(numberOfTriangles)
            #pragma omp atomic
            numberOfTriangles += traverseOctree(x, y, z + cubeSize, cubeSize);

            #pragma omp task shared(numberOfTriangles)
            #pragma omp atomic
            numberOfTriangles += traverseOctree(x + cubeSize, y + cubeSize, z, cubeSize);

            #pragma omp task shared(numberOfTriangles)
            #pragma omp atomic
            numberOfTriangles += traverseOctree(x, y + cubeSize, z + cubeSize, cubeSize);

            #pragma omp task shared(numberOfTriangles)
            #pragma omp atomic
            numberOfTriangles += traverseOctree(x + cubeSize, y, z + cubeSize, cubeSize);

            #pragma omp task shared(numberOfTriangles)
            #pragma omp atomic
            numberOfTriangles += traverseOctree(x + cubeSize, y + cubeSize, z + cubeSize, cubeSize);

            #pragma omp taskwait
            return numberOfTriangles;
        }
    }

    return 0;
}

unsigned TreeMeshBuilder::marchCubes(const ParametricScalarField &field)
{
    _fieldSize = unsigned(field.getPoints().size());

#ifdef VECTORIZED_SIMD
    const Vec3_t<float> *pPoints = field.getPoints().data();
    _field_x = (float *)aligned_alloc(64, _fieldSize * sizeof(float));
    _field_y = (float *)aligned_alloc(64, _fieldSize * sizeof(float));
    _field_z = (float *)aligned_alloc(64, _fieldSize * sizeof(float));   
    for (unsigned i = 0; i < _fieldSize; i++)
    {
        _field_x[i] = pPoints[i].x;
        _field_y[i] = pPoints[i].y;
        _field_z[i] = pPoints[i].z;
    }
#else // get a pointer to the field by default in order to avoid passing it to the octree traversal
    _field = &field;
#endif

    unsigned totalNumberOfTriangles = 0;
    #pragma omp parallel
    {
        #pragma omp master
        {
            totalNumberOfTriangles = traverseOctree(0, 0, 0, mGridSize);

            #ifdef SEPARATE_MEMORY // merge the triangles from all threads into one array
                _trianglesMerged = new Triangle_t[totalNumberOfTriangles];
                unsigned offset = 0;
                for (int i = 0; i < _numThreads; i++)
                {
                    #pragma omp task firstprivate(offset, i) // use task so that the copying can be done in parallel
                    memcpy(_trianglesMerged + offset, _perThreadTriangles[i].data(), _perThreadTriangles[i].size() * sizeof(Triangle_t));

                    offset += _perThreadTriangles[i].size();
                }
            #endif
        }
    #ifdef SEPARATE_MEMORY 
        #pragma omp taskwait
    #endif
    }

    return totalNumberOfTriangles;    
}

float TreeMeshBuilder::evaluateFieldAt(const Vec3_t<float> &pos, const ParametricScalarField &field)
{
    float value = numeric_limits<float>::max();
#ifdef VECTORIZED_SIMD
    float *pField_x = _field_x;
    float *pField_y = _field_y;
    float *pField_z = _field_z;
    #pragma omp simd reduction(min:value) simdlen(16) aligned(pField_x, pField_y, pField_z : 64)
    for(unsigned i = 0; i < _fieldSize; i++)
    {
        float distanceSquared  = (pos.x - pField_x[i]) * (pos.x - pField_x[i]);
        distanceSquared       += (pos.y - pField_y[i]) * (pos.y - pField_y[i]);
        distanceSquared       += (pos.z - pField_z[i]) * (pos.z - pField_z[i]);

        value = value < distanceSquared ? value : distanceSquared;
    }
#else
    const Vec3_t<float> *pPoints = field.getPoints().data();
    for(unsigned i = 0; i < _fieldSize; i++)
    {
        float distanceSquared  = (pos.x - pPoints[i].x) * (pos.x - pPoints[i].x);
        distanceSquared       += (pos.y - pPoints[i].y) * (pos.y - pPoints[i].y);
        distanceSquared       += (pos.z - pPoints[i].z) * (pos.z - pPoints[i].z);

        value = std::min(value, distanceSquared);
    }
#endif

    return sqrt(value);
}

void TreeMeshBuilder::emitTriangle(const BaseMeshBuilder::Triangle_t &triangle)
{
#if !defined(CAPTURE_AND_ADD) and !defined(SEPARATE_MEMORY) // default is to use critical section
    #pragma omp critical
    _sharedTriangles.push_back(triangle);
#else
    #ifdef SEPARATE_MEMORY
        _perThreadTriangles[omp_get_thread_num()].push_back(triangle);
    #else
        #ifdef CAPTURE_AND_ADD
            unsigned index;
            #pragma omp atomic capture
            {
                index = _sharedTrianglesIndex;
                _sharedTrianglesIndex++;
            }
            _sharedTriangles[index] = triangle;
        #endif
    #endif
#endif
}

const BaseMeshBuilder::Triangle_t *TreeMeshBuilder::getTrianglesArray() const
{
#if !defined(CAPTURE_AND_ADD) and !defined(SEPARATE_MEMORY) // default is to use critical section
    return _sharedTriangles.data();
#else
    #ifdef SEPARATE_MEMORY
        return _trianglesMerged;
    #endif
    #ifdef CAPTURE_AND_ADD
        return _sharedTriangles.data();
    #endif
#endif
}
