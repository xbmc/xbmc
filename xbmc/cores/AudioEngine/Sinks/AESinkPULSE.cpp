/*
 *      Copyright (C) 2010-2013 Team XBMC
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
#include "system.h"
#ifdef HAS_PULSEAUDIO
#include "AESinkPULSE.h"
#include "utils/log.h"
#include "Util.h"
#include "guilib/LocalizeStrings.h"
#include "Application.h"

using namespace std;

static const char *ContextStateToString(pa_context_state s)
{
  switch (s)
  {
    case PA_CONTEXT_UNCONNECTED:
      return "unconnected";
    case PA_CONTEXT_CONNECTING:
      return "connecting";
    case PA_CONTEXT_AUTHORIZING:
      return "authorizing";
    case PA_CONTEXT_SETTING_NAME:
      return "setting name";
    case PA_CONTEXT_READY:
      return "ready";
    case PA_CONTEXT_FAILED:
      return "failed";
    case PA_CONTEXT_TERMINATED:
      return "terminated";
    default:
      return "none";
  }
}

static const char *StreamStateToString(pa_stream_state s)
{
  switch(s)
  {
    case PA_STREAM_UNCONNECTED:
      return "unconnected";
    case PA_STREAM_CREATING:
      return "creating";
    case PA_STREAM_READY:
      return "ready";
    case PA_STREAM_FAILED:
      return "failed";
    case PA_STREAM_TERMINATED:
      return "terminated";
    default:
      return "none";
  }
}

static pa_sample_format AEFormatToPulseFormat(AEDataFormat format)
{
  switch (format)
  {
    case AE_FMT_U8     : return PA_SAMPLE_U8;
    case AE_FMT_S16NE  : return PA_SAMPLE_S16NE;
    case AE_FMT_S24NE3 : return PA_SAMPLE_S24NE;
    case AE_FMT_S24NE4 : return PA_SAMPLE_S24_32NE;
    case AE_FMT_S32NE  : return PA_SAMPLE_S32NE;
    case AE_FMT_FLOAT  : return PA_SAMPLE_FLOAT32;

    case AE_FMT_AC3:
    case AE_FMT_DTS:
    case AE_FMT_EAC3:
      return PA_SAMPLE_S16NE;

    default:
      return PA_SAMPLE_INVALID;
  }
}

static pa_encoding AEFormatToPulseEncoding(AEDataFormat format)
{
  switch (format)
  {
    case AE_FMT_AC3   : return PA_ENCODING_AC3_IEC61937;
    case AE_FMT_DTS   : return PA_ENCODING_DTS_IEC61937;
    case AE_FMT_EAC3  : return PA_ENCODING_EAC3_IEC61937;

    default:
      return PA_ENCODING_PCM;
  }
}

static AEDataFormat defaultDataFormats[] = {
  AE_FMT_U8,
  AE_FMT_S16NE,
  AE_FMT_S24NE3,
  AE_FMT_S24NE4,
  AE_FMT_S32NE,
  AE_FMT_FLOAT
};

static unsigned int defaultSampleRates[] = {
  5512,
  8000,
  11025,
  16000,
  22050,
  32000,
  44100,
  48000,
  64000,
  88200,
  96000,
  176400,
  192000,
  384000
};

/* Static callback functions */

static void ContextStateCallback(pa_context *c, void *userdata)
{
  pa_threaded_mainloop *m = (pa_threaded_mainloop *)userdata;
  switch (pa_context_get_state(c))
  {
    case PA_CONTEXT_READY:
    case PA_CONTEXT_TERMINATED:
    case PA_CONTEXT_UNCONNECTED:
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
    case PA_CONTEXT_FAILED:
      pa_threaded_mainloop_signal(m, 0);
      break;
  }
}

static void StreamStateCallback(pa_stream *s, void *userdata)
{
  pa_threaded_mainloop *m = (pa_threaded_mainloop *)userdata;
  switch (pa_stream_get_state(s))
  {
    case PA_STREAM_UNCONNECTED:
    case PA_STREAM_CREATING:
    case PA_STREAM_READY:
    case PA_STREAM_FAILED:
    case PA_STREAM_TERMINATED:
      pa_threaded_mainloop_signal(m, 0);
      break;
  }
}

static void StreamRequestCallback(pa_stream *s, size_t length, void *userdata)
{
  pa_threaded_mainloop *m = (pa_threaded_mainloop *)userdata;
  pa_threaded_mainloop_signal(m, 0);
}

