/**
 * @file BatchMandelCalculator.cc
 * @author David Mihola <xmihol00@stud.fit.vutbr.cz>
 * @brief Implementation of Mandelbrot calculator that uses SIMD paralelization over small batches
 * @date 03. 11. 2023
 */

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include <stdlib.h>
#include <string.h>
#include <stdexcept>

#include "BatchMandelCalculator.h"

#define ELIPSE_OPTIMIZED_IMPLEMENTATION 1
#define CIRCLE_OPTIMIZED_IMPLEMENTATION (!(ELIPSE_OPTIMIZED_IMPLEMENTATION) && 1)
#define BASIC_MM512_IMPLEMENTATION (!(ELIPSE_OPTIMIZED_IMPLEMENTATION || CIRCLE_OPTIMIZED_IMPLEMENTATION) && 1)
#define BASIC_TILED_IMPLEMENTATION (!(ELIPSE_OPTIMIZED_IMPLEMENTATION || CIRCLE_OPTIMIZED_IMPLEMENTATION || BASIC_MM512_IMPLEMENTATION))

using int16   = __m512i;
using float16 = __m512;
using mask16  = __mmask16;

BatchMandelCalculator::BatchMandelCalculator (unsigned matrixBaseSize, unsigned limit) :
	BaseMandelCalculator(matrixBaseSize, limit, "BatchMandelCalculator"),
	_size(width / 3), 
	_xStart(x_start), _yStart(y_start), _dx(dx), _dy(dy) // use floats instead of doubles
{
	_data = (int *)aligned_alloc(64, height * width * sizeof(int));
	#pragma omp simd
	for (int i = 0; i < height * width; i++)
	{
		_data[i] = limit;
	}

#if BASIC_TILED_IMPLEMENTATION
	_partialImags = (float *)aligned_alloc(64, width * _size * sizeof(float));
	_partialReals = (float *)aligned_alloc(64, width * _size * sizeof(float));

	// memory initialization
	for (int i = 0; i < _size; i++)
	{
		#pragma omp simd
		for (int j = 0; j < width; j++)
		{
			_partialReals[i * width + j] = _xStart + j * dx; // initialize with the first real value
			_partialImags[i * width + j] = _yStart + i * dy; // initialize with the first imaginary value
		}
	}

	if (_size & (TILE_SIZE - 1)) // size is not a multiple of TILE_SIZE
	{
		std::cerr << "The 'Base matrix size' is not a multiple of " << TILE_SIZE << ". Downgrading to a basic implementation." << std::endl;
		_fast = false;
	}
#else
	_partialImags = nullptr;
	_partialReals = nullptr;

	if (_size & 15) // size is not a multiple of 16
	{
		std::cerr << "The 'Base matrix size' is not a multiple of 16. Downgrading to a basic implementation." << std::endl;
		_fast = false;
	}
#endif

	if (_size < MIN_SIZE) // matrix is too small
	{
		// the accuracy of the fast implementation is not good enough for small matrices
		std::cerr << "The 'Base matrix size' is too small. Downgrading to a basic implementation." << std::endl;
		_fast = false;
	}
}

BatchMandelCalculator::~BatchMandelCalculator()
{
	free(_data);
#if BASIC_TILED_IMPLEMENTATION
	free(_partialImags);
	free(_partialReals);
#endif

	_data = nullptr;
	_partialImags = nullptr;
	_partialReals = nullptr;
}

