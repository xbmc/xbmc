/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AEAudioFormat.h"

#include <algorithm>

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

    float GetAmplification() const
    {
      return m_amplify;
    }

    void SetSamplerate(int samplerate)
    {
      m_samplerate = (float)samplerate;
    }

    float Run(float* frame[AE_CH_MAX], int channels, int offset = 0, bool planar = false);
};
