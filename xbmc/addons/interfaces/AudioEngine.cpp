/*
 *      Copyright (C) 2005-2017 Team KODI
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

#include "AudioEngine.h"

#include "ServiceBroker.h"
#include "addons/kodi-addon-dev-kit/include/kodi/AddonBase.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/AudioEngine/Interfaces/AEStream.h"
#include "cores/AudioEngine/Utils/AEChannelData.h"
#include "utils/log.h"

using namespace kodi; // addon-dev-kit namespace
using namespace kodi::audioengine; // addon-dev-kit namespace

namespace ADDON
{

void Interface_AudioEngine::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_audioengine = (AddonToKodiFuncTable_kodi_audioengine*)malloc(sizeof(AddonToKodiFuncTable_kodi_audioengine));

  // write KODI audio DSP specific add-on function addresses to callback table
  addonInterface->toKodi->kodi_audioengine->MakeStream = AudioEngine_MakeStream;
  addonInterface->toKodi->kodi_audioengine->FreeStream = AudioEngine_FreeStream;
  addonInterface->toKodi->kodi_audioengine->GetCurrentSinkFormat = AudioEngine_GetCurrentSinkFormat;

  // AEStream add-on function callback table
  addonInterface->toKodi->kodi_audioengine->AEStream_GetSpace = AEStream_GetSpace;
  addonInterface->toKodi->kodi_audioengine->AEStream_AddData = AEStream_AddData;
  addonInterface->toKodi->kodi_audioengine->AEStream_GetDelay = AEStream_GetDelay;
  addonInterface->toKodi->kodi_audioengine->AEStream_IsBuffering = AEStream_IsBuffering;
  addonInterface->toKodi->kodi_audioengine->AEStream_GetCacheTime = AEStream_GetCacheTime;
  addonInterface->toKodi->kodi_audioengine->AEStream_GetCacheTotal = AEStream_GetCacheTotal;
  addonInterface->toKodi->kodi_audioengine->AEStream_Pause = AEStream_Pause;
  addonInterface->toKodi->kodi_audioengine->AEStream_Resume = AEStream_Resume;
  addonInterface->toKodi->kodi_audioengine->AEStream_Drain = AEStream_Drain;
  addonInterface->toKodi->kodi_audioengine->AEStream_IsDraining = AEStream_IsDraining;
  addonInterface->toKodi->kodi_audioengine->AEStream_IsDrained = AEStream_IsDrained;
  addonInterface->toKodi->kodi_audioengine->AEStream_Flush = AEStream_Flush;
  addonInterface->toKodi->kodi_audioengine->AEStream_GetVolume = AEStream_GetVolume;
  addonInterface->toKodi->kodi_audioengine->AEStream_SetVolume = AEStream_SetVolume;
  addonInterface->toKodi->kodi_audioengine->AEStream_GetAmplification = AEStream_GetAmplification;
  addonInterface->toKodi->kodi_audioengine->AEStream_SetAmplification = AEStream_SetAmplification;
  addonInterface->toKodi->kodi_audioengine->AEStream_GetFrameSize = AEStream_GetFrameSize;
  addonInterface->toKodi->kodi_audioengine->AEStream_GetChannelCount = AEStream_GetChannelCount;
  addonInterface->toKodi->kodi_audioengine->AEStream_GetSampleRate = AEStream_GetSampleRate;
  addonInterface->toKodi->kodi_audioengine->AEStream_GetDataFormat = AEStream_GetDataFormat;
  addonInterface->toKodi->kodi_audioengine->AEStream_GetResampleRatio = AEStream_GetResampleRatio;
  addonInterface->toKodi->kodi_audioengine->AEStream_SetResampleRatio = AEStream_SetResampleRatio;
}

void Interface_AudioEngine::DeInit(AddonGlobalInterface* addonInterface)
{
  if (addonInterface->toKodi && /* <-- Safe check, needed so long old addon way is present */
      addonInterface->toKodi->kodi_audioengine)
  {
    free(addonInterface->toKodi->kodi_audioengine);
    addonInterface->toKodi->kodi_audioengine = nullptr;
  }
}