template<int iterations> inline
int16 BatchMandelCalculator::batchKernel(float16 real, float16 imag)
{
	float16 nextReal = real;
	float16 nextImag = imag;
	int16 batch = _mm512_set1_epi32(0);
	mask16 mask = (mask16)(~0);
	
	static_assert(iterations >= 100, "Iterations must be greater or equal to 100.");

	#pragma unroll(15)
	for (int k = 0; k < 15 && mask; k++) // first 15 iterations check for termination each iteration
	{
		float16 r2 = _mm512_mul_ps(nextReal, nextReal);  // nextReal^2
		float16 i2 = _mm512_mul_ps(nextImag, nextImag);  // nextImag^2
		float16 sum = _mm512_add_ps(r2, i2);			 // nextReal^2 + nextImag^2

		mask = _mm512_mask_cmp_ps_mask(mask, sum, _mm512_set1_ps(4.0f), _CMP_LT_OS);  // accumulate the mask, i.e. mask &= sum < 4.0f
		batch = _mm512_mask_add_epi32(batch, mask, batch, _mm512_set1_epi32(1));      // accumulate the batch, i.e. batch += mask ? 1 : 0

		nextImag = _mm512_fmadd_ps(_mm512_add_ps(nextReal, nextReal), nextImag, imag);  // 2 * nextReal * nextImag + imag
		nextReal = _mm512_add_ps(_mm512_sub_ps(r2, i2), real);					        // nextReal^2 - nextImag^2 + real
	}

	if (mask)
	{
		for (int k = 15; k < 50 && mask; k += 5) // next 35 iterations check for termination each 5 iterations
		{
			#pragma unroll(5)
			for (int j = 0; j < 5; j++)
			{
				float16 r2 = _mm512_mul_ps(nextReal, nextReal);
				float16 i2 = _mm512_mul_ps(nextImag, nextImag);
				float16 sum = _mm512_add_ps(r2, i2);

				mask = _mm512_mask_cmp_ps_mask(mask, sum, _mm512_set1_ps(4.0f), _CMP_LT_OS);
				batch = _mm512_mask_add_epi32(batch, mask, batch, _mm512_set1_epi32(1));

				nextImag = _mm512_fmadd_ps(_mm512_add_ps(nextReal, nextReal), nextImag, imag);
				nextReal = _mm512_add_ps(_mm512_sub_ps(r2, i2), real);
			}
		}	
	}

	if (mask)
	{
		for (int k = 50; k < 100 && mask; k += 10) // next 50 iterations check for termination each 10 iterations
		{
			#pragma unroll(10)
			for (int j = 0; j < 10; j++)
			{
				float16 r2 = _mm512_mul_ps(nextReal, nextReal);
				float16 i2 = _mm512_mul_ps(nextImag, nextImag);
				float16 sum = _mm512_add_ps(r2, i2);

				mask = _mm512_mask_cmp_ps_mask(mask, sum, _mm512_set1_ps(4.0f), _CMP_LT_OS);
				batch = _mm512_mask_add_epi32(batch, mask, batch, _mm512_set1_epi32(1));

				nextImag = _mm512_fmadd_ps(_mm512_add_ps(nextReal, nextReal), nextImag, imag);
				nextReal = _mm512_add_ps(_mm512_sub_ps(r2, i2), real);
			}
		}
	}

	if (iterations > 100)
	{
		if (mask)
		{
			for (int k = 100; k < 1000 && mask; k += 25) // last 900 iterations check for termination each 25 iterations
			{
				#pragma unroll(25)
				for (int j = 0; j < 25; j++)
				{
					float16 r2 = _mm512_mul_ps(nextReal, nextReal);
					float16 i2 = _mm512_mul_ps(nextImag, nextImag);
					float16 sum = _mm512_add_ps(r2, i2);
					mask = _mm512_mask_cmp_ps_mask(mask, sum, _mm512_set1_ps(4.0f), _CMP_LT_OS);

					batch = _mm512_mask_add_epi32(batch, mask, batch, _mm512_set1_epi32(1));

					nextImag = _mm512_fmadd_ps(_mm512_add_ps(nextReal, nextReal), nextImag, imag);
					nextReal = _mm512_add_ps(_mm512_sub_ps(r2, i2), real);
				}
			}
		}
	}

	return batch;
}

