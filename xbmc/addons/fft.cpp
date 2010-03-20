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


#include <math.h>

#include "fft.h"

#ifndef M_PI
#define M_PI  3.1415926535897932384626433832795
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.4142135623730950488016887242097
#endif




void fft( float data[], int nn, int isign )
{
  int n = nn << 1;
  int i, j, m;

  /* bit reversal section */

  j = 1;
  for ( i = 1; i < n; i += 2 )
  {
    if ( j > i )
    {
      swap( data[j], data[i] );
      swap( data[j + 1], data[i + 1] );
    }
    m = nn;
    while ( m >= 2 && j > m )
    {
      j -= m;
      m >>= 1;
    }
    j += m;
  }

  /* Daniel-Lanczos section */

  long double theta, wr, wpr, wpi, wi, wtemp;
  float tempr, tempi;
  int mmax = 2;
  while (n > mmax)
  {
    int istep = mmax << 1;
    theta = isign * ( 2.0 * M_PI / mmax );
    wtemp = sin(0.5 * theta);
    wpr = -2.0 * wtemp * wtemp;
    wpi = sin( theta );
    wr = 1.0;
    wi = 0.0;
    for ( m = 1; m < mmax; m += 2 )
    {
      for ( i = m; i <= n; i += istep )
      {
        j = i + mmax;
        if (j >= n || i >= n)
          break;
        tempr = (float) (wr * data[j] - wi * data[j + 1]);
        tempi = (float) (wr * data[j + 1] + wi * data[j]);
        data[j] = data[i] - tempr;
        data[j + 1] = data[i + 1] - tempi;
        data[i] += tempr;
        data[i + 1] += tempi;
      }
      wr = (wtemp = wr) * wpr - wi * wpi + wr;
      wi = wi * wpr + wtemp * wpi + wi;
    }
    mmax = istep;
  }
}

// By JM - packed 2 channel real fft - returns the amplitudes of the fft array
// data[] is a 2n size array, with interleaved channels, and the fft is returned in data[]
// interleaving is preserved.
void twochannelrfft(float data[], int n)
{
  float rep, rem, aip, aim;
  int nn = n + n;
  int nn1 = nn + 1;
  // data is already packed - do the transform
  fft( data - 1, n , + 1 );

  // now repack the array as needed
  data[0] = data[0] * data[0]; // only need the amplitude squared
  data[1] = data[1] * data[1];
  data[n] = data[n] * data[n];
  data[n + 1] = data[n + 1] * data[n + 1];
  // don't need the last component - this is the constant component?

  for (int j = 2; j < n; j += 2)
  {
    rep = (float)(0.5 * (data[j] + data[nn - j]));
    rem = (float)(0.5 * (data[j] - data[nn - j]));
    aip = (float)(0.5 * (data[j + 1] + data[nn1 - j]));
    aim = (float)(0.5 * (data[j + 1] - data[nn1 - j]));
    /* this works out the complex FT
    fft1[j]=rep;
    fft1[j+1]=aim;
    fft1[nn-j]=rep;
    fft1[nn1-j]=-aim;
    fft2[j]=aip;
    fft2[j+1]=-rem;
    fft2[nn-j]=aip;
    fft2[nn1-j]=rem; */ 
    // we just need the amplitudes
    data[j] = (float)(2 * (sqr(rep) + sqr(aim))); // was sqrt'd
    data[j + 1] = (float)(2 * (sqr(rem) + sqr(aip)));
  }
}

void twochanwithwindow(float data[], int n)
{
  float rep, rem, aip, aim;
  int nn = n + n;
  int nn1 = nn + 1;
  // window the data
  float wn;
  for (int i = 0; i < nn; i += 2)
  {
    wn = (float)(0.5 * (1 - cos(M_PI * i / n)));
    data[i] *= wn;
    data[i + 1] *= wn;
  }
  // data is already packed - do the transform
  fft( data - 1, n , + 1 );

  // now repack the array as needed
  data[0] = data[0] * data[0]; // only need the amplitude squared
  data[1] = data[1] * data[1];
  data[n] = data[n] * data[n];
  data[n + 1] = data[n + 1] * data[n + 1];
  // don't need the last component - this is the constant component?

  for (int j = 2; j < n; j += 2)
  {
    rep = data[j] + data[nn - j];
    rem = data[j] - data[nn - j];
    aip = data[j + 1] + data[nn1 - j];
    aim = data[j + 1] - data[nn1 - j];
    data[j] = (float)(0.5 * (sqr(rep) + sqr(aim)));
    data[j + 1] = (float)(0.5 * (sqr(rem) + sqr(aip)));
  }
}

