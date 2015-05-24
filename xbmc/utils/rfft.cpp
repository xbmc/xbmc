/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "rfft.h"

#if defined(TARGET_WINDOWS) && !defined(_USE_MATH_DEFINES)
#define _USE_MATH_DEFINES
#endif
#include <math.h>

RFFT::RFFT(int size, bool windowed) :
  m_size(size), m_windowed(windowed)
{
  m_cfg = kiss_fftr_alloc(m_size,0,nullptr,nullptr);
}

RFFT::~RFFT()
{
  // we don' use kiss_fftr_free here because
  // its hardcoded to free and doesn't pay attention
  // to SIMD (which might be used during kiss_fftr_alloc
  //in the C'tor).
  KISS_FFT_FREE(m_cfg);
}

void RFFT::calc(const float* input, float* output)
{
  // temporary buffers
  std::vector<kiss_fft_scalar> linput(m_size), rinput(m_size);
  std::vector<kiss_fft_cpx> loutput(m_size), routput(m_size);

  for (size_t i=0;i<m_size;++i)
  {
    linput[i] = input[2*i];
    rinput[i] = input[2*i+1];
  }

  if (m_windowed)
  {
    hann(linput);
    hann(rinput);
  }

  // transform channels
  kiss_fftr(m_cfg, &linput[0], &loutput[0]);
  kiss_fftr(m_cfg, &rinput[0], &routput[0]);

  auto&& filter = [&](kiss_fft_cpx& data)
  {
    return sqrt(data.r*data.r+data.i*data.i) * 2.0/m_size * (m_windowed?sqrt(8.0/3.0):1.0);
  };

  // interleave while taking magnitudes and normalizing
  for (size_t i=0;i<m_size/2;++i)
  {
    output[2*i] = filter(loutput[i]);
    output[2*i+1] = filter(routput[i]);
  }
}

#include <iostream>

void RFFT::hann(std::vector<kiss_fft_scalar>& data)
{
  for (size_t i=0;i<data.size();++i)
    data[i] *= 0.5*(1.0-cos(2*M_PI*i/(data.size()-1)));
}
