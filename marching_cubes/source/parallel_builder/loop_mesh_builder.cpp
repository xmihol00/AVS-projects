/**
 * @file    loop_mesh_builder.cpp
 *
 * @author  David Mihola <xmihol00@stud.fit.vutbr.cz>
 *
 * @brief   Parallel Marching Cubes implementation using OpenMP loops
 *
 * @date    DATE
 **/

#include <iostream>
#include <math.h>
#include <limits>
#include <omp.h>
#include <cstring>

#include "loop_mesh_builder.h"

using namespace std;

#if 1 // override compiler defines
    #undef PRINT_DEFINES
    #undef CAPTURE_AND_ADD
    #undef SEPARATE_MEMORY
    #undef VECTORIZED_SIMD
    #undef SCHEDULE
    #undef CHUNK_SIZE

    #define SEPARATE_MEMORY
    #define VECTORIZED_SIMD
    #define SCHEDULE guided
    #define CHUNK_SIZE 32
#endif

LoopMeshBuilder::LoopMeshBuilder(unsigned gridEdgeSize)
    : BaseMeshBuilder(gridEdgeSize, "OpenMP Loop")
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
    #ifdef SCHEDULE
        cerr << "SCHEDULE defined" << endl;
    #else
        cerr << "SCHEDULE not defined" << endl;
    #endif
    #ifdef CHUNK_SIZE
        cerr << "CHUNK_SIZE defined: " << CHUNK_SIZE << endl;
    #else
        cerr << "CHUNK_SIZE not defined" << endl;
    #endif
#endif

    unsigned maxNumberOfTriangles = gridEdgeSize * gridEdgeSize * gridEdgeSize * 5;
#if !defined(CAPTURE_AND_ADD) and !defined(SEPARATE_MEMORY) // default is to use critical section
    _sharedTriangles.reserve(maxNumberOfTriangles);
#else
    #ifdef SEPARATE_MEMORY
        _numThreads = omp_get_max_threads();
        _perThreadTriangles = new vector<Triangle_t>[_numThreads]; // allocate vector for each thread
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

unsigned LoopMeshBuilder::marchCubes(const ParametricScalarField &field)
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
#endif

    size_t totalCubesCount = mGridSize * mGridSize * mGridSize;
    unsigned totalNumberOfTriangles = 0;

    #pragma omp parallel shared(totalNumberOfTriangles)
    {
    #ifdef SCHEDULE
        #ifdef CHUNK_SIZE
            #pragma omp for reduction(+:totalNumberOfTriangles) schedule(SCHEDULE, CHUNK_SIZE)
        #else 
            #pragma omp for reduction(+:totalNumberOfTriangles) schedule(SCHEDULE)
        #endif
    #else 
        #pragma omp for reduction(+:totalNumberOfTriangles)
    #endif
        for(size_t i = 0; i < totalCubesCount; ++i)
        {
            Vec3_t<float> cubeOffset(i % mGridSize, (i / mGridSize) % mGridSize, i / (mGridSize*mGridSize));
            totalNumberOfTriangles += buildCube(cubeOffset, field);  
        }

    #ifdef SEPARATE_MEMORY // merge the triangles from all threads into one array
        #pragma omp master
        {
            _trianglesMerged = new Triangle_t[totalNumberOfTriangles];
            unsigned offset = 0;
            for (int i = 0; i < _numThreads; i++)
            {
                #pragma omp task firstprivate(offset, i) // use task so that the copying can be done in parallel
                memcpy(_trianglesMerged + offset, _perThreadTriangles[i].data(), _perThreadTriangles[i].size() * sizeof(Triangle_t)); 

                offset += _perThreadTriangles[i].size();
            }
        }

        #pragma omp taskwait
    #endif
    }

    return totalNumberOfTriangles;
}

float LoopMeshBuilder::evaluateFieldAt(const Vec3_t<float> &pos, const ParametricScalarField &field)
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

void LoopMeshBuilder::emitTriangle(const BaseMeshBuilder::Triangle_t &triangle)
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

const LoopMeshBuilder::Triangle_t *LoopMeshBuilder::getTrianglesArray() const
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