static void StreamLatencyUpdateCallback(pa_stream *s, void *userdata)
{
  pa_threaded_mainloop *m = (pa_threaded_mainloop *)userdata;
  pa_threaded_mainloop_signal(m, 0);
}


static void SinkInputInfoCallback(pa_context *c, const pa_sink_input_info *i, int eol, void *userdata)
{
  CAESinkPULSE *p = (CAESinkPULSE*) userdata;
  if (!p || !p->IsInitialized())
    return;

  if(i && i->has_volume)
    p->UpdateInternalVolume(&(i->volume));
}

static void SinkInputInfoChangedCallback(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *userdata)
{
  CAESinkPULSE* p = (CAESinkPULSE*) userdata;
  if (!p || !p->IsInitialized())
    return;
  
   if (idx != pa_stream_get_index(p->GetInternalStream()))
     return;
   
   pa_operation* op = pa_context_get_sink_input_info(c, idx, SinkInputInfoCallback, p);
   if (op == NULL)
     CLog::Log(LOGERROR, "PulseAudio: Failed to sync volume");
   else
    pa_operation_unref(op);
}

static void SinkChangedCallback(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *userdata)
{
  CAESinkPULSE* p = (CAESinkPULSE*) userdata;
  if(!p)
    return;

  CSingleLock lock(p->m_sec);
  if (p->IsInitialized())
  {
    if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_NEW)
    {
       CLog::Log(LOGDEBUG, "Sink appeared");
       CAEFactory::DeviceChange();
    }
    else if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE)
    {
      CLog::Log(LOGDEBUG, "Sink removed");
      CAEFactory::DeviceChange();
    }
    else if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_CHANGE)
    {
      CLog::Log(LOGDEBUG, "Sink changed");
      //CAEFactory::DeviceChange();
    }    
  }
}

struct SinkInfoStruct
{
  AEDeviceInfoList *list;
  bool isHWDevice;
  bool device_found;
  pa_threaded_mainloop *mainloop;
  int samplerate;
  SinkInfoStruct()
  {
    list = NULL;
    isHWDevice = false;
    device_found = true;
    mainloop = NULL;
    samplerate = 0;
  }
};

static void SinkInfoCallback(pa_context *c, const pa_sink_info *i, int eol, void *userdata)
{
  SinkInfoStruct *sinkStruct = (SinkInfoStruct *)userdata;
  if (!sinkStruct)
    return;

  if(i)
  {
    if (i->flags && (i->flags & PA_SINK_HARDWARE))
      sinkStruct->isHWDevice = true;

    sinkStruct->samplerate = i->sample_spec.rate;
    sinkStruct->device_found = true;
  }
  pa_threaded_mainloop_signal(sinkStruct->mainloop, 0);
}

static AEChannel PAChannelToAEChannel(pa_channel_position_t channel)
{
  AEChannel ae_channel;
  switch (channel)
  {
    case PA_CHANNEL_POSITION_FRONT_LEFT:            ae_channel = AE_CH_FL; break;
    case PA_CHANNEL_POSITION_FRONT_RIGHT:           ae_channel = AE_CH_FR; break;
    case PA_CHANNEL_POSITION_FRONT_CENTER:          ae_channel = AE_CH_FC; break;
    case PA_CHANNEL_POSITION_LFE:                   ae_channel = AE_CH_LFE; break;
    case PA_CHANNEL_POSITION_REAR_LEFT:             ae_channel = AE_CH_BL; break;
    case PA_CHANNEL_POSITION_REAR_RIGHT:            ae_channel = AE_CH_BR; break;
    case PA_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER:  ae_channel = AE_CH_FLOC; break;
    case PA_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER: ae_channel = AE_CH_FROC; break;
    case PA_CHANNEL_POSITION_REAR_CENTER:           ae_channel = AE_CH_BC; break;
    case PA_CHANNEL_POSITION_SIDE_LEFT:             ae_channel = AE_CH_SL; break;
    case PA_CHANNEL_POSITION_SIDE_RIGHT:            ae_channel = AE_CH_SR; break;
    case PA_CHANNEL_POSITION_TOP_FRONT_LEFT:        ae_channel = AE_CH_TFL; break;
    case PA_CHANNEL_POSITION_TOP_FRONT_RIGHT:       ae_channel = AE_CH_TFR; break;
    case PA_CHANNEL_POSITION_TOP_FRONT_CENTER:      ae_channel = AE_CH_TFC; break;
    case PA_CHANNEL_POSITION_TOP_CENTER:            ae_channel = AE_CH_TC; break;
    case PA_CHANNEL_POSITION_TOP_REAR_LEFT:         ae_channel = AE_CH_TBL; break;
    case PA_CHANNEL_POSITION_TOP_REAR_RIGHT:        ae_channel = AE_CH_TBR; break;
    case PA_CHANNEL_POSITION_TOP_REAR_CENTER:       ae_channel = AE_CH_TBC; break;
    default:                                        ae_channel = AE_CH_NULL; break;
  }
  return ae_channel;
}