AEStreamHandle* Interface_AudioEngine::AudioEngine_MakeStream(void* kodiBase, AudioEngineFormat streamFormat, unsigned int options)
{
  if (!kodiBase)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p')", __FUNCTION__, kodiBase);
    return nullptr;
  }

  AEAudioFormat format;
  format.m_dataFormat = streamFormat.m_dataFormat;
  format.m_sampleRate = streamFormat.m_sampleRate;
  format.m_channelLayout = streamFormat.m_channels;

  /* Translate addon options to kodi's options */
  int kodiOption = 0;
  if (options & AUDIO_STREAM_FORCE_RESAMPLE)
    kodiOption |= AESTREAM_FORCE_RESAMPLE;
  if (options & AUDIO_STREAM_PAUSED)
    kodiOption |= AESTREAM_PAUSED;
  if (options & AUDIO_STREAM_AUTOSTART)
    kodiOption |= AESTREAM_AUTOSTART;
  if (options & AUDIO_STREAM_BYPASS_ADSP)
    kodiOption |= AESTREAM_BYPASS_ADSP;

  return CServiceBroker::GetActiveAE().MakeStream(format, kodiOption);
}

void Interface_AudioEngine::AudioEngine_FreeStream(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', streamHandle='%p')", __FUNCTION__, kodiBase, streamHandle);
    return;
  }

  CServiceBroker::GetActiveAE().FreeStream(static_cast<IAEStream*>(streamHandle));
}

bool Interface_AudioEngine::AudioEngine_GetCurrentSinkFormat(void* kodiBase, AudioEngineFormat *format)
{
  if (!kodiBase || !format)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', format='%p')", __FUNCTION__, kodiBase, format);
    return false;
  }

  AEAudioFormat sinkFormat;
  if (!CServiceBroker::GetActiveAE().GetCurrentSinkFormat(sinkFormat))
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - failed to get current sink format from AE!", __FUNCTION__);
    return false;
  }

  format->m_channelCount = sinkFormat.m_channelLayout.Count();
  for (unsigned int ch = 0; ch < format->m_channelCount; ++ch)
  {
    format->m_channels[ch] = sinkFormat.m_channelLayout[ch];
  }

  format->m_dataFormat = sinkFormat.m_dataFormat;
  format->m_sampleRate = sinkFormat.m_sampleRate;
  format->m_frames = sinkFormat.m_frames;
  format->m_frameSize = sinkFormat.m_frameSize;

  return true;
}

unsigned int Interface_AudioEngine::AEStream_GetSpace(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', streamHandle='%p')", __FUNCTION__, kodiBase, streamHandle);
    return 0;
  }

  return static_cast<IAEStream*>(streamHandle)->GetSpace();
}

unsigned int Interface_AudioEngine::AEStream_AddData(void* kodiBase, AEStreamHandle* streamHandle, uint8_t* const *data, unsigned int offset, unsigned int frames)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', streamHandle='%p')", __FUNCTION__, kodiBase, streamHandle);
    return 0;
  }

  return static_cast<IAEStream*>(streamHandle)->AddData(data, offset, frames);
}

double Interface_AudioEngine::AEStream_GetDelay(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', streamHandle='%p')", __FUNCTION__, kodiBase, streamHandle);
    return -1.0;
  }

  return static_cast<IAEStream*>(streamHandle)->GetDelay();
}

bool Interface_AudioEngine::AEStream_IsBuffering(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', streamHandle='%p')", __FUNCTION__, kodiBase, streamHandle);
    return false;
  }

  return static_cast<IAEStream*>(streamHandle)->IsBuffering();
}

double Interface_AudioEngine::AEStream_GetCacheTime(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', streamHandle='%p')", __FUNCTION__, kodiBase, streamHandle);
    return -1.0;
  }

  return static_cast<IAEStream*>(streamHandle)->GetCacheTime();
}

double Interface_AudioEngine::AEStream_GetCacheTotal(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', streamHandle='%p')", __FUNCTION__, kodiBase, streamHandle);
    return -1.0;
  }

  return static_cast<IAEStream*>(streamHandle)->GetCacheTotal();
}

void Interface_AudioEngine::AEStream_Pause(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', streamHandle='%p')", __FUNCTION__, kodiBase, streamHandle);
    return;
  }

  static_cast<IAEStream*>(streamHandle)->Pause();
}

