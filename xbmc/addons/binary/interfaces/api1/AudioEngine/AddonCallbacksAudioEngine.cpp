/*
 *      Copyright (C) 2014 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include "AddonCallbacksAudioEngine.h"
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_audioengine_types.h"
#include "cores/AudioEngine/AEFactory.h"
#include "cores/AudioEngine/Interfaces/AEStream.h"
#include "cores/AudioEngine/Utils/AEChannelData.h"
#include "utils/log.h"

using namespace ADDON;

namespace V1
{
namespace KodiAPI
{
  
namespace AudioEngine
{

CAddonCallbacksAudioEngine::CAddonCallbacksAudioEngine(CAddon* addon)
  : ADDON::IAddonInterface(addon, 1, KODI_AUDIOENGINE_API_VERSION),
    m_callbacks(new CB_AudioEngineLib)
{
  // write KODI audio DSP specific add-on function addresses to callback table
  m_callbacks->MakeStream           = AudioEngine_MakeStream;
  m_callbacks->FreeStream           = AudioEngine_FreeStream;
  m_callbacks->GetCurrentSinkFormat = AudioEngine_GetCurrentSinkFormat;

  // AEStream add-on function callback table
  m_callbacks->AEStream_GetSpace              = AEStream_GetSpace;
  m_callbacks->AEStream_AddData               = AEStream_AddData;
  m_callbacks->AEStream_GetDelay              = AEStream_GetDelay;
  m_callbacks->AEStream_IsBuffering           = AEStream_IsBuffering;
  m_callbacks->AEStream_GetCacheTime          = AEStream_GetCacheTime;
  m_callbacks->AEStream_GetCacheTotal         = AEStream_GetCacheTotal;
  m_callbacks->AEStream_Pause                 = AEStream_Pause;
  m_callbacks->AEStream_Resume                = AEStream_Resume;
  m_callbacks->AEStream_Drain                 = AEStream_Drain;
  m_callbacks->AEStream_IsDraining            = AEStream_IsDraining;
  m_callbacks->AEStream_IsDrained             = AEStream_IsDrained;
  m_callbacks->AEStream_Flush                 = AEStream_Flush;
  m_callbacks->AEStream_GetVolume             = AEStream_GetVolume;
  m_callbacks->AEStream_SetVolume             = AEStream_SetVolume;
  m_callbacks->AEStream_GetAmplification      = AEStream_GetAmplification;
  m_callbacks->AEStream_SetAmplification      = AEStream_SetAmplification;
  m_callbacks->AEStream_GetFrameSize          = AEStream_GetFrameSize;
  m_callbacks->AEStream_GetChannelCount       = AEStream_GetChannelCount;
  m_callbacks->AEStream_GetSampleRate         = AEStream_GetSampleRate;
  m_callbacks->AEStream_GetDataFormat         = AEStream_GetDataFormat;
  m_callbacks->AEStream_GetResampleRatio      = AEStream_GetResampleRatio;
  m_callbacks->AEStream_SetResampleRatio      = AEStream_SetResampleRatio;
}

AEStreamHandle* CAddonCallbacksAudioEngine::AudioEngine_MakeStream(AudioEngineFormat StreamFormat, unsigned int Options)
{
  AEAudioFormat format;
  format.m_dataFormat = StreamFormat.m_dataFormat;
  format.m_sampleRate = StreamFormat.m_sampleRate;
  format.m_channelLayout = StreamFormat.m_channels;
  return CAEFactory::MakeStream(format, Options);
}

void CAddonCallbacksAudioEngine::AudioEngine_FreeStream(AEStreamHandle *StreamHandle)
{
  if (!StreamHandle)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksAudioEngine - %s - invalid stream data", __FUNCTION__);
    return;
  }

  CAEFactory::FreeStream((IAEStream*)StreamHandle);
}

bool CAddonCallbacksAudioEngine::AudioEngine_GetCurrentSinkFormat(void *AddonData, AudioEngineFormat *SinkFormat)
{
  if (!AddonData || !SinkFormat)
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - invalid input data!", __FUNCTION__);
    return false;
  }

  AEAudioFormat AESinkFormat;
  if (!CAEFactory::GetEngine() || !CAEFactory::GetEngine()->GetCurrentSinkFormat(AESinkFormat))
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - failed to get current sink format from AE!", __FUNCTION__);
    return false;
  }

  SinkFormat->m_channelCount = AESinkFormat.m_channelLayout.Count();
  for (unsigned int ch = 0; ch < SinkFormat->m_channelCount; ch++)
  {
    SinkFormat->m_channels[ch] = AESinkFormat.m_channelLayout[ch];
  }

  SinkFormat->m_dataFormat   = AESinkFormat.m_dataFormat;
  SinkFormat->m_sampleRate   = AESinkFormat.m_sampleRate;
  SinkFormat->m_frames       = AESinkFormat.m_frames;
  SinkFormat->m_frameSize    = AESinkFormat.m_frameSize;

  return true;
}

CAddonCallbacksAudioEngine::~CAddonCallbacksAudioEngine()
{
  /* delete the callback table */
  delete m_callbacks;
}

