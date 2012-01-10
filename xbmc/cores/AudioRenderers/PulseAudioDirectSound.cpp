/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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

#include "system.h"
#ifdef HAS_PULSEAUDIO
#include "PulseAudioDirectSound.h"
#include "guilib/AudioContext.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "Util.h"
#include "guilib/LocalizeStrings.h"

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

struct SinkInfoStruct
{
  bool passthrough;
  AudioSinkList *list;
  pa_threaded_mainloop *mainloop;
};

static void SinkInfo(pa_context *c, const pa_sink_info *i, int eol, void *userdata)
{
  SinkInfoStruct *sinkStruct = (SinkInfoStruct *)userdata;

  if (i && i->name)
  {
    bool       add  = false;
    if(sinkStruct->passthrough)
    {
#if PA_CHECK_VERSION(1,0,0)
      for(int idx = 0; idx < i->n_formats; ++idx)
      {
        if(!pa_format_info_is_pcm(i->formats[idx]))
        {
          add = true;
          break;
        }
      }
#endif
    }
    else
      add = true;

    if(add)
    {
      CStdString desc, sink;
      if(sinkStruct->list->size() == 0)
        sinkStruct->list->push_back(AudioSink(g_localizeStrings.Get(409) + " (PulseAudio)", "pulse:default@default"));
      desc.Format("%s  (PulseAudio)", i->description);
      sink.Format("pulse:%s@default", i->name);
      sinkStruct->list->push_back(AudioSink(desc, sink));
      CLog::Log(LOGDEBUG, "PulseAudio: Found %s with devicestring %s", desc.c_str(), sink.c_str());
    }
  }

  pa_threaded_mainloop_signal(sinkStruct->mainloop, 0);
}

/* PulseAudio class memberfunctions*/

CPulseAudioDirectSound::CPulseAudioDirectSound()
{
}

bool CPulseAudioDirectSound::Initialize(IAudioCallback* pCallback, const CStdString& device, int iChannels, enum PCMChannels* channelMap, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, bool bIsMusic, EEncoded encoded)
{
  m_remap.Reset();
  m_uiDataChannels = iChannels;
  enum PCMChannels* outLayout = NULL;

  if (encoded == ENCODED_NONE && channelMap)
  {
    /* set the input format, and get the channel layout so we know what we need to open */
    outLayout = m_remap.SetInputFormat(iChannels, channelMap, uiBitsPerSample / 8, uiSamplesPerSec);
    enum PCMChannels *channel;
    iChannels = 0;
    for(channel = outLayout; *channel != PCM_INVALID; ++channel)
      ++iChannels;

    m_remap.SetOutputFormat(iChannels, outLayout);
    if (m_uiDataChannels != (unsigned int)iChannels)
      CLog::Log(LOGDEBUG, "CPulseAudioDirectSound::CPulseAudioDirectSound - Requested channels changed from %i to %i", m_uiDataChannels, iChannels);
  }

  bool bAudioOnAllSpeakers(false);
  g_audioContext.SetupSpeakerConfig(iChannels, bAudioOnAllSpeakers, bIsMusic);
  g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE);

  m_Context = NULL;
  m_Stream = NULL;
  m_MainLoop = NULL;
  m_bPause = false;
  m_bRecentlyFlushed = true;
  m_bAutoResume = false;
  m_bIsAllocated = false;
  m_uiChannels = iChannels;
  m_uiSamplesPerSec = uiSamplesPerSec;
  m_uiBufferSize = 0;
  m_uiBitsPerSample = uiBitsPerSample;
  m_bPassthrough = encoded == ENCODED_NONE ? false : true;
  m_uiBytesPerSecond = uiSamplesPerSec * (uiBitsPerSample / 8) * iChannels;
  m_drc = 0;

  m_nCurrentVolume = g_settings.m_nVolumeLevel;

  m_dwPacketSize = iChannels*(uiBitsPerSample/8)*512;
  m_dwNumPackets = 16;

