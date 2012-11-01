/*
 *      Copyright (C) 2010-2012 Team XBMC
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

#include "PulseAEStream.h"
#include "AEFactory.h"
#include "Utils/AEUtil.h"
#include "utils/log.h"
#include "utils/MathUtils.h"
#include "threads/SingleLock.h"

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

CPulseAEStream::CPulseAEStream(pa_context *context, pa_threaded_mainloop *mainLoop, enum AEDataFormat format, unsigned int sampleRate, CAEChannelInfo channelLayout, unsigned int options) : m_fader(this)
{
  ASSERT(channelLayout.Count());
  m_Destroyed = false;
  m_Initialized = false;
  m_Paused = false;

  m_Stream = NULL;
  m_Context = context;
  m_MainLoop = mainLoop;

  m_format = format;
  m_sampleRate = sampleRate;
  m_channelLayout = channelLayout;
  m_options = options;

  m_DrainOperation = NULL;
  m_slave = NULL;

  pa_threaded_mainloop_lock(m_MainLoop);

  m_SampleSpec.channels = channelLayout.Count();
  m_SampleSpec.rate = m_sampleRate;

  switch (m_format)
  {
    case AE_FMT_U8    : m_SampleSpec.format = PA_SAMPLE_U8; break;
    case AE_FMT_S16NE : m_SampleSpec.format = PA_SAMPLE_S16NE; break;
    case AE_FMT_S16LE : m_SampleSpec.format = PA_SAMPLE_S16LE; break;
    case AE_FMT_S16BE : m_SampleSpec.format = PA_SAMPLE_S16BE; break;
    case AE_FMT_S24NE3: m_SampleSpec.format = PA_SAMPLE_S24NE; break;
    case AE_FMT_S24NE4: m_SampleSpec.format = PA_SAMPLE_S24_32NE; break;
    case AE_FMT_S32NE : m_SampleSpec.format = PA_SAMPLE_S32NE; break;
    case AE_FMT_S32LE : m_SampleSpec.format = PA_SAMPLE_S32LE; break;
    case AE_FMT_S32BE : m_SampleSpec.format = PA_SAMPLE_S32BE; break;
    case AE_FMT_FLOAT : m_SampleSpec.format = PA_SAMPLE_FLOAT32NE; break;
#if PA_CHECK_VERSION(1,0,0)
    case AE_FMT_DTS   :
    case AE_FMT_EAC3  :
    case AE_FMT_AC3   : m_SampleSpec.format = PA_SAMPLE_S16NE; break;
#endif

    default:
      CLog::Log(LOGERROR, "PulseAudio: Invalid format %i", format);
      pa_threaded_mainloop_unlock(m_MainLoop);
      m_format = AE_FMT_INVALID;
      return;
  }

  if (!pa_sample_spec_valid(&m_SampleSpec))
  {
    CLog::Log(LOGERROR, "PulseAudio: Invalid sample spec");
    pa_threaded_mainloop_unlock(m_MainLoop);
    Destroy();
    return /*false*/;
  }

  m_frameSize = pa_frame_size(&m_SampleSpec);

  struct pa_channel_map map;
  map.channels = m_channelLayout.Count();

  for (unsigned int ch = 0; ch < m_channelLayout.Count(); ++ch)
    switch(m_channelLayout[ch])
    {
      case AE_CH_NULL: break;
      case AE_CH_MAX : break;
      case AE_CH_RAW : break;
      case AE_CH_FL  : map.map[ch] = PA_CHANNEL_POSITION_FRONT_LEFT           ; break;
      case AE_CH_FR  : map.map[ch] = PA_CHANNEL_POSITION_FRONT_RIGHT          ; break;
      case AE_CH_FC  : map.map[ch] = PA_CHANNEL_POSITION_FRONT_CENTER         ; break;
      case AE_CH_BC  : map.map[ch] = PA_CHANNEL_POSITION_REAR_CENTER          ; break;
      case AE_CH_BL  : map.map[ch] = PA_CHANNEL_POSITION_REAR_LEFT            ; break;
      case AE_CH_BR  : map.map[ch] = PA_CHANNEL_POSITION_REAR_RIGHT           ; break;
      case AE_CH_LFE : map.map[ch] = PA_CHANNEL_POSITION_LFE                  ; break;
      case AE_CH_FLOC: map.map[ch] = PA_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER ; break;
      case AE_CH_FROC: map.map[ch] = PA_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER; break;
      case AE_CH_SL  : map.map[ch] = PA_CHANNEL_POSITION_SIDE_LEFT            ; break;
      case AE_CH_SR  : map.map[ch] = PA_CHANNEL_POSITION_SIDE_RIGHT           ; break;
      case AE_CH_TC  : map.map[ch] = PA_CHANNEL_POSITION_TOP_CENTER           ; break;
      case AE_CH_TFL : map.map[ch] = PA_CHANNEL_POSITION_TOP_FRONT_LEFT       ; break;
      case AE_CH_TFR : map.map[ch] = PA_CHANNEL_POSITION_TOP_FRONT_RIGHT      ; break;
      case AE_CH_TFC : map.map[ch] = PA_CHANNEL_POSITION_TOP_CENTER           ; break;
      case AE_CH_TBL : map.map[ch] = PA_CHANNEL_POSITION_TOP_REAR_LEFT        ; break;
      case AE_CH_TBR : map.map[ch] = PA_CHANNEL_POSITION_TOP_REAR_RIGHT       ; break;
      case AE_CH_TBC : map.map[ch] = PA_CHANNEL_POSITION_TOP_REAR_CENTER      ; break;
      default: break;
    }

  m_MaxVolume     = CAEFactory::GetEngine()->GetVolume();
  m_Volume        = 1.0f;
  pa_volume_t paVolume = pa_sw_volume_from_linear((double)(m_Volume * m_MaxVolume));
  pa_cvolume_set(&m_ChVolume, m_SampleSpec.channels, paVolume);

