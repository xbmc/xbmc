/*
    Copyright 2007  Niels Martin Hansen

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    Contact:
    E-mail: <jiifurusu@gmail.com>
    IRC: jfs in #aegisub on irc.rizon.net

 */

#pragma once

#ifdef _OPENMP
#include <omp.h>
#endif
#include <math.h>


// Filter an image in horizontal direction with a one-dimensional filter
// PixelWidth is the distance in bytes between pixels
template<ptrdiff_t PixelDist>
void SeparableFilterX(unsigned char *src, unsigned char *dst, int width, int height, ptrdiff_t stride, int *kernel, int kernel_size, int divisor)
{
#pragma omp parallel for
	for (int y = 0; y < height; y++) {
		unsigned char *in = src + y*stride;
		unsigned char *out = dst + y*stride;
		for (int x = 0; x < width; x++) {
			int accum = 0;
			for (int k = 0; k < kernel_size; k++) {
				int xofs = k - kernel_size/2;
				if (x+xofs < 0) xofs += width;
				if (x+xofs >= width) xofs -= width;
				accum += (int)(in[xofs*PixelDist] * kernel[k]);
			}
			accum /= divisor;
			if (accum > 255) accum = 255;
			if (accum < 0) accum = 0;
			*out = (unsigned char)accum;
			in+=PixelDist;
			out+=PixelDist;
		}
	}
}


// Filter an image in vertical direction with a one-dimensional filter
// This one templated with PixelWidth since the channel interlacing is horizontal only,
// filtering once vertically will automatically catch all channels.
// (Width must be multiplied by pixel width for that to happen though.)
template<ptrdiff_t PixelDist>
void SeparableFilterY(unsigned char *src, unsigned char *dst, int width, int height, ptrdiff_t stride, int *kernel, int kernel_size, int divisor)
{
#pragma omp parallel for
	width *= PixelDist;
	for (int  x = 0; x < width; x+=PixelDist) {
		unsigned char *in = src + x;
		unsigned char *out = dst + x;
		for (int y = 0; y < height; y++) {
			int accum = 0;
			for (int k = 0; k < kernel_size; k++) {
				int yofs = k - kernel_size/2;
				if (y+yofs < 0) yofs += height;
				if (y+yofs >= height) yofs -= height;
				accum += (int)(in[yofs*stride] * kernel[k]);
			}
			accum /= divisor;
			if (accum > 255) accum = 255;
			if (accum < 0) accum = 0;
			*out = (unsigned char)accum;
			in += stride;
			out += stride;
		}
	}
}


static inline double NormalDist(double sigma, double x)
{
	if (sigma <= 0 && x == 0) return 1;
	else if (sigma <= 0) return 0;
	else return exp(-(x*x)/(2*sigma*sigma)) / (sigma * sqrt(2*3.1415926535));
}


struct GaussianKernel {
	int *kernel;
	int width;
	int divisor;
	inline GaussianKernel(double sigma)
	{
		width = (int)(sigma*3 + 0.5) | 1; // binary-or with 1 to make sure the number is odd
		if (width < 3) width = 3;
		kernel = DNew int[width];
		kernel[width/2] = (int)(NormalDist(sigma, 0) * 255);
		divisor = kernel[width/2];
		for (int x = width/2-1; x >= 0; x--) {
			int val = (int)(NormalDist(sigma, width/2-x) * 255 + 0.5);
			divisor += val*2;
			kernel[x] = val;
			kernel[width - x - 1] = val;
		}
	}
	inline ~GaussianKernel()
	{
		delete[] kernel;
	}
};