#if !PA_CHECK_VERSION(1,0,0)
  /* Open the device */
  if (m_bPassthrough)
  {
    CLog::Log(LOGWARNING, "PulseAudio: Does not support passthrough");
    return false;
  }
#endif

  std::vector<CStdString> hostdevice;
  CUtil::Tokenize(device, hostdevice, "@");

  const char *host = (hostdevice.size() < 2 || hostdevice[1].Equals("default") ? NULL : hostdevice[1].c_str());
  if (!SetupContext(host, &m_Context, &m_MainLoop))
  {
    CLog::Log(LOGERROR, "PulseAudio: Failed to create context");
    Deinitialize();
    return false;
  }

  pa_threaded_mainloop_lock(m_MainLoop);


  struct pa_channel_map map;

  // Build the channel map, we dont need to remap, but we still need PCMRemap to handle mono to dual mono stereo
  map.channels = iChannels;
  if (outLayout)
  {
    for(int ch = 0; ch < iChannels; ++ch)
    {
      switch(outLayout[ch])
      {
        case PCM_INVALID              : break;
        case PCM_FRONT_LEFT           : map.map[ch] = PA_CHANNEL_POSITION_FRONT_LEFT           ; break;
        case PCM_FRONT_RIGHT          : map.map[ch] = PA_CHANNEL_POSITION_FRONT_RIGHT          ; break;
        case PCM_FRONT_CENTER         : map.map[ch] = PA_CHANNEL_POSITION_FRONT_CENTER         ; break;
        case PCM_BACK_CENTER          : map.map[ch] = PA_CHANNEL_POSITION_REAR_CENTER          ; break;
        case PCM_BACK_LEFT            : map.map[ch] = PA_CHANNEL_POSITION_REAR_LEFT            ; break;
        case PCM_BACK_RIGHT           : map.map[ch] = PA_CHANNEL_POSITION_REAR_RIGHT           ; break;
        case PCM_LOW_FREQUENCY        : map.map[ch] = PA_CHANNEL_POSITION_LFE                  ; break;
        case PCM_FRONT_LEFT_OF_CENTER : map.map[ch] = PA_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER ; break;
        case PCM_FRONT_RIGHT_OF_CENTER: map.map[ch] = PA_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER; break;
        case PCM_SIDE_LEFT            : map.map[ch] = PA_CHANNEL_POSITION_SIDE_LEFT            ; break;
        case PCM_SIDE_RIGHT           : map.map[ch] = PA_CHANNEL_POSITION_SIDE_RIGHT           ; break;
        case PCM_TOP_CENTER           : map.map[ch] = PA_CHANNEL_POSITION_TOP_CENTER           ; break;
        case PCM_TOP_FRONT_LEFT       : map.map[ch] = PA_CHANNEL_POSITION_TOP_FRONT_LEFT       ; break;
        case PCM_TOP_FRONT_RIGHT      : map.map[ch] = PA_CHANNEL_POSITION_TOP_FRONT_RIGHT      ; break;
        case PCM_TOP_FRONT_CENTER     : map.map[ch] = PA_CHANNEL_POSITION_TOP_CENTER           ; break;
        case PCM_TOP_BACK_LEFT        : map.map[ch] = PA_CHANNEL_POSITION_TOP_REAR_LEFT        ; break;
        case PCM_TOP_BACK_RIGHT       : map.map[ch] = PA_CHANNEL_POSITION_TOP_REAR_RIGHT       ; break;
        case PCM_TOP_BACK_CENTER      : map.map[ch] = PA_CHANNEL_POSITION_TOP_REAR_CENTER      ; break;
      }
    }
  }
  else
    pa_channel_map_init_auto(&map, m_uiChannels, PA_CHANNEL_MAP_ALSA);

  pa_cvolume_reset(&m_Volume, m_uiChannels);

  if(m_bPassthrough)
  {
#if PA_CHECK_VERSION(1,0,0)
    pa_format_info *info[1];
    info[0] = pa_format_info_new();
    switch(encoded)
    {
      case ENCODED_IEC61937_AC3 : info[0]->encoding = PA_ENCODING_AC3_IEC61937 ; break;
      case ENCODED_IEC61937_DTS : info[0]->encoding = PA_ENCODING_DTS_IEC61937 ; break;
      case ENCODED_IEC61937_EAC3: info[0]->encoding = PA_ENCODING_EAC3_IEC61937; break;
      case ENCODED_IEC61937_MPEG: info[0]->encoding = PA_ENCODING_MPEG_IEC61937; break;
      default:                    info[0]->encoding = PA_ENCODING_INVALID      ; break;
    }
    pa_format_info_set_rate(info[0], m_uiSamplesPerSec);
    pa_format_info_set_channels(info[0], m_uiChannels);
    pa_format_info_set_sample_format(info[0], PA_SAMPLE_S16NE);
    m_Stream = pa_stream_new_extended(m_Context, "audio stream", info, 1, NULL);
    pa_format_info_free(info[0]);
#endif
  }
  else
  {

    pa_sample_spec spec;
    spec.channels = iChannels;
    spec.rate     = uiSamplesPerSec;
    spec.format   = PA_SAMPLE_S16NE;

    if (!pa_sample_spec_valid(&spec))
    {
      CLog::Log(LOGERROR, "PulseAudio: Invalid sample spec");
      Deinitialize();
      return false;
    }

    m_Stream = pa_stream_new(m_Context, "audio stream", &spec, &map);
  }

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

  const char *sink = hostdevice.size() < 1 || hostdevice[0].Equals("default") ? NULL : hostdevice[0].c_str();
  if (pa_stream_connect_playback(m_Stream, sink, NULL, ((pa_stream_flags)(PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_AUTO_TIMING_UPDATE)), &m_Volume, NULL) < 0)
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
      CLog::Log(LOGERROR, "PulseAudio: %s", pa_strerror(pa_context_errno(m_Context)));
  else
  {
    m_dwPacketSize = a->minreq;
    CLog::Log(LOGDEBUG, "PulseAudio: Default buffer attributes, maxlength=%u, tlength=%u, prebuf=%u, minreq=%u", a->maxlength, a->tlength, a->prebuf, a->minreq);
    pa_buffer_attr b;
    b.prebuf = (uint32_t)-1;
    b.minreq = a->minreq;
    b.tlength = m_uiBufferSize = a->tlength;
    b.maxlength = a->maxlength;
    b.fragsize = a->fragsize;

    WaitForOperation(pa_stream_set_buffer_attr(m_Stream, &b, NULL, NULL), m_MainLoop, "SetBuffer");

    if (!(a = pa_stream_get_buffer_attr(m_Stream)))
        CLog::Log(LOGERROR, "PulseAudio: %s", pa_strerror(pa_context_errno(m_Context)));
    else
    {
      m_dwPacketSize = a->minreq;
      m_uiBufferSize = a->tlength;
      CLog::Log(LOGDEBUG, "PulseAudio: Choosen buffer attributes, maxlength=%u, tlength=%u, prebuf=%u, minreq=%u", a->maxlength, a->tlength, a->prebuf, a->minreq);
    }
  }

  pa_threaded_mainloop_unlock(m_MainLoop);

  m_bIsAllocated = true;

  SetCurrentVolume(m_nCurrentVolume);
  Resume();
  return true;
}

