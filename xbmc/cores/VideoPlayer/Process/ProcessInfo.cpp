/*
 *      Copyright (C) 2005-2016 Team XBMC
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

#include "ProcessInfo.h"
#include "ServiceBroker.h"
#include "cores/DataCacheCore.h"
#include "threads/SingleLock.h"

// Override for platform ports
#if !defined(PLATFORM_OVERRIDE)

CProcessInfo* CProcessInfo::CreateInstance()
{
  return new CProcessInfo();
}

#endif


// base class definitions
CProcessInfo::CProcessInfo()
{

}

CProcessInfo::~CProcessInfo()
{

}

EINTERLACEMETHOD CProcessInfo::GetFallbackDeintMethod()
{
  return VS_INTERLACEMETHOD_DEINTERLACE;
}

bool CProcessInfo::AllowDTSHDDecode()
{
  return true;
}

void CProcessInfo::ResetVideoCodecInfo()
{
  CSingleLock lock(m_videoCodecSection);

  m_videoIsHWDecoder = false;
  m_videoDecoderName = "unknown";
  m_videoDeintMethod = "unknown";
  m_videoPixelFormat = "unknown";
  m_videoWidth = 0;
  m_videoHeight = 0;
  m_videoFPS = 0.0;

  CServiceBroker::GetDataCacheCore().SetVideoDecoderName(m_videoDecoderName, m_videoIsHWDecoder);
  CServiceBroker::GetDataCacheCore().SetVideoDeintMethod(m_videoDeintMethod);
  CServiceBroker::GetDataCacheCore().SetVideoPixelFormat(m_videoPixelFormat);
  CServiceBroker::GetDataCacheCore().SetVideoDimensions(m_videoWidth, m_videoHeight);
  CServiceBroker::GetDataCacheCore().SetVideoFps(m_videoFPS);
}

void CProcessInfo::SetVideoDecoderName(std::string name, bool isHw)
{
  CSingleLock lock(m_videoCodecSection);

  m_videoIsHWDecoder = isHw;
  m_videoDecoderName = name;

  CServiceBroker::GetDataCacheCore().SetVideoDecoderName(m_videoDecoderName, m_videoIsHWDecoder);
}

std::string CProcessInfo::GetVideoDecoderName()
{
  CSingleLock lock(m_videoCodecSection);

  return m_videoDecoderName;
}

bool CProcessInfo::IsVideoHwDecoder()
{
  CSingleLock lock(m_videoCodecSection);

  return m_videoIsHWDecoder;
}

void CProcessInfo::SetVideoDeintMethod(std::string method)
{
  CSingleLock lock(m_videoCodecSection);

  m_videoDeintMethod = method;

  CServiceBroker::GetDataCacheCore().SetVideoDeintMethod(m_videoDeintMethod);
}

std::string CProcessInfo::GetVideoDeintMethod()
{
  CSingleLock lock(m_videoCodecSection);

  return m_videoDeintMethod;
}

void CProcessInfo::SetVideoPixelFormat(std::string pixFormat)
{
  CSingleLock lock(m_videoCodecSection);

  m_videoPixelFormat = pixFormat;

  CServiceBroker::GetDataCacheCore().SetVideoPixelFormat(m_videoPixelFormat);
}

std::string CProcessInfo::GetVideoPixelFormat()
{
  CSingleLock lock(m_videoCodecSection);

  return m_videoPixelFormat;
}

void CProcessInfo::SetVideoDimensions(int width, int height)
{
  CSingleLock lock(m_videoCodecSection);

  m_videoWidth = width;
  m_videoHeight = height;

  CServiceBroker::GetDataCacheCore().SetVideoDimensions(m_videoWidth, m_videoHeight);
}

void CProcessInfo::GetVideoDimensions(int &width, int &height)
{
  CSingleLock lock(m_videoCodecSection);

  width = m_videoWidth;
  height = m_videoHeight;
}

void CProcessInfo::SetVideoFps(float fps)
{
  CSingleLock lock(m_videoCodecSection);

  m_videoFPS = fps;

  CServiceBroker::GetDataCacheCore().SetVideoFps(m_videoFPS);
}

float CProcessInfo::GetVideoFps()
{
  CSingleLock lock(m_videoCodecSection);

  return m_videoFPS;
}

void CProcessInfo::SetVideoDAR(float dar)
{
  CSingleLock lock(m_videoCodecSection);

  m_videoDAR = dar;

  CServiceBroker::GetDataCacheCore().SetVideoDAR(m_videoDAR);
}

float CProcessInfo::GetVideoDAR()
{
  CSingleLock lock(m_videoCodecSection);

  return m_videoDAR;
}

// player audio info
void CProcessInfo::ResetAudioCodecInfo()
{
  CSingleLock lock(m_audioCodecSection);

  m_audioDecoderName = "unknown";
  m_audioChannels = "unknown";
  m_audioSampleRate = 0;;
  m_audioBitsPerSample = 0;

  CServiceBroker::GetDataCacheCore().SetAudioDecoderName(m_audioDecoderName);
  CServiceBroker::GetDataCacheCore().SetAudioChannels(m_audioChannels);
  CServiceBroker::GetDataCacheCore().SetAudioSampleRate(m_audioSampleRate);
  CServiceBroker::GetDataCacheCore().SetAudioBitsPerSample(m_audioBitsPerSample);
}

void CProcessInfo::SetAudioDecoderName(std::string name)
{
  CSingleLock lock(m_audioCodecSection);

  m_audioDecoderName = name;

  CServiceBroker::GetDataCacheCore().SetAudioDecoderName(m_audioDecoderName);
}

std::string CProcessInfo::GetAudioDecoderName()
{
  CSingleLock lock(m_audioCodecSection);

  return m_audioDecoderName;
}

void CProcessInfo::SetAudioChannels(std::string channels)
{
  CSingleLock lock(m_audioCodecSection);

  m_audioChannels = channels;

  CServiceBroker::GetDataCacheCore().SetAudioChannels(m_audioChannels);
}

std::string CProcessInfo::GetAudioChannels()
{
  CSingleLock lock(m_audioCodecSection);

  return m_audioChannels;
}

void CProcessInfo::SetAudioSampleRate(int sampleRate)
{
  CSingleLock lock(m_audioCodecSection);

  m_audioSampleRate = sampleRate;

  CServiceBroker::GetDataCacheCore().SetAudioSampleRate(m_audioSampleRate);
}

int CProcessInfo::GetAudioSampleRate()
{
  CSingleLock lock(m_audioCodecSection);

  return m_audioSampleRate;
}

void CProcessInfo::SetAudioBitsPerSample(int bitsPerSample)
{
  CSingleLock lock(m_audioCodecSection);

  m_audioBitsPerSample = bitsPerSample;

  CServiceBroker::GetDataCacheCore().SetAudioBitsPerSample(m_audioBitsPerSample);
}

int CProcessInfo::GetAudioBitsPerSampe()
{
  CSingleLock lock(m_audioCodecSection);

  return m_audioBitsPerSample;
}

void CProcessInfo::SetRenderClockSync(bool enabled)
{
  CSingleLock lock(m_renderSection);

  m_isClockSync = enabled;

  CServiceBroker::GetDataCacheCore().SetRenderClockSync(enabled);
}

bool CProcessInfo::IsRenderClockSync()
{
  CSingleLock lock(m_renderSection);

  return m_isClockSync;
}