#if PA_CHECK_VERSION(1,0,0)
  pa_format_info *info[1];
  info[0] = pa_format_info_new();
  switch(m_format)
  {
    case AE_FMT_DTS : info[0]->encoding = PA_ENCODING_DTS_IEC61937 ; break;
    case AE_FMT_EAC3: info[0]->encoding = PA_ENCODING_EAC3_IEC61937; break;
    case AE_FMT_AC3 : info[0]->encoding = PA_ENCODING_AC3_IEC61937 ; break;
    default:          info[0]->encoding = PA_ENCODING_PCM          ; break;
  }
  pa_format_info_set_rate         (info[0], m_SampleSpec.rate);
  pa_format_info_set_channels     (info[0], m_SampleSpec.channels);
  pa_format_info_set_sample_format(info[0], m_SampleSpec.format);
  m_Stream = pa_stream_new_extended(m_Context, "audio stream", info, 1, NULL);
  pa_format_info_free(info[0]);
#else
  m_Stream = pa_stream_new(m_Context, "audio stream", &m_SampleSpec, &map);
#endif

  if (m_Stream == NULL)
  {
    CLog::Log(LOGERROR, "PulseAudio: Could not create a stream");
    pa_threaded_mainloop_unlock(m_MainLoop);
    Destroy();
    return /*false*/;
  }

  pa_stream_set_state_callback(m_Stream, CPulseAEStream::StreamStateCallback, this);
  pa_stream_set_write_callback(m_Stream, CPulseAEStream::StreamRequestCallback, this);
  pa_stream_set_latency_update_callback(m_Stream, CPulseAEStream::StreamLatencyUpdateCallback, this);
  pa_stream_set_underflow_callback(m_Stream, CPulseAEStream::StreamUnderflowCallback, this);

  int flags = PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_AUTO_TIMING_UPDATE;
  if (options && AESTREAM_FORCE_RESAMPLE)
    flags |= PA_STREAM_VARIABLE_RATE;

  if (pa_stream_connect_playback(m_Stream, NULL, NULL, (pa_stream_flags)flags, &m_ChVolume, NULL) < 0)
  {
    CLog::Log(LOGERROR, "PulseAudio: Failed to connect stream to output");
    pa_threaded_mainloop_unlock(m_MainLoop);
    Destroy();
    return /*false*/;
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
    Destroy();
    return /*false*/;
  }

  m_cacheSize = pa_stream_writable_size(m_Stream);

  pa_threaded_mainloop_unlock(m_MainLoop);

  m_Initialized = true;

  CLog::Log(LOGINFO, "PulseAEStream::Initialized");
  CLog::Log(LOGINFO, "  Sample Rate   : %d", m_sampleRate);
  CLog::Log(LOGINFO, "  Sample Format : %s", CAEUtil::DataFormatToStr(m_format));
  CLog::Log(LOGINFO, "  Channel Count : %d", m_channelLayout.Count());
  CLog::Log(LOGINFO, "  Channel Layout: %s", ((std::string)m_channelLayout).c_str());
  CLog::Log(LOGINFO, "  Frame Size    : %d", m_frameSize);
  CLog::Log(LOGINFO, "  Cache Size    : %d", m_cacheSize);

  Resume();

  return /*true*/;
}