static pa_channel_position_t AEChannelToPAChannel(AEChannel ae_channel)
{
  pa_channel_position_t pa_channel;
  switch (ae_channel)
  {
    case AE_CH_FL:    pa_channel = PA_CHANNEL_POSITION_FRONT_LEFT; break;
    case AE_CH_FR:    pa_channel = PA_CHANNEL_POSITION_FRONT_RIGHT; break;
    case AE_CH_FC:    pa_channel = PA_CHANNEL_POSITION_FRONT_CENTER; break;
    case AE_CH_LFE:   pa_channel = PA_CHANNEL_POSITION_LFE; break;
    case AE_CH_BL:    pa_channel = PA_CHANNEL_POSITION_REAR_LEFT; break;
    case AE_CH_BR:    pa_channel = PA_CHANNEL_POSITION_REAR_RIGHT; break;
    case AE_CH_FLOC:  pa_channel = PA_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER; break;
    case AE_CH_FROC:  pa_channel = PA_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER; break;
    case AE_CH_BC:    pa_channel = PA_CHANNEL_POSITION_REAR_CENTER; break;
    case AE_CH_SL:    pa_channel = PA_CHANNEL_POSITION_SIDE_LEFT; break;
    case AE_CH_SR:    pa_channel = PA_CHANNEL_POSITION_SIDE_RIGHT; break;
    case AE_CH_TFL:   pa_channel = PA_CHANNEL_POSITION_TOP_FRONT_LEFT; break;
    case AE_CH_TFR:   pa_channel = PA_CHANNEL_POSITION_TOP_FRONT_RIGHT; break;
    case AE_CH_TFC:   pa_channel = PA_CHANNEL_POSITION_TOP_FRONT_CENTER; break;
    case AE_CH_TC:    pa_channel = PA_CHANNEL_POSITION_TOP_CENTER; break;
    case AE_CH_TBL:   pa_channel = PA_CHANNEL_POSITION_TOP_REAR_LEFT; break;
    case AE_CH_TBR:   pa_channel = PA_CHANNEL_POSITION_TOP_REAR_RIGHT; break;
    case AE_CH_TBC:   pa_channel = PA_CHANNEL_POSITION_TOP_REAR_CENTER; break;
    default:          pa_channel = PA_CHANNEL_POSITION_INVALID; break;
  }
  return pa_channel;
}

static pa_channel_map AEChannelMapToPAChannel(const CAEChannelInfo& info)
{
  pa_channel_map map;
  pa_channel_map_init(&map);
  pa_channel_position_t pos;
  for (unsigned int i = 0; i < info.Count(); ++i)
  {
    pos = AEChannelToPAChannel(info[i]);
    if(pos != PA_CHANNEL_POSITION_INVALID)
    {
      // remember channel name and increase channel count
      map.map[map.channels++] = pos;
    }
  }
  return map;
}

static CAEChannelInfo PAChannelToAEChannelMap(const pa_channel_map& channels)
{
  CAEChannelInfo info;
  AEChannel ch;
  info.Reset();
  for (unsigned int i=0; i<channels.channels; i++)
  {
    ch = PAChannelToAEChannel(channels.map[i]);
    if(ch != AE_CH_NULL)
      info += ch;
  }
  return info;
}

