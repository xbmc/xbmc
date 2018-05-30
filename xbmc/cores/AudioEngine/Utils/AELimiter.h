/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#pragma once

#include <algorithm>
#include "AEAudioFormat.h"

class CAELimiter
{
  private:
    float m_amplify;
    float m_attenuation;
    float m_samplerate;
    int   m_holdcounter;
    float m_increase;

  public:
    CAELimiter();

    void SetAmplification(float amplify)
    {
      m_amplify = std::max(std::min(amplify, 1000.0f), 0.0f);
    }

    float GetAmplification()
    {
      return m_amplify;
    }

    void SetSamplerate(int samplerate)
    {
      m_samplerate = (float)samplerate;
    }

    float Run(float* frame[AE_CH_MAX], int channels, int offset = 0, bool planar = false);
};
