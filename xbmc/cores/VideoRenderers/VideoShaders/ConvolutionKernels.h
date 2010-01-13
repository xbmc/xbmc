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

#ifndef CONVOLUTIONKERNELS
#define CONVOLUTIONKERNELS

#include "../../../settings/VideoSettings.h"

class CConvolutionKernel
{
  public:
    CConvolutionKernel(ESCALINGMETHOD method, int size);
    ~CConvolutionKernel();

    float* GetPixels() { return m_pixels; }

  private:

    void Lanczos2(int size);
    void Lanczos3Fast(int size);
    void Lanczos3(int size);
    void Bicubic(int size, double B, double C);

    double LanczosWeight(double x, double radius);
    double BicubicWeight(double x, double B, double C);

    float* m_pixels;
};

#endif //CONVOLUTIONKERNELS