static void SinkInfoRequestCallback(pa_context *c, const pa_sink_info *i, int eol, void *userdata)
{

  SinkInfoStruct *sinkStruct = (SinkInfoStruct *)userdata;
  if (!sinkStruct)
    return;

  if(sinkStruct->list->empty())
  {
    //add a default device first
    CAEDeviceInfo defaultDevice;
    defaultDevice.m_deviceName = std::string("Default");
    defaultDevice.m_displayName = std::string("Default");
    defaultDevice.m_displayNameExtra = std::string("Default Output Device (PULSEAUDIO)");
    defaultDevice.m_dataFormats.insert(defaultDevice.m_dataFormats.end(), defaultDataFormats, defaultDataFormats + ARRAY_SIZE(defaultDataFormats));
    defaultDevice.m_channels = CAEChannelInfo(AE_CH_LAYOUT_2_0);
    defaultDevice.m_sampleRates.assign(defaultSampleRates, defaultSampleRates + ARRAY_SIZE(defaultSampleRates));
    defaultDevice.m_deviceType = AE_DEVTYPE_PCM;
    sinkStruct->list->push_back(defaultDevice);
  }
  if (i && i->name)
  {
    CAEDeviceInfo device;
    bool valid = true;
    device.m_deviceName = string(i->name);
    device.m_displayName = string(i->description);
    if (i->active_port && i->active_port->description)
      device.m_displayNameExtra = std::string((i->active_port->description)).append(" (PULSEAUDIO)");
    else
      device.m_displayNameExtra = std::string((i->description)).append(" (PULSEAUDIO)");
    unsigned int device_type = AE_DEVTYPE_PCM; //0

    device.m_channels = PAChannelToAEChannelMap(i->channel_map);

    // Don't add devices that would not have a channel map
    if(device.m_channels.Count() == 0)
      valid = false;

    device.m_sampleRates.assign(defaultSampleRates, defaultSampleRates + ARRAY_SIZE(defaultSampleRates));

    for (unsigned int j = 0; j < i->n_formats; j++)
    {
      switch(i->formats[j]->encoding)
      {
        case PA_ENCODING_AC3_IEC61937:
          device.m_dataFormats.push_back(AE_FMT_AC3);
          device_type = AE_DEVTYPE_IEC958;
          break;
        case PA_ENCODING_DTS_IEC61937:
          device.m_dataFormats.push_back(AE_FMT_DTS);
          device_type = AE_DEVTYPE_IEC958;
          break;
        case PA_ENCODING_EAC3_IEC61937:
          device.m_dataFormats.push_back(AE_FMT_EAC3);
          device_type = AE_DEVTYPE_IEC958;
          break;
        case PA_ENCODING_PCM:
          device.m_dataFormats.insert(device.m_dataFormats.end(), defaultDataFormats, defaultDataFormats + ARRAY_SIZE(defaultDataFormats));
          break;
        default:
          break;
      }
    }
    // passthrough is only working when device has Stereo channel config
    if (device_type > AE_DEVTYPE_PCM && device.m_channels.Count() == 2)
      device.m_deviceType = AE_DEVTYPE_IEC958;
    else
      device.m_deviceType = AE_DEVTYPE_PCM;
    if(valid)
    {
      CLog::Log(LOGDEBUG, "PulseAudio: Found %s with devicestring %s", device.m_displayName.c_str(), device.m_deviceName.c_str());
      sinkStruct->list->push_back(device);
    }
    else
    {
      CLog::Log(LOGDEBUG, "PulseAudio: Skipped %s with devicestring %s", device.m_displayName.c_str(), device.m_deviceName.c_str());
    }
  }
  pa_threaded_mainloop_signal(sinkStruct->mainloop, 0);
}

/* PulseAudio class memberfunctions*/


CAESinkPULSE::CAESinkPULSE()
{
  m_IsAllocated = false;
  m_passthrough = false;
  m_MainLoop = NULL;
  m_BytesPerSecond = 0;
  m_BufferSize = 0;
  m_Channels = 0;
  m_Stream = NULL;
  m_Context = NULL;
  m_IsStreamPaused = false;
  m_volume_needs_update = false;
  pa_cvolume_init(&m_Volume);
}

CAESinkPULSE::~CAESinkPULSE()
{
  Deinitialize();
}