CPulseAEStream::~CPulseAEStream()
{
  Destroy();
}

/*
  this method may be called inside the pulse main loop,
  so be VERY careful with locking
*/
void CPulseAEStream::Destroy()
{
  if (!m_Initialized)
    return;

  if (m_Destroyed)
    return;

  m_fader.StopThread(true);

  pa_threaded_mainloop_lock(m_MainLoop);

  if (m_DrainOperation)
  {
    pa_operation_cancel(m_DrainOperation);
    pa_operation_unref(m_DrainOperation);
    m_DrainOperation = NULL;
  }

  if (m_Stream)
  {
    pa_stream_disconnect(m_Stream);
    pa_stream_unref(m_Stream);
    m_Stream = NULL;
  }

  /* signal CPulseAE to free us */
  m_Destroyed = true;
  m_Initialized = false;

  pa_threaded_mainloop_unlock(m_MainLoop);
}

unsigned int CPulseAEStream::GetSpace()
{
  if (!m_Initialized)
    return 0;

  pa_threaded_mainloop_lock(m_MainLoop);
  unsigned int size = pa_stream_writable_size(m_Stream);
  pa_threaded_mainloop_unlock(m_MainLoop);

  if(size > m_cacheSize)
    m_cacheSize = size;

  return size;
}

unsigned int CPulseAEStream::AddData(void *data, unsigned int size)
{
  if (!m_Initialized)
    return size;

  pa_threaded_mainloop_lock(m_MainLoop);

  int length = std::min((int)pa_stream_writable_size(m_Stream), (int)size);
  if (length == 0)
  {
    pa_threaded_mainloop_unlock(m_MainLoop);
    return 0;
  }

  int written = pa_stream_write(m_Stream, data, length, NULL, 0, PA_SEEK_RELATIVE);
  pa_threaded_mainloop_unlock(m_MainLoop);

  if (written < 0)
  {
    CLog::Log(LOGERROR, "PulseAudio: AddPackets - pa_stream_write failed\n");
    return 0;
  }

  return length;
}

double CPulseAEStream::GetDelay()
{
  if (!m_Initialized)
    return 0.0;

  pa_usec_t latency = 0;
  pa_threaded_mainloop_lock(m_MainLoop);

  if (pa_stream_get_latency(m_Stream, &latency, NULL) == PA_ERR_NODATA)
    CLog::Log(LOGERROR, "PulseAudio: pa_stream_get_latency() failed");

  pa_threaded_mainloop_unlock(m_MainLoop);
  return (double)((double)latency / 1000000.0);
}

double CPulseAEStream::GetCacheTime()
{
  if (!m_Initialized)
    return 0.0;

  return (double)(m_cacheSize - GetSpace()) / (double)(m_sampleRate * m_frameSize);
}

double CPulseAEStream::GetCacheTotal()
{
  if (!m_Initialized)
    return 0.0;

  return (double)m_cacheSize / (double)(m_sampleRate * m_frameSize);
}

