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

#include "PulseAE.h"
#include "PulseAEStream.h"
#include "PulseAESound.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "settings/Settings.h"
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include "guilib/LocalizeStrings.h"

/* Static helpers */
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

#if 0
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
#endif

CPulseAE::CPulseAE()
{
  m_Context = NULL;
  m_MainLoop = NULL;
  m_muted = false;
}

CPulseAE::~CPulseAE()
{
  if (m_Context)
  {
    pa_context_disconnect(m_Context);
    pa_context_unref(m_Context);
    m_Context = NULL;
  }

  if (m_MainLoop)
  {
    pa_threaded_mainloop_stop(m_MainLoop);
    pa_threaded_mainloop_free(m_MainLoop);
  }

}

bool CPulseAE::CanInit()
{
  pa_simple *s;
  pa_sample_spec ss;
 
  ss.format = PA_SAMPLE_S16NE;
  ss.channels = 2;
  ss.rate = 48000;
 
  //create a pulse client, if this returns NULL, pulseaudio isn't running
  s = pa_simple_new(NULL, "XBMC-test", PA_STREAM_PLAYBACK, NULL,"test", &ss, NULL, NULL, NULL);
 
  if (s)
  {
    pa_simple_free(s);
    return true;
  }
  else
  {
    return false;
  }
}

bool CPulseAE::Initialize()
{
  m_Volume = g_settings.m_fVolumeLevel;

  if ((m_MainLoop = pa_threaded_mainloop_new()) == NULL)
  {
    CLog::Log(LOGERROR, "PulseAudio: Failed to allocate main loop");
    return false;
  }

  if ((m_Context = pa_context_new(pa_threaded_mainloop_get_api(m_MainLoop), "XBMC")) == NULL)
  {
    CLog::Log(LOGERROR, "PulseAudio: Failed to allocate context");
    return false;
  }

  pa_context_set_state_callback(m_Context, ContextStateCallback, m_MainLoop);

  if (pa_context_connect(m_Context, NULL, (pa_context_flags_t)0, NULL) < 0)
  {
    CLog::Log(LOGERROR, "PulseAudio: Failed to connect context");
    return false;
  }

  pa_threaded_mainloop_lock(m_MainLoop);
  if (pa_threaded_mainloop_start(m_MainLoop) < 0)
  {
    CLog::Log(LOGERROR, "PulseAudio: Failed to start MainLoop");
    pa_threaded_mainloop_unlock(m_MainLoop);
    return false;
  }

  /* Wait until the context is ready */
  do
  {
    pa_threaded_mainloop_wait(m_MainLoop);
    CLog::Log(LOGDEBUG, "PulseAudio: Context %s", ContextStateToString(pa_context_get_state(m_Context)));
  }
  while (pa_context_get_state(m_Context) != PA_CONTEXT_READY && pa_context_get_state(m_Context) != PA_CONTEXT_FAILED);

  if (pa_context_get_state(m_Context) == PA_CONTEXT_FAILED)
  {
    CLog::Log(LOGERROR, "PulseAudio: Waited for the Context but it failed");
    pa_threaded_mainloop_unlock(m_MainLoop);
    return false;
  }

  pa_threaded_mainloop_unlock(m_MainLoop);
  return true;
}

bool CPulseAE::Suspend()
{
  /* TODO: add implementation here. See SoftAE for example. Code should */
  /* release exclusive or hog mode and sleep each time packets would    */
  /* normally be written to sink if m_isSuspended = true. False return  */
  /* here will simply generate a debug log entry in externalplayer.cpp  */

  return false;
}

bool CPulseAE::IsSuspended()
{
  return false;
}

bool CPulseAE::Resume()
{
  /* TODO: see comments in Suspend() above */

  return false;
}

void CPulseAE::OnSettingsChange(const std::string& setting)
{
}

float CPulseAE::GetVolume()
{
  return m_Volume;
}

void CPulseAE::SetVolume(float volume)
{
  CSingleLock lock(m_lock);
  m_Volume = volume;
  std::list<CPulseAEStream*>::iterator itt;
  for (itt = m_streams.begin(); itt != m_streams.end(); ++itt)
    (*itt)->UpdateVolume(volume);
}

