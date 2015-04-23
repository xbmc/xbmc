/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "threads/SingleLock.h"
#include "utils/log.h"
#include "DVDAudio.h"
#include "DVDClock.h"
#include "DVDCodecs/Audio/DVDAudioCodec.h"
#include "cores/AudioEngine/AEFactory.h"
#include "cores/AudioEngine/Interfaces/AEStream.h"
#include "settings/MediaSettings.h"

using namespace std;


CDVDAudio::CDVDAudio(volatile bool &bStop)
  : m_bStop(bStop)
{
  m_pAudioStream = NULL;
  m_bPassthrough = false;
  m_iBitsPerSample = 0;
  m_iBitrate = 0;
  m_SecondsPerByte = 0.0;
  m_bPaused = true;
  m_playingPts = DVD_NOPTS_VALUE; //silence coverity uninitialized warning, is set elsewhere
  m_timeOfPts = 0.0; //silence coverity uninitialized warning, is set elsewhere
}

CDVDAudio::~CDVDAudio()
{
  CSingleLock lock (m_critSection);
  if (m_pAudioStream)
    CAEFactory::FreeStream(m_pAudioStream);
}

bool CDVDAudio::Create(const DVDAudioFrame &audioframe, AVCodecID codec, bool needresampler)
{
  CLog::Log(LOGNOTICE,
    "Creating audio stream (codec id: %i, channels: %i, sample rate: %i, %s)",
    codec,
    audioframe.channel_count,
    audioframe.sample_rate,
    audioframe.passthrough ? "pass-through" : "no pass-through"
  );

  // if passthrough isset do something else
  CSingleLock lock(m_critSection);
  unsigned int options = needresampler && !audioframe.passthrough ? AESTREAM_FORCE_RESAMPLE : 0;
  options |= AESTREAM_AUTOSTART;

  m_pAudioStream = CAEFactory::MakeStream(
    audioframe.data_format,
    audioframe.sample_rate,
    audioframe.encoded_sample_rate,
    audioframe.channel_layout,
    options
  );
  if (!m_pAudioStream) return false;

  m_iBitrate       = audioframe.sample_rate;
  m_iBitsPerSample = audioframe.bits_per_sample;
  m_bPassthrough   = audioframe.passthrough;
  m_channelLayout  = audioframe.channel_layout;

  if(m_channelLayout.Count() && m_iBitrate && m_iBitsPerSample)
    m_SecondsPerByte = 1.0 / (m_channelLayout.Count() * m_iBitrate * (m_iBitsPerSample>>3));
  else
    m_SecondsPerByte = 0.0;

  SetDynamicRangeCompression((long)(CMediaSettings::Get().GetCurrentVideoSettings().m_VolumeAmplification * 100));

  return true;
}

void CDVDAudio::Destroy()
{
  CSingleLock lock (m_critSection);

  if (m_pAudioStream)
    CAEFactory::FreeStream(m_pAudioStream);

  m_pAudioStream = NULL;
  m_iBitrate = 0;
  m_iBitsPerSample = 0;
  m_bPassthrough = false;
  m_bPaused = true;
  m_playingPts = DVD_NOPTS_VALUE;
}

unsigned int CDVDAudio::AddPackets(const DVDAudioFrame &audioframe)
{
  CSingleLock lock (m_critSection);

  m_playingPts = audioframe.pts - GetDelay();
  m_timeOfPts = CDVDClock::GetAbsoluteClock();

  if(!m_pAudioStream)
    return 0;

  //Calculate a timeout when this definitely should be done
  double timeout;
  timeout  = DVD_SEC_TO_TIME(m_pAudioStream->GetDelay() + audioframe.nb_frames*audioframe.framesize * m_SecondsPerByte);
  timeout += DVD_SEC_TO_TIME(1.0);
  timeout += CDVDClock::GetAbsoluteClock();

  unsigned int total = audioframe.nb_frames;
  unsigned int frames = audioframe.nb_frames;
  unsigned int offset = 0;
  do
  {
    unsigned int copied = m_pAudioStream->AddData(audioframe.data, offset, frames);
    offset += copied;
    frames -= copied;
    if (frames <= 0)
      break;

    if (copied == 0 && timeout < CDVDClock::GetAbsoluteClock())
    {
      CLog::Log(LOGERROR, "CDVDAudio::AddPacketsRenderer - timeout adding data to renderer");
      break;
    }

    lock.Leave();
    Sleep(1);
    lock.Enter();
  } while (!m_bStop);

  return total - frames;
}