void Interface_AudioEngine::AEStream_Resume(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', streamHandle='%p')", __FUNCTION__, kodiBase, streamHandle);
    return;
  }

  static_cast<IAEStream*>(streamHandle)->Resume();
}

void Interface_AudioEngine::AEStream_Drain(void* kodiBase, AEStreamHandle* streamHandle, bool wait)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', streamHandle='%p')", __FUNCTION__, kodiBase, streamHandle);
    return;
  }

  static_cast<IAEStream*>(streamHandle)->Drain(wait);
}

bool Interface_AudioEngine::AEStream_IsDraining(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', streamHandle='%p')", __FUNCTION__, kodiBase, streamHandle);
    return false;
  }

  return static_cast<IAEStream*>(streamHandle)->IsDraining();
}

bool Interface_AudioEngine::AEStream_IsDrained(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', streamHandle='%p')", __FUNCTION__, kodiBase, streamHandle);
    return false;
  }

  return static_cast<IAEStream*>(streamHandle)->IsDrained();
}

void Interface_AudioEngine::AEStream_Flush(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', streamHandle='%p')", __FUNCTION__, kodiBase, streamHandle);
    return;
  }

  static_cast<IAEStream*>(streamHandle)->Flush();
}

float Interface_AudioEngine::AEStream_GetVolume(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', streamHandle='%p')", __FUNCTION__, kodiBase, streamHandle);
    return -1.0f;
  }

  return static_cast<IAEStream*>(streamHandle)->GetVolume();
}

void  Interface_AudioEngine::AEStream_SetVolume(void* kodiBase, AEStreamHandle* streamHandle, float volume)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', streamHandle='%p')", __FUNCTION__, kodiBase, streamHandle);
    return;
  }

  static_cast<IAEStream*>(streamHandle)->SetVolume(volume);
}

float Interface_AudioEngine::AEStream_GetAmplification(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', streamHandle='%p')", __FUNCTION__, kodiBase, streamHandle);
    return -1.0f;
  }

  return static_cast<IAEStream*>(streamHandle)->GetAmplification();
}

void Interface_AudioEngine::AEStream_SetAmplification(void* kodiBase, AEStreamHandle* streamHandle, float amplify)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', streamHandle='%p')", __FUNCTION__, kodiBase, streamHandle);
    return;
  }

  static_cast<IAEStream*>(streamHandle)->SetAmplification(amplify);
}

unsigned int Interface_AudioEngine::AEStream_GetFrameSize(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', streamHandle='%p')", __FUNCTION__, kodiBase, streamHandle);
    return 0;
  }

  return static_cast<IAEStream*>(streamHandle)->GetFrameSize();
}

unsigned int Interface_AudioEngine::AEStream_GetChannelCount(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', streamHandle='%p')", __FUNCTION__, kodiBase, streamHandle);
    return 0;
  }

  return static_cast<IAEStream*>(streamHandle)->GetChannelCount();
}

unsigned int Interface_AudioEngine::AEStream_GetSampleRate(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', streamHandle='%p')", __FUNCTION__, kodiBase, streamHandle);
    return 0;
  }

  return static_cast<IAEStream*>(streamHandle)->GetSampleRate();
}

AEDataFormat Interface_AudioEngine::AEStream_GetDataFormat(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', streamHandle='%p')", __FUNCTION__, kodiBase, streamHandle);
    return AE_FMT_INVALID;
  }

  return static_cast<IAEStream*>(streamHandle)->GetDataFormat();
}

double Interface_AudioEngine::AEStream_GetResampleRatio(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', streamHandle='%p')", __FUNCTION__, kodiBase, streamHandle);
    return -1.0f;
  }

  return static_cast<IAEStream*>(streamHandle)->GetResampleRatio();
}

void Interface_AudioEngine::AEStream_SetResampleRatio(void* kodiBase, AEStreamHandle* streamHandle, double ratio)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::%s - invalid stream data (kodiBase='%p', streamHandle='%p')", __FUNCTION__, kodiBase, streamHandle);
    return;
  }

  static_cast<IAEStream*>(streamHandle)->SetResampleRatio(ratio);
}

} /* namespace ADDON */
