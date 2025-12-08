/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AudioEngine.h"

#include "ServiceBroker.h"
#include "addons/kodi-dev-kit/include/kodi/AddonBase.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/AudioEngine/Interfaces/AEStream.h"
#include "cores/AudioEngine/Utils/AEStreamData.h"
#include "utils/log.h"

using namespace kodi; // addon-dev-kit namespace
using namespace kodi::audioengine; // addon-dev-kit namespace

namespace ADDON
{

void Interface_AudioEngine::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_audioengine = new AddonToKodiFuncTable_kodi_audioengine();

  // write KODI audio DSP specific add-on function addresses to callback table
  addonInterface->toKodi->kodi_audioengine->make_stream = audioengine_make_stream;
  addonInterface->toKodi->kodi_audioengine->free_stream = audioengine_free_stream;
  addonInterface->toKodi->kodi_audioengine->get_current_sink_format = get_current_sink_format;

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
  addonInterface->toKodi->kodi_audioengine->aestream_get_resample_ratio =
      aestream_get_resample_ratio;
  addonInterface->toKodi->kodi_audioengine->aestream_set_resample_ratio =
      aestream_set_resample_ratio;
}

void Interface_AudioEngine::DeInit(AddonGlobalInterface* addonInterface)
{
  if (addonInterface->toKodi) /* <-- Safe check, needed so long old addon way is present */
  {
    delete addonInterface->toKodi->kodi_audioengine;
    addonInterface->toKodi->kodi_audioengine = nullptr;
  }
}

AEChannel Interface_AudioEngine::TranslateAEChannelToKodi(AudioEngineChannel channel)
{
  switch (channel)
  {
    case AUDIOENGINE_CH_RAW:
      return AE_CH_RAW;
    case AUDIOENGINE_CH_FL:
      return AE_CH_FL;
    case AUDIOENGINE_CH_FR:
      return AE_CH_FR;
    case AUDIOENGINE_CH_FC:
      return AE_CH_FC;
    case AUDIOENGINE_CH_LFE:
      return AE_CH_LFE;
    case AUDIOENGINE_CH_BL:
      return AE_CH_BL;
    case AUDIOENGINE_CH_BR:
      return AE_CH_BR;
    case AUDIOENGINE_CH_FLOC:
      return AE_CH_FLOC;
    case AUDIOENGINE_CH_FROC:
      return AE_CH_FROC;
    case AUDIOENGINE_CH_BC:
      return AE_CH_BC;
    case AUDIOENGINE_CH_SL:
      return AE_CH_SL;
    case AUDIOENGINE_CH_SR:
      return AE_CH_SR;
    case AUDIOENGINE_CH_TFL:
      return AE_CH_TFL;
    case AUDIOENGINE_CH_TFR:
      return AE_CH_TFR;
    case AUDIOENGINE_CH_TFC:
      return AE_CH_TFC;
    case AUDIOENGINE_CH_TC:
      return AE_CH_TC;
    case AUDIOENGINE_CH_TBL:
      return AE_CH_TBL;
    case AUDIOENGINE_CH_TBR:
      return AE_CH_TBR;
    case AUDIOENGINE_CH_TBC:
      return AE_CH_TBC;
    case AUDIOENGINE_CH_BLOC:
      return AE_CH_BLOC;
    case AUDIOENGINE_CH_BROC:
      return AE_CH_BROC;
    case AUDIOENGINE_CH_MAX:
      return AE_CH_MAX;
    case AUDIOENGINE_CH_NULL:
    default:
      return AE_CH_NULL;
  }
}

