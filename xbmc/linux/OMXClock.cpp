/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#elif defined(_WIN32)
#include "system.h"
#endif

#if defined(HAVE_OMXLIB)

#include "video/VideoReferenceClock.h"
#include "settings/GUISettings.h"

#include "OMXClock.h"
#include "utils/MathUtils.h"

int64_t OMXClock::m_systemOffset;
int64_t OMXClock::m_systemFrequency;
bool    OMXClock::m_ismasterclock;

OMXClock::OMXClock()
{
  m_has_video   = false;
  m_has_audio   = false;
  m_video_start = false;
  m_audio_start = false;
  m_pause       = false;

  m_systemOffset = 0;
  m_systemFrequency = 0;

  CheckSystemClock();

  m_systemUsed = m_systemFrequency;
  m_pauseClock = 0;
  m_bReset = true;
  m_iDisc = 0;
  m_maxspeedadjust = 0.0;
  m_speedadjust = false;
  m_ismasterclock = true;
  m_ClockOffset = 0;
  m_fps = 25.0f;
  m_omx_speed = DVD_PLAYSPEED_NORMAL;
  m_audio_buffer = false;

  pthread_mutex_init(&m_lock, NULL);
}

OMXClock::~OMXClock()
{
  OMXDeinitialize();
  pthread_mutex_destroy(&m_lock);
}

void OMXClock::Lock()
{
  pthread_mutex_lock(&m_lock);
}

void OMXClock::UnLock()
{
  pthread_mutex_unlock(&m_lock);
}

double OMXClock::GetAbsoluteClock(bool interpolated /*= true*/)
{
  Lock();
  CheckSystemClock();

  int64_t current;
  current = g_VideoReferenceClock.GetTime(interpolated);

  UnLock();
  return SystemToAbsolute(current);
}

double OMXClock::WaitAbsoluteClock(double target)
{
  Lock();
  CheckSystemClock();

  int64_t systemtarget, freq, offset;
  freq   = m_systemFrequency;
  offset = m_systemOffset;
  UnLock();

  systemtarget = (int64_t)(target / DVD_TIME_BASE * (double)freq);
  systemtarget += offset;
  systemtarget = g_VideoReferenceClock.Wait(systemtarget);
  systemtarget -= offset;
  return (double)systemtarget / freq * DVD_TIME_BASE;
}

// Returns the current absolute clock in units of DVD_TIME_BASE (usually microseconds).
void OMXClock::CheckSystemClock()
{
  if(!m_systemFrequency)
    m_systemFrequency = g_VideoReferenceClock.GetFrequency();

  if(!m_systemOffset)
    m_systemOffset = g_VideoReferenceClock.GetTime();
}

double OMXClock::GetClock(bool interpolated /*= true*/)
{
  Lock();
  double clock = SystemToPlaying(g_VideoReferenceClock.GetTime(interpolated));
  UnLock();
  return clock;
}

double OMXClock::GetClock(double& absolute, bool interpolated /*= true*/)
{
  int64_t current = g_VideoReferenceClock.GetTime(interpolated);

  Lock();
  CheckSystemClock();
  absolute = SystemToAbsolute(current);
  current = SystemToPlaying(current);
  UnLock();

  return current;
}

void OMXClock::SetSpeed(int iSpeed)
{
  // this will sometimes be a little bit of due to rounding errors, ie clock might jump abit when changing speed
  Lock();

  if(iSpeed == DVD_PLAYSPEED_PAUSE)
  {
    if(!m_pauseClock)
      m_pauseClock = g_VideoReferenceClock.GetTime();
    UnLock();
    return;
  }

  int64_t current;
  int64_t newfreq = m_systemFrequency * DVD_PLAYSPEED_NORMAL / iSpeed;

  current = g_VideoReferenceClock.GetTime();
  if( m_pauseClock )
  {
    m_startClock += current - m_pauseClock;
    m_pauseClock = 0;
  }

  m_startClock = current - (int64_t)((double)(current - m_startClock) * newfreq / m_systemUsed);
  m_systemUsed = newfreq;
  UnLock();
}