CPulseAudioDirectSound::~CPulseAudioDirectSound()
{
  Deinitialize();
}

bool CPulseAudioDirectSound::Deinitialize()
{
  m_bIsAllocated = false;

  if (m_Stream)
    WaitCompletion();

  if (m_MainLoop)
    pa_threaded_mainloop_stop(m_MainLoop);

  if (m_Stream)
  {
    pa_stream_disconnect(m_Stream);
    pa_stream_unref(m_Stream);
    m_Stream = NULL;
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

  g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);
  return true;
}

inline bool CPulseAudioDirectSound::WaitForOperation(pa_operation *op, pa_threaded_mainloop *mainloop, const char *LogEntry = "")
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

void CPulseAudioDirectSound::Flush()
{
  if (!m_bIsAllocated)
     return;

  Pause();

  pa_threaded_mainloop_lock(m_MainLoop);
  WaitForOperation(pa_stream_flush(m_Stream, NULL, NULL), m_MainLoop, "Flush");
  m_bRecentlyFlushed = true;
  pa_threaded_mainloop_unlock(m_MainLoop);
}

bool CPulseAudioDirectSound::Cork(bool cork)
{
  pa_threaded_mainloop_lock(m_MainLoop);

  if (!WaitForOperation(pa_stream_cork(m_Stream, cork ? 1 : 0, NULL, NULL), m_MainLoop, cork ? "Pause" : "Resume"))
    cork = !cork;

  pa_threaded_mainloop_unlock(m_MainLoop);

  return cork;
}