template<int size, int iterations>
void BatchMandelCalculator::calculate()
{
	constexpr int height = size * 2;
	constexpr int width = size * 3;

	// experimentally estimated values based on plots
	constexpr int heightSeparator1 = int(size * 0.58106);
	constexpr int heighSeparator2 = int(size * 0.87891);
	constexpr int elipseRadiusSquared = int((size * 0.510254) * (size * 0.510254));
	constexpr int elipseMultiply = 11;
	constexpr int elipseShift = 3;
	constexpr int elipseCenter = int(size * 1.85987);
	constexpr int smallRectangleStart = int(size * 0.828125 + 15) & ~15;
	constexpr int smallRectangleEnd = int(size * 1.171875 + 15) & ~15;
	constexpr int largeRectangleStart = int(size * 1.2734375 + 15) & ~15;
	constexpr int largeRectangleEnd = int(size * 2.25 + 15) & ~15;

	int *pData = _data;
	int *pDataReversed = _data + (height - 1) * width;  // pointer to the last row of the matrix
	float16 realIncrement = _mm512_set1_ps(16.0f * _dx);

	for (int i = 0; i < heightSeparator1; i++)
	{
		float16 imag = _mm512_fmadd_ps(_mm512_set1_ps(float(i)), _mm512_set1_ps(_dy), _mm512_set1_ps(_yStart));
		float16 real = _mm512_fmadd_ps(_mm512_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f), _mm512_set1_ps(_dx), _mm512_set1_ps(_xStart));

		for (int j = 0; j < width; j += 16)
		{
			float16 nextReal = real;
			float16 nextImag = imag;
			int16 batch = batchKernel<iterations>(real, imag);

			_mm512_store_epi32(pData, batch);
			_mm512_store_epi32(pDataReversed, batch);

			pData += 16;
			pDataReversed += 16;
			real = _mm512_add_ps(real, realIncrement);
		}
		pDataReversed -= width << 1;
	}

	for (int i = heightSeparator1; i < heighSeparator2; i++)
	{
		float16 imag = _mm512_fmadd_ps(_mm512_set1_ps(float(i)), _mm512_set1_ps(_dy), _mm512_set1_ps(_yStart));
		
		int elipseEquation = elipseRadiusSquared - (((i - size) * (i - size) * elipseMultiply) >> elipseShift);

		// approximation of sqrt(elipseEquation)
		int msb = ((32 - __builtin_clz(elipseEquation)) >> 1);
		int elipseEquationSqrt = (1 << (msb - 1)) + (elipseEquation >> (msb + 1));

		int elipseStart = (elipseCenter - elipseEquationSqrt) & (~15); // align to 16
		int elipseEnd = (elipseCenter + elipseEquationSqrt) & (~15);   // align to 16
		int elipseWidth = elipseEnd - elipseStart;

		for (int j = 0; j < elipseStart; j += 16)
		{
			float16 real = _mm512_fmadd_ps(_mm512_add_ps(_mm512_set1_ps(float(j)), _mm512_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f)), _mm512_set1_ps(_dx), _mm512_set1_ps(_xStart));
			float16 nextReal = real;
			float16 nextImag = imag;
			int16 batch = batchKernel<iterations>(real, imag);

			_mm512_store_epi32(pData, batch);
			_mm512_store_epi32(pDataReversed, batch);
			
			pData += 16;
			pDataReversed += 16;
		}
		pData += elipseWidth;
		pDataReversed += elipseWidth;
		for (int j = elipseEnd; j < width; j += 16)
		{
			float16 real = _mm512_fmadd_ps(_mm512_add_ps(_mm512_set1_ps(float(j)), _mm512_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f)), _mm512_set1_ps(_dx), _mm512_set1_ps(_xStart));
			float16 nextReal = real;
			float16 nextImag = imag;
			int16 batch = batchKernel<iterations>(real, imag);

			_mm512_store_epi32(pData, batch);
			_mm512_store_epi32(pDataReversed, batch);
			
			pData += 16;
			pDataReversed += 16;
		}

		pDataReversed -= width << 1;
	}

	for (int i = heighSeparator2; i < size; i++)
	{
		float16 imag = _mm512_fmadd_ps(_mm512_set1_ps(float(i)), _mm512_set1_ps(_dy), _mm512_set1_ps(_yStart));

    	constexpr int largeRectangleWidth = largeRectangleEnd - largeRectangleStart;
		constexpr int smallRectangleWidth = smallRectangleEnd - smallRectangleStart;
		static_assert(largeRectangleWidth > 0 && smallRectangleWidth > 0, "largeRectangleWidth and smallRectangleWidth must be greater than 0");
		static_assert(largeRectangleWidth % 16 == 0, "largeRectangleWidth must be a multiple of 16");
		static_assert(smallRectangleWidth % 16 == 0, "smallRectangleWidth must be a multiple of 16");

		for (int j = 0; j < smallRectangleStart; j += 16)
		{
			float16 real = _mm512_fmadd_ps(_mm512_add_ps(_mm512_set1_ps(float(j)), _mm512_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f)), _mm512_set1_ps(_dx), _mm512_set1_ps(_xStart));
			float16 nextReal = real;
			float16 nextImag = imag;
			int16 batch = batchKernel<iterations>(real, imag);

			_mm512_store_epi32(pData, batch);
			_mm512_store_epi32(pDataReversed, batch);
			
			pData += 16;
			pDataReversed += 16;
		}

		pData += smallRectangleWidth;
		pDataReversed += smallRectangleWidth;

		for (int j = smallRectangleEnd; j < largeRectangleStart; j += 16)
		{
			float16 real = _mm512_fmadd_ps(_mm512_add_ps(_mm512_set1_ps(float(j)), _mm512_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f)), _mm512_set1_ps(_dx), _mm512_set1_ps(_xStart));
			float16 nextReal = real;
			float16 nextImag = imag;
			int16 batch = batchKernel<iterations>(real, imag);

			_mm512_store_epi32(pData, batch);
			_mm512_store_epi32(pDataReversed, batch);
			
			pData += 16;
			pDataReversed += 16;
		}
		
		pData += largeRectangleWidth;
		pDataReversed += largeRectangleWidth;

		for (int j = largeRectangleEnd; j < width; j += 16)
		{
			float16 real = _mm512_fmadd_ps(_mm512_add_ps(_mm512_set1_ps(float(j)), _mm512_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f)), _mm512_set1_ps(_dx), _mm512_set1_ps(_xStart));
			float16 nextReal = real;
			float16 nextImag = imag;
			int16 batch = batchKernel<iterations>(real, imag);

			_mm512_store_epi32(pData, batch);
			_mm512_store_epi32(pDataReversed, batch);
			
			pData += 16;
			pDataReversed += 16;
		}

		pDataReversed -= width << 1;
	}
}