void OMXClock::Discontinuity(double currentPts)
{
  Lock();
  m_startClock = g_VideoReferenceClock.GetTime();
  if(m_pauseClock)
    m_pauseClock = m_startClock;
  m_iDisc = currentPts;
  m_bReset = false;
  UnLock();
}

void OMXClock::Pause()
{
  Lock();
  if(!m_pauseClock)
    m_pauseClock = g_VideoReferenceClock.GetTime();
  UnLock();
}

void OMXClock::Resume()
{
  Lock();
  if( m_pauseClock )
  {
    int64_t current;
    current = g_VideoReferenceClock.GetTime();

    m_startClock += current - m_pauseClock;
    m_pauseClock = 0;
  }
  UnLock();
}

bool OMXClock::SetMaxSpeedAdjust(double speed)
{
  Lock();
  m_maxspeedadjust = speed;
  UnLock();
  return m_speedadjust;
}

//returns the refreshrate if the videoreferenceclock is running, -1 otherwise
int OMXClock::UpdateFramerate(double fps, double* interval /*= NULL*/)
{
  //sent with fps of 0 means we are not playing video
  if(fps == 0.0)
  {
    Lock();
    m_speedadjust = false;
    UnLock();
    return -1;
  }

  //check if the videoreferenceclock is running, will return -1 if not
  int rate = g_VideoReferenceClock.GetRefreshRate(interval);

  if (rate <= 0)
    return -1;

  Lock();

  m_speedadjust = true;

  double weight = (double)rate / (double)MathUtils::round_int(fps);

  //set the speed of the videoreferenceclock based on fps, refreshrate and maximum speed adjust set by user
  if (m_maxspeedadjust > 0.05)
  {
    if (weight / MathUtils::round_int(weight) < 1.0 + m_maxspeedadjust / 100.0
    &&  weight / MathUtils::round_int(weight) > 1.0 - m_maxspeedadjust / 100.0)
      weight = MathUtils::round_int(weight);
  }
  double speed = (double)rate / (fps * weight);
  UnLock();

  g_VideoReferenceClock.SetSpeed(speed);

  return rate;
}

double OMXClock::SystemToAbsolute(int64_t system)
{
  return DVD_TIME_BASE * (double)(system - m_systemOffset) / m_systemFrequency;
}

double OMXClock::SystemToPlaying(int64_t system)
{
  int64_t current;

  if (m_bReset)
  {
    m_startClock = system;
    m_systemUsed = m_systemFrequency;
    m_pauseClock = 0;
    m_iDisc = 0;
    m_bReset = false;
  }

  if (m_pauseClock)
    current = m_pauseClock;
  else
    current = system;

  return DVD_TIME_BASE * (double)(current - m_startClock) / m_systemUsed + m_iDisc;
}

void OMXClock::OMXSetClockPorts(OMX_TIME_CONFIG_CLOCKSTATETYPE *clock)
{
  if(m_omx_clock.GetComponent() == NULL)
    return;

  if(!clock)
    return;

  if(m_has_audio)
  {
    m_audio_start = true;
    clock->nWaitMask |= OMX_CLOCKPORT0;
  }

  if(m_has_video)
  {
    m_video_start = true;
    clock->nWaitMask |= OMX_CLOCKPORT1;
  }
}