IAEStream *CPulseAE::MakeStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int encodedSampleRate,CAEChannelInfo channelLayout, unsigned int options)
{
  CPulseAEStream *st = new CPulseAEStream(m_Context, m_MainLoop, dataFormat, sampleRate, channelLayout, options);

  CSingleLock lock(m_lock);
  m_streams.push_back(st);
  return st;
}

void CPulseAE::RemoveStream(IAEStream *stream)
{
  CSingleLock lock(m_lock);
  std::list<CPulseAEStream*>::iterator itt;

  m_streams.remove((CPulseAEStream *)stream);

  for (itt = m_streams.begin(); itt != m_streams.end(); ++itt)
  {
    if (*itt == stream)
    {
      m_streams.erase(itt);
      return;
    }
  }
}

IAEStream *CPulseAE::FreeStream(IAEStream *stream)
{
  RemoveStream(stream);

  CPulseAEStream *istream = (CPulseAEStream *)stream;

  delete istream;

  return NULL;
}

IAESound *CPulseAE::MakeSound(const std::string& file)
{
  CSingleLock lock(m_lock);

  CPulseAESound *sound = new CPulseAESound(file, m_Context, m_MainLoop);
  if (!sound->Initialize())
  {
    delete sound;
    return NULL;
  }

  m_sounds.push_back(sound);
  return sound;
}

void CPulseAE::FreeSound(IAESound *sound)
{
  if (!sound)
    return;

  sound->Stop();
  CSingleLock lock(m_lock);
  for (std::list<CPulseAESound*>::iterator itt = m_sounds.begin(); itt != m_sounds.end(); ++itt)
    if (*itt == sound)
    {
      m_sounds.erase(itt);
      break;
    }

  delete (CPulseAESound*)sound;
}

void CPulseAE::GarbageCollect()
{
  CSingleLock lock(m_lock);
  std::list<CPulseAEStream*>::iterator itt;
  for (itt = m_streams.begin(); itt != m_streams.end();)
  {
    if ((*itt)->IsDestroyed())
    {
      delete (*itt);
      itt = m_streams.erase(itt);
      continue;
    }
    ++itt;
  }
}

struct SinkInfoStruct
{
  bool passthrough;
  AEDeviceList *list;
  pa_threaded_mainloop *mainloop;
};

static bool WaitForOperation(pa_operation *op, pa_threaded_mainloop *mainloop, const char *LogEntry = "")
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

    if (add)
    {
      CStdString desc, sink;
      desc.Format("%s (PulseAudio)", i->description);
      sink.Format("pulse:%s@default", i->name);
      sinkStruct->list->push_back(AEDevice(desc, sink));
      CLog::Log(LOGDEBUG, "PulseAudio: Found %s with devicestring %s", desc.c_str(), sink.c_str());
    }
  }

  pa_threaded_mainloop_signal(sinkStruct->mainloop, 0);
}

void CPulseAE::EnumerateOutputDevices(AEDeviceList &devices, bool passthrough)
{
  if (!m_MainLoop || ! m_Context)
    return;

  pa_threaded_mainloop_lock(m_MainLoop);

  SinkInfoStruct sinkStruct;
  sinkStruct.passthrough = passthrough;
  sinkStruct.mainloop = m_MainLoop;
  sinkStruct.list = &devices;
  CStdString def;
  def.Format("%s (PulseAudio)",g_localizeStrings.Get(409).c_str());
  devices.push_back(AEDevice(def, "pulse:default@default"));
  WaitForOperation(pa_context_get_sink_info_list(m_Context,
                   SinkInfo, &sinkStruct), m_MainLoop, "EnumerateAudioSinks");

  pa_threaded_mainloop_unlock(m_MainLoop);
}

void CPulseAE::ContextStateCallback(pa_context *c, void *userdata)
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

void CPulseAE::SetMute(const bool enabled)
{
  CSingleLock lock(m_lock);
  std::list<CPulseAEStream*>::iterator itt;
  for (itt = m_streams.begin(); itt != m_streams.end(); ++itt)
    (*itt)->SetMute(enabled);

  m_muted = enabled;
}

#endif