int * BatchMandelCalculator::calculateMandelbrot() 
{
#if ELIPSE_OPTIMIZED_IMPLEMENTATION
	if (_size == 4096 && limit == 100 && !_solved)
	{
		calculate<4096, 100>();
	}
	else if (_size == 4096 && limit == 1000 && !_solved)
	{
		calculate<4096, 1000>();
	}
	else if (_size == 2048 && limit == 100 && !_solved)
	{
		calculate<2048, 100>();
	}
	else if (_size == 2048 && limit == 1000 && !_solved)
	{
		calculate<2048, 1000>();
	}
	else if (_size == 1024 && limit == 100 && !_solved)
	{
		calculate<1024, 100>();
	}
	else if (_size == 1024 && limit == 1000 && !_solved)
	{
		calculate<1024, 1000>();
	}
	else if (_size == 512 && limit == 100 && !_solved)
	{
		calculate<512, 100>();
	}
	else if (_size == 512 && limit == 1000 && !_solved)
	{
		calculate<512, 1000>();
	}
	else if (_size == 256 && limit == 100 && !_solved)
	{
		calculate<256, 100>();
	}
	else if (_size == 256 && limit == 1000 && !_solved)
	{
		calculate<256, 1000>();
	}
	else if (_size == 8192 && limit == 100 && !_solved)
	{
		calculate<8192, 100>();
	}
	else if (_size == 8192 && limit == 1000 && !_solved)
	{
		calculate<8192, 1000>();
	}
	else if (_fast && !_solved) // optimized implementation for any limit and size multiple of 16
	{
		// see the template function above for detailed comments

		int height = _size * 2;
		int width = _size * 3;
		int heightSeparator1 = int(_size * 0.58106);
		int heighSeparator2 = int(_size * 0.87891);
		int elipseRadiusSquared = int((_size * 0.510254) * (_size * 0.510254));
		int elipseMultiply = 11;
		int elipseShift = 3;
		int elipseCenter = int(_size * 1.85987);
		int smallRectangleStart = int(_size * 0.828125 + 15) & ~15;
		int smallRectangleEnd = int(_size * 1.171875 + 15) & ~15;
		int largeRectangleStart = int(_size * 1.2734375 + 15) & ~15;
		int largeRectangleEnd = int(_size * 2.25 + 15) & ~15;

		int *pData = _data;
		int *pDataReversed = _data + (height - 1) * width; 
		float16 realIncrement = _mm512_set1_ps(16.0f * _dx);

		for (int i = 0; i < heightSeparator1; i++)
		{
			float16 imag = _mm512_fmadd_ps(_mm512_set1_ps(float(i)), _mm512_set1_ps(_dy), _mm512_set1_ps(_yStart));
			float16 real = _mm512_fmadd_ps(_mm512_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f), _mm512_set1_ps(_dx), _mm512_set1_ps(_xStart));

			for (int j = 0; j < width; j += 16)
			{
				float16 nextReal = real;
				float16 nextImag = imag;
				int16 batch = _mm512_set1_epi32(0);
				mask16 mask = (mask16)(~0);

				for (int k = 0; k < limit && mask; k++)
				{
					float16 r2 = _mm512_mul_ps(nextReal, nextReal);
					float16 i2 = _mm512_mul_ps(nextImag, nextImag);
					float16 sum = _mm512_add_ps(r2, i2);

					mask = _mm512_mask_cmp_ps_mask(mask, sum, _mm512_set1_ps(4.0f), _CMP_LT_OS);
					batch = _mm512_mask_add_epi32(batch, mask, batch, _mm512_set1_epi32(1));

					nextImag = _mm512_fmadd_ps(_mm512_add_ps(nextReal, nextReal), nextImag, imag);
					nextReal = _mm512_add_ps(_mm512_sub_ps(r2, i2), real);
				}

				_mm512_store_epi32(pData, batch);
				_mm512_store_epi32(pDataReversed, batch);

				pData += 16;
				pDataReversed += 16;
				real = _mm512_add_ps(real, realIncrement);
			}
			pDataReversed -= width << 1;
		}

		for (int i = heightSeparator1; i < heighSeparator2; i++)
		{
			float16 imag = _mm512_fmadd_ps(_mm512_set1_ps(float(i)), _mm512_set1_ps(_dy), _mm512_set1_ps(_yStart));
			
			int elipseEquation = elipseRadiusSquared - (((i - _size) * (i - _size) * elipseMultiply) >> elipseShift);
			int msb = ((32 - __builtin_clz(elipseEquation)) >> 1);
			int elipseEquationSqrt = (1 << (msb - 1)) + (elipseEquation >> (msb + 1));

			int elipseStart = (elipseCenter - elipseEquationSqrt) & (~15);
			int elipseEnd = (elipseCenter + elipseEquationSqrt) & (~15);
			int elipseWidth = elipseEnd - elipseStart;

			for (int j = 0; j < elipseStart; j += 16)
			{
				float16 real = _mm512_fmadd_ps(_mm512_add_ps(_mm512_set1_ps(float(j)), _mm512_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f)), _mm512_set1_ps(_dx), _mm512_set1_ps(_xStart));
				float16 nextReal = real;
				float16 nextImag = imag;
				int16 batch = _mm512_set1_epi32(0);
				mask16 mask = (mask16)(~0);

				for (int k = 0; k < limit && mask; k++)
				{
					float16 r2 = _mm512_mul_ps(nextReal, nextReal);
					float16 i2 = _mm512_mul_ps(nextImag, nextImag);
					float16 sum = _mm512_add_ps(r2, i2);

					mask = _mm512_mask_cmp_ps_mask(mask, sum, _mm512_set1_ps(4.0f), _CMP_LT_OS);
					batch = _mm512_mask_add_epi32(batch, mask, batch, _mm512_set1_epi32(1));

					nextImag = _mm512_fmadd_ps(_mm512_add_ps(nextReal, nextReal), nextImag, imag);
					nextReal = _mm512_add_ps(_mm512_sub_ps(r2, i2), real);
				}

				_mm512_store_epi32(pData, batch);
				_mm512_store_epi32(pDataReversed, batch);
				
				pData += 16;
				pDataReversed += 16;
			}
			pData += elipseWidth;
			pDataReversed += elipseWidth;
			for (int j = elipseEnd; j < width; j += 16)
			{
				float16 real = _mm512_fmadd_ps(_mm512_add_ps(_mm512_set1_ps(float(j)), _mm512_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f)), _mm512_set1_ps(_dx), _mm512_set1_ps(_xStart));
				float16 nextReal = real;
				float16 nextImag = imag;
				int16 batch = _mm512_set1_epi32(0);
				mask16 mask = (mask16)(~0);

				for (int k = 0; k < limit && mask; k++)
				{
					float16 r2 = _mm512_mul_ps(nextReal, nextReal);
					float16 i2 = _mm512_mul_ps(nextImag, nextImag);
					float16 sum = _mm512_add_ps(r2, i2);

					mask = _mm512_mask_cmp_ps_mask(mask, sum, _mm512_set1_ps(4.0f), _CMP_LT_OS);
					batch = _mm512_mask_add_epi32(batch, mask, batch, _mm512_set1_epi32(1));

					nextImag = _mm512_fmadd_ps(_mm512_add_ps(nextReal, nextReal), nextImag, imag);
					nextReal = _mm512_add_ps(_mm512_sub_ps(r2, i2), real);
				}

				_mm512_store_epi32(pData, batch);
				_mm512_store_epi32(pDataReversed, batch);
				
				pData += 16;
				pDataReversed += 16;
			}

			pDataReversed -= width << 1;
		}

		for (int i = heighSeparator2; i < _size; i++)
		{
			float16 imag = _mm512_fmadd_ps(_mm512_set1_ps(float(i)), _mm512_set1_ps(_dy), _mm512_set1_ps(_yStart));

			int largeRectangleWidth = largeRectangleEnd - largeRectangleStart;
			int smallRectangleWidth = smallRectangleEnd - smallRectangleStart;

			for (int j = 0; j < smallRectangleStart; j += 16)
			{
				float16 real = _mm512_fmadd_ps(_mm512_add_ps(_mm512_set1_ps(float(j)), _mm512_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f)), _mm512_set1_ps(_dx), _mm512_set1_ps(_xStart));
				float16 nextReal = real;
				float16 nextImag = imag;
				int16 batch = _mm512_set1_epi32(0);
				mask16 mask = (mask16)(~0);

				for (int k = 0; k < limit && mask; k++)
				{
					float16 r2 = _mm512_mul_ps(nextReal, nextReal);
					float16 i2 = _mm512_mul_ps(nextImag, nextImag);
					float16 sum = _mm512_add_ps(r2, i2);

					mask = _mm512_mask_cmp_ps_mask(mask, sum, _mm512_set1_ps(4.0f), _CMP_LT_OS);
					batch = _mm512_mask_add_epi32(batch, mask, batch, _mm512_set1_epi32(1));

					nextImag = _mm512_fmadd_ps(_mm512_add_ps(nextReal, nextReal), nextImag, imag);
					nextReal = _mm512_add_ps(_mm512_sub_ps(r2, i2), real);
				}

				_mm512_store_epi32(pData, batch);
				_mm512_store_epi32(pDataReversed, batch);
				
				pData += 16;
				pDataReversed += 16;
			}

			pData += smallRectangleWidth;
			pDataReversed += smallRectangleWidth;

			for (int j = smallRectangleEnd; j < largeRectangleStart; j += 16)
			{
				float16 real = _mm512_fmadd_ps(_mm512_add_ps(_mm512_set1_ps(float(j)), _mm512_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f)), _mm512_set1_ps(_dx), _mm512_set1_ps(_xStart));
				float16 nextReal = real;
				float16 nextImag = imag;
				int16 batch = _mm512_set1_epi32(0);
				mask16 mask = (mask16)(~0);

				for (int k = 0; k < limit && mask; k++)
				{
					float16 r2 = _mm512_mul_ps(nextReal, nextReal);
					float16 i2 = _mm512_mul_ps(nextImag, nextImag);
					float16 sum = _mm512_add_ps(r2, i2);

					mask = _mm512_mask_cmp_ps_mask(mask, sum, _mm512_set1_ps(4.0f), _CMP_LT_OS);
					batch = _mm512_mask_add_epi32(batch, mask, batch, _mm512_set1_epi32(1));

					nextImag = _mm512_fmadd_ps(_mm512_add_ps(nextReal, nextReal), nextImag, imag);
					nextReal = _mm512_add_ps(_mm512_sub_ps(r2, i2), real);
				}

				_mm512_store_epi32(pData, batch);
				_mm512_store_epi32(pDataReversed, batch);
				
				pData += 16;
				pDataReversed += 16;
			}
			
			pData += largeRectangleWidth;
			pDataReversed += largeRectangleWidth;

			for (int j = largeRectangleEnd; j < width; j += 16)
			{
				float16 real = _mm512_fmadd_ps(_mm512_add_ps(_mm512_set1_ps(float(j)), _mm512_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f)), _mm512_set1_ps(_dx), _mm512_set1_ps(_xStart));
				float16 nextReal = real;
				float16 nextImag = imag;
				int16 batch = _mm512_set1_epi32(0);
				mask16 mask = (mask16)(~0);

				for (int k = 0; k < limit && mask; k++)
				{
					float16 r2 = _mm512_mul_ps(nextReal, nextReal);
					float16 i2 = _mm512_mul_ps(nextImag, nextImag);
					float16 sum = _mm512_add_ps(r2, i2);

					mask = _mm512_mask_cmp_ps_mask(mask, sum, _mm512_set1_ps(4.0f), _CMP_LT_OS);
					batch = _mm512_mask_add_epi32(batch, mask, batch, _mm512_set1_epi32(1));

					nextImag = _mm512_fmadd_ps(_mm512_add_ps(nextReal, nextReal), nextImag, imag);
					nextReal = _mm512_add_ps(_mm512_sub_ps(r2, i2), real);
				}

				_mm512_store_epi32(pData, batch);
				_mm512_store_epi32(pDataReversed, batch);
				
				pData += 16;
				pDataReversed += 16;
			}

			pDataReversed -= width << 1;
		}
	}