bool OMXClock::OMXSetReferenceClock(bool lock /* = true */)
{
  if(lock)
    Lock();

  bool ret = true;
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  OMX_TIME_CONFIG_ACTIVEREFCLOCKTYPE refClock;
  OMX_INIT_STRUCTURE(refClock);

  if(g_guiSettings.GetBool("videoplayer.usedisplayasclock") && m_has_video)
    refClock.eClock = OMX_TIME_RefClockVideo;
  else if(m_has_audio)
    refClock.eClock = OMX_TIME_RefClockAudio;
  else
    refClock.eClock = OMX_TIME_RefClockVideo;

  CLog::Log(LOGNOTICE, "OMXClock using %s as reference\n", refClock.eClock == OMX_TIME_RefClockVideo ? "video" : "audio");

  omx_err = m_omx_clock.SetConfig(OMX_IndexConfigTimeActiveRefClock, &refClock);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "OMXClock::OMXSetReferenceClock error setting OMX_IndexConfigTimeActiveRefClock\n");
    ret = false;
  }

  UnLock();

  return ret;
}

bool OMXClock::OMXInitialize(bool has_video, bool has_audio)
{
  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  std::string componentName = "";

  m_has_video = has_video;
  m_has_audio = has_audio;

  m_video_start = false;
  m_audio_start = false;
  m_pause       = false;
  m_audio_buffer = false;

  componentName = "OMX.broadcom.clock";
  if(!m_omx_clock.Initialize((const std::string)componentName, OMX_IndexParamOtherInit))
    return false;

  if(!OMXSetReferenceClock(false))
    return false;

  OMX_TIME_CONFIG_CLOCKSTATETYPE clock;
  OMX_INIT_STRUCTURE(clock);

  clock.eState = OMX_TIME_ClockStateWaitingForStartTime;

  omx_err = m_omx_clock.SetConfig(OMX_IndexConfigTimeClockState, &clock);
  if(omx_err != OMX_ErrorNone)
    CLog::Log(LOGWARNING, "OMXClock::OMXInitialize setting OMX_IndexConfigTimeClockState\n");

  return true;
}

void OMXClock::OMXDeinitialize()
{
  if(m_omx_clock.GetComponent() == NULL)
    return;

  m_omx_clock.Deinitialize();

  m_omx_speed = DVD_PLAYSPEED_NORMAL;
}

bool OMXClock::OMXStatePause(bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return false;

  if(lock)
    Lock();

  if(m_omx_clock.GetState() != OMX_StatePause)
  {
    OMX_ERRORTYPE omx_err = OMX_ErrorNone;
    omx_err = m_omx_clock.SetStateForComponent(OMX_StatePause);
    if (omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "OMXClock::StatePause m_omx_clock.SetStateForComponent\n");
      if(lock)
        UnLock();
      return false;
    }
  }

  if(lock)
    UnLock();

  return true;
}

bool OMXClock::OMXStateExecute(bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return false;

  if(lock)
    Lock();

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  if(m_omx_clock.GetState() != OMX_StateExecuting)
  {

    OMXStateIdle(false);

    omx_err = m_omx_clock.SetStateForComponent(OMX_StateExecuting);
    if (omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "OMXClock::StateExecute m_omx_clock.SetStateForComponent\n");
      if(lock)
        UnLock();
      return false;
    }
  }

  /*
  if(m_has_audio)
  {
    omx_err = m_omx_clock.EnablePort(m_omx_clock.GetInputPort(), true);
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "OMXClock::StateExecute - Error enable port %d on component %s omx_err(0x%08x)", 
            m_omx_clock.GetInputPort(), m_omx_clock.GetName().c_str(), (int)omx_err);
    }
  }

  if(m_has_video)
  {
    omx_err = m_omx_clock.EnablePort(m_omx_clock.GetInputPort() + 1, true);
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "OMXClock::StateExecute - Error enable port %d on component %s omx_err(0x%08x)", 
            m_omx_clock.GetInputPort(), m_omx_clock.GetName().c_str(), (int)omx_err);
    }
  }

  OMX_TIME_CONFIG_CLOCKSTATETYPE clock;
  OMX_INIT_STRUCTURE(clock);

  clock.eState      = OMX_TIME_ClockStateWaitingForStartTime;
  clock.nStartTime  = ToOMXTime(0LL);
  clock.nOffset     = ToOMXTime(0LL);
  clock.nWaitMask   = 0;

  omx_err = m_omx_clock.SetConfig(OMX_IndexConfigTimeClockState, &clock);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "OMXClock::OMXStateExecute error setting OMX_IndexConfigTimeClockState\n");
  }
  */

  //OMXStart(lock);

  if(lock)
    UnLock();

  return true;
}

