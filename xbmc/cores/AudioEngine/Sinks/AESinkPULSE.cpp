/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "AESinkPULSE.h"

#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationVolumeHandling.h"
#include "cores/AudioEngine/AESinkFactory.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <array>
#include <memory>
#include <mutex>

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

class CDriverMonitor
{
public:
  CDriverMonitor() = default;
  virtual ~CDriverMonitor();
  bool Start();
  bool IsInitialized();

  CCriticalSection m_sec;

protected:
  pa_context* m_pContext = nullptr;
  pa_threaded_mainloop* m_pMainLoop = nullptr;
  bool m_isInit = false;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

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

static pa_sample_format AEStreamFormatToPulseFormat(CAEStreamInfo::DataType type)
{
  switch (type)
  {
    case CAEStreamInfo::STREAM_TYPE_AC3:
    case CAEStreamInfo::STREAM_TYPE_DTS_512:
    case CAEStreamInfo::STREAM_TYPE_DTS_1024:
    case CAEStreamInfo::STREAM_TYPE_DTS_2048:
    case CAEStreamInfo::STREAM_TYPE_DTSHD_CORE:
    case CAEStreamInfo::STREAM_TYPE_EAC3:
      return PA_SAMPLE_S16NE;

    default:
      return PA_SAMPLE_INVALID;
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

    default:
      return PA_SAMPLE_INVALID;
  }
}

static pa_encoding AEStreamFormatToPulseEncoding(CAEStreamInfo::DataType type)
{
  switch (type)
  {
    case CAEStreamInfo::STREAM_TYPE_AC3:
      return PA_ENCODING_AC3_IEC61937;

    case CAEStreamInfo::STREAM_TYPE_DTS_512:
    case CAEStreamInfo::STREAM_TYPE_DTS_1024:
    case CAEStreamInfo::STREAM_TYPE_DTS_2048:
    case CAEStreamInfo::STREAM_TYPE_DTSHD_CORE:
      return PA_ENCODING_DTS_IEC61937;

    case CAEStreamInfo::STREAM_TYPE_EAC3:
      return PA_ENCODING_EAC3_IEC61937;

    default:
      return PA_ENCODING_INVALID;
  }
}

static pa_encoding AEFormatToPulseEncoding(AEDataFormat format)
{
  switch (format)
  {
    case AE_FMT_RAW:
      return PA_ENCODING_INVALID;

    default:
      return PA_ENCODING_PCM;
  }
}

namespace
{

// clang-format off
constexpr std::array<AEDataFormat, 6> defaultDataFormats = {
  AE_FMT_U8,
  AE_FMT_S16NE,
  AE_FMT_S24NE3,
  AE_FMT_S24NE4,
  AE_FMT_S32NE,
  AE_FMT_FLOAT
};

constexpr std::array<unsigned int, 14> defaultSampleRates = {
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
// clang-format on

} // namespace

/* Static callback functions */

static void ContextStateCallback(pa_context *c, void *userdata)
{
  pa_threaded_mainloop* m = static_cast<pa_threaded_mainloop*>(userdata);
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
  pa_threaded_mainloop* m = static_cast<pa_threaded_mainloop*>(userdata);
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
  CAESinkPULSE* p = static_cast<CAESinkPULSE*>(userdata);
  if (!p)
    return;

  pa_threaded_mainloop* m = p->GetInternalMainLoop();
  // pulse always tells us the total number of bytes
  // we can add.
  p->m_requestedBytes = static_cast<int>(length);
  pa_threaded_mainloop_signal(m, 0);
}

static void StreamLatencyUpdateCallback(pa_stream *s, void *userdata)
{
  pa_threaded_mainloop* m = static_cast<pa_threaded_mainloop*>(userdata);
  pa_threaded_mainloop_signal(m, 0);
}


static void SinkInputInfoCallback(pa_context *c, const pa_sink_input_info *i, int eol, void *userdata)
{
  CAESinkPULSE *p = static_cast<CAESinkPULSE*>(userdata);
  if (!p || !p->IsInitialized())
    return;

  if(i && i->has_volume && !i->corked)
    p->UpdateInternalVolume(&(i->volume));
}

static void SinkCallback(pa_context* c,
                         pa_subscription_event_type_t t,
                         uint32_t idx,
                         void* userdata)
{
  CDriverMonitor* p = static_cast<CDriverMonitor*>(userdata);
  if (!p)
    return;

  std::unique_lock<CCriticalSection> lock(p->m_sec);
  if (p->IsInitialized())
  {
    if ((t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SINK)
    {
      if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_NEW)
      {
        CLog::Log(LOGDEBUG, "Sink appeared");
        CServiceBroker::GetActiveAE()->DeviceCountChange("PULSE");
      }
      else if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE)
      {
        CLog::Log(LOGDEBUG, "Sink removed");
        CServiceBroker::GetActiveAE()->DeviceCountChange("PULSE");
      }
      else if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_CHANGE)
      {
        CLog::Log(LOGDEBUG, "Sink changed");
      }
    }
    else
    {
      CLog::Log(LOGDEBUG, "Not subscribed to Event: {}", static_cast<int>(t));
    }
  }
}