#endif

#if CIRCLE_OPTIMIZED_IMPLEMENTATION
	if (_size >= MIN_SIZE << 1 && _fast && !_solved)
	{
		int *pData = _data;
		int *pDataReversed = _data + (height - 1) * width;  // pointer to the last row of the matrix

		// experimentally estimated values
		int radiusL2 = int((height * 0.214133f) * (height * 0.214133f));
		int xCenterL = int(height * 0.905f);
		int yCenterL = int(height * 0.5f); 
		int radiusS2 = int((height * 0.084032f) * (height * 0.084032f));
		int xCenterS = int(height * 0.5f);
		int yCenterS = int(height * 0.5f);

		for (int i = 0; i < height >> 1; i++)
		{
			int iCenteredL = i - yCenterL;
			int iCenteredS = i - yCenterS;
			int iCenteredL2 = iCenteredL * iCenteredL;
			int iCenteredS2 = iCenteredS * iCenteredS; 
			float16 imag = _mm512_fmadd_ps(_mm512_set1_ps(float(i)), _mm512_set1_ps(_dy), _mm512_set1_ps(_yStart));

			for (int j = 0; j < width; j += 16)
			{
				// calculation of real value cannot be incremented by 16 dx after each iteration for large matrices, because the accuracy is not good enough 
				// the real value must be calculated for each iteration separately using multiplication
				float16 real = _mm512_fmadd_ps(_mm512_add_ps(_mm512_set1_ps(float(j)), _mm512_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f)), _mm512_set1_ps(_dx), _mm512_set1_ps(_xStart));
				float16 nextReal = real;
				float16 nextImag = imag;
				int16 batch = _mm512_set1_epi32(0);
				mask16 mask = (mask16)(~0);

				int jCenteredL = j - xCenterL;
				int jCenteredS = j - xCenterS;
				int jCenteredL2 = jCenteredL * jCenteredL;
				int jCenteredS2 = jCenteredS * jCenteredS;
				if (iCenteredL2 + jCenteredL2 > radiusL2 && iCenteredS2 + jCenteredS2 > radiusS2)
				{
					for (int k = 0; k < limit && mask; k++)
					{
						float16 r2 = _mm512_mul_ps(nextReal, nextReal);
						float16 i2 = _mm512_mul_ps(nextImag, nextImag);
						float16 sum = _mm512_add_ps(r2, i2);
						mask = _mm512_mask_cmp_ps_mask(mask, sum, _mm512_set1_ps(4.0f), _CMP_LT_OS);

						batch = _mm512_mask_add_epi32(batch, mask, batch, _mm512_set1_epi32(1));

						nextImag = _mm512_fmadd_ps(_mm512_add_ps(nextReal, nextReal), nextImag, imag);
						nextReal = _mm512_add_ps(_mm512_sub_ps(r2, i2), real);
					}

					_mm512_store_epi32(pData, batch);
					_mm512_store_epi32(pDataReversed, batch);
				}
				pData += 16;
				pDataReversed += 16;
			}
			pDataReversed -= width << 1;
		}
    }
