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

#include "utils/TimeUtils.h"
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
  m_volume = g_settings.m_fVolumeLevel;

  /* open the renderer */
  enum AEStdChLayout chLayout = AE_CH_LAYOUT_2_0;
  switch(g_guiSettings.GetInt("audiooutput.channellayout")) {
    default:
    case 0: chLayout = AE_CH_LAYOUT_2_0; break;
    case 1: chLayout = AE_CH_LAYOUT_2_1; break;
    case 2: chLayout = AE_CH_LAYOUT_3_0; break;
    case 3: chLayout = AE_CH_LAYOUT_3_1; break;
    case 4: chLayout = AE_CH_LAYOUT_4_0; break;
    case 5: chLayout = AE_CH_LAYOUT_4_1; break;
    case 6: chLayout = AE_CH_LAYOUT_5_0; break;
    case 7: chLayout = AE_CH_LAYOUT_5_1; break;
    case 8: chLayout = AE_CH_LAYOUT_7_0; break;
    case 9: chLayout = AE_CH_LAYOUT_7_1; break;
  }
  m_chLayout     = CAEUtil::GetStdChLayout(chLayout);
  m_channelCount = CAEUtil::GetChLayoutCount(m_chLayout);
  CLog::Log(LOGINFO, "CAE::Initialize: Configured speaker layout: %s", CAEUtil::GetStdChLayoutName(chLayout));

  m_renderer = new CALSADirectSound();
  if (!m_renderer->Initialize(NULL, "default", m_chLayout, 48000, 32, false, false, false))
  {
    delete m_renderer;
    m_renderer = NULL;
    return false;
  }

  m_format       = m_renderer->GetAudioFormat();
  m_frameSize    = sizeof(float) * m_channelCount;
  m_convertFn    = CAEConvert::FrFloat(m_format.m_dataFormat);
  m_buffer       = new uint8_t[m_format.m_frameSize * 2];
  m_bufferSize   = 0;

  m_remap.Initialize(m_chLayout, m_format.m_channelLayout, true);

  /* re-intiialize sounds */
  map<const CStdString, CAESound*>::iterator sitt;
  for(sitt = m_sounds.begin(); sitt != m_sounds.end(); ++sitt)
    sitt->second->Initialize();

  /* re-initialize streams */
  list<CAEStream*>::iterator itt;
  for(itt = m_streams.begin(); itt != m_streams.end(); ++itt)
    (*itt)->Initialize();

  m_state = AE_STATE_READY;
  return true;
}

void CAE::DeInitialize()
{
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
    CAEUtil::GetChLayoutStr(channelLayout).c_str()
  );

  CSingleLock lock(m_critSection);
  CAEStream *stream = new CAEStream(dataFormat, sampleRate, channelCount, channelLayout);
  m_streams.push_back(stream);
  return stream;
}

CAESound *CAE::GetSound(CStdString file)
{
  CLog::Log(LOGINFO, "CAE::GetSound - %s", file.c_str());
  CSingleLock lock(m_critSection);
  CAESound *sound;

  /* see if we have a valid sound */
  if ((sound = m_sounds[file]))
  {
    /* increment the reference count */
    ++sound->m_refcount;
    return sound;
  }

  sound = new CAESound(file);
  if (!sound->Initialize())
  {
    delete sound;
    return NULL;
  }

  m_sounds[file] = sound;
  sound->m_refcount = 1;

  return sound;
}

void CAE::PlaySound(CAESound *sound)
{
   SoundState ss = {
      owner: sound,
      frame: 0
   };
   CSingleLock lock(m_critSection);
   m_playing_sounds.push_back(ss);
}

void CAE::FreeSound(CAESound *sound)
{
  CSingleLock lock(m_critSection);
  /* decrement the sound's ref count */
  --sound->m_refcount;
  ASSERT(sound->m_refcount >= 0);

  /* if other processes are using the sound, dont remove it */
  if (sound->m_refcount > 0)
    return;

  /* set the timeout to 30 seconds */
  sound->m_ts = CTimeUtils::GetTimeMS() + 30000;

  /* stop the sound playing */
  list<SoundState>::iterator itt;
  for(itt = m_playing_sounds.begin(); itt != m_playing_sounds.end(); )
  {
    if ((*itt).owner == sound) itt = m_playing_sounds.erase(itt);
    else ++itt;
  }
}

void CAE::GarbageCollect()
{
  CSingleLock lock(m_critSection);

  unsigned int ts = CTimeUtils::GetTimeMS();
  map<const CStdString, CAESound*>::iterator itt;
  list<map<const CStdString, CAESound*>::iterator> remove;

  for(itt = m_sounds.begin(); itt != m_sounds.end(); ++itt)
  {
    CAESound *sound = itt->second;
    /* free any sounds that are no longer used and are > 30 seconds old */
    if (sound->m_refcount == 0 && ts > sound->m_ts)
    {
      delete sound;
      remove.push_back(itt);
      continue;
    }
  }

  /* erase the entries from the map */
  while(!remove.empty())
  {
    m_sounds.erase(remove.front());
    remove.pop_front();
  }
}

