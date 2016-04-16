/*
 *      Copyright (C) 2005-2013 Team XBMC
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
#include "VideoReferenceClock.h"
#include "utils/MathUtils.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "threads/SingleLock.h"
#include "guilib/GraphicContext.h"
#include "video/videosync/VideoSync.h"
#include "settings/Settings.h"

#if defined(HAS_GLX)
#include "video/videosync/VideoSyncGLX.h"
#endif
#if defined(HAVE_X11)
#include "video/videosync/VideoSyncDRM.h"
#include "windowing/WindowingFactory.h"
#elif defined(TARGET_RASPBERRY_PI)
#include "video/videosync/VideoSyncPi.h"
#elif defined(HAS_IMXVPU)
#include "video/videosync/VideoSyncIMX.h"
#endif
#if defined(TARGET_WINDOWS)
#include "video/videosync/VideoSyncD3D.h"
#endif
#if defined(TARGET_DARWIN_OSX)
#include "video/videosync/VideoSyncOsx.h"
#endif
#if defined(TARGET_DARWIN_IOS)
#include "video/videosync/VideoSyncIos.h"
#endif
#if defined(TARGET_ANDROID)
#include "video/videosync/VideoSyncAndroid.h"
#endif

#ifdef TARGET_POSIX
#include "linux/XTimeUtils.h"
#endif

CVideoReferenceClock::CVideoReferenceClock() : CThread("RefClock")
{
  m_SystemFrequency = CurrentHostFrequency();
  m_ClockSpeed = 1.0;
  m_ClockOffset = 0;
  m_TotalMissedVblanks = 0;
  m_UseVblank = false;

  m_CurrTime = 0;
  m_LastIntTime = 0;
  m_CurrTimeFract = 0.0;
  m_RefreshRate = 0.0;
  m_MissedVblanks = 0;
  m_VblankTime = 0;

  m_pVideoSync = nullptr;

  Start();
}

CVideoReferenceClock::~CVideoReferenceClock()
{
  StopThread();
}

void CVideoReferenceClock::Start()
{
  m_ClockOffset = CurrentHostCounter();
  if(CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOPLAYER_USEDISPLAYASCLOCK) && !IsRunning())
    Create();
}

void CVideoReferenceClock::CBUpdateClock(int NrVBlanks, uint64_t time, CVideoReferenceClock *clock)
{
  {
    CSingleLock lock(clock->m_CritSection);
    clock->m_VblankTime = time;
    clock->UpdateClock(NrVBlanks, true);
  }
}

void CVideoReferenceClock::Process()
{
  bool SetupSuccess = false;
  int64_t Now;

  while(!m_bStop)
  {
    //set up the vblank clock
#if defined(HAVE_X11)
  std::string gpuvendor = g_Windowing.GetRenderVendor();
  std::transform(gpuvendor.begin(), gpuvendor.end(), gpuvendor.begin(), ::tolower);
  if ((gpuvendor.compare(0, 5, "intel") == 0 ||
       gpuvendor.compare(0, 5, "x.org") == 0)) // AMD
    m_pVideoSync = new CVideoSyncDRM(this);
#if defined(HAS_GLX)
  else
    m_pVideoSync = new CVideoSyncGLX(this);
#endif
#elif defined(TARGET_WINDOWS)
    m_pVideoSync = new CVideoSyncD3D(this);
#elif defined(TARGET_DARWIN_OSX)
    m_pVideoSync = new CVideoSyncOsx(this);
#elif defined(TARGET_DARWIN_IOS)
    m_pVideoSync = new CVideoSyncIos(this);
#elif defined(TARGET_RASPBERRY_PI)
    m_pVideoSync = new CVideoSyncPi(this);
#elif defined(HAS_IMXVPU)
    m_pVideoSync = new CVideoSyncIMX(this);
#elif defined(TARGET_ANDROID)
    m_pVideoSync = new CVideoSyncAndroid(this);
#endif

    if (m_pVideoSync)
    {
      SetupSuccess = m_pVideoSync->Setup(CBUpdateClock);
      UpdateRefreshrate();
    }

    CSingleLock SingleLock(m_CritSection);
    Now = CurrentHostCounter();
    m_CurrTime = Now + m_ClockOffset; //add the clock offset from the previous time we stopped
    m_LastIntTime = m_CurrTime;
    m_CurrTimeFract = 0.0;
    m_ClockSpeed = 1.0;
    m_TotalMissedVblanks = 0;
    m_MissedVblanks = 0;

    if (SetupSuccess)
    {
      m_UseVblank = true;          //tell other threads we're using vblank as clock
      m_VblankTime = Now;          //initialize the timestamp of the last vblank
      SingleLock.Leave();

      //run the clock
      m_pVideoSync->Run(m_bStop);
    }
    else
    {
      SingleLock.Leave();
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Setup failed, falling back to CurrentHostCounter()");
    }

    SingleLock.Enter();
    m_UseVblank = false;                       //we're back to using the systemclock
    Now = CurrentHostCounter();                //set the clockoffset between the vblank clock and systemclock
    m_ClockOffset = m_CurrTime - Now;
    SingleLock.Leave();

    //clean up the vblank clock
    if (m_pVideoSync)
    {
      m_pVideoSync->Cleanup();
      delete m_pVideoSync;
      m_pVideoSync = nullptr;
    }

    if (!SetupSuccess)
      break;
  }
}

//this is called from the vblank run function and from CVideoReferenceClock::Wait in case of a late update
void CVideoReferenceClock::UpdateClock(int NrVBlanks, bool CheckMissed)
{
  if (CheckMissed) //set to true from the vblank run function, set to false from Wait and GetTime
  {
    if (NrVBlanks < m_MissedVblanks) //if this is true the vblank detection in the run function is wrong
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: detected %i vblanks, missed %i, refreshrate might have changed",
                NrVBlanks, m_MissedVblanks);

    NrVBlanks -= m_MissedVblanks; //subtract the vblanks we missed
    m_MissedVblanks = 0;
  }
  else
  {
    m_MissedVblanks += NrVBlanks;      //tell the vblank clock how many vblanks it missed
    m_TotalMissedVblanks += NrVBlanks; //for the codec information screen
    m_VblankTime += m_SystemFrequency * static_cast<int64_t>(NrVBlanks) / MathUtils::round_int(m_RefreshRate); //set the vblank time forward
  }

  if (NrVBlanks > 0) //update the clock with the adjusted frequency if we have any vblanks
  {
    double increment = UpdateInterval() * NrVBlanks;
    double integer   = floor(increment);
    m_CurrTime      += static_cast<int64_t>(integer + 0.5); //make sure it gets correctly converted to int

    //accumulate what we lost due to rounding in m_CurrTimeFract, then add the integer part of that to m_CurrTime
    m_CurrTimeFract += increment - integer;
    integer          = floor(m_CurrTimeFract);
    m_CurrTime      += static_cast<int64_t>(integer + 0.5);
    m_CurrTimeFract -= integer;
  }
}

double CVideoReferenceClock::UpdateInterval() const
{
  return m_ClockSpeed / m_RefreshRate * static_cast<double>(m_SystemFrequency);
}

//called from dvdclock to get the time
int64_t CVideoReferenceClock::GetTime(bool interpolated /* = true*/)
{
  CSingleLock SingleLock(m_CritSection);

  //when using vblank, get the time from that, otherwise use the systemclock
  if (m_UseVblank)
  {
    int64_t  NextVblank;
    int64_t  Now;

    Now = CurrentHostCounter();        //get current system time
    NextVblank = TimeOfNextVblank();   //get time when the next vblank should happen

    while(Now >= NextVblank)  //keep looping until the next vblank is in the future
    {
      UpdateClock(1, false);           //update clock when next vblank should have happened already
      NextVblank = TimeOfNextVblank(); //get time when the next vblank should happen
    }

    if (interpolated)
    {
      //interpolate from the last time the clock was updated
      double elapsed = static_cast<double>(Now - m_VblankTime) * m_ClockSpeed;
      //don't interpolate more than 2 vblank periods
      elapsed = std::min(elapsed, UpdateInterval() * 2.0);

      //make sure the clock doesn't go backwards
      int64_t intTime = m_CurrTime + static_cast<int64_t>(elapsed);
      if (intTime > m_LastIntTime)
        m_LastIntTime = intTime;

      return m_LastIntTime;
    }
    else
    {
      return m_CurrTime;
    }
  }
  else
  {
    return CurrentHostCounter() + m_ClockOffset;
  }
}