bool CAESinkPULSE::Initialize(AEAudioFormat &format, std::string &device)
{
  {
    CSingleLock lock(m_sec);
    m_IsAllocated = false;
  }
  m_passthrough = false;
  m_BytesPerSecond = 0;
  m_BufferSize = 0;
  m_Channels = 0;
  m_Stream = NULL;
  m_Context = NULL;

  if (!SetupContext(NULL, &m_Context, &m_MainLoop))
  {
    CLog::Log(LOGNOTICE, "PulseAudio might not be running. Context was not created.");
    Deinitialize();
    return false;
  }

  pa_threaded_mainloop_lock(m_MainLoop);

  struct pa_channel_map map;
  pa_channel_map_init(&map);

   // PULSE cannot cope with e.g. planar formats so we fallback to FLOAT
   // when we receive an invalid pulse format
   if (AEFormatToPulseFormat(format.m_dataFormat) == PA_SAMPLE_INVALID)
   {
     CLog::Log(LOGDEBUG, "PULSE does not support format: %s - will fallback to AE_FMT_FLOAT", CAEUtil::DataFormatToStr(format.m_dataFormat));
     format.m_dataFormat = AE_FMT_FLOAT;
   }

  m_passthrough = AE_IS_RAW(format.m_dataFormat);

  if(m_passthrough)
  {
    map.channels = 2;
    format.m_channelLayout = AE_CH_LAYOUT_2_0;
  }
  else
  {
    map = AEChannelMapToPAChannel(format.m_channelLayout);
    // if count has changed we need to fit the AE Map
    if(map.channels != format.m_channelLayout.Count())
      format.m_channelLayout = PAChannelToAEChannelMap(map);
  }
  m_Channels = format.m_channelLayout.Count();

  // store information about current sink
  SinkInfoStruct sinkStruct;
  sinkStruct.mainloop = m_MainLoop;
  sinkStruct.device_found = false;

  // get real sample rate of the device we want to open - to avoid resampling
  bool isDefaultDevice = (device == "Default");
  WaitForOperation(pa_context_get_sink_info_by_name(m_Context, isDefaultDevice ? NULL : device.c_str(), SinkInfoCallback, &sinkStruct), m_MainLoop, "Get Sink Info");
  // only check if the device is existing - don't alter the sample rate
  if (!sinkStruct.device_found)
  {
    CLog::Log(LOGERROR, "PulseAudio: Sink %s not found", device.c_str());
    pa_threaded_mainloop_unlock(m_MainLoop);
    Deinitialize();
    return false;
  }

  // Pulse can resample everything between 1 hz and 192000 hz
  // Make sure we are in the range that we originally added
  format.m_sampleRate = std::max(5512U, std::min(format.m_sampleRate, 192000U));

  pa_format_info *info[1];
  info[0] = pa_format_info_new();
  info[0]->encoding = AEFormatToPulseEncoding(format.m_dataFormat);
  if(!m_passthrough)
  {
    pa_format_info_set_sample_format(info[0], AEFormatToPulseFormat(format.m_dataFormat));
    pa_format_info_set_channel_map(info[0], &map);
  }
  pa_format_info_set_channels(info[0], m_Channels);

  // PA requires m_encodedRate in order to do EAC3
  unsigned int samplerate = format.m_sampleRate;
  if (m_passthrough && (AEFormatToPulseEncoding(format.m_dataFormat) == PA_ENCODING_EAC3_IEC61937))
  {
    // this is only used internally for PA to use EAC3
    samplerate = format.m_encodedRate;
  }

  pa_format_info_set_rate(info[0], samplerate);

  if (!pa_format_info_valid(info[0]))
  {
    CLog::Log(LOGERROR, "PulseAudio: Invalid format info");
    pa_format_info_free(info[0]);
    pa_threaded_mainloop_unlock(m_MainLoop);
    Deinitialize();
    return false;
  }

  pa_sample_spec spec;
  #if PA_CHECK_VERSION(2,0,0)
    pa_format_info_to_sample_spec(info[0], &spec, NULL);
  #else
    spec.rate = (AEFormatToPulseEncoding(format.m_dataFormat) == PA_ENCODING_EAC3_IEC61937) ? 4 * samplerate : samplerate;
    spec.format = AEFormatToPulseFormat(format.m_dataFormat);
    spec.channels = m_Channels;
  #endif
  if (!pa_sample_spec_valid(&spec))
  {
    CLog::Log(LOGERROR, "PulseAudio: Invalid sample spec");
    pa_format_info_free(info[0]);
    pa_threaded_mainloop_unlock(m_MainLoop);
    Deinitialize();
    return false;
  }

  m_BytesPerSecond = pa_bytes_per_second(&spec);
  unsigned int frameSize = pa_frame_size(&spec);

  m_Stream = pa_stream_new_extended(m_Context, "kodi audio stream", info, 1, NULL);
  pa_format_info_free(info[0]);

  if (m_Stream == NULL)
  {
    CLog::Log(LOGERROR, "PulseAudio: Could not create a stream");
    pa_threaded_mainloop_unlock(m_MainLoop);
    Deinitialize();
    return false;
  }

  pa_stream_set_state_callback(m_Stream, StreamStateCallback, m_MainLoop);
  pa_stream_set_write_callback(m_Stream, StreamRequestCallback, m_MainLoop);
  pa_stream_set_latency_update_callback(m_Stream, StreamLatencyUpdateCallback, m_MainLoop);

  pa_buffer_attr buffer_attr;

  // 200ms max latency
  // 50ms min packet size
  if(sinkStruct.isHWDevice || isDefaultDevice)
  {
    unsigned int latency = m_BytesPerSecond / 5;
    unsigned int process_time = latency / 4;
    memset(&buffer_attr, 0, sizeof(buffer_attr));
    buffer_attr.tlength = (uint32_t) latency;
    buffer_attr.minreq = (uint32_t) process_time;
    buffer_attr.maxlength = (uint32_t) -1;
    buffer_attr.prebuf = (uint32_t) -1;
    buffer_attr.fragsize = (uint32_t) latency;
  }

  if (pa_stream_connect_playback(m_Stream, isDefaultDevice ? NULL : device.c_str(), sinkStruct.isHWDevice ? &buffer_attr : NULL, ((pa_stream_flags)(PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_ADJUST_LATENCY)), NULL, NULL) < 0)
  {
    CLog::Log(LOGERROR, "PulseAudio: Failed to connect stream to output");
    pa_threaded_mainloop_unlock(m_MainLoop);
    Deinitialize();
    return false;
  }

  /* Wait until the stream is ready */
  do
  {
    pa_threaded_mainloop_wait(m_MainLoop);
    CLog::Log(LOGDEBUG, "PulseAudio: Stream %s", StreamStateToString(pa_stream_get_state(m_Stream)));
  }
  while (pa_stream_get_state(m_Stream) != PA_STREAM_READY && pa_stream_get_state(m_Stream) != PA_STREAM_FAILED);

  if (pa_stream_get_state(m_Stream) == PA_STREAM_FAILED)
  {
    CLog::Log(LOGERROR, "PulseAudio: Waited for the stream but it failed");
    pa_threaded_mainloop_unlock(m_MainLoop);
    Deinitialize();
    return false;
  }

  const pa_buffer_attr *a;

  if (!(a = pa_stream_get_buffer_attr(m_Stream)))
  {
    CLog::Log(LOGERROR, "PulseAudio: %s", pa_strerror(pa_context_errno(m_Context)));
    pa_threaded_mainloop_unlock(m_MainLoop);
    Deinitialize();
    return false;
  }
  else
  {
    unsigned int packetSize = a->minreq;
    m_BufferSize = a->tlength;

    format.m_frames = packetSize / frameSize;
  }

  {
    CSingleLock lock(m_sec);
    // Register Callback for Sink changes
    pa_context_set_subscribe_callback(m_Context, SinkChangedCallback, this);
    const pa_subscription_mask_t mask = PA_SUBSCRIPTION_MASK_SINK;
    pa_operation *op = pa_context_subscribe(m_Context, mask, NULL, this);
    if (op != NULL)
      pa_operation_unref(op);

    // Register Callback for Sink Info changes - this handles volume
    pa_context_set_subscribe_callback(m_Context, SinkInputInfoChangedCallback, this);
    const pa_subscription_mask_t mask_input = PA_SUBSCRIPTION_MASK_SINK_INPUT;
    pa_operation* op_sinfo = pa_context_subscribe(m_Context, mask_input, NULL, this);
    if (op_sinfo != NULL)
      pa_operation_unref(op_sinfo);
  }

  pa_threaded_mainloop_unlock(m_MainLoop);
  
  format.m_frameSize = frameSize;
  format.m_frameSamples = format.m_frames * format.m_channelLayout.Count();
  m_format = format;
  format.m_dataFormat = m_passthrough ? AE_FMT_S16NE : format.m_dataFormat;

  CLog::Log(LOGNOTICE, "PulseAudio: Opened device %s in %s mode with Buffersize %u ms",
                      device.c_str(), m_passthrough ? "passthrough" : "pcm",
                      (unsigned int) ((m_BufferSize / (float) m_BytesPerSecond) * 1000));

  // Cork stream will resume when adding first package
  Pause(true);
  {
    CSingleLock lock(m_sec);
    m_IsAllocated = true;
  }

  return true;
}

