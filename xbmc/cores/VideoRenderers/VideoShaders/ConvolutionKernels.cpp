/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "ConvolutionKernels.h"
#include "system.h"
#include "MathUtils.h"

#define PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679
#define SINC(x) (sin(PI * (x)) / (PI * (x)))

CConvolutionKernel::CConvolutionKernel(ESCALINGMETHOD method, int size)
{
  m_pixels = NULL;

  if (method == VS_SCALINGMETHOD_LANCZOS2)
    Lanczos2(size);
  else if (method == VS_SCALINGMETHOD_LANCZOS3)
    Lanczos3(size);
  else if (method == VS_SCALINGMETHOD_CUBIC) 
    Bicubic(size, 1.0 / 3.0, 1.0 / 3.0);
}

CConvolutionKernel::~CConvolutionKernel()
{
  delete [] m_pixels;
}

//generate a lanczos2 kernel which can be loaded with RGBA format
//each value of RGBA has one tap, so a shader can load 4 taps with a single pixel lookup
void CConvolutionKernel::Lanczos2(int size)
{
  m_pixels = new float[size * 4];

  for (int i = 0; i < size; i++)
  {
    double x = (double)i / (double)size;

    //generate taps
    for (int j = 0; j < 4; j++)
      m_pixels[i * 4 + j] = LanczosWeight(x + (double)(j - 2), 2.0);

    //any collection of 4 taps added together needs to be exactly 1.0
    //for lanczos this is not always the case, so we take each collection of 4 taps
    //and divide those taps by the sum of the taps
    float weight = 0.0;
    for (int j = 0; j < 4; j++)
      weight += m_pixels[i * 4 + j];

    for (int j = 0; j < 4; j++)
      m_pixels[i * 4 + j] /= weight;
  }
}

//generate a lanczos3 kernel which can be loaded with RGBA format
//each value of RGB has one tap, so a shader can load 3 taps with a single pixel lookup
void CConvolutionKernel::Lanczos3(int size)
{
  m_pixels = new float[size * 4];

  for (int i = 0; i < size; i++)
  {
    double x = (double)i / (double)size;

    //generate taps
    for (int j = 0; j < 3; j++)
      m_pixels[i * 4 + j] = LanczosWeight(x * 2.0 + (double)(j * 2 - 3), 3.0);

    m_pixels[i * 4 + 3] = 0.0;
  }

  //any collection of 6 taps added together needs to be exactly 1.0
  //for lanczos this is not always the case, so we take each collection of 6 taps
  //and divide those taps by the sum of the taps
  for (int i = 0; i < size / 2; i++)
  {
    float weight = 0.0;
    for (int j = 0; j < 3; j++)
    {
      weight += m_pixels[i * 4 + j];
      weight += m_pixels[(i + size / 2) * 4 + j];
    }
    for (int j = 0; j < 3; j++)
    {
      m_pixels[i * 4 + j] /= weight;
      m_pixels[(i + size / 2) * 4 + j] /= weight;
    }
  }
}

//generate a bicubic kernel which can be loaded with RGBA format
//each value of RGBA has one tap, so a shader can load 4 taps with a single pixel lookup
void CConvolutionKernel::Bicubic(int size, double B, double C)
{
  m_pixels = new float[size * 4];

  for (int i = 0; i < size; i++)
  {
    double x = (double)i / (double)size;

    //generate taps
    for (int j = 0; j < 4; j++)
      m_pixels[i * 4 + j] = BicubicWeight(x + (double)(j - 2), B, C);
  }
}

double CConvolutionKernel::LanczosWeight(double x, double radius)
{
  double ax = fabs(x);

  if (ax == 0.0)
    return 1.0;
  else if (ax < radius)
    return SINC(ax) * SINC(ax / radius);
  else
    return 0.0;
}

double CConvolutionKernel::BicubicWeight(double x, double B, double C)
{
  double ax = fabs(x);

  if (ax<1.0)
  {
    return ((12 - 9*B - 6*C) * ax * ax * ax +
            (-18 + 12*B + 6*C) * ax * ax +
            (6 - 2*B))/6;
  }
  else if (ax<2.0)
  {
    return ((-B - 6*C) * ax * ax * ax + 
            (6*B + 30*C) * ax * ax + (-12*B - 48*C) * 
             ax + (8*B + 24*C)) / 6;
  }
  else
  {
    return 0.0;
  }
}