static void SinkChangedCallback(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *userdata)
{
  CAESinkPULSE* p = static_cast<CAESinkPULSE*>(userdata);
  if(!p)
    return;

  std::unique_lock<CCriticalSection> lock(p->m_sec);
  if (p->IsInitialized())
  {
    if ((t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SINK_INPUT)
    {
      // when we get a sink input event volume might have changed
      if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_CHANGE)
      {
        if (idx != pa_stream_get_index(p->GetInternalStream()))
          return;

        // we need to leave the lock as we trigger a second callback
        CSingleExit exitlock(p->m_sec);
        pa_operation* op = pa_context_get_sink_input_info(c, idx, SinkInputInfoCallback, p);
        if (op == NULL)
          CLog::Log(LOGERROR, "PulseAudio: Failed to sync volume");
        else
          pa_operation_unref(op);
      }
    }
    else
    {
      CLog::Log(LOGDEBUG, "Not subscribed to Event: {}", static_cast<int>(t));
    }
  }
}

struct SinkInfoStruct
{
  AEDeviceInfoList *list;
  bool isHWDevice;
  bool isNWDevice;
  bool isBTDevice;
  bool device_found;
  pa_threaded_mainloop *mainloop;
  int samplerate;
  pa_channel_map map;
  SinkInfoStruct()
  {
    list = nullptr;
    isHWDevice = false;
    isNWDevice = false;
    isBTDevice = false;
    device_found = true;
    mainloop = NULL; //called into C
    samplerate = 0;
    pa_channel_map_init(&map);
  }
};

struct ModuleInfoStruct
{
  pa_threaded_mainloop *mainloop;
  bool hasAllowPT;
  ModuleInfoStruct()
  {
    mainloop = NULL; //called into C
    hasAllowPT = false;
  }
};

