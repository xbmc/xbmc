/*
 *      Copyright (C) 2016-2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GameClientTiming.h"
#include "GameClientCallbacks.h"
#include "utils/MathUtils.h"

#include <cmath>

using namespace GAME;

void CGameClientTiming::Reset()
{
  m_framerate = 0.0;
  m_samplerate = 0.0;
  m_audioCorrectionFactor = 1.0;
}

bool CGameClientTiming::NormalizeAudio(IGameAudioCallback* audio)
{
  m_audioCorrectionFactor = audio->NormalizeSamplerate(static_cast<unsigned int>(m_samplerate)) / m_samplerate;

  const double correctionPercent = std::abs(m_audioCorrectionFactor - 1.0) * 100;

  return correctionPercent < MAX_CORRECTION_FACTOR_PERCENT;
}

double CGameClientTiming::GetFrameRate() const 
{
  return m_framerate * m_audioCorrectionFactor;
}

unsigned int CGameClientTiming::GetSampleRate() const 
{
  return MathUtils::round_int(m_samplerate * m_audioCorrectionFactor);
}
