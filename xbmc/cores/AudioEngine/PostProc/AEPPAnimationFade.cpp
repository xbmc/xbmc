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
#include "AEFactory.h"

CAEPPAnimationFade::CAEPPAnimationFade(float from, float to, unsigned int duration) :
  m_stream  (NULL),
  m_callback(NULL)
{
  m_from     = from;
  m_to       = to;
  m_duration = duration;
}

CAEPPAnimationFade::~CAEPPAnimationFade()
{
}

bool CAEPPAnimationFade::Initialize(IAEStream *stream)
{
  if (m_stream != stream)
    DeInitialize();

  m_stream       = stream;
  m_channelCount = stream->GetChannelCount();
  m_step         = (m_to - m_from) / ((AE.GetSampleRate() / 1000) * (float)m_duration);
  m_running      = false;
  return true;
}

void CAEPPAnimationFade::DeInitialize()
{
  m_stream = NULL;
}

void CAEPPAnimationFade::Flush()
{
  /* we dont buffer, nothing to do */
}

void CAEPPAnimationFade::Process(float *data, unsigned int frames)
{
  /* apply the current level */
  for(unsigned int f = 0; f < frames; ++f)
  {
    for(unsigned int c = 0; c < m_channelCount; ++c, ++data)
      *data *= m_position;

    /* if we are not fading, we are done */
    if (!m_running) continue;

    /* perform the step and clamp the result */
    m_position += m_step;
    m_position = std::min(1.0f, std::max(0.0f, m_position));

    if (
      (m_step > 0.0f && m_position > m_to - m_step && m_position < m_to + m_step) ||
      (m_step < 0.0f && m_position > m_to + m_step && m_position < m_to - m_step)
    )
    {
      m_position = m_to;
      m_running  = false;
      if (m_callback)
        m_callback(this, m_cbArg);
    }
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

void CAEPPAnimationFade::SetDuration(const unsigned int duration)
{
  m_duration = duration;
  m_step     = (m_to - m_from) / ((AE.GetSampleRate() / 1000) * (float)m_duration);
}