static void SinkInfoCallback(pa_context *c, const pa_sink_info *i, int eol, void *userdata)
{
  SinkInfoStruct *sinkStruct = static_cast<SinkInfoStruct*>(userdata);
  if (!sinkStruct)
    return;

  if(i)
  {
    if (i->flags)
    {
      if (i->flags & PA_SINK_HARDWARE)
        sinkStruct->isHWDevice = true;

      if (i->flags & PA_SINK_NETWORK)
        sinkStruct->isNWDevice = true;

      sinkStruct->isBTDevice =
          StringUtils::EndsWithNoCase(std::string(i->name), std::string("a2dp_sink"));
      if (sinkStruct->isBTDevice)
        CLog::Log(LOGINFO, "Found BT Device - will adjust buffers to larger values");

      sinkStruct->samplerate = i->sample_spec.rate;
      sinkStruct->device_found = true;
      sinkStruct->map = i->channel_map;
    }
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

static void ModuleInfoCallback(pa_context* c, const pa_module_info *i, int eol, void *userdata)
{
  ModuleInfoStruct *mis = static_cast<ModuleInfoStruct*>(userdata);
  if (!mis)
    return;

  if (i)
  {
    if (strcmp(i->name, "module-allow-passthrough") == 0)
      mis->hasAllowPT = true;
  }
  pa_threaded_mainloop_signal(mis->mainloop, 0);
}

static void SinkInfoRequestCallback(pa_context *c, const pa_sink_info *i, int eol, void *userdata)
{

  SinkInfoStruct *sinkStruct = static_cast<SinkInfoStruct*>(userdata);
  if (!sinkStruct)
    return;

  if(sinkStruct->list->empty())
  {
    //add a default device first
    CAEDeviceInfo defaultDevice;
    defaultDevice.m_deviceName = std::string("Default");
    defaultDevice.m_displayName = std::string("Default");
    defaultDevice.m_displayNameExtra = std::string("Default Output Device (PULSEAUDIO)");
    defaultDevice.m_dataFormats.insert(defaultDevice.m_dataFormats.end(),
                                       defaultDataFormats.begin(), defaultDataFormats.end());
    defaultDevice.m_channels = CAEChannelInfo(AE_CH_LAYOUT_2_0);
    defaultDevice.m_sampleRates.assign(defaultSampleRates.begin(), defaultSampleRates.end());
    defaultDevice.m_deviceType = AE_DEVTYPE_PCM;
    defaultDevice.m_wantsIECPassthrough = true;
    sinkStruct->list->push_back(defaultDevice);
  }
  if (i && i->name)
  {
    CAEDeviceInfo device;
    bool valid = true;
    device.m_deviceName = std::string(i->name);
    device.m_displayName = std::string(i->description);
    if (i->active_port && i->active_port->description)
      device.m_displayNameExtra = std::string((i->active_port->description)).append(" (PULSEAUDIO)");
    else
      device.m_displayNameExtra = std::string((i->description)).append(" (PULSEAUDIO)");
    unsigned int device_type = AE_DEVTYPE_PCM; //0

    device.m_channels = PAChannelToAEChannelMap(i->channel_map);

    // Don't add devices that would not have a channel map
    if(device.m_channels.Count() == 0)
      valid = false;

    device.m_sampleRates.assign(defaultSampleRates.begin(), defaultSampleRates.end());

    for (unsigned int j = 0; j < i->n_formats; j++)
    {
      switch(i->formats[j]->encoding)
      {
        case PA_ENCODING_AC3_IEC61937:
          device.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_AC3);
          device_type = AE_DEVTYPE_IEC958;
          break;
        case PA_ENCODING_DTS_IEC61937:
          device.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTSHD_CORE);
          device.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_1024);
          device.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_512);
          device.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_DTS_2048);
          device_type = AE_DEVTYPE_IEC958;
          break;
        case PA_ENCODING_EAC3_IEC61937:
          device.m_streamTypes.push_back(CAEStreamInfo::STREAM_TYPE_EAC3);
          device_type = AE_DEVTYPE_IEC958;
          break;
        case PA_ENCODING_PCM:
          device.m_dataFormats.insert(device.m_dataFormats.end(), defaultDataFormats.begin(),
                                      defaultDataFormats.end());
          break;
        default:
          break;
      }
    }
    // passthrough is only working when device has Stereo channel config
    if (device_type > AE_DEVTYPE_PCM && device.m_channels.Count() == 2)
    {
      device.m_deviceType = AE_DEVTYPE_IEC958;
      device.m_dataFormats.push_back(AE_FMT_RAW);
    }
    else
      device.m_deviceType = AE_DEVTYPE_PCM;

    device.m_wantsIECPassthrough = true;

    if(valid)
    {
      CLog::Log(LOGDEBUG, "PulseAudio: Found {} with devicestring {}", device.m_displayName,
                device.m_deviceName);
      sinkStruct->list->push_back(device);
    }
    else
    {
      CLog::Log(LOGDEBUG, "PulseAudio: Skipped {} with devicestring {}", device.m_displayName,
                device.m_deviceName);
    }
  }
  pa_threaded_mainloop_signal(sinkStruct->mainloop, 0);
}

static bool SetupContext(const char* host,
                         const char* appname,
                         pa_context** context,
                         pa_threaded_mainloop** mainloop)
{
  if ((*mainloop = pa_threaded_mainloop_new()) == nullptr)
  {
    CLog::Log(LOGERROR, "PulseAudio: Failed to allocate main loop");
    return false;
  }

  if (((*context) = pa_context_new(pa_threaded_mainloop_get_api(*mainloop), appname)) == nullptr)
  {
    CLog::Log(LOGERROR, "PulseAudio: Failed to allocate context");
    return false;
  }

  pa_context_set_state_callback(*context, ContextStateCallback, *mainloop);

  if (pa_context_connect(*context, host, (pa_context_flags_t)0, nullptr) < 0)
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
    CLog::Log(LOGDEBUG, "PulseAudio: Context {}",
              ContextStateToString(pa_context_get_state(*context)));
  } while (pa_context_get_state(*context) != PA_CONTEXT_READY &&
           pa_context_get_state(*context) != PA_CONTEXT_FAILED);

  if (pa_context_get_state(*context) == PA_CONTEXT_FAILED)
  {
    CLog::Log(LOGERROR, "PulseAudio: Waited for the Context but it failed");
    pa_threaded_mainloop_unlock(*mainloop);
    return false;
  }

  pa_threaded_mainloop_unlock(*mainloop);
  return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

