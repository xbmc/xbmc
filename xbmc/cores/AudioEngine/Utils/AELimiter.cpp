/*
 *      Copyright (C) 2010-2012 Team XBMC
 *      http://xbmc.org
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

#include "system.h"
#include "AELimiter.h"
#include "settings/AdvancedSettings.h"
#include "utils/MathUtils.h"
#include <algorithm>
#include <math.h>

CAELimiter::CAELimiter()
{
  m_amplify = 1.0f;
  m_attenuation = 1.0f;
  m_samplerate = 48000.0f;
  m_holdcounter = 0;
  m_increase = 0.0f;
}

float CAELimiter::Run(float* frame, int channels)
{
  float* end = frame + channels;
  float highest = 0.0f;
  while (frame != end)
    highest = std::max(highest, fabsf(*(frame++)));

  float sample = highest * m_amplify;
  if (sample * m_attenuation > 1.0f)
  {
    m_attenuation = 1.0f / sample;
    m_holdcounter = MathUtils::round_int(m_samplerate * g_advancedSettings.m_limiterHold);
    m_increase = powf(std::min(sample, 10000.0f), 1.0f / (g_advancedSettings.m_limiterRelease * m_samplerate));
  }

  float attenuation = m_attenuation;

  if (m_holdcounter > 0)
  {
    m_holdcounter--;
  }
  else
  {
    if (m_increase > 0.0f)
    {
      m_attenuation *= m_increase;
      if (m_attenuation > 1.0f)
      {
        m_increase = 0.0f;
        m_attenuation = 1.0f;
      }
    }
  }

  return attenuation * m_amplify;
}