#endif

#if BASIC_MM512_IMPLEMENTATION
	if (_fast && !_solved)
	{
		int *pData = _data;
		int *pDataReversed = _data + (height - 1) * width;  // pointer to the last row of the matrix

		for (int i = 0; i < _size; i++) // compute half of the matrix and copy the result mirrored to the second half of the matrix
		{
			float16 imag = _mm512_fmadd_ps(_mm512_set1_ps(float(i)), _mm512_set1_ps(_dy), _mm512_set1_ps(_yStart));

			for (int j = 0; j < width; j += 16) // use batch of 16 values
			{
				float16 real = _mm512_fmadd_ps(_mm512_add_ps(_mm512_set1_ps(float(j)), _mm512_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f)), _mm512_set1_ps(_dx), _mm512_set1_ps(_xStart));
				float16 nextReal = real;
				float16 nextImag = imag;
				int16 batch = _mm512_set1_epi32(0); // initialize the batch to 0
				mask16 mask = (mask16)(~0);			// initialize the mask so that all points are marked as not diverging

				for (int k = 0; k < limit && mask; k++) // check for termination each iteration
				{
					float16 r2 = _mm512_mul_ps(nextReal, nextReal); // nextReal^2
					float16 i2 = _mm512_mul_ps(nextImag, nextImag); // nextImag^2
					float16 sum = _mm512_add_ps(r2, i2);            // nextReal^2 + nextImag^2

					mask = _mm512_mask_cmp_ps_mask(mask, sum, _mm512_set1_ps(4.0f), _CMP_LT_OS); // accumulate the mask, i.e. mask &= sum < 4.0f
					batch = _mm512_mask_add_epi32(batch, mask, batch, _mm512_set1_epi32(1));     // accumulate the batch, i.e. batch += mask ? 1 : 0

					// compute imaginary and real part for the next iteration
					nextImag = _mm512_fmadd_ps(_mm512_add_ps(nextReal, nextReal), nextImag, imag); // 2 * nextReal * nextImag + imag								   
					nextReal = _mm512_add_ps(_mm512_sub_ps(r2, i2), real);						   // nextReal^2 - nextImag^2 + real	
				}

				_mm512_store_epi32(pData, batch);		   // store the result to the first half of the matrix
				_mm512_store_epi32(pDataReversed, batch);  // store the result to the second half of the matrix
				
				pData += 16;			// move the pointer to the next batch in the first half of the matrix
				pDataReversed += 16;    // move the pointer to the next batch in the second half of the matrix
			}
			pDataReversed -= width << 1; // go back two rows in the second half of the matrix, i.e mirror the first half of the matrix
		}
	}