void CVideoReferenceClock::SetSpeed(double Speed)
{
  CSingleLock SingleLock(m_CritSection);
  //VideoPlayer can change the speed to fit the rereshrate
  if (m_UseVblank)
  {
    if (Speed != m_ClockSpeed)
    {
      m_ClockSpeed = Speed;
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Clock speed %f%%", GetSpeed() * 100.0);
    }
  }
}

double CVideoReferenceClock::GetSpeed()
{
  CSingleLock SingleLock(m_CritSection);

  //VideoPlayer needs to know the speed for the resampler
  if (m_UseVblank)
    return m_ClockSpeed;
  else
    return 1.0;
}

void CVideoReferenceClock::UpdateRefreshrate()
{
  CSingleLock SingleLock(m_CritSection);
  m_RefreshRate = m_pVideoSync->GetFps();
  m_ClockSpeed = 1.0;

  CLog::Log(LOGDEBUG, "CVideoReferenceClock: Detected refreshrate: %.3f hertz", m_RefreshRate);
}

//VideoPlayer needs to know the refreshrate for matching the fps of the video playing to it
double CVideoReferenceClock::GetRefreshRate(double* interval /*= NULL*/)
{
  CSingleLock SingleLock(m_CritSection);

  if (m_UseVblank)
  {
    if (interval)
      *interval = m_ClockSpeed / m_RefreshRate;

    return m_RefreshRate;
  }
  else
    return -1;
}

#define MAXVBLANKDELAY 13LL
//guess when the next vblank should happen,
//based on the refreshrate and when the previous one happened
//increase that by 30% to allow for errors
int64_t CVideoReferenceClock::TimeOfNextVblank() const
{
  return m_VblankTime + (m_SystemFrequency / MathUtils::round_int(m_RefreshRate) * MAXVBLANKDELAY / 10LL);
}

//for the codec information screen
bool CVideoReferenceClock::GetClockInfo(int& MissedVblanks, double& ClockSpeed, double& RefreshRate) const
{
  if (m_UseVblank)
  {
    MissedVblanks = m_TotalMissedVblanks;
    ClockSpeed = m_ClockSpeed * 100.0;
    RefreshRate = m_RefreshRate;
    return true;
  }
  return false;
}