void CAESinkPULSE::Deinitialize()
{
  CSingleLock lock(m_sec);
  m_IsAllocated = false;
  m_passthrough = false;

  if (m_Stream)
    Drain();

  if (m_MainLoop)
    pa_threaded_mainloop_stop(m_MainLoop);

  if (m_Stream)
  {
    pa_stream_disconnect(m_Stream);
    pa_stream_unref(m_Stream);
    m_Stream = NULL;
    m_IsStreamPaused = false;
  }

  if (m_Context)
  {
    pa_context_disconnect(m_Context);
    pa_context_unref(m_Context);
    m_Context = NULL;
  }

  if (m_MainLoop)
  {
    pa_threaded_mainloop_free(m_MainLoop);
    m_MainLoop = NULL;
  }
}

void CAESinkPULSE::GetDelay(AEDelayStatus& status)
{
  if (!m_IsAllocated)
  {
    status.SetDelay(0);
    return;
  }
  int error = 0;
  pa_usec_t latency = (pa_usec_t) -1;
  pa_threaded_mainloop_lock(m_MainLoop);
  if ((error = pa_stream_get_latency(m_Stream, &latency, NULL)) < 0)
  {
    if (error == -PA_ERR_NODATA)
    {
      WaitForOperation(pa_stream_update_timing_info(m_Stream, NULL,NULL), m_MainLoop, "Update Timing Information");
      if ((error = pa_stream_get_latency(m_Stream, &latency, NULL)) < 0)
      {
        CLog::Log(LOGDEBUG, "GetDelay - Failed to get Latency %d", error); 
      }
    }
  }
  if (error < 0 )
    latency = (pa_usec_t) 0;

  pa_threaded_mainloop_unlock(m_MainLoop);
  status.SetDelay(latency / 1000000.0);
}