void OMXClock::OMXStateIdle(bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return;

  if(lock)
    Lock();

  if(m_omx_clock.GetState() == OMX_StateExecuting)
    m_omx_clock.SetStateForComponent(OMX_StatePause);

  if(m_omx_clock.GetState() != OMX_StateIdle)
    m_omx_clock.SetStateForComponent(OMX_StateIdle);

  if(lock)
    UnLock();
}

COMXCoreComponent *OMXClock::GetOMXClock()
{
  return &m_omx_clock;
}

void OMXClock::OMXSaveState(bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return;

  if(lock)
    Lock();

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  OMX_INIT_STRUCTURE(m_clock_state);

  omx_err = m_omx_clock.GetConfig(OMX_IndexConfigTimeClockState, &m_clock_state);
  if(omx_err != OMX_ErrorNone)
    CLog::Log(LOGERROR, "OMXClock::SaveState error geting OMX_IndexConfigTimeClockState\n");

  if(lock)
    UnLock();
}

void OMXClock::OMXRestoreState(bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return;

  if(lock)
    Lock();

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  omx_err = m_omx_clock.SetConfig(OMX_IndexConfigTimeClockState, &m_clock_state);
  if(omx_err != OMX_ErrorNone)
    CLog::Log(LOGERROR, "OMXClock::RestoreState error setting OMX_IndexConfigTimeClockState\n");

  if(lock)
    UnLock();
}

bool  OMXClock::OMXStop(bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return false;

  if(lock)
    Lock();

  CLog::Log(LOGDEBUG, "OMXClock::OMXStop\n");

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  OMX_TIME_CONFIG_CLOCKSTATETYPE clock;
  OMX_INIT_STRUCTURE(clock);

  clock.eState = OMX_TIME_ClockStateStopped;

  omx_err = m_omx_clock.SetConfig(OMX_IndexConfigTimeClockState, &clock);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "OMXClock::Stop error setting OMX_IndexConfigTimeClockState\n");
    if(lock)
      UnLock();
    return false;
  }

  if(lock)
    UnLock();

  return true;
}

bool OMXClock::OMXStart(bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return false;
  
  if(lock)
    Lock();

  CLog::Log(LOGDEBUG, "OMXClock::OMXStart\n");

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  OMX_TIME_CONFIG_CLOCKSTATETYPE clock;
  OMX_INIT_STRUCTURE(clock);

  clock.eState = OMX_TIME_ClockStateRunning;

  omx_err = m_omx_clock.SetConfig(OMX_IndexConfigTimeClockState, &clock);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "OMXClock::Start error setting OMX_IndexConfigTimeClockState\n");
    if(lock)
      UnLock();
    return false;
  }

  if(lock)
    UnLock();

  return true;
}

void OMXClock::VideoStart(bool video_start)
{ 
  Lock();
  m_video_start = video_start; 
  UnLock();
};

void OMXClock::AudioStart(bool audio_start) 
{ 
  Lock();
  m_audio_start = audio_start; 
  UnLock();
};

bool OMXClock::OMXReset(bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return false;

  if(lock)
    Lock();

  CLog::Log(LOGDEBUG, "OMXClock::OMXReset 0x%08x\n", m_omx_clock.GetState());

  m_audio_buffer = false;

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;

  if(!OMXSetReferenceClock(false))
  {
    UnLock();
    return false;
  }

  OMX_TIME_CONFIG_CLOCKSTATETYPE clock;
  OMX_INIT_STRUCTURE(clock);

  clock.eState    = OMX_TIME_ClockStateWaitingForStartTime;
  //clock.nOffset   = ToOMXTime(-1000LL * 200);

  OMXSetClockPorts(&clock);

  if(clock.nWaitMask)
  {
    omx_err = m_omx_clock.SetConfig(OMX_IndexConfigTimeClockState, &clock);
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "OMXClock::OMXReset error setting OMX_IndexConfigTimeClockState\n");
      if(lock)
        UnLock();
      return false;
    }
  }

  if(lock)
    UnLock();

  return true;
}

