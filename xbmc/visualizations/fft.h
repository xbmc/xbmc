#ifndef fft_hh
#define fft_hh
/*
 *                            COPYRIGHT
 *
 *  XAnalyser, frequence spectrum analyser for X Window
 *  Copyright (C) 1998 Arvin Schnell
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses:
 *  arvin@informatik.uni-bremen.de
 *  Arvin Schnell, Am Heidberg 8, 28865 Lilienthal, Germany
 *
 */
static __inline long double sqr( long double arg )
{
  return arg * arg;
}


static __inline void swap( float &a, float &b )
{
  float t = a; a = b; b = t;
}

// (complex) fast fourier transformation
// Based on four1() in Numerical Recipes in C, Page 507-508.

// The input in data[1..2*nn] is replaced by its fft or inverse fft, depending
// only on isign (+1 for fft, -1 for inverse fft). The number of complex numbers
// n must be a power of 2 (which is not checked).

void fft( float data[], int nn, int isign );

void twochannelrfft(float data[], int n);
void twochanwithwindow(float data[], int n); // test


#endif