CDriverMonitor::~CDriverMonitor()
{
  m_isInit = false;

  if (m_pMainLoop)
    pa_threaded_mainloop_stop(m_pMainLoop);

  if (m_pContext)
  {
    pa_context_disconnect(m_pContext);
    pa_context_unref(m_pContext);
    m_pContext = nullptr;
  }

  if (m_pMainLoop)
  {
    pa_threaded_mainloop_free(m_pMainLoop);
    m_pMainLoop = nullptr;
  }
}

bool CDriverMonitor::IsInitialized()
{
  return m_isInit;
}

bool CDriverMonitor::Start()
{
  if (!SetupContext(nullptr, "KodiDriver", &m_pContext, &m_pMainLoop))
  {
    CLog::Log(LOGINFO, "PulseAudio might not be running. Context was not created.");
    return false;
  }

  pa_threaded_mainloop_lock(m_pMainLoop);

  m_isInit = true;

  std::unique_lock<CCriticalSection> lock(m_sec);
  // Register Callback for Sink changes
  pa_context_set_subscribe_callback(m_pContext, SinkCallback, this);
  const pa_subscription_mask_t mask = pa_subscription_mask_t(PA_SUBSCRIPTION_MASK_SINK);
  pa_operation* op = pa_context_subscribe(m_pContext, mask, nullptr, this);
  if (op != nullptr)
    pa_operation_unref(op);

  pa_threaded_mainloop_unlock(m_pMainLoop);

  return true;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/* PulseAudio class memberfunctions*/

std::unique_ptr<CDriverMonitor> CAESinkPULSE::m_pMonitor;

bool CAESinkPULSE::Register()
{
  // check if pulseaudio is actually available
  pa_simple *s;
  pa_sample_spec ss;
  ss.format = PA_SAMPLE_S16NE;
  ss.channels = 2;
  ss.rate = 44100;
  s = pa_simple_new(NULL, "Kodi-Tester", PA_STREAM_PLAYBACK, NULL, "Test", &ss, NULL, NULL, NULL);
  if (!s)
  {
    CLog::Log(LOGINFO, "PulseAudio: Server not running");
    return false;
  }
  else
  {
    CLog::Log(LOGINFO, "PulseAudio: Server found running - will try to use Pulse");
    pa_simple_free(s);
  }

  m_pMonitor = std::make_unique<CDriverMonitor>();
  m_pMonitor->Start();

  AE::AESinkRegEntry entry;
  entry.sinkName = "PULSE";
  entry.createFunc = CAESinkPULSE::Create;
  entry.enumerateFunc = CAESinkPULSE::EnumerateDevicesEx;
  entry.cleanupFunc = CAESinkPULSE::Cleanup;
  AE::CAESinkFactory::RegisterSink(entry);
  return true;
}

std::unique_ptr<IAESink> CAESinkPULSE::Create(std::string& device, AEAudioFormat& desiredFormat)
{
  auto sink = std::make_unique<CAESinkPULSE>();
  if (sink->Initialize(desiredFormat, device))
    return sink;

  return {};
}
CAESinkPULSE::CAESinkPULSE()
{
  m_IsAllocated = false;
  m_passthrough = false;
  m_MainLoop = NULL;
  m_BytesPerSecond = 0;
  m_BufferSize = 0;
  m_Channels = 0;
  m_maxLatency = 0.0;
  m_Stream = NULL;
  m_Context = NULL;
  m_IsStreamPaused = false;
  m_volume_needs_update = false;
  m_periodSize = 0;
  pa_cvolume_init(&m_Volume);
}

CAESinkPULSE::~CAESinkPULSE()
{
  Deinitialize();
}

bool CAESinkPULSE::Initialize(AEAudioFormat &format, std::string &device)
{
  {
    std::unique_lock<CCriticalSection> lock(m_sec);
    m_IsAllocated = false;
  }
  m_passthrough = false;
  m_BytesPerSecond = 0;
  m_BufferSize = 0;
  m_Channels = 0;
  m_maxLatency = 0.0;
  m_Stream = NULL;
  m_Context = NULL;
  m_periodSize = 0;

  if (!SetupContext(NULL, "KodiSink", &m_Context, &m_MainLoop))
  {
    CLog::Log(LOGINFO, "PulseAudio might not be running. Context was not created.");
    Deinitialize();
    return false;
  }

  pa_threaded_mainloop_lock(m_MainLoop);

  struct pa_channel_map map;
  pa_channel_map_init(&map);

   // PULSE cannot cope with e.g. planar formats so we fall back to FLOAT
   // when we receive an invalid pulse format
   pa_sample_format pa_fmt;
   // PA can only handle IEC packed RAW format if we get a RAW format
   if (format.m_dataFormat == AE_FMT_RAW)
   {
     pa_fmt = AEStreamFormatToPulseFormat(format.m_streamInfo.m_type);
     m_passthrough = true;
   }
   else
    pa_fmt = AEFormatToPulseFormat(format.m_dataFormat);

   if (pa_fmt == PA_SAMPLE_INVALID)
   {
     CLog::Log(LOGDEBUG, "PULSE does not support format: {} - will fallback to AE_FMT_FLOAT",
               CAEUtil::DataFormatToStr(format.m_dataFormat));
     format.m_dataFormat = AE_FMT_FLOAT;
     pa_fmt = PA_SAMPLE_FLOAT32;
     m_passthrough = false;
   }
  // store information about current sink
  SinkInfoStruct sinkStruct;
  sinkStruct.mainloop = m_MainLoop;
  sinkStruct.device_found = false;

  // get real sample rate of the device we want to open - to avoid resampling
  bool isDefaultDevice = false;
  if(StringUtils::EndsWithNoCase(device, std::string("default")))
    isDefaultDevice = true;

  WaitForOperation(pa_context_get_sink_info_by_name(m_Context, isDefaultDevice ? NULL : device.c_str(), SinkInfoCallback, &sinkStruct), m_MainLoop, "Get Sink Info");
  // only check if the device is existing - don't alter the sample rate
  if (!sinkStruct.device_found)
  {
    CLog::Log(LOGERROR, "PulseAudio: Sink {} not found", device);
    pa_threaded_mainloop_unlock(m_MainLoop);
    Deinitialize();
    return false;
  }

  if(m_passthrough)
  {
    map.channels = 2;
    format.m_channelLayout = AE_CH_LAYOUT_2_0;
  }
  else
  {
    map = AEChannelMapToPAChannel(format.m_channelLayout);
    format.m_channelLayout = PAChannelToAEChannelMap(map);
  }
  m_Channels = format.m_channelLayout.Count();

  // Pulse can resample everything between 5 khz and 384 khz (since 9.0)
  unsigned int max_pulse_sample_rate = 384000U;
  format.m_sampleRate = std::max(5512U, std::min(format.m_sampleRate, max_pulse_sample_rate));

  pa_format_info *info[1];
  info[0] = pa_format_info_new();
  if (m_passthrough)
    info[0]->encoding = AEStreamFormatToPulseEncoding(format.m_streamInfo.m_type);
  else
   info[0]->encoding = AEFormatToPulseEncoding(format.m_dataFormat);

  if (info[0]->encoding == PA_ENCODING_INVALID)
  {
    CLog::Log(LOGERROR, "PulseAudio: Invalid Encoding");
    pa_format_info_free(info[0]);
    pa_threaded_mainloop_unlock(m_MainLoop);
    Deinitialize();
    return false;
  }

  if(!m_passthrough)
  {
    pa_format_info_set_sample_format(info[0], pa_fmt);
    pa_format_info_set_channel_map(info[0], &map);
  }
  pa_format_info_set_channels(info[0], m_Channels);

  // PA requires the original encoded rate in order to do EAC3
  unsigned int samplerate = format.m_sampleRate;
  if (m_passthrough && (info[0]->encoding == PA_ENCODING_EAC3_IEC61937))
  {
    // this is only used internally for PA to use EAC3
    samplerate = format.m_streamInfo.m_sampleRate;
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
  pa_format_info_to_sample_spec(info[0], &spec, NULL);
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
  pa_stream_set_write_callback(m_Stream, StreamRequestCallback, this);
  pa_stream_set_latency_update_callback(m_Stream, StreamLatencyUpdateCallback, m_MainLoop);

  // default buffer construction
  // align with AE's max buffer
  unsigned int latency = m_BytesPerSecond / 2.5; // 400 ms
  unsigned int process_time = latency / 4; // 100 ms
  if (sinkStruct.isHWDevice && !sinkStruct.isNWDevice && !sinkStruct.isBTDevice)
  {
    // on hw devices buffers can be further reduced
    // 200ms max latency
    // 50ms min packet size
    latency = m_BytesPerSecond / 5;
    process_time = latency / 4;
  }

  pa_buffer_attr buffer_attr;
  buffer_attr.fragsize = latency;
  buffer_attr.maxlength = (uint32_t) -1;
  buffer_attr.minreq = process_time;
  buffer_attr.prebuf = (uint32_t) -1;
  buffer_attr.tlength = latency;
  int flags = (PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_ADJUST_LATENCY);

  if (m_passthrough)
    flags |= PA_STREAM_PASSTHROUGH;

  if (pa_stream_connect_playback(m_Stream, isDefaultDevice ? NULL : device.c_str(), &buffer_attr, (pa_stream_flags) flags, NULL, NULL) < 0)
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
    CLog::Log(LOGDEBUG, "PulseAudio: Stream {}",
              StreamStateToString(pa_stream_get_state(m_Stream)));
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
    CLog::Log(LOGERROR, "PulseAudio: {}", pa_strerror(pa_context_errno(m_Context)));
    pa_threaded_mainloop_unlock(m_MainLoop);
    Deinitialize();
    return false;
  }
  else
  {
    unsigned int packetSize = a->minreq;
    m_BufferSize = a->tlength;
    m_periodSize = a->minreq;

    format.m_frames = packetSize / frameSize;
    m_maxLatency = static_cast<double>(m_BufferSize) / m_BytesPerSecond;
  }

  {
    std::unique_lock<CCriticalSection> lock(m_sec);
    // Register Callback for Sink changes
    pa_context_set_subscribe_callback(m_Context, SinkChangedCallback, this);
    const pa_subscription_mask_t mask = pa_subscription_mask_t(PA_SUBSCRIPTION_MASK_SINK_INPUT);
    pa_operation *op = pa_context_subscribe(m_Context, mask, NULL, this);
    if (op != NULL)
      pa_operation_unref(op);
  }

  pa_threaded_mainloop_unlock(m_MainLoop);

  format.m_frameSize = frameSize;
  m_format = format;
  format.m_dataFormat = m_passthrough ? AE_FMT_S16NE : format.m_dataFormat;

  CLog::Log(LOGINFO,
            "PulseAudio: Opened device {} in {} mode with Buffersize {} ms Periodsize {} ms",
            device, m_passthrough ? "passthrough" : "pcm",
            static_cast<unsigned int>(1000.0 * m_BufferSize / m_BytesPerSecond),
            static_cast<unsigned int>(1000.0 * m_periodSize / m_BytesPerSecond));

  // Cork stream will resume when adding first package
  Pause(true);
  {
    std::unique_lock<CCriticalSection> lock(m_sec);
    m_IsAllocated = true;
  }
  return true;
}

void CAESinkPULSE::Deinitialize()
{
  std::unique_lock<CCriticalSection> lock(m_sec);
  m_IsAllocated = false;
  m_passthrough = false;
  m_periodSize = 0;
  m_requestedBytes = 0;
  m_maxLatency = 0.0;

  if (m_Stream)
  {
    CSingleExit exit(m_sec);
    pa_threaded_mainloop_lock(m_MainLoop);
    WaitForOperation(pa_stream_flush(m_Stream, NULL, NULL), m_MainLoop, "Flush");
    pa_threaded_mainloop_unlock(m_MainLoop);
  }

  {
    CSingleExit exit(m_sec);
    if (m_MainLoop)
      pa_threaded_mainloop_stop(m_MainLoop);
  }

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

  pa_threaded_mainloop_lock(m_MainLoop);
  pa_usec_t r_usec;
  int negative;

  if (pa_stream_get_latency(m_Stream, &r_usec, &negative) < 0)
    r_usec = 0;

  double delay = r_usec / 1000000.0;
  if (delay > m_maxLatency)
    m_maxLatency = delay;

  pa_threaded_mainloop_unlock(m_MainLoop);
  status.SetDelay(delay);
}

double CAESinkPULSE::GetCacheTotal()
{
  return m_maxLatency;
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
  unsigned int length = m_periodSize;
  void *buffer = data[0]+offset*m_format.m_frameSize;
  auto wait_time =
      std::chrono::duration<double>(static_cast<double>(m_BufferSize) / m_BytesPerSecond);
  XbmcThreads::EndTime<std::chrono::duration<double>> timer(wait_time);
  // we don't want to block forever - if timer expires pa_stream_write will
  // fail - therefore we don't care and just return 0;
  while (!timer.IsTimePast())
  {
    if (m_requestedBytes > 0)
      break;
    pa_threaded_mainloop_wait(m_MainLoop);
  }

  if (timer.IsTimePast())
  {
    CLog::Log(LOGERROR, "Sink Timer expired for more than buffer time: {}s", wait_time.count());
    pa_threaded_mainloop_unlock(m_MainLoop);
    return 0;
  }

  length = std::min(length, available);
  int error = pa_stream_write(m_Stream, buffer, length, NULL, 0, PA_SEEK_RELATIVE);
  pa_threaded_mainloop_unlock(m_MainLoop);
  if (error)
  {
    CLog::Log(LOGERROR, "CAESinkPULSE::AddPackets - pa_stream_write failed: {}", error);
    return 0;
  }

  // subtract here, as we might come back earlier than our callback and there is
  // still space in the buffer to write another time
  m_requestedBytes -= length;

  unsigned int res = length / m_format.m_frameSize;

  return res;
}

void CAESinkPULSE::Drain()
{
  if (!m_IsAllocated)
    return;

  pa_threaded_mainloop_lock(m_MainLoop);
  WaitForOperation(pa_stream_drain(m_Stream, NULL, NULL), m_MainLoop, "Drain");
  WaitForOperation(pa_stream_cork(m_Stream, 1, NULL, NULL), m_MainLoop, "Pause");
  m_IsStreamPaused = true;
  pa_threaded_mainloop_unlock(m_MainLoop);
}

// This is a helper to get stream info during the PA callbacks
// it shall never be called from real outside
pa_stream* CAESinkPULSE::GetInternalStream()
{
  return m_Stream;
}

// This is a helper to use the internal mainloop from another thread, e.g. a RequestCallback
// it is shipped via the userdata. Don't use it for other purposes than signalling
pa_threaded_mainloop* CAESinkPULSE::GetInternalMainLoop()
{
  return m_MainLoop;
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
       per_cent_volume = static_cast<float>(n_vol) / PA_VOLUME_NORM;
       // only update internal volume
       pa_threaded_mainloop_unlock(m_MainLoop);
       auto& components = CServiceBroker::GetAppComponents();
       const auto appVolume = components.GetComponent<CApplicationVolumeHandling>();
       appVolume->SetVolume(per_cent_volume, false);
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

  if (!SetupContext(NULL, "KodiSink", &context, &mainloop))
  {
    CLog::Log(LOGINFO, "PulseAudio might not be running. Context was not created.");
    return;
  }

  pa_threaded_mainloop_lock(mainloop);

  SinkInfoStruct sinkStruct;
  sinkStruct.mainloop = mainloop;
  sinkStruct.list = &list;

  ModuleInfoStruct mis;
  mis.mainloop = mainloop;

  WaitForOperation(pa_context_get_module_info_list(context, ModuleInfoCallback, &mis), mainloop, "Check PA Modules");
  if (!mis.hasAllowPT)
  {
    CLog::Log(LOGWARNING, "Pulseaudio module module-allow-passthrough not loaded - opening PT devices might fail");
  }
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
  std::unique_lock<CCriticalSection> lock(m_sec);
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

  bool success = true;

  while (pa_operation_get_state(op) == PA_OPERATION_RUNNING)
    pa_threaded_mainloop_wait(mainloop);

  if (pa_operation_get_state(op) != PA_OPERATION_DONE)
  {
    CLog::Log(LOGERROR, "PulseAudio: {} Operation failed", LogEntry);
    success = false;
  }

  pa_operation_unref(op);
  return success;
}

void CAESinkPULSE::Cleanup()
{
  m_pMonitor.reset();
}