bool CPulseAEStream::IsPaused()
{
  return m_Paused;
}

bool CPulseAEStream::IsDraining()
{
  if (m_DrainOperation)
  {
    if (pa_operation_get_state(m_DrainOperation) == PA_OPERATION_RUNNING)
      return true;

    pa_operation_unref(m_DrainOperation);
    m_DrainOperation = NULL;
  }

  return false;
}

bool CPulseAEStream::IsDrained()
{
  return m_DrainOperation == NULL;
}

bool CPulseAEStream::IsDestroyed()
{
  return m_Destroyed;
}

void CPulseAEStream::Pause()
{
  if (m_Initialized)
    m_Paused = Cork(true);
}

void CPulseAEStream::Resume()
{
  if (m_Initialized)
    m_Paused = Cork(false);
}

void CPulseAEStream::Drain()
{
  if (!m_Initialized)
    return;

  if (m_DrainOperation)
    return;

  pa_threaded_mainloop_lock(m_MainLoop);
  m_DrainOperation = pa_stream_drain(m_Stream, CPulseAEStream::StreamDrainComplete, this);
  pa_threaded_mainloop_unlock(m_MainLoop);
}

void CPulseAEStream::Flush()
{
  if (!m_Initialized)
    return;

  pa_threaded_mainloop_lock(m_MainLoop);
  pa_operation_unref(pa_stream_flush(m_Stream, NULL, NULL));
  pa_threaded_mainloop_unlock(m_MainLoop);
}

float CPulseAEStream::GetVolume()
{
  return m_Volume;
}

float CPulseAEStream::GetReplayGain()
{
  return 0.0f;
}

void CPulseAEStream::SetVolume(float volume)
{
  if (!m_Initialized)
    return;

  if (!pa_threaded_mainloop_in_thread(m_MainLoop))
    pa_threaded_mainloop_lock(m_MainLoop);

  if (volume > 0.f)
  {
    m_Volume = volume;
    pa_volume_t paVolume = pa_sw_volume_from_linear((double)(m_Volume * m_MaxVolume));

    pa_cvolume_set(&m_ChVolume, m_SampleSpec.channels, paVolume);
  } 
  else
    pa_cvolume_mute(&m_ChVolume,m_SampleSpec.channels);

  pa_operation *op = pa_context_set_sink_input_volume(m_Context, pa_stream_get_index(m_Stream), &m_ChVolume, NULL, NULL);

  if (op == NULL)
    CLog::Log(LOGERROR, "PulseAudio: Failed to set volume");
  else
    pa_operation_unref(op);

  if (!pa_threaded_mainloop_in_thread(m_MainLoop))
    pa_threaded_mainloop_unlock(m_MainLoop);
}

void CPulseAEStream::UpdateVolume(float max)
{
   if (!m_Initialized)
    return;

  m_MaxVolume = max;
  SetVolume(m_Volume);
}

void CPulseAEStream::SetMute(const bool mute)
{
  if (mute)
    SetVolume(-1.f);
  else
    SetVolume(m_Volume);
}

void CPulseAEStream::SetReplayGain(float factor)
{
}

const unsigned int CPulseAEStream::GetFrameSize() const
{
  return m_frameSize;
}

const unsigned int CPulseAEStream::GetChannelCount() const
{
  return m_channelLayout.Count();
}

const unsigned int CPulseAEStream::GetSampleRate() const
{
  return m_sampleRate;
}

const enum AEDataFormat CPulseAEStream::GetDataFormat() const
{
  return m_format;
}

double CPulseAEStream::GetResampleRatio()
{
  return 1.0;
}

bool CPulseAEStream::SetResampleRatio(double ratio)
{
  return false;
}

void CPulseAEStream::RegisterAudioCallback(IAudioCallback* pCallback)
{
  m_AudioCallback = pCallback;
}

void CPulseAEStream::UnRegisterAudioCallback()
{
  m_AudioCallback = NULL;
}

void CPulseAEStream::FadeVolume(float from, float target, unsigned int time)
{
  if (!m_Initialized)
    return;

  m_fader.SetupFader(from, target, time);
}

