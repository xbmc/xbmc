/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "PulseAEStream.h"
#include "AEFactory.h"
#include "AEUtil.h"
#include "log.h"
#include "utils/SingleLock.h"
#include "MathUtils.h"

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

CPulseAEStream::CPulseAEStream(pa_context *context, pa_threaded_mainloop *mainLoop, enum AEDataFormat format, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options) :
  m_AudioDataThread (NULL),
  m_AudioDrainThread(NULL)
{
  m_Destroyed = false;
  m_Initialized = false;
  m_Paused = false;

  m_Stream = NULL;
  m_Context = context;
  m_MainLoop = mainLoop;

  m_format = format;
  m_sampleRate = sampleRate;
  m_channelCount = channelCount;
  m_channelLayout = channelLayout;
  m_options = options;

  m_draining = false;

  printf("Started initializing %i %i\n", sampleRate, channelCount);
  pa_threaded_mainloop_lock(m_MainLoop);

  m_SampleSpec.channels = m_channelCount;
  m_SampleSpec.rate = m_sampleRate;

  switch (m_format)
  {
    case AE_FMT_U8    : m_SampleSpec.format = PA_SAMPLE_U8; break;
    case AE_FMT_S16NE : m_SampleSpec.format = PA_SAMPLE_S16NE; break;
    case AE_FMT_S16LE : m_SampleSpec.format = PA_SAMPLE_S16LE; break;
    case AE_FMT_S16BE : m_SampleSpec.format = PA_SAMPLE_S16BE; break;
    case AE_FMT_S24NE3: m_SampleSpec.format = PA_SAMPLE_S24NE; break;
    case AE_FMT_S24NE : m_SampleSpec.format = PA_SAMPLE_S24_32NE; break;
    case AE_FMT_S32NE : m_SampleSpec.format = PA_SAMPLE_S32NE; break;
    case AE_FMT_S32LE : m_SampleSpec.format = PA_SAMPLE_S32LE; break;
    case AE_FMT_S32BE : m_SampleSpec.format = PA_SAMPLE_S32BE; break;
    case AE_FMT_FLOAT : m_SampleSpec.format = PA_SAMPLE_FLOAT32NE; break;

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
  map.channels = m_channelCount;
  if (!m_channelLayout)
    m_channelLayout = CAEUtil::GuessChLayout(m_channelCount);

  for(unsigned int ch = 0; ch < m_channelCount; ++ch)
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
    }

  m_MaxVolume     = AE.GetVolume();
  m_Volume        = 1.0f;
  float useVolume = m_Volume * m_MaxVolume;

  pa_volume_t paVolume = MathUtils::round_int(useVolume * PA_VOLUME_NORM);
  printf("%f %f %f %d\n", m_Volume, m_MaxVolume, useVolume, paVolume);

  pa_cvolume_set(&m_ChVolume, m_SampleSpec.channels, paVolume);
  if ((m_Stream = pa_stream_new(m_Context, "audio stream", &m_SampleSpec, &map)) == NULL)
  {
    CLog::Log(LOGERROR, "PulseAudio: Could not create a stream");
    pa_threaded_mainloop_unlock(m_MainLoop);
    Destroy();
    return /*false*/;
  }

  pa_stream_set_state_callback(m_Stream, CPulseAEStream::StreamStateCallback, this);
  pa_stream_set_write_callback(m_Stream, CPulseAEStream::StreamRequestCallback, this);
  pa_stream_set_latency_update_callback(m_Stream, CPulseAEStream::StreamLatencyUpdateCallback, this);

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

  pa_buffer_attr a;
  m_frameSamples = m_sampleRate / 1000;
  a.fragsize  = (uint32_t)-1;
  a.maxlength = (uint32_t)-1;
  a.prebuf    = 0;
  a.minreq    = m_frameSamples * m_frameSize;
  a.tlength   = m_frameSamples * m_frameSize;
  WaitForOperation(pa_stream_set_buffer_attr(m_Stream, &a, NULL, NULL), m_MainLoop, "SetBuffer");

  pa_threaded_mainloop_unlock(m_MainLoop);

  m_Initialized = true;
  printf("Initialized\n");

  Resume();
  return /*true*/;
}

CPulseAEStream::~CPulseAEStream()
{
  delete m_AudioDataThread;
  delete m_AudioDrainThread;
  m_AudioDataThread  = NULL;
  m_AudioDrainThread = NULL;

  if (!m_Initialized)
    return;

  printf("delete 1\n");
  pa_threaded_mainloop_lock(m_MainLoop);

  if (m_Stream)
  {
    printf("delete 2\n");
    pa_stream_disconnect(m_Stream);
    pa_stream_unref(m_Stream);
    m_Stream = NULL;
  }

  printf("delete 3\n");
  pa_threaded_mainloop_unlock(m_MainLoop);
}

