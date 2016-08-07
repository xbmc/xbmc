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

#include "DVDAudio.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "DVDClock.h"
#include "DVDCodecs/Audio/DVDAudioCodec.h"
#include "cores/AudioEngine/AEFactory.h"
#include "cores/AudioEngine/Utils/AEAudioFormat.h"
#include "settings/MediaSettings.h"
#ifdef TARGET_POSIX
#include "linux/XTimeUtils.h"
#endif

CDVDAudio::CDVDAudio(CDVDClock *clock) : m_pClock(clock)
{
  m_pAudioStream = NULL;
  m_bPassthrough = false;
  m_iBitsPerSample = 0;
  m_sampeRate = 0;
  m_bPaused = true;
  m_playingPts = DVD_NOPTS_VALUE; //silence coverity uninitialized warning, is set elsewhere
  m_timeOfPts = 0.0; //silence coverity uninitialized warning, is set elsewhere
  m_syncError = 0.0;
  m_syncErrorTime = 0;
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
    audioframe.format.m_channelLayout.Count(),
    audioframe.format.m_sampleRate,
    audioframe.passthrough ? "pass-through" : "no pass-through"
  );

  // if passthrough isset do something else
  CSingleLock lock(m_critSection);
  unsigned int options = needresampler && !audioframe.passthrough ? AESTREAM_FORCE_RESAMPLE : 0;
  options |= AESTREAM_PAUSED;

  AEAudioFormat format = audioframe.format;
  m_pAudioStream = CAEFactory::MakeStream(
    format,
    options,
    this
  );
  if (!m_pAudioStream)
    return false;

  m_sampeRate = audioframe.format.m_sampleRate;
  m_iBitsPerSample = audioframe.bits_per_sample;
  m_bPassthrough = audioframe.passthrough;
  m_channelLayout = audioframe.format.m_channelLayout;

  if (m_pAudioStream->HasDSP())
    m_pAudioStream->SetFFmpegInfo(audioframe.profile, audioframe.matrix_encoding, audioframe.audio_service_type);

  SetDynamicRangeCompression((long)(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_VolumeAmplification * 100));

  return true;
}

void CDVDAudio::Destroy()
{
  CSingleLock lock (m_critSection);

  if (m_pAudioStream)
    CAEFactory::FreeStream(m_pAudioStream);

  m_pAudioStream = NULL;
  m_sampeRate = 0;
  m_iBitsPerSample = 0;
  m_bPassthrough = false;
  m_bPaused = true;
  m_playingPts = DVD_NOPTS_VALUE;
}

unsigned int CDVDAudio::AddPackets(const DVDAudioFrame &audioframe)
{
  m_bAbort = false;

  CSingleLock lock (m_critSection);

  if(!m_pAudioStream)
    return 0;

  CAESyncInfo info = m_pAudioStream->GetSyncInfo();
  if (info.state == CAESyncInfo::SYNC_INSYNC)
  {
    unsigned int newTime = info.errortime;
    if (newTime != m_syncErrorTime)
    {
      m_syncErrorTime = info.errortime;
      m_syncError = info.error / 1000 * DVD_TIME_BASE;
      m_resampleRatio = info.rr;
    }
  }
  else
  {
    m_syncErrorTime = 0;
    m_syncError = 0.0;
  }

  //Calculate a timeout when this definitely should be done
  double timeout;
  timeout  = DVD_SEC_TO_TIME(m_pAudioStream->GetDelay()) + audioframe.duration;
  timeout += DVD_SEC_TO_TIME(1.0);
  timeout += m_pClock->GetAbsoluteClock();

  unsigned int total = audioframe.nb_frames;
  unsigned int frames = audioframe.nb_frames;
  unsigned int offset = 0;
  do
  {
    double pts = (offset == 0) ? audioframe.pts / DVD_TIME_BASE * 1000 : 0.0;
    unsigned int copied = m_pAudioStream->AddData(audioframe.data, offset, frames, pts);
    offset += copied;
    frames -= copied;
    if (frames <= 0)
      break;

    if (copied == 0 && timeout < m_pClock->GetAbsoluteClock())
    {
      CLog::Log(LOGERROR, "CDVDAudio::AddPacketsRenderer - timeout adding data to renderer");
      break;
    }

    lock.Leave();
    Sleep(1);
    lock.Enter();
  } while (!m_bAbort);

  m_playingPts = audioframe.pts + audioframe.duration - GetDelay();
  m_timeOfPts = m_pClock->GetAbsoluteClock();

  return total - frames;
}

void CDVDAudio::Drain()
{
  CSingleLock lock (m_critSection);
  if (m_pAudioStream)
    m_pAudioStream->Drain(true);
}

void CDVDAudio::SetVolume(float volume)
{
  CSingleLock lock (m_critSection);
  if (m_pAudioStream)
    m_pAudioStream->SetVolume(volume);
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
  CLog::Log(LOGDEBUG,"CDVDAudio::Pause - pausing audio stream");
  m_playingPts = DVD_NOPTS_VALUE;
}

void CDVDAudio::Resume()
{
  CSingleLock lock(m_critSection);
  if (m_pAudioStream)
    m_pAudioStream->Resume();
  CLog::Log(LOGDEBUG,"CDVDAudio::Resume - resume audio stream");
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
  m_bAbort = true;

  CSingleLock lock (m_critSection);
  if (m_pAudioStream)
  {
    m_pAudioStream->Flush();
    CLog::Log(LOGDEBUG,"CDVDAudio::Flush - flush audio stream");
  }
  m_playingPts = DVD_NOPTS_VALUE;
  m_syncError = 0.0;
  m_syncErrorTime = 0;
}

void CDVDAudio::AbortAddPackets()
{
  m_bAbort = true;
}

bool CDVDAudio::IsValidFormat(const DVDAudioFrame &audioframe)
{
  if(!m_pAudioStream)
    return false;

  if(audioframe.passthrough != m_bPassthrough)
    return false;

  if(m_sampeRate != audioframe.format.m_sampleRate ||
     m_iBitsPerSample != audioframe.bits_per_sample ||
     m_channelLayout != audioframe.format.m_channelLayout)
    return false;

  return true;
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

double CDVDAudio::GetPlayingPts()
{
  if (m_playingPts == DVD_NOPTS_VALUE)
    return 0.0;

  double now = m_pClock->GetAbsoluteClock();
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

double CDVDAudio::GetSyncError()
{
  return m_syncError;
}

void CDVDAudio::SetSyncErrorCorrection(double correction)
{
  m_syncError += correction;
}

double CDVDAudio::GetResampleRatio()
{
  return m_resampleRatio;
}

void CDVDAudio::SetResampleMode(int mode)
{
  CSingleLock lock (m_critSection);
  if(m_pAudioStream)
  {
    m_pAudioStream->SetResampleMode(mode);
  }
}

double CDVDAudio::GetClock()
{
  if (m_pClock)
    return (m_pClock->GetClock() + m_pClock->GetVsyncAdjust()) / DVD_TIME_BASE * 1000;
  else
    return 0.0;
}

double CDVDAudio::GetClockSpeed()
{
  if (m_pClock)
    return m_pClock->GetClockSpeed();
  else
    return 1.0;
}
