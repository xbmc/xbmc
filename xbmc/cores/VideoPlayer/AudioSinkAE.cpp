/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AudioSinkAE.h"

#include "DVDClock.h"
#include "DVDCodecs/Audio/DVDAudioCodec.h"
#include "ServiceBroker.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/AudioEngine/Utils/AEAudioFormat.h"
#include "cores/AudioEngine/Utils/AEStreamData.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include <mutex>

using namespace std::chrono_literals;

CAudioSinkAE::CAudioSinkAE(CDVDClock *clock) : m_pClock(clock)
{
  m_bPassthrough = false;
  m_iBitsPerSample = 0;
  m_sampleRate = 0;
  m_bPaused = true;
  m_playingPts = DVD_NOPTS_VALUE; //silence coverity uninitialized warning, is set elsewhere
  m_timeOfPts = 0.0; //silence coverity uninitialized warning, is set elsewhere
  m_syncError = 0.0;
  m_syncErrorTime = 0;
}

CAudioSinkAE::~CAudioSinkAE()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
}

bool CAudioSinkAE::Create(const DVDAudioFrame &audioframe, AVCodecID codec, bool needresampler)
{
  CLog::Log(LOGINFO, "Creating audio stream (codec id: {}, channels: {}, sample rate: {}, {})",
            codec, audioframe.format.m_channelLayout.Count(), audioframe.format.m_sampleRate,
            audioframe.passthrough ? "pass-through" : "no pass-through");

  // if passthrough isset do something else
  std::unique_lock<CCriticalSection> lock(m_critSection);
  unsigned int options = needresampler && !audioframe.passthrough ? AESTREAM_FORCE_RESAMPLE : 0;
  options |= AESTREAM_PAUSED;

  AEAudioFormat format = audioframe.format;
  m_pAudioStream = CServiceBroker::GetActiveAE()->MakeStream(
    format,
    options,
    this
  );
  if (!m_pAudioStream)
    return false;

  m_dataFormat = audioframe.format.m_dataFormat;
  m_sampleRate = audioframe.format.m_sampleRate;
  m_iBitsPerSample = audioframe.bits_per_sample;
  m_bPassthrough = audioframe.passthrough;
  m_channelLayout = audioframe.format.m_channelLayout;
  m_dataType = audioframe.format.m_streamInfo.m_type;

  return true;
}

void CAudioSinkAE::Destroy(bool finish)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (m_pAudioStream)
  {
    m_pAudioStream.get_deleter().setFinish(finish);
    m_pAudioStream.reset();
  }

  m_pAudioStream = NULL;
  m_sampleRate = 0;
  m_iBitsPerSample = 0;
  m_bPassthrough = false;
  m_bPaused = true;
  m_playingPts = DVD_NOPTS_VALUE;
}

unsigned int CAudioSinkAE::AddPackets(const DVDAudioFrame &audioframe)
{
  m_bAbort = false;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (!m_pAudioStream)
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

  // Calculate a timeout when this definitely should be done
  double timeout;
  timeout  = DVD_SEC_TO_TIME(m_pAudioStream->GetDelay()) + audioframe.duration;
  timeout += DVD_SEC_TO_TIME(1.0);
  timeout += m_pClock->GetAbsoluteClock();
  timeout *= m_pClock->GetClockSpeed();

  unsigned int total = audioframe.nb_frames - audioframe.framesOut;
  unsigned int frames = total;
  unsigned int offset = audioframe.framesOut;
  do
  {
    IAEStream::ExtData ext;
    if (offset == 0)
    {
      ext.pts = audioframe.pts / DVD_TIME_BASE * 1000;
    }
    if (audioframe.hasDownmix)
    {
      ext.hasDownmix = true;
      ext.centerMixLevel = audioframe.centerMixLevel;
    }
    unsigned int copied = m_pAudioStream->AddData(audioframe.data, offset, frames, &ext);
    offset += copied;
    frames -= copied;
    if (frames <= 0)
      break;

    if (copied == 0 && timeout < m_pClock->GetAbsoluteClock())
    {
      CLog::Log(LOGERROR, "CDVDAudio::AddPacketsRenderer - timeout adding data to renderer");
      break;
    }

    lock.unlock();
    KODI::TIME::Sleep(1ms);
    lock.lock();
  } while (!m_bAbort);

  m_playingPts = audioframe.pts + audioframe.duration - GetDelay();
  m_timeOfPts = m_pClock->GetAbsoluteClock();

  return total - frames;
}

void CAudioSinkAE::Drain()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_pAudioStream)
    m_pAudioStream->Drain(true);
}

void CAudioSinkAE::SetVolume(float volume)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_pAudioStream)
    m_pAudioStream->SetVolume(volume);
}

void CAudioSinkAE::SetDynamicRangeCompression(long drc)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_pAudioStream)
    m_pAudioStream->SetAmplification(powf(10.0f, (float)drc / 2000.0f));
}

