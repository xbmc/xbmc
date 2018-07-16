/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoSettings.h"

#include <stdint.h>

class CConvolutionKernel
{
  public:
    CConvolutionKernel(ESCALINGMETHOD method, int size);
    ~CConvolutionKernel();

    int      GetSize()           { return m_size; }
    float*   GetFloatPixels()    { return m_floatpixels; }
    uint8_t* GetIntFractPixels() { return m_intfractpixels; }
    uint8_t* GetUint8Pixels()    { return m_uint8pixels; }

  private:
    CConvolutionKernel(const CConvolutionKernel&) = delete;
    CConvolutionKernel& operator=(const CConvolutionKernel&) = delete;
    void Lanczos2();
    void Lanczos3Fast();
    void Lanczos3();
    void Spline36Fast();
    void Spline36();
    void Bicubic(double B, double C);

    static double LanczosWeight(double x, double radius);
    static double Spline36Weight(double x);
    static double BicubicWeight(double x, double B, double C);

    void ToIntFract();
    void ToUint8();

    int      m_size;
    float*   m_floatpixels;
    uint8_t* m_intfractpixels;
    uint8_t* m_uint8pixels;
};