AudioEngineChannel Interface_AudioEngine::TranslateAEChannelToAddon(AEChannel channel)
{
  switch (channel)
  {
    case AE_CH_RAW:
      return AUDIOENGINE_CH_RAW;
    case AE_CH_FL:
      return AUDIOENGINE_CH_FL;
    case AE_CH_FR:
      return AUDIOENGINE_CH_FR;
    case AE_CH_FC:
      return AUDIOENGINE_CH_FC;
    case AE_CH_LFE:
      return AUDIOENGINE_CH_LFE;
    case AE_CH_BL:
      return AUDIOENGINE_CH_BL;
    case AE_CH_BR:
      return AUDIOENGINE_CH_BR;
    case AE_CH_FLOC:
      return AUDIOENGINE_CH_FLOC;
    case AE_CH_FROC:
      return AUDIOENGINE_CH_FROC;
    case AE_CH_BC:
      return AUDIOENGINE_CH_BC;
    case AE_CH_SL:
      return AUDIOENGINE_CH_SL;
    case AE_CH_SR:
      return AUDIOENGINE_CH_SR;
    case AE_CH_TFL:
      return AUDIOENGINE_CH_TFL;
    case AE_CH_TFR:
      return AUDIOENGINE_CH_TFR;
    case AE_CH_TFC:
      return AUDIOENGINE_CH_TFC;
    case AE_CH_TC:
      return AUDIOENGINE_CH_TC;
    case AE_CH_TBL:
      return AUDIOENGINE_CH_TBL;
    case AE_CH_TBR:
      return AUDIOENGINE_CH_TBR;
    case AE_CH_TBC:
      return AUDIOENGINE_CH_TBC;
    case AE_CH_BLOC:
      return AUDIOENGINE_CH_BLOC;
    case AE_CH_BROC:
      return AUDIOENGINE_CH_BROC;
    case AE_CH_MAX:
      return AUDIOENGINE_CH_MAX;
    case AE_CH_NULL:
    default:
      return AUDIOENGINE_CH_NULL;
  }
}

AEDataFormat Interface_AudioEngine::TranslateAEFormatToKodi(AudioEngineDataFormat format)
{
  switch (format)
  {
    case AUDIOENGINE_FMT_U8:
      return AE_FMT_U8;
    case AUDIOENGINE_FMT_S16BE:
      return AE_FMT_S16BE;
    case AUDIOENGINE_FMT_S16LE:
      return AE_FMT_S16LE;
    case AUDIOENGINE_FMT_S16NE:
      return AE_FMT_S16NE;
    case AUDIOENGINE_FMT_S32BE:
      return AE_FMT_S32BE;
    case AUDIOENGINE_FMT_S32LE:
      return AE_FMT_S32LE;
    case AUDIOENGINE_FMT_S32NE:
      return AE_FMT_S32NE;
    case AUDIOENGINE_FMT_S24BE4:
      return AE_FMT_S24BE4;
    case AUDIOENGINE_FMT_S24LE4:
      return AE_FMT_S24LE4;
    case AUDIOENGINE_FMT_S24NE4:
      return AE_FMT_S24NE4;
    case AUDIOENGINE_FMT_S24NE4MSB:
      return AE_FMT_S24NE4MSB;
    case AUDIOENGINE_FMT_S24BE3:
      return AE_FMT_S24BE3;
    case AUDIOENGINE_FMT_S24LE3:
      return AE_FMT_S24LE3;
    case AUDIOENGINE_FMT_S24NE3:
      return AE_FMT_S24NE3;
    case AUDIOENGINE_FMT_DOUBLE:
      return AE_FMT_DOUBLE;
    case AUDIOENGINE_FMT_FLOAT:
      return AE_FMT_FLOAT;
    case AUDIOENGINE_FMT_RAW:
      return AE_FMT_RAW;
    case AUDIOENGINE_FMT_U8P:
      return AE_FMT_U8P;
    case AUDIOENGINE_FMT_S16NEP:
      return AE_FMT_S16NEP;
    case AUDIOENGINE_FMT_S32NEP:
      return AE_FMT_S32NEP;
    case AUDIOENGINE_FMT_S24NE4P:
      return AE_FMT_S24NE4P;
    case AUDIOENGINE_FMT_S24NE4MSBP:
      return AE_FMT_S24NE4MSBP;
    case AUDIOENGINE_FMT_S24NE3P:
      return AE_FMT_S24NE3P;
    case AUDIOENGINE_FMT_DOUBLEP:
      return AE_FMT_DOUBLEP;
    case AUDIOENGINE_FMT_FLOATP:
      return AE_FMT_FLOATP;
    case AUDIOENGINE_FMT_MAX:
      return AE_FMT_MAX;
    case AUDIOENGINE_FMT_INVALID:
    default:
      return AE_FMT_INVALID;
  }
}