double CAESinkPULSE::GetCacheTotal()
{
  return (float)m_BufferSize / (float)m_BytesPerSecond;
}

unsigned int CAESinkPULSE::AddPackets(uint8_t **data, unsigned int frames, unsigned int offset)
{
  if (!m_IsAllocated)
    return 0;

  if (m_IsStreamPaused)
  {
    Pause(false);
  }

  pa_threaded_mainloop_lock(m_MainLoop);

  unsigned int available = frames * m_format.m_frameSize;
  unsigned int length = 0;
  void *buffer = data[0]+offset*m_format.m_frameSize;
  // revisit me after Gotham - should use a callback for the write function
  while ((length = pa_stream_writable_size(m_Stream)) == 0)
    pa_threaded_mainloop_wait(m_MainLoop);

  length =  std::min((unsigned int)length, available);

  int error = pa_stream_write(m_Stream, buffer, length, NULL, 0, PA_SEEK_RELATIVE);
  pa_threaded_mainloop_unlock(m_MainLoop);

  if (error)
  {
    CLog::Log(LOGERROR, "CPulseAudioDirectSound::AddPackets - pa_stream_write failed\n");
    return 0;
  }

  return (unsigned int)(length / m_format.m_frameSize);
}

void CAESinkPULSE::Drain()
{
  if (!m_IsAllocated)
    return;

  pa_threaded_mainloop_lock(m_MainLoop);
  WaitForOperation(pa_stream_drain(m_Stream, NULL, NULL), m_MainLoop, "Drain");
  pa_threaded_mainloop_unlock(m_MainLoop);
}

// This is a helper to get stream info during the PA callbacks
// it shall never be called from real outside
pa_stream* CAESinkPULSE::GetInternalStream()
{
  return m_Stream;
}

void CAESinkPULSE::UpdateInternalVolume(const pa_cvolume* nVol)
{
  if (!nVol)
    return;

  pa_volume_t o_vol = pa_cvolume_avg(&m_Volume);
  pa_volume_t n_vol = pa_cvolume_avg(nVol);

  if (o_vol != n_vol)
  {
    pa_cvolume_set(&m_Volume, m_Channels, n_vol);
    m_volume_needs_update = true;
  }
}

void CAESinkPULSE::SetVolume(float volume)
{
  if (m_IsAllocated && !m_passthrough)
  {
    pa_threaded_mainloop_lock(m_MainLoop);
    // clamp possibly too large / low values
    float per_cent_volume = std::max(0.0f, std::min(volume, 1.0f));
    
    if (m_volume_needs_update)
    {
       m_volume_needs_update = false;
       pa_volume_t n_vol = pa_cvolume_avg(&m_Volume); 
       n_vol = std::min(n_vol, PA_VOLUME_NORM);
       per_cent_volume = (float) n_vol / PA_VOLUME_NORM; 
       // only update internal volume
       pa_threaded_mainloop_unlock(m_MainLoop);
       g_application.SetVolume(per_cent_volume, false);
       return;
    }
    
    pa_volume_t pavolume = per_cent_volume * PA_VOLUME_NORM;
    unsigned int sink_input_idx = pa_stream_get_index(m_Stream);
    
    if ( pavolume <= 0 )
      pa_cvolume_mute(&m_Volume, m_Channels);
    else
      pa_cvolume_set(&m_Volume, m_Channels, pavolume);
        
      pa_operation *op = pa_context_set_sink_input_volume(m_Context, sink_input_idx, &m_Volume, NULL, NULL);
      if (op == NULL)
        CLog::Log(LOGERROR, "PulseAudio: Failed to set volume");
      else
        pa_operation_unref(op);

    pa_threaded_mainloop_unlock(m_MainLoop);
  }
}

