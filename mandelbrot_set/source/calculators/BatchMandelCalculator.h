/**
 * @file BatchMandelCalculator.h
 * @author David Mihola <xmihol00@stud.fit.vutbr.cz>
 * @brief Implementation of Mandelbrot calculator that uses SIMD parallelization over small batches
 * @date 03. 11. 2023
 */
#ifndef BATCHMANDELCALCULATOR_H
#define BATCHMANDELCALCULATOR_H

#include <BaseMandelCalculator.h>
#include <immintrin.h>

class BatchMandelCalculator : public BaseMandelCalculator
{
public:
    BatchMandelCalculator(unsigned matrixBaseSize, unsigned limit);
    ~BatchMandelCalculator();
    
    /**
     * @brief Calculates the Mandelbrot set, the result shall not be modified nor freed.
     */
    int *calculateMandelbrot();

private:
    using int16   = __m512i;
	using float16 = __m512;
	using mask16  = __mmask16;

    template<int size, int iterations>
    void calculate();

    template<int iterations> inline
    int16 batchKernel(float16 real, float16 imag);

    const int MIN_SIZE = 256;
    const int TILE_SIZE = 64;
    const int TILE_SHIFT = 6;

    int      _size;
    float    _xStart;
    float    _yStart;
    float    _dx;
    float    _dy;
    bool     _solved = false;
    bool     _fast   = true;

    int   *_data;
    float *_partialReals;
    float *_partialImags;
};

#endif