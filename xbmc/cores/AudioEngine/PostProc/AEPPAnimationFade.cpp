/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "AEPPAnimationFade.h"
#include "AE.h"

bool CAEPPAnimationFade::Initialize(CAEStream *stream)
{
  m_sampleRate   = AE.GetSampleRate();
  m_channelCount = AE.GetChannelCount();
  m_step         = (m_to - m_from) / m_length;
  m_running      = false;
  return true;
}

void CAEPPAnimationFade::Drain()
{
  /* we dont buffer, nothing to do */
}

void CAEPPAnimationFade::Process(float *data, unsigned int samples)
{
  /* apply the current level */
  unsigned int s;
  for(s = 0; s < samples; ++s)
    data[s] *= m_position;

  /* if we are not fading, we are done */
  if (!m_running) return;

  /* perform the step */
  m_position += m_step;
  /* clamp the result */
  m_position = std::min(1.0f, std::max(0.0f, m_position));
  /* if we are finished */
  if (m_position > m_to - 0.001 && m_position < m_to + 0.001)
  {
    m_position = m_to;
    m_running  = false;
  }
}

void CAEPPAnimationFade::Run()
{
  m_running  = true;
}

void CAEPPAnimationFade::Stop()
{
  m_running = false;
}

void CAEPPAnimationFade::SetPosition(const float position)
{
  m_position = std::min(1.0f, std::max(0.0f, position));
}