bool CPulseAEStream::IsFading()
{
  return m_fader.IsRunning();
}

void CPulseAEStream::StreamRequestCallback(pa_stream *s, size_t length, void *userdata)
{
  CPulseAEStream *stream = (CPulseAEStream *)userdata;
  pa_threaded_mainloop_signal(stream->m_MainLoop, 0);
}

void CPulseAEStream::StreamLatencyUpdateCallback(pa_stream *s, void *userdata)
{
  CPulseAEStream *stream = (CPulseAEStream *)userdata;
  pa_threaded_mainloop_signal(stream->m_MainLoop, 0);
}

void CPulseAEStream::StreamStateCallback(pa_stream *s, void *userdata)
{
  CPulseAEStream *stream = (CPulseAEStream *)userdata;
  pa_stream_state_t state = pa_stream_get_state(s);

  switch (state)
  {
    case PA_STREAM_UNCONNECTED:
    case PA_STREAM_CREATING:
    case PA_STREAM_READY:
    case PA_STREAM_FAILED:
    case PA_STREAM_TERMINATED:
      pa_threaded_mainloop_signal(stream->m_MainLoop, 0);
      break;
  }
}

void CPulseAEStream::StreamUnderflowCallback(pa_stream *s, void *userdata)
{
  CPulseAEStream *stream = (CPulseAEStream *)userdata;
  CLog::Log(LOGWARNING, "PulseAudio: Stream underflow");
  pa_threaded_mainloop_signal(stream->m_MainLoop, 0);
}

void CPulseAEStream::StreamDrainComplete(pa_stream *s, int success, void *userdata)
{
  CPulseAEStream *stream = (CPulseAEStream *)userdata;
  if (stream->m_slave)
    stream->m_slave->Resume();
  pa_threaded_mainloop_signal(stream->m_MainLoop, 0);
}

inline bool CPulseAEStream::WaitForOperation(pa_operation *op, pa_threaded_mainloop *mainloop, const char *LogEntry = "")
{
  if (op == NULL)
    return false;

  bool sucess = true;
  ASSERT(!pa_threaded_mainloop_in_thread(mainloop));

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

bool CPulseAEStream::Cork(bool cork)
{
  pa_threaded_mainloop_lock(m_MainLoop);

  pa_operation *op = pa_stream_cork(m_Stream, cork ? 1 : 0, NULL, NULL);
  if (!WaitForOperation(op, m_MainLoop, cork ? "Pause" : "Resume"))
    cork = !cork;

  pa_threaded_mainloop_unlock(m_MainLoop);
  return cork;
}

void CPulseAEStream::RegisterSlave(IAEStream *stream)
{
  m_slave = stream;
}

CPulseAEStream::CLinearFader::CLinearFader(IAEStream *stream) : CThread("AE Stream"), m_stream(stream)
{
  m_from = 0;
  m_target = 0;
  m_time = 0;
  m_isRunning = false;
}

void CPulseAEStream::CLinearFader::SetupFader(float from, float target, unsigned int time)
{
  StopThread(true);

  m_from = from;
  m_target = target;
  m_time = time;

  if (m_time > 0)
    Create();
  else
    m_stream->SetVolume(m_target);
}

void CPulseAEStream::CLinearFader::Process()
{
  if (m_stream == NULL)
    return;

  m_isRunning = true;
  m_stream->SetVolume(m_from);
  float k = m_target - m_from;

  unsigned int begin = XbmcThreads::SystemClockMillis();
  unsigned int end = begin + m_time;
  unsigned int current = begin;
  unsigned int step = std::max(1u, m_time / 100);

  do
  {
    float x = ((float)current - (float)begin) / (float)m_time;

    m_stream->SetVolume(m_from + k * x);
    usleep(step * 1000);
    current = XbmcThreads::SystemClockMillis();
  } while (current <= end && !m_bStop);

  m_stream->SetVolume(m_target);
  m_isRunning = false;
}

bool CPulseAEStream::CLinearFader::IsRunning()
{
  return !m_isRunning;
}
#endif
