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

#include "utils/SingleLock.h"
#include "utils/log.h"
#include "GUISettings.h"

#include "AE.h"
#include "AEUtil.h"
#include "AudioRenderers/ALSADirectSound.h"

CAE::CAE():
  m_state   (AE_STATE_INVALID),
  m_renderer(NULL),
  m_buffer  (NULL)
{
}

CAE::~CAE()
{
  DeInitialize();

  /* free the streams */
  while(!m_streams.empty())
  {
    CAEStream *s = m_streams.front();
    /* note: the stream will call RemoveStream via it's dtor */
    delete s;
  }
}

bool CAE::Initialize()
{
  DeInitialize();
  CSingleLock lock(m_critSection);

  /* open the renderer */
  m_chLayout = CAEUtil::GetStdChLayout((enum AEStdChLayout)g_guiSettings.GetInt("audiooutput.channellayout"));
  m_channelCount = CAEUtil::GetChLayoutCount(m_chLayout);
  CLog::Log(LOGINFO, "CAE::Initialize: Configured speaker layout: %s", CAEUtil::GetStdChLayoutName((enum AEStdChLayout)g_guiSettings.GetInt("audiooutput.channellayout")));

  m_renderer = new CALSADirectSound();
  if (!m_renderer->Initialize(NULL, "default", m_chLayout, 48000, 32, false, false, false))
  {
    delete m_renderer;
    m_renderer = NULL;
    return false;
  }

  m_frameSize    = sizeof(float) * m_channelCount;
  m_format       = m_renderer->GetAudioFormat();
  m_convertFn    = CAEConvert::FrFloat(m_format.m_dataFormat);
  m_buffer       = new uint8_t[m_format.m_frameSize];
  m_bufferSize   = 0;

  m_remap.Initialize(m_chLayout, m_format.m_channelLayout, true);
  m_state        = AE_STATE_READY;

  list<CAEStream*>::iterator itt;
  for(itt = m_streams.begin(); itt != m_streams.end(); ++itt)
    (*itt)->Initialize();

  return true;
}

void CAE::DeInitialize()
{
  enum AEState s = GetState();
  if (s == AE_STATE_INVALID) return;
  if (s == AE_STATE_RUN) Stop();

  CSingleLock lock(m_critSection);

  m_state = AE_STATE_SHUTDOWN;
  if (m_renderer)
  {
    m_renderer->Deinitialize();
    delete m_renderer;
    m_renderer = NULL;
  }

  delete[] m_buffer;
  m_buffer = NULL;

  m_state = AE_STATE_INVALID;
}

enum AEState CAE::GetState()
{
  CSingleLock lock(m_critSection);
  return m_state;
}

CAEStream *CAE::GetStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout)
{
  CLog::Log(LOGINFO, "CAE::GetStream - %d, %u, %u, %s",
    CAEUtil::DataFormatToBits(dataFormat),
    sampleRate,
    channelCount,
    CAEUtil::GetChLayoutStr(channelLayout)
  );

  CSingleLock lock(m_critSection);
  CAEStream *stream = new CAEStream(dataFormat, sampleRate, channelCount, channelLayout);
  m_streams.push_back(stream);
  return stream;
}

void CAE::RemoveStream(CAEStream *stream)
{
  CSingleLock lock(m_critSection);
  m_streams.remove(stream);
}

void CAE::Run()
{
  list<CAEStream*>::iterator itt;
  CAEStream *stream;
  
  float        out[m_channelCount];
  unsigned int div;
  unsigned int i;

  CSingleLock lock(m_critSection);
  m_state = AE_STATE_RUN;
  lock.Leave();

  while(GetState() == AE_STATE_RUN)
  {

    /* this normally only loops once, its not really needed (implement audio events!) */
    while(m_bufferSize >= m_format.m_frameSize)
    {
        int frames = m_bufferSize / m_frameSize;
        float buffer[frames * m_format.m_channelCount];
        m_remap.Remap((float*)m_buffer, buffer, frames);

        int wrote = m_renderer->AddPackets(buffer, sizeof(buffer));
        if (!wrote)
		continue;

        wrote /= m_format.m_channelCount;
        wrote *= m_channelCount;
	int left  = m_bufferSize - wrote;
        memmove(&m_buffer[0], &m_buffer[wrote], left);
        m_bufferSize -= wrote;
    }

    memset(out, 0, sizeof(out));
    div = 1;

    lock.Enter();
    for(itt = m_streams.begin(); itt != m_streams.end(); ++itt)
    {
      if (m_state != AE_STATE_RUN) break;
      stream = *itt;

      /* dont process streams that are paused */
      if (stream->IsPaused()) continue;

      float *frame = stream->GetFrame();
      if (!frame)
        continue;

      for(i = 0; i < m_channelCount; ++i)
        out[i] += frame[i];

      ++div;
    }
    lock.Leave();

    if (div > 1)
    {
      float mul = 1.0f / div;
      for(i = 0; i < m_channelCount; ++i)
        out[i] *= mul;
    }

    /* do we need to convert */
    if (m_convertFn)
      m_bufferSize += m_convertFn(out, m_channelCount, &m_buffer[m_bufferSize]);
    else
    {
      memcpy(&m_buffer[m_bufferSize], out, sizeof(out));
      m_bufferSize += sizeof(out);
    }
  }

  m_renderer->Stop();
  lock.Enter();
  m_state = AE_STATE_READY;
  lock.Leave();
}

void CAE::Stop()
{
  CSingleLock lock(m_critSection);
  if (m_state == AE_STATE_READY) return;
  m_state = AE_STATE_STOP;
  lock.Leave();
}

float CAE::GetDelay()
{
  CSingleLock lock(m_critSection);
  if (m_state == AE_STATE_INVALID) return 0.0f;

  return m_renderer->GetDelay() + m_bufferSize / m_frameSize / m_format.m_sampleRate;
}


