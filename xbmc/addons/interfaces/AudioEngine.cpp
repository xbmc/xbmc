/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AudioEngine.h"

#include "ServiceBroker.h"
#include "addons/kodi-addon-dev-kit/include/kodi/AddonBase.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/AudioEngine/Interfaces/AEStream.h"
#include "cores/AudioEngine/Utils/AEChannelData.h"
#include "cores/AudioEngine/Utils/AEStreamData.h"
#include "utils/log.h"

using namespace kodi; // addon-dev-kit namespace
using namespace kodi::audioengine; // addon-dev-kit namespace

namespace ADDON
{

void Interface_AudioEngine::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_audioengine = static_cast<AddonToKodiFuncTable_kodi_audioengine*>(malloc(sizeof(AddonToKodiFuncTable_kodi_audioengine)));

  // write KODI audio DSP specific add-on function addresses to callback table
  addonInterface->toKodi->kodi_audioengine->make_stream = audioengine_make_stream;
  addonInterface->toKodi->kodi_audioengine->free_stream = audioengine_free_stream;
  addonInterface->toKodi->kodi_audioengine->get_current_sink_format = audioengine_get_current_sink_format;

  // AEStream add-on function callback table
  addonInterface->toKodi->kodi_audioengine->aestream_get_space = aestream_get_space;
  addonInterface->toKodi->kodi_audioengine->aestream_add_data = aestream_add_data;
  addonInterface->toKodi->kodi_audioengine->aestream_get_delay = aestream_get_delay;
  addonInterface->toKodi->kodi_audioengine->aestream_is_buffering = aestream_is_buffering;
  addonInterface->toKodi->kodi_audioengine->aestream_get_cache_time = aestream_get_cache_time;
  addonInterface->toKodi->kodi_audioengine->aestream_get_cache_total = aestream_get_cache_total;
  addonInterface->toKodi->kodi_audioengine->aestream_pause = aestream_pause;
  addonInterface->toKodi->kodi_audioengine->aestream_resume = aestream_resume;
  addonInterface->toKodi->kodi_audioengine->aestream_drain = aestream_drain;
  addonInterface->toKodi->kodi_audioengine->aestream_is_draining = aestream_is_draining;
  addonInterface->toKodi->kodi_audioengine->aestream_is_drained = aestream_is_drained;
  addonInterface->toKodi->kodi_audioengine->aestream_flush = aestream_flush;
  addonInterface->toKodi->kodi_audioengine->aestream_get_volume = aestream_get_volume;
  addonInterface->toKodi->kodi_audioengine->aestream_set_volume = aestream_set_volume;
  addonInterface->toKodi->kodi_audioengine->aestream_get_amplification = aestream_get_amplification;
  addonInterface->toKodi->kodi_audioengine->aestream_set_amplification = aestream_set_amplification;
  addonInterface->toKodi->kodi_audioengine->aestream_get_frame_size = aestream_get_frame_size;
  addonInterface->toKodi->kodi_audioengine->aestream_get_channel_count = aestream_get_channel_count;
  addonInterface->toKodi->kodi_audioengine->aestream_get_sample_rate = aestream_get_sample_rate;
  addonInterface->toKodi->kodi_audioengine->aestream_get_data_format = aestream_get_data_format;
  addonInterface->toKodi->kodi_audioengine->aestream_get_resample_ratio = aestream_get_resample_ratio;
  addonInterface->toKodi->kodi_audioengine->aestream_set_resample_ratio = aestream_set_resample_ratio;
}

void Interface_AudioEngine::DeInit(AddonGlobalInterface* addonInterface)
{
  if (addonInterface->toKodi) /* <-- Safe check, needed so long old addon way is present */
  {
    free(addonInterface->toKodi->kodi_audioengine);
    addonInterface->toKodi->kodi_audioengine = nullptr;
  }
}

AEStreamHandle* Interface_AudioEngine::audioengine_make_stream(void* kodiBase, AudioEngineFormat* streamFormat, unsigned int options)
{
  if (!kodiBase || !streamFormat)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamFormat='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamFormat));
    return nullptr;
  }

  IAE* engine = CServiceBroker::GetActiveAE();
  if (!engine)
    return nullptr;

  AEAudioFormat format;
  format.m_dataFormat = streamFormat->m_dataFormat;
  format.m_sampleRate = streamFormat->m_sampleRate;
  format.m_channelLayout = streamFormat->m_channels;

  /* Translate addon options to kodi's options */
  int kodiOption = 0;
  if (options & AUDIO_STREAM_FORCE_RESAMPLE)
    kodiOption |= AESTREAM_FORCE_RESAMPLE;
  if (options & AUDIO_STREAM_PAUSED)
    kodiOption |= AESTREAM_PAUSED;
  if (options & AUDIO_STREAM_AUTOSTART)
    kodiOption |= AESTREAM_AUTOSTART;

  return engine->MakeStream(format, kodiOption);
}

void Interface_AudioEngine::audioengine_free_stream(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return;
  }

  IAE* engine = CServiceBroker::GetActiveAE();
  if (engine)
    engine->FreeStream(static_cast<IAEStream*>(streamHandle), true);
}

bool Interface_AudioEngine::audioengine_get_current_sink_format(void* kodiBase, AudioEngineFormat *format)
{
  if (!kodiBase || !format)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', format='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(format));
    return false;
  }

  IAE* engine = CServiceBroker::GetActiveAE();
  if (!engine)
    return false;

  AEAudioFormat sinkFormat;
  if (!engine->GetCurrentSinkFormat(sinkFormat))
  {
    CLog::Log(LOGERROR, "Interface_AudioEngine::{} - failed to get current sink format from AE!", __FUNCTION__);
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

unsigned int Interface_AudioEngine::aestream_get_space(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return 0;
  }

  return static_cast<IAEStream*>(streamHandle)->GetSpace();
}