void CAE::StopSound(CAESound *sound)
{
  CSingleLock lock(m_critSection);
  list<SoundState>::iterator itt;
  for(itt = m_playing_sounds.begin(); itt != m_playing_sounds.end(); )
  {
    if ((*itt).owner == sound) itt = m_playing_sounds.erase(itt);
    else ++itt;
  }
}

bool CAE::IsPlaying(CAESound *sound)
{
  CSingleLock lock(m_critSection);
  list<SoundState>::iterator itt;
  for(itt = m_playing_sounds.begin(); itt != m_playing_sounds.end(); ++itt)
    if ((*itt).owner == sound) return true;
  return false;
}

void CAE::RemoveStream(CAEStream *stream)
{
  CSingleLock lock(m_critSection);
  m_streams.remove(stream);
}

void CAE::Run()
{
  CSingleLock lock(m_critSection);
  if (!AE.Initialize())
  {
    CLog::Log(LOGERROR, "CAE::Run - Failed to initialize");
    return;
  }

  m_state = AE_STATE_RUN;
  lock.Leave();

  list<CAEStream*>::iterator itt;
  list<SoundState>::iterator sitt;
  CAEStream *stream;
  
  float        out[m_channelCount];
  unsigned int div;
  unsigned int i;

  CLog::Log(LOGINFO, "CAE::Run - Thread Started");
  while(GetState() == AE_STATE_RUN)
  {

    /* this normally only loops once, its not really needed (implement audio events!) */
    while(m_bufferSize >= m_format.m_frameSize)
    {
        int frames = m_bufferSize / m_frameSize;
        float buffer[frames * m_format.m_channelCount];
        m_remap.Remap((float*)m_buffer, buffer, frames);

        /* this call must block! */
        int wrote = m_renderer->AddPackets(buffer, sizeof(buffer));
        if (!wrote) continue;

        wrote /= m_format.m_channelCount;
        wrote *= m_channelCount;
	int left = m_bufferSize - wrote;
        memmove(&m_buffer[0], &m_buffer[wrote], left);
        m_bufferSize -= wrote;
    }

    memset(out, 0, sizeof(out));
    div = 1;

    lock.Enter();
    /* mix in any sounds */
    for(sitt = m_playing_sounds.begin(); sitt != m_playing_sounds.end(); )
    {
      if (m_state != AE_STATE_RUN) break;

      float *frame = (*sitt).owner->GetFrame((*sitt).frame++);
      /* if no more frames, remove it from the list */
      if (frame == NULL)
      {
        sitt = m_playing_sounds.erase(sitt);
        continue;
      }

      /* we still need to take frames when muted */
      if (!g_settings.m_bMute) {
        /* mix the frame into the output */
        float volume = (*sitt).owner->GetVolume();
        for(i = 0; i < m_channelCount; ++i)
          out[i] += frame[i] * volume;
        ++div;
      }

      ++sitt;
    }

    /* mix in any running streams */
    for(itt = m_streams.begin(); itt != m_streams.end(); ++itt)
    {
      if (m_state != AE_STATE_RUN) break;
      stream = *itt;

      /* dont process streams that are paused */
      if (stream->IsPaused()) continue;

      float *frame = stream->GetFrame();
      if (!frame)
        continue;

      /* we still need to take frames when muted */
      if (!g_settings.m_bMute) {
        float volume = stream->GetVolume();
        for(i = 0; i < m_channelCount; ++i)
          out[i] += frame[i] * volume;
        ++div;
      }
    }
    lock.Leave();

    /* if muted just zero the data and continue */
    if (g_settings.m_bMute) {
      memset(&m_buffer[m_bufferSize], 0, sizeof(out));
      m_bufferSize += sizeof(out);
      continue;
    }

    for(i = 0; i < m_channelCount; ++i)
      out[i] *= m_volume;

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

  CLog::Log(LOGINFO, "CAE::Run - Thread Terminating");
  lock.Enter();
  m_renderer->Stop();
  DeInitialize();
  m_state = AE_STATE_INVALID;
  lock.Leave();
}

void CAE::Stop()
{
  CSingleLock lock(m_critSection);
  if (m_state == AE_STATE_READY) return;
  m_state = AE_STATE_STOP;
}

float CAE::GetDelay()
{
  CSingleLock lock(m_critSection);
  if (m_state == AE_STATE_INVALID) return 0.0f;
  return m_renderer->GetDelay() + m_bufferSize / m_frameSize / m_format.m_sampleRate;
}

float CAE::GetVolume()
{
  CSingleLock lock(m_critSection);
  return m_volume;
}

void CAE::SetVolume(float volume)
{
  CSingleLock lock(m_critSection);
  g_settings.m_fVolumeLevel = volume;
  m_volume = volume;
}