AudioEngineDataFormat Interface_AudioEngine::TranslateAEFormatToAddon(AEDataFormat format)
{
  switch (format)
  {
    case AE_FMT_U8:
      return AUDIOENGINE_FMT_U8;
    case AE_FMT_S16BE:
      return AUDIOENGINE_FMT_S16BE;
    case AE_FMT_S16LE:
      return AUDIOENGINE_FMT_S16LE;
    case AE_FMT_S16NE:
      return AUDIOENGINE_FMT_S16NE;
    case AE_FMT_S32BE:
      return AUDIOENGINE_FMT_S32BE;
    case AE_FMT_S32LE:
      return AUDIOENGINE_FMT_S32LE;
    case AE_FMT_S32NE:
      return AUDIOENGINE_FMT_S32NE;
    case AE_FMT_S24BE4:
      return AUDIOENGINE_FMT_S24BE4;
    case AE_FMT_S24LE4:
      return AUDIOENGINE_FMT_S24LE4;
    case AE_FMT_S24NE4:
      return AUDIOENGINE_FMT_S24NE4;
    case AE_FMT_S24NE4MSB:
      return AUDIOENGINE_FMT_S24NE4MSB;
    case AE_FMT_S24BE3:
      return AUDIOENGINE_FMT_S24BE3;
    case AE_FMT_S24LE3:
      return AUDIOENGINE_FMT_S24LE3;
    case AE_FMT_S24NE3:
      return AUDIOENGINE_FMT_S24NE3;
    case AE_FMT_DOUBLE:
      return AUDIOENGINE_FMT_DOUBLE;
    case AE_FMT_FLOAT:
      return AUDIOENGINE_FMT_FLOAT;
    case AE_FMT_RAW:
      return AUDIOENGINE_FMT_RAW;
    case AE_FMT_U8P:
      return AUDIOENGINE_FMT_U8P;
    case AE_FMT_S16NEP:
      return AUDIOENGINE_FMT_S16NEP;
    case AE_FMT_S32NEP:
      return AUDIOENGINE_FMT_S32NEP;
    case AE_FMT_S24NE4P:
      return AUDIOENGINE_FMT_S24NE4P;
    case AE_FMT_S24NE4MSBP:
      return AUDIOENGINE_FMT_S24NE4MSBP;
    case AE_FMT_S24NE3P:
      return AUDIOENGINE_FMT_S24NE3P;
    case AE_FMT_DOUBLEP:
      return AUDIOENGINE_FMT_DOUBLEP;
    case AE_FMT_FLOATP:
      return AUDIOENGINE_FMT_FLOATP;
    case AE_FMT_MAX:
      return AUDIOENGINE_FMT_MAX;
    case AE_FMT_INVALID:
    default:
      return AUDIOENGINE_FMT_INVALID;
  }
}