void CAudioSinkAE::Pause()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_pAudioStream)
    m_pAudioStream->Pause();
  CLog::Log(LOGDEBUG,"CDVDAudio::Pause - pausing audio stream");
  m_playingPts = DVD_NOPTS_VALUE;
}

void CAudioSinkAE::Resume()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_pAudioStream)
    m_pAudioStream->Resume();
  CLog::Log(LOGDEBUG,"CDVDAudio::Resume - resume audio stream");
}

double CAudioSinkAE::GetDelay()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  double delay = 0.3;
  if(m_pAudioStream)
    delay = m_pAudioStream->GetDelay();

  return delay * DVD_TIME_BASE;
}

void CAudioSinkAE::Flush()
{
  m_bAbort = true;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_pAudioStream)
  {
    m_pAudioStream->Flush();
    CLog::Log(LOGDEBUG,"CDVDAudio::Flush - flush audio stream");
  }
  m_playingPts = DVD_NOPTS_VALUE;
  m_syncError = 0.0;
  m_syncErrorTime = 0;
}

void CAudioSinkAE::AbortAddPackets()
{
  m_bAbort = true;
}

bool CAudioSinkAE::IsValidFormat(const DVDAudioFrame &audioframe)
{
  if (!m_pAudioStream)
    return false;

  if (audioframe.passthrough != m_bPassthrough)
    return false;

  if (m_dataFormat != audioframe.format.m_dataFormat ||
      m_sampleRate != audioframe.format.m_sampleRate ||
      m_iBitsPerSample != audioframe.bits_per_sample ||
      m_channelLayout != audioframe.format.m_channelLayout)
    return false;

  if (m_bPassthrough &&
      m_dataType != audioframe.format.m_streamInfo.m_type)
    return false;

  return true;
}

double CAudioSinkAE::GetCacheTime()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (!m_pAudioStream)
    return 0.0;

  return m_pAudioStream->GetCacheTime();
}

double CAudioSinkAE::GetCacheTotal()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (!m_pAudioStream)
    return 0.0;
  return m_pAudioStream->GetCacheTotal();
}

double CAudioSinkAE::GetMaxDelay()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (!m_pAudioStream)
    return 0.0;
  return m_pAudioStream->GetMaxDelay();
}

double CAudioSinkAE::GetPlayingPts()
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

double CAudioSinkAE::GetSyncError()
{
  return m_syncError;
}

void CAudioSinkAE::SetSyncErrorCorrection(double correction)
{
  m_syncError += correction;
}

double CAudioSinkAE::GetResampleRatio()
{
  return m_resampleRatio;
}

void CAudioSinkAE::SetResampleMode(int mode)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if(m_pAudioStream)
  {
    m_pAudioStream->SetResampleMode(mode);
  }
}

double CAudioSinkAE::GetClock()
{
  if (m_pClock)
    return (m_pClock->GetClock() + m_pClock->GetVsyncAdjust()) / DVD_TIME_BASE * 1000;
  else
    return 0.0;
}

double CAudioSinkAE::GetClockSpeed()
{
  if (m_pClock)
    return m_pClock->GetClockSpeed();
  else
    return 1.0;
}

CAEStreamInfo::DataType CAudioSinkAE::GetPassthroughStreamType(AVCodecID codecId,
                                                               int samplerate,
                                                               int profile)
{
  AEAudioFormat format;
  format.m_dataFormat = AE_FMT_RAW;
  format.m_sampleRate = samplerate;
  format.m_streamInfo.m_type = CAEStreamInfo::DataType::STREAM_TYPE_NULL;
  switch (codecId)
  {
    case AV_CODEC_ID_AC3:
      format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_AC3;
      format.m_streamInfo.m_sampleRate = samplerate;
      break;

    case AV_CODEC_ID_EAC3:
      format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_EAC3;
      format.m_streamInfo.m_sampleRate = samplerate;
      break;

    case AV_CODEC_ID_DTS:
      if (profile == FF_PROFILE_DTS_HD_HRA)
        format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_DTSHD;
      else if (profile == FF_PROFILE_DTS_HD_MA)
        format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_DTSHD_MA;
      else
        format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_DTSHD_CORE;
      format.m_streamInfo.m_sampleRate = samplerate;
      break;

    case AV_CODEC_ID_TRUEHD:
      format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_TRUEHD;
      format.m_streamInfo.m_sampleRate = samplerate;
      break;

    default:
      format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_NULL;
  }

  bool supports = CServiceBroker::GetActiveAE()->SupportsRaw(format);

  if (!supports && codecId == AV_CODEC_ID_DTS &&
      format.m_streamInfo.m_type != CAEStreamInfo::STREAM_TYPE_DTSHD_CORE &&
      CServiceBroker::GetActiveAE()->UsesDtsCoreFallback())
  {
    format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_DTSHD_CORE;
    supports = CServiceBroker::GetActiveAE()->SupportsRaw(format);
  }

  if (supports)
    return format.m_streamInfo.m_type;
  else
    return CAEStreamInfo::DataType::STREAM_TYPE_NULL;
}