/*
  this method may be called inside the pulse main loop,
  so be VERY careful with locking
*/
void CPulseAEStream::Destroy()
{
  if (m_Destroyed)
    return;

  if (m_AudioDataThread ) m_AudioDataThread ->SetCallback(NULL, NULL);
  if (m_AudioDrainThread) m_AudioDrainThread->SetCallback(NULL, NULL);

  /* signal CPulseAE to free us */
  m_Destroyed = true;
}

void CPulseAEStream::SetDataCallback(AECBFunc *cbFunc, void *arg)
{
  if (!m_AudioDataThread)
    m_AudioDataThread = new CPulseAEEventThread(this);
  m_AudioDataThread->SetCallback(cbFunc, arg);
}

void CPulseAEStream::SetDrainCallback(AECBFunc *cbFunc, void *arg)
{
  if (!m_AudioDrainThread)
    m_AudioDrainThread = new CPulseAEEventThread(this);
  m_AudioDrainThread->SetCallback(cbFunc, arg);
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

float CPulseAEStream::GetDelay()
{
  if (!m_Initialized)
    return 0.0f;

  pa_usec_t latency = 0;
  pa_threaded_mainloop_lock(m_MainLoop);

  if (pa_stream_get_latency(m_Stream, &latency, NULL) == PA_ERR_NODATA)
    CLog::Log(LOGERROR, "PulseAudio: pa_stream_get_latency() failed");

  pa_threaded_mainloop_unlock(m_MainLoop);
  return (float)((float)latency / 1000000.0f);
}

int CPulseAEStream::GetSpace()
{
  if (!m_Initialized)
    return 0;

  pa_threaded_mainloop_lock(m_MainLoop);
  int space = pa_stream_writable_size(m_Stream);
  pa_threaded_mainloop_unlock(m_MainLoop);

  return space / m_frameSize;
}

bool CPulseAEStream::IsPaused()
{
  return m_Paused;
}

bool CPulseAEStream::IsDraining()
{
  return m_draining;
}

bool CPulseAEStream::IsFreeOnDrain()
{
  return m_options & AESTREAM_FREE_ON_DRAIN;
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
  if (!m_Initialized || m_draining)
    return;

  printf("drain1\n");
  m_draining = true;
  pa_threaded_mainloop_lock(m_MainLoop);

  printf("drain2\n");
  pa_operation_unref(pa_stream_drain(m_Stream, CPulseAEStream::StreamDrainComplete, this));
  pa_threaded_mainloop_unlock(m_MainLoop);
  printf("drain3\n");
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

  printf("Stream SetVolume %f\n", volume);

  if (!pa_threaded_mainloop_in_thread(m_MainLoop))
    pa_threaded_mainloop_lock(m_MainLoop);

  m_Volume = volume;
  float useVolume = volume * m_MaxVolume;
  pa_volume_t paVolume = MathUtils::round_int(useVolume * PA_VOLUME_NORM);

  pa_cvolume_set(&m_ChVolume, m_SampleSpec.channels, paVolume);
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

void CPulseAEStream::SetReplayGain(float factor)
{
}

void CPulseAEStream::AppendPostProc(IAEPostProc *pp)
{
}

void CPulseAEStream::PrependPostProc(IAEPostProc *pp)
{
}

void CPulseAEStream::RemovePostProc(IAEPostProc *pp)
{
}

unsigned int CPulseAEStream::GetFrameSize()
{
  return m_frameSize;
}

unsigned int CPulseAEStream::GetChannelCount()
{
  return m_channelCount;
}

unsigned int CPulseAEStream::GetSampleRate()
{
  return m_sampleRate;
}

enum AEDataFormat CPulseAEStream::GetDataFormat()
{
  return m_format;
}

double CPulseAEStream::GetResampleRatio()
{
  return 1.0;
}

void CPulseAEStream::SetResampleRatio(double ratio)
{
}

void CPulseAEStream::RegisterAudioCallback(IAudioCallback* pCallback)
{
  m_AudioCallback = pCallback;
}

void CPulseAEStream::UnRegisterAudioCallback()
{
  m_AudioCallback = NULL;
}

void CPulseAEStream::StreamRequestCallback(pa_stream *s, size_t length, void *userdata)
{
  CPulseAEStream *stream = (CPulseAEStream *)userdata;

  pa_threaded_mainloop_signal(stream->m_MainLoop, 0);
  if (stream->m_AudioDataThread)
    stream->m_AudioDataThread->Trigger();
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

void CPulseAEStream::StreamDrainComplete(pa_stream *s, int success, void *userdata)
{
  CPulseAEStream *stream = (CPulseAEStream *)userdata;
  pa_threaded_mainloop_signal(stream->m_MainLoop, 0);
  if (stream->m_AudioDrainThread)
    stream->m_AudioDrainThread->Trigger();

  if (stream->IsFreeOnDrain())
    stream->Destroy();
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

