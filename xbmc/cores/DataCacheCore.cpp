/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/DataCacheCore.h"
#include "threads/SingleLock.h"
#include "ServiceBroker.h"

CDataCacheCore::CDataCacheCore() :
  m_playerVideoInfo {},
  m_playerAudioInfo {},
  m_renderInfo {},
  m_stateInfo {}
{
  m_hasAVInfoChanges = false;
}

CDataCacheCore& CDataCacheCore::GetInstance()
{
  return CServiceBroker::GetDataCacheCore();
}

void CDataCacheCore::Reset()
{
  CSingleLock lock(m_stateSection);

  m_stateInfo.m_speed = 1.0;
  m_stateInfo.m_tempo = 1.0;
  m_stateInfo.m_stateSeeking = false;
  m_stateInfo.m_renderGuiLayer = false;
  m_stateInfo.m_renderVideoLayer = false;
  m_playerStateChanged = false;
}

bool CDataCacheCore::HasAVInfoChanges()
{
  bool ret = m_hasAVInfoChanges;
  m_hasAVInfoChanges = false;
  return ret;
}

void CDataCacheCore::SignalVideoInfoChange()
{
  m_hasAVInfoChanges = true;
}

void CDataCacheCore::SignalAudioInfoChange()
{
  m_hasAVInfoChanges = true;
}

void CDataCacheCore::SignalSubtitleInfoChange()
{
  m_hasAVInfoChanges = true;
}

void CDataCacheCore::SetVideoDecoderName(std::string name, bool isHw)
{
  CSingleLock lock(m_videoPlayerSection);

  m_playerVideoInfo.decoderName = name;
  m_playerVideoInfo.isHwDecoder = isHw;
}

std::string CDataCacheCore::GetVideoDecoderName()
{
  CSingleLock lock(m_videoPlayerSection);

  return m_playerVideoInfo.decoderName;
}

bool CDataCacheCore::IsVideoHwDecoder()
{
  CSingleLock lock(m_videoPlayerSection);

  return m_playerVideoInfo.isHwDecoder;
}


void CDataCacheCore::SetVideoDeintMethod(std::string method)
{
  CSingleLock lock(m_videoPlayerSection);

  m_playerVideoInfo.deintMethod = method;
}

std::string CDataCacheCore::GetVideoDeintMethod()
{
  CSingleLock lock(m_videoPlayerSection);

  return m_playerVideoInfo.deintMethod;
}

void CDataCacheCore::SetVideoPixelFormat(std::string pixFormat)
{
  CSingleLock lock(m_videoPlayerSection);

  m_playerVideoInfo.pixFormat = pixFormat;
}

std::string CDataCacheCore::GetVideoPixelFormat()
{
  CSingleLock lock(m_videoPlayerSection);

  return m_playerVideoInfo.pixFormat;
}

void CDataCacheCore::SetVideoStereoMode(std::string mode)
{
  CSingleLock lock(m_videoPlayerSection);

  m_playerVideoInfo.stereoMode = mode;
}

std::string CDataCacheCore::GetVideoStereoMode()
{
  CSingleLock lock(m_videoPlayerSection);

  return m_playerVideoInfo.stereoMode;
}

void CDataCacheCore::SetVideoDimensions(int width, int height)
{
  CSingleLock lock(m_videoPlayerSection);

  m_playerVideoInfo.width = width;
  m_playerVideoInfo.height = height;
}

int CDataCacheCore::GetVideoWidth()
{
  CSingleLock lock(m_videoPlayerSection);

  return m_playerVideoInfo.width;
}

int CDataCacheCore::GetVideoHeight()
{
  CSingleLock lock(m_videoPlayerSection);

  return m_playerVideoInfo.height;
}

void CDataCacheCore::SetVideoFps(float fps)
{
  CSingleLock lock(m_videoPlayerSection);

  m_playerVideoInfo.fps = fps;
}

float CDataCacheCore::GetVideoFps()
{
  CSingleLock lock(m_videoPlayerSection);

  return m_playerVideoInfo.fps;
}

void CDataCacheCore::SetVideoDAR(float dar)
{
  CSingleLock lock(m_videoPlayerSection);

  m_playerVideoInfo.dar = dar;
}

float CDataCacheCore::GetVideoDAR()
{
  CSingleLock lock(m_videoPlayerSection);

  return m_playerVideoInfo.dar;
}

// player audio info
void CDataCacheCore::SetAudioDecoderName(std::string name)
{
  CSingleLock lock(m_audioPlayerSection);

  m_playerAudioInfo.decoderName = name;
}

std::string CDataCacheCore::GetAudioDecoderName()
{
  CSingleLock lock(m_audioPlayerSection);

  return m_playerAudioInfo.decoderName;
}

void CDataCacheCore::SetAudioChannels(std::string channels)
{
  CSingleLock lock(m_audioPlayerSection);

  m_playerAudioInfo.channels = channels;
}

std::string CDataCacheCore::GetAudioChannels()
{
  CSingleLock lock(m_audioPlayerSection);

  return m_playerAudioInfo.channels;
}