double OMXClock::OMXWallTime(bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return 0;

  if(lock)
    Lock();

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  double pts = 0;

  OMX_TIME_CONFIG_TIMESTAMPTYPE timeStamp;
  OMX_INIT_STRUCTURE(timeStamp);
  timeStamp.nPortIndex = m_omx_clock.GetInputPort();

  omx_err = m_omx_clock.GetConfig(OMX_IndexConfigTimeCurrentWallTime, &timeStamp);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "OMXClock::WallTime error getting OMX_IndexConfigTimeCurrentWallTime\n");
    if(lock)
      UnLock();
    return 0;
  }

  pts = FromOMXTime(timeStamp.nTimestamp);

  if(lock)
    UnLock();
  
  return pts;
}

double OMXClock::OMXMediaTime(bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return 0;

  if(lock)
    Lock();

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  double pts = 0;

  OMX_TIME_CONFIG_TIMESTAMPTYPE timeStamp;
  OMX_INIT_STRUCTURE(timeStamp);
  timeStamp.nPortIndex = m_omx_clock.GetInputPort();

  omx_err = m_omx_clock.GetConfig(OMX_IndexConfigTimeCurrentMediaTime, &timeStamp);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "OMXClock::MediaTime error getting OMX_IndexConfigTimeCurrentMediaTime\n");
    if(lock)
      UnLock();
    return 0;
  }

  pts = FromOMXTime(timeStamp.nTimestamp);
  if(lock)
    UnLock();
  
  return pts;
}

bool OMXClock::OMXPause(bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return false;

  if(m_pause)
    return true;

  if(lock)
    Lock();

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  OMX_TIME_CONFIG_SCALETYPE scaleType;
  OMX_INIT_STRUCTURE(scaleType);

  scaleType.xScale = 0; // pause

  omx_err = m_omx_clock.SetConfig(OMX_IndexConfigTimeScale, &scaleType);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "OMXClock::Pause error setting OMX_IndexConfigTimeClockState\n");
    if(lock)
      UnLock();
    return false;
  }

  CLog::Log(LOGDEBUG, "OMXClock::OMXPause\n");

  m_pause = true;

  if(lock)
    UnLock();

  return true;
}

bool OMXClock::OMXResume(bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return false;

  if(!m_pause)
    return true;

  if(lock)
    Lock();

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  OMX_TIME_CONFIG_SCALETYPE scaleType;
  OMX_INIT_STRUCTURE(scaleType);

  scaleType.xScale = (1<<16); // normal speed

  omx_err = m_omx_clock.SetConfig(OMX_IndexConfigTimeScale, &scaleType);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "OMXClock::Resume error setting OMX_IndexConfigTimeClockState\n");
    if(lock)
      UnLock();
    return false;
  }

  m_pause = false;

  CLog::Log(LOGDEBUG, "OMXClock::OMXResume\n");

  if(lock)
    UnLock();

  return true;
}