void CAESinkPULSE::EnumerateDevicesEx(AEDeviceInfoList &list, bool force)
{
  pa_context *context;
  pa_threaded_mainloop *mainloop;

  if (!SetupContext(NULL, &context, &mainloop))
  {
    CLog::Log(LOGNOTICE, "PulseAudio might not be running. Context was not created.");
    return;
  }

  pa_threaded_mainloop_lock(mainloop);

  SinkInfoStruct sinkStruct;
  sinkStruct.mainloop = mainloop;
  sinkStruct.list = &list;
  WaitForOperation(pa_context_get_sink_info_list(context, SinkInfoRequestCallback, &sinkStruct), mainloop, "EnumerateAudioSinks");

  pa_threaded_mainloop_unlock(mainloop);

  if (mainloop)
    pa_threaded_mainloop_stop(mainloop);

  if (context)
  {
    pa_context_disconnect(context);
    pa_context_unref(context);
    context = NULL;
  }

  if (mainloop)
  {
    pa_threaded_mainloop_free(mainloop);
    mainloop = NULL;
  }
}

bool CAESinkPULSE::IsInitialized()
{
 CSingleLock lock(m_sec);
 return m_IsAllocated; 
}

void CAESinkPULSE::Pause(bool pause)
{
  pa_threaded_mainloop_lock(m_MainLoop);

  if (!WaitForOperation(pa_stream_cork(m_Stream, pause ? 1 : 0, NULL, NULL), m_MainLoop, pause ? "Pause" : "Resume"))
    pause = !pause;

  m_IsStreamPaused = pause;
  pa_threaded_mainloop_unlock(m_MainLoop);
}

inline bool CAESinkPULSE::WaitForOperation(pa_operation *op, pa_threaded_mainloop *mainloop, const char *LogEntry = "")
{
  if (op == NULL)
    return false;

  bool sucess = true;

  while (pa_operation_get_state(op) == PA_OPERATION_RUNNING)
    pa_threaded_mainloop_wait(mainloop);

  if (pa_operation_get_state(op) != PA_OPERATION_DONE)
  {
    CLog::Log(LOGERROR, "PulseAudio: %s Operation failed", LogEntry);
    sucess = false;
  }

  pa_operation_unref(op);
  return sucess;
}

bool CAESinkPULSE::SetupContext(const char *host, pa_context **context, pa_threaded_mainloop **mainloop)
{
  if ((*mainloop = pa_threaded_mainloop_new()) == NULL)
  {
    CLog::Log(LOGERROR, "PulseAudio: Failed to allocate main loop");
    return false;
  }

  if (((*context) = pa_context_new(pa_threaded_mainloop_get_api(*mainloop), "Kodi")) == NULL)
  {
    CLog::Log(LOGERROR, "PulseAudio: Failed to allocate context");
    return false;
  }

  pa_context_set_state_callback(*context, ContextStateCallback, *mainloop);

  if (pa_context_connect(*context, host, (pa_context_flags_t)0, NULL) < 0)
  {
    CLog::Log(LOGERROR, "PulseAudio: Failed to connect context");
    return false;
  }
  pa_threaded_mainloop_lock(*mainloop);

  if (pa_threaded_mainloop_start(*mainloop) < 0)
  {
    CLog::Log(LOGERROR, "PulseAudio: Failed to start MainLoop");
    pa_threaded_mainloop_unlock(*mainloop);
    return false;
  }

  /* Wait until the context is ready */
  do
  {
    pa_threaded_mainloop_wait(*mainloop);
    CLog::Log(LOGDEBUG, "PulseAudio: Context %s", ContextStateToString(pa_context_get_state(*context)));
  }
  while (pa_context_get_state(*context) != PA_CONTEXT_READY && pa_context_get_state(*context) != PA_CONTEXT_FAILED);

  if (pa_context_get_state(*context) == PA_CONTEXT_FAILED)
  {
    CLog::Log(LOGERROR, "PulseAudio: Waited for the Context but it failed");
    pa_threaded_mainloop_unlock(*mainloop);
    return false;
  }

  pa_threaded_mainloop_unlock(*mainloop);
  return true;
}
#endif