void CDataCacheCore::SetAudioSampleRate(int sampleRate)
{
  CSingleLock lock(m_audioPlayerSection);

  m_playerAudioInfo.sampleRate = sampleRate;
}

int CDataCacheCore::GetAudioSampleRate()
{
  CSingleLock lock(m_audioPlayerSection);

  return m_playerAudioInfo.sampleRate;
}

void CDataCacheCore::SetAudioBitsPerSample(int bitsPerSample)
{
  CSingleLock lock(m_audioPlayerSection);

  m_playerAudioInfo.bitsPerSample = bitsPerSample;
}

int CDataCacheCore::GetAudioBitsPerSample()
{
  CSingleLock lock(m_audioPlayerSection);

  return m_playerAudioInfo.bitsPerSample;
}

void CDataCacheCore::SetRenderClockSync(bool enable)
{
  CSingleLock lock(m_renderSection);

  m_renderInfo.m_isClockSync = enable;
}

bool CDataCacheCore::IsRenderClockSync()
{
  CSingleLock lock(m_renderSection);

  return m_renderInfo.m_isClockSync;
}

// player states
void CDataCacheCore::SetStateSeeking(bool active)
{
  CSingleLock lock(m_stateSection);

  m_stateInfo.m_stateSeeking = active;
  m_playerStateChanged = true;
}

bool CDataCacheCore::IsSeeking()
{
  CSingleLock lock(m_stateSection);

  return m_stateInfo.m_stateSeeking;
}

void CDataCacheCore::SetSpeed(float tempo, float speed)
{
  CSingleLock lock(m_stateSection);

  m_stateInfo.m_tempo = tempo;
  m_stateInfo.m_speed = speed;
}

float CDataCacheCore::GetSpeed()
{
  CSingleLock lock(m_stateSection);

  return m_stateInfo.m_speed;
}

float CDataCacheCore::GetTempo()
{
  CSingleLock lock(m_stateSection);

  return m_stateInfo.m_tempo;
}

void CDataCacheCore::SetFrameAdvance(bool fa)
{
  CSingleLock lock(m_stateSection);

  m_stateInfo.m_frameAdvance = fa;
}

bool CDataCacheCore::IsFrameAdvance()
{
  CSingleLock lock(m_stateSection);

  return m_stateInfo.m_frameAdvance;
}

bool CDataCacheCore::IsPlayerStateChanged()
{
  CSingleLock lock(m_stateSection);

  bool ret(m_playerStateChanged);
  m_playerStateChanged = false;

  return ret;
}

void CDataCacheCore::SetGuiRender(bool gui)
{
  CSingleLock lock(m_stateSection);

  m_stateInfo.m_renderGuiLayer = gui;
  m_playerStateChanged = true;
}

bool CDataCacheCore::GetGuiRender()
{
  CSingleLock lock(m_stateSection);

  return m_stateInfo.m_renderGuiLayer;
}

void CDataCacheCore::SetVideoRender(bool video)
{
  CSingleLock lock(m_stateSection);

  m_stateInfo.m_renderVideoLayer = video;
  m_playerStateChanged = true;
}

bool CDataCacheCore::GetVideoRender()
{
  CSingleLock lock(m_stateSection);

  return m_stateInfo.m_renderVideoLayer;
}

void CDataCacheCore::SetPlayTimes(time_t start, int64_t current, int64_t min, int64_t max)
{
  CSingleLock lock(m_stateSection);
  m_timeInfo.m_startTime = start;
  m_timeInfo.m_time = current;
  m_timeInfo.m_timeMin = min;
  m_timeInfo.m_timeMax = max;
}

void CDataCacheCore::GetPlayTimes(time_t &start, int64_t &current, int64_t &min, int64_t &max)
{
  CSingleLock lock(m_stateSection);
  start = m_timeInfo.m_startTime;
  current = m_timeInfo.m_time;
  min = m_timeInfo.m_timeMin;
  max = m_timeInfo.m_timeMax;
}

time_t CDataCacheCore::GetStartTime()
{
  CSingleLock lock(m_stateSection);
  return m_timeInfo.m_startTime;
}

int64_t CDataCacheCore::GetPlayTime()
{
  CSingleLock lock(m_stateSection);
  return m_timeInfo.m_time;
}

int64_t CDataCacheCore::GetMinTime()
{
  CSingleLock lock(m_stateSection);
  return m_timeInfo.m_timeMin;
}

int64_t CDataCacheCore::GetMaxTime()
{
  CSingleLock lock(m_stateSection);
  return m_timeInfo.m_timeMax;
}

float CDataCacheCore::GetPlayPercentage()
{
  CSingleLock lock(m_stateSection);

  // Note: To calculate accurate percentage, all time data must be consistent,
  //       which is the case for data cache core. Calculation can not be done
  //       outside of data cache core or a possibility to lock the data cache
  //       core from outside would be needed.
  int64_t iTotalTime = m_timeInfo.m_timeMax - m_timeInfo.m_timeMin;
  if (iTotalTime <= 0)
    return 0;

  return m_timeInfo.m_time * 100 / static_cast<float>(iTotalTime);
}