unsigned int CAddonCallbacksAudioEngine::AEStream_GetSpace(void *AddonData, AEStreamHandle *StreamHandle)
{
  if (!AddonData || !StreamHandle)
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - invalid stream data", __FUNCTION__);
    return 0;
  }

  return ((IAEStream*)StreamHandle)->GetSpace();
}

unsigned int CAddonCallbacksAudioEngine::AEStream_AddData(void *AddonData, AEStreamHandle *StreamHandle, uint8_t* const *Data, unsigned int Offset, unsigned int Frames)
{
  if (!AddonData || !StreamHandle)
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - invalid stream data", __FUNCTION__);
    return 0;
  }

  return ((IAEStream*)StreamHandle)->AddData(Data, Offset, Frames);
}

double CAddonCallbacksAudioEngine::AEStream_GetDelay(void *AddonData, AEStreamHandle *StreamHandle)
{
  if (!AddonData || !StreamHandle)
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - invalid stream data", __FUNCTION__);
    return -1.0;
  }

  return ((IAEStream*)StreamHandle)->GetDelay();
}

bool CAddonCallbacksAudioEngine::AEStream_IsBuffering(void *AddonData, AEStreamHandle *StreamHandle)
{
  if (!AddonData || !StreamHandle)
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - invalid stream data", __FUNCTION__);
    return false;
  }

  return ((IAEStream*)StreamHandle)->IsBuffering();
}

double CAddonCallbacksAudioEngine::AEStream_GetCacheTime(void *AddonData, AEStreamHandle *StreamHandle)
{
  if (!AddonData || !StreamHandle)
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - invalid stream data", __FUNCTION__);
    return -1.0;
  }

  return ((IAEStream*)StreamHandle)->GetCacheTime();
}

double CAddonCallbacksAudioEngine::AEStream_GetCacheTotal(void *AddonData, AEStreamHandle *StreamHandle)
{
  // prevent compiler warnings
  void *addonData = AddonData;
  if (!addonData || !StreamHandle)
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - invalid stream data", __FUNCTION__);
    return -1.0;
  }

  return ((IAEStream*)StreamHandle)->GetCacheTotal();
}

void CAddonCallbacksAudioEngine::AEStream_Pause(void *AddonData, AEStreamHandle *StreamHandle)
{
  // prevent compiler warnings
  void *addonData = AddonData;
  if (!addonData || !StreamHandle)
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - invalid stream data", __FUNCTION__);
    return;
  }

  ((IAEStream*)StreamHandle)->Pause();
}

void CAddonCallbacksAudioEngine::AEStream_Resume(void *AddonData, AEStreamHandle *StreamHandle)
{
  // prevent compiler warnings
  void *addonData = AddonData;
  if (!addonData || !StreamHandle)
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - invalid stream data", __FUNCTION__);
    return;
  }

  ((IAEStream*)StreamHandle)->Resume();
}

void CAddonCallbacksAudioEngine::AEStream_Drain(void *AddonData, AEStreamHandle *StreamHandle, bool Wait)
{
  // prevent compiler warnings
  void *addonData = AddonData;
  if (!addonData || !StreamHandle)
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - invalid stream data", __FUNCTION__);
    return;
  }

  ((IAEStream*)StreamHandle)->Drain(Wait);
}

bool CAddonCallbacksAudioEngine::AEStream_IsDraining(void *AddonData, AEStreamHandle *StreamHandle)
{
  // prevent compiler warnings
  void *addonData = AddonData;
  if (!addonData || !StreamHandle)
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - invalid stream data", __FUNCTION__);
    return false;
  }

  return ((IAEStream*)StreamHandle)->IsDraining();
}

bool CAddonCallbacksAudioEngine::AEStream_IsDrained(void *AddonData, AEStreamHandle *StreamHandle)
{
  // prevent compiler warnings
  void *addonData = AddonData;
  if (!addonData || !StreamHandle)
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - invalid stream data", __FUNCTION__);
    return false;
  }

  return ((IAEStream*)StreamHandle)->IsDrained();
}