bool CPulseAudioDirectSound::Pause()
{
  if (!m_bIsAllocated)
    return -1;

  if (m_bPause)
    return true;

  m_bPause = Cork(true);

  return m_bPause;
}

bool CPulseAudioDirectSound::Resume()
{
  if (!m_bIsAllocated)
     return false;

  bool result = false;

  if(m_bPause && !m_bRecentlyFlushed)
  {
    m_bPause = Cork(false);
    result = !m_bPause;
  }
  else if (m_bPause)
    result = m_bAutoResume = true;

  return result;
}

bool CPulseAudioDirectSound::Stop()
{
  if (!m_bIsAllocated)
    return false;

  Flush();

  return true;
}

long CPulseAudioDirectSound::GetCurrentVolume() const
{
  return m_nCurrentVolume;
}

void CPulseAudioDirectSound::Mute(bool bMute)
{
  if (!m_bIsAllocated)
    return;

  if (bMute)
    SetCurrentVolume(VOLUME_MINIMUM);
  else
    SetCurrentVolume(m_nCurrentVolume);
}

bool CPulseAudioDirectSound::SetCurrentVolume(long nVolume)
{
  if (!m_bIsAllocated || m_bPassthrough)
    return -1;

  pa_threaded_mainloop_lock(m_MainLoop);
  pa_volume_t volume = pa_sw_volume_from_dB((float)nVolume*1.5f / 200.0f);
  if ( nVolume <= VOLUME_MINIMUM )
    pa_cvolume_mute(&m_Volume, m_uiChannels);
  else
    pa_cvolume_set(&m_Volume, m_uiChannels, volume);
  pa_operation *op = pa_context_set_sink_input_volume(m_Context, pa_stream_get_index(m_Stream), &m_Volume, NULL, NULL);
  if (op == NULL)
    CLog::Log(LOGERROR, "PulseAudio: Failed to set volume");
  else
    pa_operation_unref(op);

  pa_threaded_mainloop_unlock(m_MainLoop);

  return true;
}

unsigned int CPulseAudioDirectSound::GetSpace()
{
  if (!m_bIsAllocated)
    return 0;

  size_t l;
  pa_threaded_mainloop_lock(m_MainLoop);
  l = pa_stream_writable_size(m_Stream);
  pa_threaded_mainloop_unlock(m_MainLoop);
  return (l / m_uiChannels) * m_uiDataChannels;
}

