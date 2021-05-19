/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AELimiter.h"

#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
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

float CAELimiter::Run(float* frame[AE_CH_MAX], int channels, int offset /*= 0*/, bool planar /*= false*/)
{
  float highest = 0.0f;
  if (!planar)
  {
    for(int i=0; i<channels; i++)
    {
      highest = std::max(highest, fabsf(*(frame[0]+offset+i)));
    }
  }
  else
  {
    for(int i=0; i<channels; i++)
    {
      highest = std::max(highest, fabsf(*(frame[i]+offset)));
    }
  }

  float sample = highest * m_amplify;
  if (sample * m_attenuation > 1.0f)
  {
    m_attenuation = 1.0f / sample;
    m_holdcounter = MathUtils::round_int(static_cast<double>(
        m_samplerate *
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_limiterHold));
    m_increase = powf(std::min(sample, 10000.0f), 1.0f / (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_limiterRelease * m_samplerate));
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

