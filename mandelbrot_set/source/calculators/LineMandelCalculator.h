/**
 * @file LineMandelCalculator.h
 * @author David Mihola <xmihol00@stud.fit.vutbr.cz>
 * @brief Implementation of Mandelbrot calculator that uses SIMD paralelization over lines
 * @date 03. 11. 2023
 */

#include <BaseMandelCalculator.h>

class LineMandelCalculator : public BaseMandelCalculator
{
public:
    LineMandelCalculator(unsigned matrixBaseSize, unsigned limit);
    ~LineMandelCalculator();

    /**
     * @brief Calculates the Mandelbrot set, the result shall not be modified nor freed.
     */
    int *calculateMandelbrot();
    


private:
    template<int size, int iterations>
    void calculate();

    const int MIN_SIZE = 128;

    int      _size;
    int      _width;
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