unsigned int CPulseAudioDirectSound::AddPackets(const void* data, unsigned int len)
{
  if (!m_bIsAllocated)
    return len;

  pa_threaded_mainloop_lock(m_MainLoop);

  len = (len / m_uiDataChannels) * m_uiChannels;
  int length = std::min((int)pa_stream_writable_size(m_Stream), (int)len);
  int frames = length / m_uiChannels / (m_uiBitsPerSample >> 3);
  if (frames == 0)
  {
    pa_threaded_mainloop_unlock(m_MainLoop);
    return 0;
  }

  if (m_remap.CanRemap())
  {
    /* remap the data to the correct channels */
    uint8_t outData[length];
    m_remap.Remap((void *)data, outData, frames, m_drc);
    if (pa_stream_write(m_Stream, outData, length, NULL, 0, PA_SEEK_RELATIVE) < 0)
      CLog::Log(LOGERROR, "CPulseAudioDirectSound::AddPackets - pa_stream_write failed\n");

  }
  else
    if (pa_stream_write(m_Stream, data, length, NULL, 0, PA_SEEK_RELATIVE) < 0)
      CLog::Log(LOGERROR, "CPulseAudioDirectSound::AddPackets - pa_stream_write failed\n");


  if (m_bRecentlyFlushed)
    m_bRecentlyFlushed = false;

  pa_threaded_mainloop_unlock(m_MainLoop);

  if (m_bAutoResume)
    m_bAutoResume = !Resume();

  return (length / m_uiChannels) * m_uiDataChannels;
}

float CPulseAudioDirectSound::GetCacheTime()
{
  return (float)(m_uiBufferSize - GetSpace()) / (float)m_uiBytesPerSecond;
}

float CPulseAudioDirectSound::GetCacheTotal()
{
  return (float)m_uiBufferSize / (float)m_uiBytesPerSecond;
}

float CPulseAudioDirectSound::GetDelay()
{
  if (!m_bIsAllocated)
    return 0;

  pa_usec_t latency = (pa_usec_t) -1;
  pa_threaded_mainloop_lock(m_MainLoop);
  while (pa_stream_get_latency(m_Stream, &latency, NULL) < 0)
  {
    if (pa_context_errno(m_Context) != PA_ERR_NODATA)
    {
      CLog::Log(LOGERROR, "PulseAudio: pa_stream_get_latency() failed");
      break;
    }
    /* Wait until latency data is available again */
    pa_threaded_mainloop_wait(m_MainLoop);
  }
  pa_threaded_mainloop_unlock(m_MainLoop);
  return latency / 1000000.0;
}

unsigned int CPulseAudioDirectSound::GetChunkLen()
{
  return (m_dwPacketSize / m_uiChannels) * m_uiDataChannels;
}

int CPulseAudioDirectSound::SetPlaySpeed(int iSpeed)
{
  return 0;
}

void CPulseAudioDirectSound::RegisterAudioCallback(IAudioCallback *pCallback)
{
  m_pCallback = pCallback;
}

void CPulseAudioDirectSound::UnRegisterAudioCallback()
{
  m_pCallback = NULL;
}

void CPulseAudioDirectSound::WaitCompletion()
{
  if (!m_bIsAllocated)
    return;

  pa_threaded_mainloop_lock(m_MainLoop);
  WaitForOperation(pa_stream_drain(m_Stream, NULL, NULL), m_MainLoop, "Drain");
  pa_threaded_mainloop_unlock(m_MainLoop);
}

void CPulseAudioDirectSound::SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers)
{
}

void CPulseAudioDirectSound::EnumerateAudioSinks(AudioSinkList& vAudioSinks, bool passthrough)
{
  pa_context *context;
  pa_threaded_mainloop *mainloop;

  if (!SetupContext(NULL, &context, &mainloop))
  {
    CLog::Log(LOGERROR, "PulseAudio: Failed to create context");
    return;
  }

    pa_threaded_mainloop_lock(mainloop);

    SinkInfoStruct sinkStruct;
    sinkStruct.passthrough = passthrough;
    sinkStruct.mainloop = mainloop;
    sinkStruct.list = &vAudioSinks;
    WaitForOperation(pa_context_get_sink_info_list(context,	SinkInfo, &sinkStruct), mainloop, "EnumerateAudioSinks");

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

bool CPulseAudioDirectSound::SetupContext(const char *host, pa_context **context, pa_threaded_mainloop **mainloop)
{
  if ((*mainloop = pa_threaded_mainloop_new()) == NULL)
  {
    CLog::Log(LOGERROR, "PulseAudio: Failed to allocate main loop");
    return false;
  }

  if (((*context) = pa_context_new(pa_threaded_mainloop_get_api(*mainloop), "XBMC")) == NULL)
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