bool OMXClock::OMXUpdateClock(double pts, bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return false;

  if(lock)
    Lock();

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  OMX_TIME_CONFIG_TIMESTAMPTYPE ts;
  OMX_INIT_STRUCTURE(ts);

  ts.nPortIndex = OMX_ALL;
  ts.nTimestamp = ToOMXTime((uint64_t)pts);

  CLog::Log(LOGDEBUG, "OMXClock::OMXUpdateClock %f", pts / DVD_TIME_BASE);

  if(m_has_audio)
  {
    omx_err = m_omx_clock.SetConfig(OMX_IndexConfigTimeCurrentAudioReference, &ts);
    if(omx_err != OMX_ErrorNone)
      CLog::Log(LOGERROR, "OMXClock::OMXUpdateClock error setting OMX_IndexConfigTimeCurrentAudioReference\n");
  }
  else
  {
    omx_err = m_omx_clock.SetConfig(OMX_IndexConfigTimeCurrentVideoReference, &ts);
    if(omx_err != OMX_ErrorNone)
      CLog::Log(LOGERROR, "OMXClock::OMXUpdateClock error setting OMX_IndexConfigTimeCurrentVideoReference\n");
  }

  if(lock)
    UnLock();

  return true;
}

bool OMXClock::OMXWaitStart(double pts, bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return false;

  if(lock)
    Lock();

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  OMX_TIME_CONFIG_CLOCKSTATETYPE clock;
  OMX_INIT_STRUCTURE(clock);

  clock.eState = OMX_TIME_ClockStateWaitingForStartTime;
  OMXSetClockPorts(&clock);
  clock.nStartTime  = ToOMXTime((uint64_t)pts);
  //clock.nOffset     = ToOMXTime(-1000LL * 200);

  omx_err = m_omx_clock.SetConfig(OMX_IndexConfigTimeClockState, &clock);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "OMXClock::OMXWaitStart error setting OMX_IndexConfigTimeClockState\n");
    if(lock)
      UnLock();
    return false;
  }

  if(lock)
    UnLock();

  return true;
}

void OMXClock::OMXHandleBackward(bool lock /* = true */)
{
  /*
  if(m_omx_clock.GetComponent() == NULL)
    return;

  if(lock)
    Lock();

  if(m_omx_speed < 0)
  {
    OMX_ERRORTYPE omx_err = OMX_ErrorNone;
    OMX_TIME_CONFIG_CLOCKSTATETYPE clock;
    OMX_INIT_STRUCTURE(clock);
    
    clock.eState = OMX_TIME_ClockStateRunning;

    omx_err = m_omx_clock.SetConfig(OMX_IndexConfigTimeClockState, &clock);
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "OMXClock::OMXHandleBackward error setting OMX_IndexConfigTimeClockState\n");
    }
  }

  if(lock)
    UnLock();
  */
}

bool OMXClock::OMXSetSpeed(int speed, bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return false;

  if(m_pause)
    return true;

  if(lock)
    Lock();

  m_omx_speed = speed;

  CLog::Log(LOGDEBUG, "OMXClock::OMXSetSpeed fake %d\n\n", m_omx_speed / DVD_PLAYSPEED_NORMAL);

  // only adjust speed when not audio buffering
  if(!m_audio_buffer)
  {
    OMX_ERRORTYPE omx_err = OMX_ErrorNone;
    OMX_TIME_CONFIG_SCALETYPE scaleType;
    OMX_INIT_STRUCTURE(scaleType);

    scaleType.xScale = ((m_omx_speed / DVD_PLAYSPEED_NORMAL) << 16);

    CLog::Log(LOGDEBUG, "OMXClock::OMXSetSpeed real %d", m_omx_speed / DVD_PLAYSPEED_NORMAL);

    omx_err = m_omx_clock.SetConfig(OMX_IndexConfigTimeScale, &scaleType);
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "OMXClock::OMXSetSpeed error setting OMX_IndexConfigTimeClockState\n");
      if(lock)
        UnLock();
      return false;
    }
  }

  if(lock)
    UnLock();

  return true;
}

void OMXClock::AddTimespecs(struct timespec &time, long millisecs)
{
   time.tv_sec  += millisecs / 1000;
   time.tv_nsec += (millisecs % 1000) * 1000000;
   if (time.tv_nsec > 1000000000)
   {
      time.tv_sec  += 1;
      time.tv_nsec -= 1000000000;
   }
}

