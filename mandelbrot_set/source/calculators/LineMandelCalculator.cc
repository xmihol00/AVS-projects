/**
 * @file LineMandelCalculator.cc
 * @author David Mihola <xmihol00@stud.fit.vutbr.cz>
 * @brief Implementation of Mandelbrot calculator that uses SIMD parallelization over lines
 * @date 03. 11. 2023
 */
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <string.h>

#include <stdlib.h>

#include "LineMandelCalculator.h"

#define OPTIMIZED_IMPLEMENTATION 1
#define BASIC_IMPLEMENTATION !(OPTIMIZED_IMPLEMENTATION)

LineMandelCalculator::LineMandelCalculator (unsigned matrixBaseSize, unsigned limit) :
	BaseMandelCalculator(matrixBaseSize, limit, "LineMandelCalculator"), 
	_size(width / 3), _width(width & ~15),
	_xStart(x_start), _yStart(y_start), _dx(dx), _dy(dy) // use floats instead of doubles
{
	_data = (int *)aligned_alloc(64, height * width * sizeof(int));
	_partialReals = (float *)aligned_alloc(64, height * width * sizeof(float));
	_partialImags = (float *)aligned_alloc(64, height * width * sizeof(float));

	// memory initialization
	for (int i = 0; i < height; i++)
	{
		#pragma omp simd
		for (int j = 0; j < width; j++)
		{
			_data[i * width + j] = limit;                    // initialize the data to the limit 
			_partialReals[i * width + j] = _xStart + j * dx; // initialize with the first real value
			_partialImags[i * width + j] = _yStart + i * dy; // initialize with the first imaginary value
		}
	}

	if (_size & 15) // size is not a multiple of 16
	{
		std::cerr << "The 'Base matrix size' is not a multiple of 16. Downgrading to a basic implementation." << std::endl;
		_fast = false;
	}

	if (_size < MIN_SIZE) // size is too small
	{
		// the accuracy is not good enough for small matrices
		std::cerr << "The 'Base matrix size' is too small. Downgrading to a basic implementation." << std::endl;
		_fast = false;
	}
}

LineMandelCalculator::~LineMandelCalculator() 
{
	free(_data);
	free(_partialReals);
	free(_partialImags);

	_data = nullptr;
	_partialReals = nullptr;
	_partialImags = nullptr;
}

