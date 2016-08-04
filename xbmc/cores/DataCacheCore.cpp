/*
*      Copyright (C) 2005-2014 Team XBMC
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

#include "cores/DataCacheCore.h"
#include "threads/SingleLock.h"
#include "ServiceBroker.h"

CDataCacheCore::CDataCacheCore()
{
  m_hasAVInfoChanges = false;
}

CDataCacheCore& GetInstance()
{
  return CServiceBroker::GetDataCacheCore();
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

int CDataCacheCore::GetAudioBitsPerSampe()
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