void CDVDAudio::Finish()
{
  CSingleLock lock (m_critSection);
  if (!m_pAudioStream)
    return;
}

void CDVDAudio::Drain()
{
  Finish();
  CSingleLock lock (m_critSection);
  if (m_pAudioStream)
    m_pAudioStream->Drain(true);
}

void CDVDAudio::SetVolume(float volume)
{
  CSingleLock lock (m_critSection);
  if (m_pAudioStream) m_pAudioStream->SetVolume(volume);
}

void CDVDAudio::SetDynamicRangeCompression(long drc)
{
  CSingleLock lock (m_critSection);
  if (m_pAudioStream)
    m_pAudioStream->SetAmplification(powf(10.0f, (float)drc / 2000.0f));
}

float CDVDAudio::GetCurrentAttenuation()
{
  CSingleLock lock (m_critSection);
  if (m_pAudioStream)
    return m_pAudioStream->GetVolume();
  else
    return 1.0f;
}

void CDVDAudio::Pause()
{
  CSingleLock lock (m_critSection);
  if (m_pAudioStream)
    m_pAudioStream->Pause();
  m_playingPts = DVD_NOPTS_VALUE;
}

void CDVDAudio::Resume()
{
  CSingleLock lock (m_critSection);
  if (m_pAudioStream) m_pAudioStream->Resume();
}

double CDVDAudio::GetDelay()
{
  CSingleLock lock (m_critSection);

  double delay = 0.3;
  if(m_pAudioStream)
    delay = m_pAudioStream->GetDelay();

  return delay * DVD_TIME_BASE;
}

void CDVDAudio::Flush()
{
  CSingleLock lock (m_critSection);

  if (m_pAudioStream)
  {
    m_pAudioStream->Flush();
  }
  m_playingPts = DVD_NOPTS_VALUE;
}

bool CDVDAudio::IsValidFormat(const DVDAudioFrame &audioframe)
{
  if(!m_pAudioStream)
    return false;

  if(audioframe.passthrough != m_bPassthrough)
    return false;

  if(m_iBitrate       != audioframe.sample_rate
  || m_iBitsPerSample != audioframe.bits_per_sample
  || m_channelLayout  != audioframe.channel_layout)
    return false;

  return true;
}

void CDVDAudio::SetResampleRatio(double ratio)
{
  CSingleLock lock (m_critSection);

  if(m_pAudioStream)
    m_pAudioStream->SetResampleRatio(ratio);
}

double CDVDAudio::GetCacheTime()
{
  CSingleLock lock (m_critSection);
  if(!m_pAudioStream)
    return 0.0;

  double delay = 0.0;
  if(m_pAudioStream)
    delay = m_pAudioStream->GetCacheTime();

  return delay;
}

double CDVDAudio::GetCacheTotal()
{
  CSingleLock lock (m_critSection);
  if(!m_pAudioStream)
    return 0.0;
  return m_pAudioStream->GetCacheTotal();
}

void CDVDAudio::SetPlayingPts(double pts)
{
  CSingleLock lock(m_critSection);
  m_playingPts = pts - GetDelay();
  m_timeOfPts = CDVDClock::GetAbsoluteClock();
}

double CDVDAudio::GetPlayingPts()
{
  if (m_playingPts == DVD_NOPTS_VALUE)
    return 0.0;

  double now = CDVDClock::GetAbsoluteClock();
  double diff = now - m_timeOfPts;
  double cache = GetCacheTime();
  double played = 0.0;

  if (diff < cache)
    played = diff;
  else
    played = cache;

  m_timeOfPts = now;
  m_playingPts += played;
  return m_playingPts;
}
