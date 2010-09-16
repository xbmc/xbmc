#include "PulseAE.h"
#include "PulseStream.h"
#include "PulseSound.h"
#include "log.h"
#include <pulse/pulseaudio.h>

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

CPulseAE::CPulseAE()
{
  m_Context = NULL;
  m_MainLoop = NULL;
}

CPulseAE::~CPulseAE()
{
  if (m_MainLoop)
    pa_threaded_mainloop_stop(m_MainLoop);

  if (m_Context)
  {
    pa_context_disconnect(m_Context);
    pa_context_unref(m_Context);
    m_Context = NULL;
  }
}

bool CPulseAE::Initialize()
{
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

void CPulseAE::OnSettingsChange(CStdString setting)
{
}

float CPulseAE::GetVolume()
{
  return 0.0f;
}

void CPulseAE::SetVolume(float volume)
{
  printf("AE SetVolume %f\n", volume);
}

IAEStream *CPulseAE::GetStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options)
{
  return new CPulseStream(m_Context, m_MainLoop, dataFormat, sampleRate, channelCount, channelLayout, options);
}

IAESound *CPulseAE::GetSound(CStdString file)
{
  printf("GetSound %s\n", file.c_str());
  return new CPulseSound(file);
}

void CPulseAE::FreeSound(IAESound *sound)
{
  printf("FreeSound\n");
  delete sound;
}

void CPulseAE::GarbageCollect()
{
}

void CPulseAE::EnumerateOutputDevices(AEDeviceList &devices, bool passthrough)
{
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