unsigned int Interface_AudioEngine::aestream_add_data(void* kodiBase, AEStreamHandle* streamHandle, uint8_t* const *data,
                                                      unsigned int offset, unsigned int frames, double pts, bool hasDownmix,
                                                      double centerMixLevel)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return 0;
  }

  if (!CServiceBroker::GetActiveAE())
    return 0;

  IAEStream::ExtData extData;
  extData.pts = pts;
  extData.hasDownmix = hasDownmix;
  extData.centerMixLevel = centerMixLevel;
  return static_cast<IAEStream*>(streamHandle)->AddData(data, offset, frames, &extData);
}

double Interface_AudioEngine::aestream_get_delay(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return -1.0;
  }

  if (!CServiceBroker::GetActiveAE())
    return -1.0;

  return static_cast<IAEStream*>(streamHandle)->GetDelay();
}

bool Interface_AudioEngine::aestream_is_buffering(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return false;
  }

  if (!CServiceBroker::GetActiveAE())
    return false;

  return static_cast<IAEStream*>(streamHandle)->IsBuffering();
}

double Interface_AudioEngine::aestream_get_cache_time(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return -1.0;
  }

  if (!CServiceBroker::GetActiveAE())
    return -1.0;

  return static_cast<IAEStream*>(streamHandle)->GetCacheTime();
}

double Interface_AudioEngine::aestream_get_cache_total(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return -1.0;
  }

  if (!CServiceBroker::GetActiveAE())
    return -1.0;

  return static_cast<IAEStream*>(streamHandle)->GetCacheTotal();
}

void Interface_AudioEngine::aestream_pause(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return;
  }

  if (!CServiceBroker::GetActiveAE())
    return;

  static_cast<IAEStream*>(streamHandle)->Pause();
}

void Interface_AudioEngine::aestream_resume(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return;
  }

  static_cast<IAEStream*>(streamHandle)->Resume();
}

void Interface_AudioEngine::aestream_drain(void* kodiBase, AEStreamHandle* streamHandle, bool wait)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return;
  }

  if (!CServiceBroker::GetActiveAE())
    return;

  static_cast<IAEStream*>(streamHandle)->Drain(wait);
}

bool Interface_AudioEngine::aestream_is_draining(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return false;
  }

  if (!CServiceBroker::GetActiveAE())
    return false;

  return static_cast<IAEStream*>(streamHandle)->IsDraining();
}

bool Interface_AudioEngine::aestream_is_drained(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return false;
  }

  if (!CServiceBroker::GetActiveAE())
    return false;

  return static_cast<IAEStream*>(streamHandle)->IsDrained();
}

void Interface_AudioEngine::aestream_flush(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return;
  }

  if (!CServiceBroker::GetActiveAE())
    return;

  static_cast<IAEStream*>(streamHandle)->Flush();
}

float Interface_AudioEngine::aestream_get_volume(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return -1.0f;
  }

  if (!CServiceBroker::GetActiveAE())
    return -1.0f;

  return static_cast<IAEStream*>(streamHandle)->GetVolume();
}

void  Interface_AudioEngine::aestream_set_volume(void* kodiBase, AEStreamHandle* streamHandle, float volume)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return;
  }

  if (!CServiceBroker::GetActiveAE())
    return;

  static_cast<IAEStream*>(streamHandle)->SetVolume(volume);
}

float Interface_AudioEngine::aestream_get_amplification(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return -1.0f;
  }

  if (!CServiceBroker::GetActiveAE())
    return -1.0f;

  return static_cast<IAEStream*>(streamHandle)->GetAmplification();
}

void Interface_AudioEngine::aestream_set_amplification(void* kodiBase, AEStreamHandle* streamHandle, float amplify)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return;
  }

  if (!CServiceBroker::GetActiveAE())
    return;

  static_cast<IAEStream*>(streamHandle)->SetAmplification(amplify);
}

unsigned int Interface_AudioEngine::aestream_get_frame_size(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return 0;
  }

  if (!CServiceBroker::GetActiveAE())
    return 0;

  return static_cast<IAEStream*>(streamHandle)->GetFrameSize();
}

unsigned int Interface_AudioEngine::aestream_get_channel_count(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return 0;
  }

  if (!CServiceBroker::GetActiveAE())
    return 0;

  return static_cast<IAEStream*>(streamHandle)->GetChannelCount();
}

unsigned int Interface_AudioEngine::aestream_get_sample_rate(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return 0;
  }

  if (!CServiceBroker::GetActiveAE())
    return 0;

  return static_cast<IAEStream*>(streamHandle)->GetSampleRate();
}

AEDataFormat Interface_AudioEngine::aestream_get_data_format(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return AE_FMT_INVALID;
  }

  if (!CServiceBroker::GetActiveAE())
    return AE_FMT_INVALID;

  return static_cast<IAEStream*>(streamHandle)->GetDataFormat();
}

double Interface_AudioEngine::aestream_get_resample_ratio(void* kodiBase, AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return -1.0f;
  }

  if (!CServiceBroker::GetActiveAE())
    return -1.0f;

  return static_cast<IAEStream*>(streamHandle)->GetResampleRatio();
}

void Interface_AudioEngine::aestream_set_resample_ratio(void* kodiBase, AEStreamHandle* streamHandle, double ratio)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return;
  }

  if (!CServiceBroker::GetActiveAE())
    return;

  static_cast<IAEStream*>(streamHandle)->SetResampleRatio(ratio);
}

} /* namespace ADDON */