AEStreamHandle* Interface_AudioEngine::audioengine_make_stream(void* kodiBase,
                                                               AUDIO_ENGINE_FORMAT* streamFormat,
                                                               unsigned int options)
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

  CAEChannelInfo layout;
  for (unsigned int ch = 0; ch < AUDIOENGINE_CH_MAX; ++ch)
  {
    if (streamFormat->m_channels[ch] == AUDIOENGINE_CH_NULL)
      break;
    layout += TranslateAEChannelToKodi(streamFormat->m_channels[ch]);
  }

  AEAudioFormat format;
  format.m_channelLayout = layout;
  format.m_dataFormat = TranslateAEFormatToKodi(streamFormat->m_dataFormat);
  format.m_sampleRate = streamFormat->m_sampleRate;

  /* Translate addon options to kodi's options */
  int kodiOption = 0;
  if (options & AUDIO_STREAM_FORCE_RESAMPLE)
    kodiOption |= AESTREAM_FORCE_RESAMPLE;
  if (options & AUDIO_STREAM_PAUSED)
    kodiOption |= AESTREAM_PAUSED;
  if (options & AUDIO_STREAM_AUTOSTART)
    kodiOption |= AESTREAM_AUTOSTART;

  return engine->MakeStream(format, kodiOption).release();
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

bool Interface_AudioEngine::get_current_sink_format(void* kodiBase, AUDIO_ENGINE_FORMAT* format)
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
    CLog::Log(LOGERROR, "Interface_AudioEngine::{} - failed to get current sink format from AE!",
              __FUNCTION__);
    return false;
  }

  format->m_dataFormat = TranslateAEFormatToAddon(sinkFormat.m_dataFormat);
  format->m_sampleRate = sinkFormat.m_sampleRate;
  format->m_frames = sinkFormat.m_frames;
  format->m_frameSize = sinkFormat.m_frameSize;
  format->m_channelCount = sinkFormat.m_channelLayout.Count();
  for (unsigned int ch = 0; ch < format->m_channelCount && ch < AUDIOENGINE_CH_MAX; ++ch)
  {
    format->m_channels[ch] = TranslateAEChannelToAddon(sinkFormat.m_channelLayout[ch]);
  }

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

unsigned int Interface_AudioEngine::aestream_add_data(void* kodiBase,
                                                      AEStreamHandle* streamHandle,
                                                      uint8_t* const* data,
                                                      unsigned int offset,
                                                      unsigned int frames,
                                                      double pts,
                                                      bool hasDownmix,
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

void Interface_AudioEngine::aestream_set_volume(void* kodiBase,
                                                AEStreamHandle* streamHandle,
                                                float volume)
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

float Interface_AudioEngine::aestream_get_amplification(void* kodiBase,
                                                        AEStreamHandle* streamHandle)
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

void Interface_AudioEngine::aestream_set_amplification(void* kodiBase,
                                                       AEStreamHandle* streamHandle,
                                                       float amplify)
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

unsigned int Interface_AudioEngine::aestream_get_frame_size(void* kodiBase,
                                                            AEStreamHandle* streamHandle)
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

unsigned int Interface_AudioEngine::aestream_get_channel_count(void* kodiBase,
                                                               AEStreamHandle* streamHandle)
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

unsigned int Interface_AudioEngine::aestream_get_sample_rate(void* kodiBase,
                                                             AEStreamHandle* streamHandle)
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

AudioEngineDataFormat Interface_AudioEngine::aestream_get_data_format(void* kodiBase,
                                                                      AEStreamHandle* streamHandle)
{
  if (!kodiBase || !streamHandle)
  {
    CLog::Log(LOGERROR,
              "Interface_AudioEngine::{} - invalid stream data (kodiBase='{}', streamHandle='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(streamHandle));
    return AUDIOENGINE_FMT_INVALID;
  }

  if (!CServiceBroker::GetActiveAE())
    return AUDIOENGINE_FMT_INVALID;

  return TranslateAEFormatToAddon(static_cast<IAEStream*>(streamHandle)->GetDataFormat());
}

double Interface_AudioEngine::aestream_get_resample_ratio(void* kodiBase,
                                                          AEStreamHandle* streamHandle)
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

  return static_cast<IAEStream*>(streamHandle)->GetResampleRatio();
}

void Interface_AudioEngine::aestream_set_resample_ratio(void* kodiBase,
                                                        AEStreamHandle* streamHandle,
                                                        double ratio)
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