template<int size, int iterations>
void LineMandelCalculator::calculate()
{
	constexpr int width = size * 3;
	constexpr int height = size * 2;

	// experimentally estimated values based on plots
	constexpr int heightSeparator1 = int(size * 0.400391);
	constexpr int heighSeparator2 = int(size * 0.722657);
	constexpr int rectangleStart = int(size * 1.390625) & (~15);
	constexpr int rectangleEnd = int(size * 2.25) & (~15);

	int *pData = _data;
	float *pReals = _partialReals;
	float *pImags = _partialImags;

	float iFp = 0.0f; // use float instead of int to avoid data conversions

	for (int i = 0; i < heightSeparator1; i++) // compute the first part of a half of the image
	{
		float imag = _yStart + iFp * _dy;
		
		int any = 1; // reduction variable
		for (int k = 0; k < iterations && any; k++) // use the reduction to exit the loop as soon as possible
		{
			float jFp = 0.0f; // use float instead of int to avoid data conversions
			any = 0; 

			#pragma omp simd aligned(pData, pReals, pImags : 64) reduction(+ : any) simdlen(16)
			for (int j = 0; j < width; j++)
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

		// move the pointers to the next line
		pData += width;
		pReals += width;
		pImags += width;

		iFp += 1.0f;
	}

	for (int i = heightSeparator1; i < heighSeparator2; i++) // compute the second part of a half of the image
	{
		float imag = _yStart + iFp * _dy;
		
		for (int k = 0; k < iterations; k++) // always compute all iterations, at least some values will be in the 4.0f bound for all of them
		{
			float jFp = 0.0f; // use float instead of int to avoid data conversions

			#pragma omp simd aligned(pData, pReals, pImags : 64) simdlen(64) // no reduction needed
			for (int j = 0; j < width; j++)
			{
				float real = _xStart + jFp * _dx;

				float r2 = pReals[j] * pReals[j];
				float i2 = pImags[j] * pImags[j];

				pData[j] = r2 + i2 > 4.0f && pData[j] == limit ? k : pData[j];

				pImags[j] = 2.0f * pReals[j] * pImags[j] + imag;
				pReals[j] = r2 - i2 + real;
				jFp += 1.0f;
			}
		}

		pData += width;
		pReals += width;
		pImags += width;
		iFp += 1.0f;
	}

	for (int i = heighSeparator2; i < size; i++) // compute the third part of a half of the image
	{
		float imag = _yStart + iFp * _dy;
		
		for (int k = 0; k < iterations; k++) // always compute all iterations, at least some values will be in the 4.0f bound for all of them
		{
			float jFp = 0.0f; // use float instead of int to avoid data conversions

			#pragma omp simd aligned(pData, pReals, pImags : 64) simdlen(64) // no reduction needed
			for (int j = 0; j < rectangleStart; j++)
			{
				float real = _xStart + jFp * _dx;

				float r2 = pReals[j] * pReals[j];
				float i2 = pImags[j] * pImags[j];

				pData[j] = r2 + i2 > 4.0f && pData[j] == limit ? k : pData[j];

				pImags[j] = 2.0f * pReals[j] * pImags[j] + imag;
				pReals[j] = r2 - i2 + real;
				jFp += 1.0f;
			}
			jFp = float(rectangleEnd);

			#pragma omp simd aligned(pData, pReals, pImags : 64) simdlen(64) // no reduction needed
			for (int j = rectangleEnd; j < width; j++)
			{
				float real = _xStart + jFp * _dx;

				float r2 = pReals[j] * pReals[j];
				float i2 = pImags[j] * pImags[j];

				pData[j] = r2 + i2 > 4.0f && pData[j] == limit ? k : pData[j];

				pImags[j] = 2.0f * pReals[j] * pImags[j] + imag;
				pReals[j] = r2 - i2 + real;
				jFp += 1.0f;
			}
		}

		pData += width;
		pReals += width;
		pImags += width;
		iFp += 1.0f;
	}

	pData = _data;
	#pragma omp simd aligned(pData : 64) simdlen(64)
	for (int i = 0; i < size; i++)
	{
		memcpy(pData + (height - i - 1) * width, pData + i * width, width * sizeof(int)); // mirror the upper part of the image to the lower part
	}
}

int *LineMandelCalculator::calculateMandelbrot()
{	
#if OPTIMIZED_IMPLEMENTATION
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
	else if (_fast && !_solved)
	{
		// see the template function above for detailed comments

		int heightSeparator1 = int(_size * 0.400391);
		int heighSeparator2 = int(_size * 0.722657);
		int rectangleStart = int(_size * 1.390625) & (~15);
		int rectangleEnd = int(_size * 2.25) & (~15);

		int *pData = _data;
		float *pReals = _partialReals;
		float *pImags = _partialImags;

		float iFp = 0.0f;

		for (int i = 0; i < heightSeparator1; i++)
		{
			float imag = _yStart + iFp * _dy;
			
			int any = 1;
			for (int k = 0; k < limit && any; k++)
			{
				float jFp = 0.0f;
				any = 0; 

				#pragma omp simd aligned(pData, pReals, pImags : 64) reduction(+ : any) simdlen(16)
				for (int j = 0; j < _width; j++)
				{
					float real = _xStart + jFp * _dx;

					float r2 = pReals[j] * pReals[j];
					float i2 = pImags[j] * pImags[j];

					pData[j] = r2 + i2 > 4.0f && pData[j] == limit ? k : pData[j];
					any = any + (pData[j] == limit);

					pImags[j] = 2.0f * pReals[j] * pImags[j] + imag;
					pReals[j] = r2 - i2 + real;
					jFp += 1.0f;
				}
			}

			pData += _width;
			pReals += _width;
			pImags += _width;

			iFp += 1.0f;
		}

		for (int i = heightSeparator1; i < heighSeparator2; i++)
		{
			float imag = _yStart + iFp * _dy;
			
			for (int k = 0; k < limit; k++)
			{
				float jFp = 0.0f;

				#pragma omp simd aligned(pData, pReals, pImags : 64) simdlen(64)
				for (int j = 0; j < _width; j++)
				{
					float real = _xStart + jFp * _dx;

					float r2 = pReals[j] * pReals[j];
					float i2 = pImags[j] * pImags[j];

					pData[j] = r2 + i2 > 4.0f && pData[j] == limit ? k : pData[j];

					pImags[j] = 2.0f * pReals[j] * pImags[j] + imag;
					pReals[j] = r2 - i2 + real;
					jFp += 1.0f;
				}
			}

			pData += _width;
			pReals += _width;
			pImags += _width;
			iFp += 1.0f;
		}

		for (int i = heighSeparator2; i < _size; i++)
		{
			float imag = _yStart + iFp * _dy;
			
			for (int k = 0; k < limit; k++)
			{
				float jFp = 0.0f;

				#pragma omp simd aligned(pData, pReals, pImags : 64) simdlen(64)
				for (int j = 0; j < rectangleStart; j++)
				{
					float real = _xStart + jFp * _dx;

					float r2 = pReals[j] * pReals[j];
					float i2 = pImags[j] * pImags[j];

					pData[j] = r2 + i2 > 4.0f && pData[j] == limit ? k : pData[j];

					pImags[j] = 2.0f * pReals[j] * pImags[j] + imag;
					pReals[j] = r2 - i2 + real;
					jFp += 1.0f;
				}
				jFp = float(rectangleEnd);

				#pragma omp simd aligned(pData, pReals, pImags : 64) simdlen(64)
				for (int j = rectangleEnd; j < _width; j++)
				{
					float real = _xStart + jFp * _dx;

					float r2 = pReals[j] * pReals[j];
					float i2 = pImags[j] * pImags[j];

					pData[j] = r2 + i2 > 4.0f && pData[j] == limit ? k : pData[j];

					pImags[j] = 2.0f * pReals[j] * pImags[j] + imag;
					pReals[j] = r2 - i2 + real;
					jFp += 1.0f;
				}
			}

			pData += _width;
			pReals += _width;
			pImags += _width;
			iFp += 1.0f;
		}

		pData = _data;
		#pragma omp simd aligned(pData : 64) simdlen(64)
		for (int i = 0; i < _size; i++)
		{
			memcpy(pData + (height - i - 1) * _width, pData + i * _width, _width * sizeof(int));
		}
	}
#endif
#if BASIC_IMPLEMENTATION
	if (_fast && !_solved)
	{
		int *pData = _data;
		float *pReals = _partialReals;
		float *pImags = _partialImags;

		float iFp = 0.0f; // use float instead of int to avoid data conversions

		for (int i = 0; i < height >> 1; i++) // compute upper part of a half of the image
		{
			float imag = _yStart + iFp * _dy;
			
			int any = 1; // reduction variable
			for (int k = 0; k < limit && any; k++) // use the reduction to exit the loop as soon as possible
			{
				float jFp = 0.0f; // use float instead of int to avoid data conversions
				any = 0; 

				#pragma omp simd aligned(pData, pReals, pImags : 64) reduction(+ : any) simdlen(16)
				for (int j = 0; j < _width; j++)
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

			// move the pointers to the next line
			pData += _width;
			pReals += _width;
			pImags += _width;

			iFp += 1.0f;
		}

		pData = _data;
		#pragma omp simd aligned(pData : 64)
		for (int i = 0; i < height >> 1; i++)
		{
			memcpy(pData + (height - i - 1) * _width, pData + i * _width, _width * sizeof(int)); // mirror the upper part of the image to the lower part
		}
	}
#endif
	else if (!_solved) // fallback implementation if the the matrix is too small
	{
		int *pData = _data;

		// calculate half of the image plus 1 line to ensure proper results for odd heights (although not possible in this project)
		for (int i = 0; i < (height >> 1) + 1; i++) 
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
		for (int i = 0; i < height >> 1; i++)
		{
			memcpy(pData + (height - i - 1) * width, pData + i * width, width * sizeof(int));	// mirror the first half of the matrix to the second half	
		}
	}

	_solved = true;
	return _data;
}
