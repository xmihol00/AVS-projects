/**
 * @file    loop_mesh_builder.h
 *
 * @author  David Mihola <xmihol00@stud.fit.vutbr.cz>
 *
 * @brief   Parallel Marching Cubes implementation using OpenMP loops
 *
 * @date    15. 12. 2023
 **/

#ifndef LOOP_MESH_BUILDER_H
#define LOOP_MESH_BUILDER_H

#include <vector>
#include "base_mesh_builder.h"

class LoopMeshBuilder : public BaseMeshBuilder
{
public:
    LoopMeshBuilder(unsigned gridEdgeSize);

protected:
    unsigned marchCubes(const ParametricScalarField &field);
    float evaluateFieldAt(const Vec3_t<float> &pos, const ParametricScalarField &field);
    void emitTriangle(const Triangle_t &triangle);
    const Triangle_t *getTrianglesArray() const;

private:
    unsigned _fieldSize = 0;
    std::vector<Triangle_t> *_perThreadTriangles;
    std::vector<Triangle_t> _sharedTriangles;
    unsigned _sharedTrianglesIndex = 0;
    Triangle_t *_trianglesMerged = nullptr;
    float *_field_x = nullptr;
    float *_field_y = nullptr;
    float *_field_z = nullptr;
    int _numThreads = 0;
};

#endif // LOOP_MESH_BUILDER_H