void CAddonCallbacksAudioEngine::AEStream_Flush(void *AddonData, AEStreamHandle *StreamHandle)
{
  // prevent compiler warnings
  void *addonData = AddonData;
  if (!addonData || !StreamHandle)
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - invalid stream data", __FUNCTION__);
    return;
  }

  ((IAEStream*)StreamHandle)->Flush();
}

float CAddonCallbacksAudioEngine::AEStream_GetVolume(void *AddonData, AEStreamHandle *StreamHandle)
{
  // prevent compiler warnings
  void *addonData = AddonData;
  if (!addonData || !StreamHandle)
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - invalid stream data", __FUNCTION__);
    return -1.0f;
  }

  return ((IAEStream*)StreamHandle)->GetVolume();
}

void  CAddonCallbacksAudioEngine::AEStream_SetVolume(void *AddonData, AEStreamHandle *StreamHandle, float Volume)
{
  // prevent compiler warnings
  void *addonData = AddonData;
  if (!addonData || !StreamHandle)
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - invalid stream data", __FUNCTION__);
    return;
  }

  ((IAEStream*)StreamHandle)->SetVolume(Volume);
}

float CAddonCallbacksAudioEngine::AEStream_GetAmplification(void *AddonData, AEStreamHandle *StreamHandle)
{
  // prevent compiler warnings
  void *addonData = AddonData;
  if (!addonData || !StreamHandle)
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - invalid stream data", __FUNCTION__);
    return -1.0f;
  }

  return ((IAEStream*)StreamHandle)->GetAmplification();
}

void CAddonCallbacksAudioEngine::AEStream_SetAmplification(void *AddonData, AEStreamHandle *StreamHandle, float Amplify)
{
  // prevent compiler warnings
  void *addonData = AddonData;
  if (!addonData || !StreamHandle)
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - invalid stream data", __FUNCTION__);
    return;
  }

  ((IAEStream*)StreamHandle)->SetAmplification(Amplify);
}

const unsigned int CAddonCallbacksAudioEngine::AEStream_GetFrameSize(void *AddonData, AEStreamHandle *StreamHandle)
{
  // prevent compiler warnings
  void *addonData = AddonData;
  if (!addonData || !StreamHandle)
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - invalid stream data", __FUNCTION__);
    return 0;
  }

  return ((IAEStream*)StreamHandle)->GetFrameSize();
}

const unsigned int CAddonCallbacksAudioEngine::AEStream_GetChannelCount(void *AddonData, AEStreamHandle *StreamHandle)
{
  // prevent compiler warnings
  void *addonData = AddonData;
  if (!addonData || !StreamHandle)
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - invalid stream data", __FUNCTION__);
    return 0;
  }

  return ((IAEStream*)StreamHandle)->GetChannelCount();
}

const unsigned int CAddonCallbacksAudioEngine::AEStream_GetSampleRate(void *AddonData, AEStreamHandle *StreamHandle)
{
  // prevent compiler warnings
  void *addonData = AddonData;
  if (!addonData || !StreamHandle)
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - invalid stream data", __FUNCTION__);
    return 0;
  }

  return ((IAEStream*)StreamHandle)->GetSampleRate();
}

const AEDataFormat CAddonCallbacksAudioEngine::AEStream_GetDataFormat(void *AddonData, AEStreamHandle *StreamHandle)
{
  // prevent compiler warnings
  void *addonData = AddonData;
  if (!addonData || !StreamHandle)
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - invalid stream data", __FUNCTION__);
    return AE_FMT_INVALID;
  }

  return ((IAEStream*)StreamHandle)->GetDataFormat();
}

double CAddonCallbacksAudioEngine::AEStream_GetResampleRatio(void *AddonData, AEStreamHandle *StreamHandle)
{
  // prevent compiler warnings
  void *addonData = AddonData;
  if (!addonData || !StreamHandle)
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - invalid stream data", __FUNCTION__);
    return -1.0f;
  }

  return ((IAEStream*)StreamHandle)->GetResampleRatio();
}

void CAddonCallbacksAudioEngine::AEStream_SetResampleRatio(void *AddonData, AEStreamHandle *StreamHandle, double Ratio)
{
  // prevent compiler warnings
  void *addonData = AddonData;
  if (!addonData || !StreamHandle)
  {
    CLog::Log(LOGERROR, "libKODI_audioengine - %s - invalid stream data", __FUNCTION__);
    return;
  }

  ((IAEStream*)StreamHandle)->SetResampleRatio(Ratio);
}

} /* namespace AudioEngine */

} /* namespace KodiAPI */
} /* namespace V1 */