bool OMXClock::HDMIClockSync(bool lock /* = true */)
{
  if(m_omx_clock.GetComponent() == NULL)
    return false;

  if(lock)
    Lock();

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  OMX_CONFIG_LATENCYTARGETTYPE latencyTarget;
  OMX_INIT_STRUCTURE(latencyTarget);

  latencyTarget.nPortIndex = OMX_ALL;
  latencyTarget.bEnabled = OMX_TRUE;
  latencyTarget.nFilter = 10;
  latencyTarget.nTarget = 0;
  latencyTarget.nShift = 3;
  latencyTarget.nSpeedFactor = -200;
  latencyTarget.nInterFactor = 100;
  latencyTarget.nAdjCap = 100;

  omx_err = m_omx_clock.SetConfig(OMX_IndexConfigLatencyTarget, &latencyTarget);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "OMXClock::Speed error setting OMX_IndexConfigLatencyTarget\n");
    if(lock)
      UnLock();
    return false;
  }

  if(lock)
    UnLock();

  return true;
}

int64_t OMXClock::CurrentHostCounter(void)
{
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return( ((int64_t)now.tv_sec * 1000000000L) + now.tv_nsec );
}

int64_t OMXClock::CurrentHostFrequency(void)
{
  return( (int64_t)1000000000L );
}

void OMXClock::AddTimeSpecNano(struct timespec &time, uint64_t nanoseconds)
{
   time.tv_sec  += nanoseconds / 1000000000;
   time.tv_nsec += (nanoseconds % 1000000000);
   if (time.tv_nsec > 1000000000)
   {
      time.tv_sec  += 1;
      time.tv_nsec -= 1000000000;
   }
}

int OMXClock::GetRefreshRate(double* interval)
{
  if(!interval)
    return false;

  *interval = m_fps;
  return true;
}

void OMXClock::OMXAudioBufferStart()
{
  Lock();

  m_audio_buffer = true;

  if(m_omx_clock.GetComponent() == NULL)
  {
    UnLock();
    return;
  }

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  OMX_TIME_CONFIG_SCALETYPE scaleType;
  OMX_INIT_STRUCTURE(scaleType);

  scaleType.xScale = 0;

  CLog::Log(LOGDEBUG, "OMXClock::OMXAudioBufferStart");

  omx_err = m_omx_clock.SetConfig(OMX_IndexConfigTimeScale, &scaleType);
  if(omx_err != OMX_ErrorNone)
    CLog::Log(LOGERROR, "OMXClock::OMXAudioBufferStart error setting OMX_IndexConfigTimeClockState\n");

  UnLock();
}

void OMXClock::OMXAudioBufferStop()
{
  Lock();

  CLog::Log(LOGDEBUG, "OMXClock::OMXAudioBufferStop");

  m_audio_buffer = false;

  OMXSetSpeed(m_omx_speed, false);

  UnLock();
}

double OMXClock::NormalizeFrameduration(double frameduration)
{
  //if the duration is within 20 microseconds of a common duration, use that
  const double durations[] = {DVD_TIME_BASE * 1.001 / 24.0, DVD_TIME_BASE / 24.0, DVD_TIME_BASE / 25.0,
                              DVD_TIME_BASE * 1.001 / 30.0, DVD_TIME_BASE / 30.0, DVD_TIME_BASE / 50.0,
                              DVD_TIME_BASE * 1.001 / 60.0, DVD_TIME_BASE / 60.0};

  double lowestdiff = DVD_TIME_BASE;
  int    selected   = -1;
  for (size_t i = 0; i < sizeof(durations) / sizeof(durations[0]); i++)
  {
    double diff = fabs(frameduration - durations[i]);
    if (diff < DVD_MSEC_TO_TIME(0.02) && diff < lowestdiff)
    {
      selected = i;
      lowestdiff = diff;
    }
  }

  if (selected != -1)
    return durations[selected];
  else
    return frameduration;
}

#endif
