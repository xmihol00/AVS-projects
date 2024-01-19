/**
 * @file    tree_mesh_builder.h
 *
 * @author  David Mihola <xmihol00@stud.fit.vutbr.cz>
 *
 * @brief   Parallel Marching Cubes implementation using OpenMP tasks + octree early elimination
 *
 * @date    15. 12. 2023
 **/

#ifndef TREE_MESH_BUILDER_H
#define TREE_MESH_BUILDER_H

#include "base_mesh_builder.h"

class TreeMeshBuilder : public BaseMeshBuilder
{
public:
    TreeMeshBuilder(unsigned gridEdgeSize);
    ~TreeMeshBuilder();

protected:
    unsigned marchCubes(const ParametricScalarField &field);
    float evaluateFieldAt(const Vec3_t<float> &pos, const ParametricScalarField &field);
    void emitTriangle(const Triangle_t &triangle);
    const Triangle_t *getTrianglesArray() const;

private:
    const float SQRT3DIV2 = 0.86602540378;
    unsigned CUT_OFF;

    unsigned _fieldSize = 0;
    ParametricScalarField const *_field = nullptr;
    std::vector<Triangle_t> *_perThreadTriangles;
    std::vector<Triangle_t> _sharedTriangles;
    unsigned _sharedTrianglesIndex = 0;
    Triangle_t *_trianglesMerged = nullptr;
    float *_field_x = nullptr;
    float *_field_y = nullptr;
    float *_field_z = nullptr;
    unsigned totalNumberOfTriangles = 0;
    int _numThreads = 0;

    unsigned traverseOctree(unsigned x, unsigned y, unsigned z, unsigned cubeSize);
};

#endif // TREE_MESH_BUILDER_H