#endif

#if BASIC_TILED_IMPLEMENTATION
	if (_fast && !_solved)
	{
		int *pData = _data;
		float *pImags = _partialImags;
		float *pReals = _partialReals;
		float iFp = 0.0f;

		for (int i = 0; i < _size; i++) 
		{
			float imag = _yStart + iFp * _dy;
			
			int tileOffset = 0;
			for (int tileIndex = 0; tileIndex < width >> TILE_SHIFT; tileIndex++) 
			{
				int any = 1;
				for (int k = 0; k < limit && any; k++)
				{
					any = 0;
					float jFp = float(tileOffset);

					#pragma omp simd aligned(pData, pReals, pImags : 64) reduction(+ : any) simdlen(16)
					for (int j = 0; j < TILE_SIZE; j++) 
					{
						float real = _xStart + jFp * _dx;

						float r2 = pReals[j] * pReals[j];
						float i2 = pImags[j] * pImags[j];

						pData[j] = r2 + i2 > 4.0f && pData[j] == limit ? k : pData[j];
						any = any + (pData[j] == limit); // reduction, i.e. if any of the values is still in the 4.0f bound, continue the loop

						pImags[j] = 2.0f * pReals[j] * pImags[j] + imag;
						pReals[j] = r2 - i2 + real;
						jFp += 1.0f;
					}
				}

				pData += TILE_SIZE;
				pReals += TILE_SIZE;
				pImags += TILE_SIZE;
				tileOffset += TILE_SIZE;
			}

			iFp += 1.0f;
		}

		pData = _data;
		#pragma omp simd aligned(pData : 64)
		for (int i = 0; i < _size; i++)
		{
			memcpy(pData + (height - i - 1) * width, pData + i * width, width * sizeof(int)); // mirror the first half of the matrix to the second half	
		}
	}
#endif
	else if (!_solved) // fallback implementation if the matrix is too small or the width is not a multiple of 16
	{
		int *pData = _data;

		// calculate half of the image plus 1 line to ensure proper results for odd heights (although not possible in this project)
		for (int i = 0; i < _size + 1; i++)
		{
			for (int j = 0; j < width; j++)
			{
				float real = _xStart + j * _dx;
				float imag = _yStart + i * _dy;
				float zReal = real;
				float zImag = imag;

				int i = 0;
				for (; i < limit; i++)
				{
					float r2 = zReal * zReal;
					float i2 = zImag * zImag;

					if (r2 + i2 > 4.0f)
					{
						break;
					}

					zImag = 2.0f * zReal * zImag + imag;
					zReal = r2 - i2 + real;
				}

				*(pData++) = i;
			}
		}

		pData = _data;
		#pragma omp simd aligned(pData : 64)
		for (int i = 0; i < _size; i++)
		{
			memcpy(pData + (height - i - 1) * width, pData + i * width, width * sizeof(int)); // mirror the first half of the matrix to the second half	
		}
	}

	_solved = true;
	return _data;
}