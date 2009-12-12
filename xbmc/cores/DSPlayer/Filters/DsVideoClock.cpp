/*
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2007 see AUTHORS
 *
 * This file is part of mplayerc.
 *
 * Mplayerc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mplayerc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */








#pragma comment (lib,"d3d9.lib")
  #if (D3DX_SDK_VERSION >= 42) //aug 2009 sdk and up there is no dxerr9 anymore
    #include <Dxerr.h>
    #pragma comment (lib,"DxErr.lib")
  #else
    #include <dxerr9.h>
    #define DXGetErrorString(hr)      DXGetErrorString9(hr)
    #define DXGetErrorDescription(hr) DXGetErrorDescription9(hr)
    #pragma comment (lib,"Dxerr9.lib")
  #endif
#include "WindowingFactory.h" //d3d device and d3d interface

#include <iostream> //for debugging, please remove
#include <list>
#include "StdString.h"
#include "DSVideoClock.h"
#include "MathUtils.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "utils/SingleLock.h"

void CDSVideoCallback::Reset()
{
  m_devicevalid = true;
  m_deviceused = false;
}

void CDSVideoCallback::OnDestroyDevice()
{
  CSingleLock lock(m_critsection);
  m_devicevalid = false;
  while (m_deviceused)
  {
    lock.Leave();
    m_releaseevent.Wait();
    lock.Enter();
  }
}

void CDSVideoCallback::OnCreateDevice()
{
  CSingleLock lock(m_critsection);
  m_devicevalid = true;
  m_createevent.Set();
}

void CDSVideoCallback::Aquire()
{
  CSingleLock lock(m_critsection);
  while(!m_devicevalid)
  {
    lock.Leave();
    m_createevent.Wait();
    lock.Enter();
  }
  m_deviceused = true;
}

void CDSVideoCallback::Release()
{
  CSingleLock lock(m_critsection);
  m_deviceused = false;
  m_releaseevent.Set();
}

bool CDSVideoCallback::IsValid()
{
  return m_devicevalid;
}




CDSVideoClock::CDSVideoClock()
{
  m_SystemFrequency = CurrentHostFrequency();
  m_AdjustedFrequency = m_SystemFrequency;
  m_ClockOffset = 0;
  m_TotalMissedVblanks = 0;
  m_UseVblank = false;
  m_Started.Reset();
}

void CDSVideoClock::Process()
{
  bool SetupSuccess = false;
  int64_t Now;

  //register callback
  m_DsVideoCallback.Reset();
  g_Windowing.Register(&m_DsVideoCallback);


  while(!m_bStop)
  {
    //set up the vblank clock
    SetupSuccess = SetupD3D();
    CLog::Log(LOGDEBUG, "CDSVideoClock: only available on directx build");

    CSingleLock SingleLock(m_CritSection);
    Now = CurrentHostCounter();
    m_CurrTime = Now + m_ClockOffset; //add the clock offset from the previous time we stopped
    m_AdjustedFrequency = m_SystemFrequency;
    m_TotalMissedVblanks = 0;
    m_Started.Set();

    if (SetupSuccess)
    {
      m_UseVblank = true;          //tell other threads we're using vblank as clock
      m_VblankTime = Now;          //initialize the timestamp of the last vblank
      SingleLock.Leave();

      //run the clock
      RunD3D();
    }
    else
    {
      SingleLock.Leave();
      CLog::Log(LOGDEBUG, "CDSVideoClock: Setup failed, falling back to CurrentHostCounter()");
    }

    SingleLock.Enter();
    m_UseVblank = false;                       //we're back to using the systemclock
    Now = CurrentHostCounter();                //set the clockoffset between the vblank clock and systemclock
    m_ClockOffset = m_CurrTime - Now;
    m_Started.Reset();
    SingleLock.Leave();

    //clean up the vblank clock
    CleanupD3D();
    if (!SetupSuccess) break;
  }
  g_Windowing.Unregister(&m_DsVideoCallback);
}

bool CDSVideoClock::WaitStarted(int MSecs)
{
  //we don't wait on windows, because we have to measure the refreshrate

  return true;
}

void CDSVideoClock::RunD3D()
{
  D3DRASTER_STATUS RasterStatus;
  int64_t       Now;
  int64_t       LastVBlankTime;
  unsigned int  LastLine;
  int           NrVBlanks;
  double        VBlankTime;
  int           ReturnV;

  CSingleLock SingleLock(m_CritSection);
  SingleLock.Leave();

  //get the scanline we're currently at
  m_D3dDev->GetRasterStatus(0, &RasterStatus);
  if (RasterStatus.InVBlank) LastLine = 0;
  else LastLine = RasterStatus.ScanLine;

  //init the vblanktime
  Now = CurrentHostCounter();
  LastVBlankTime = Now;

  while(!m_bStop && m_DsVideoCallback.IsValid())
  {
    //get the scanline we're currently at
    ReturnV = m_D3dDev->GetRasterStatus(0, &RasterStatus);
    if (ReturnV != D3D_OK)
    {
      CLog::Log(LOGDEBUG, "CDSVideoClock: GetRasterStatus returned returned %s: %s",
                DXGetErrorString(ReturnV), DXGetErrorDescription(ReturnV));
      return;
    }

    //if InVBlank is set, or the current scanline is lower than the previous scanline, a vblank happened
    if ((RasterStatus.InVBlank && LastLine > 0) || (RasterStatus.ScanLine < LastLine))
    {
      //calculate how many vblanks happened
      Now = CurrentHostCounter();
      VBlankTime = (double)(Now - LastVBlankTime) / (double)m_SystemFrequency;
      NrVBlanks = MathUtils::round_int(VBlankTime * (double)m_RefreshRate);

      //update the vblank timestamp, update the clock and send a signal that we got a vblank
      SingleLock.Enter();
      m_VblankTime = Now;
      UpdateClock(NrVBlanks, true);
      SingleLock.Leave();
      SendVblankSignal();

      if (UpdateRefreshrate())
      {
        //we have to measure the refreshrate again
        CLog::Log(LOGDEBUG, "CDSVideoClock: Displaymode changed");
        return;
      }

      //save the timestamp of this vblank so we can calculate how many vblanks happened next time
      LastVBlankTime = Now;

      //because we had a vblank, sleep until half the refreshrate period
      Now = CurrentHostCounter();
      int SleepTime = (int)((LastVBlankTime + (m_SystemFrequency / m_RefreshRate / 2) - Now) * 1000 / m_SystemFrequency);
      if (SleepTime > 100) SleepTime = 100; //failsafe
      if (SleepTime > 0) ::Sleep(SleepTime);
    }
    else
    {
      ::Sleep(1);
    }

    if (RasterStatus.InVBlank) LastLine = 0;
    else LastLine = RasterStatus.ScanLine;
  }
}

//how many times we measure the refreshrate
#define NRMEASURES 6
//how long to measure in milliseconds
#define MEASURETIME 250

bool CDSVideoClock::SetupD3D()
{
  int ReturnV;

  CLog::Log(LOGDEBUG, "CDSVideoClock: Setting up Direct3d");

  m_DsVideoCallback.Aquire();

  //get d3d device
  m_D3dDev = g_Windowing.Get3DDevice();

  //we need a high priority thread to get accurate timing
  if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
    CLog::Log(LOGDEBUG, "CDSVideoClock: SetThreadPriority failed");

  D3DCAPS9 DevCaps;
  ReturnV = m_D3dDev->GetDeviceCaps(&DevCaps);
  if (ReturnV != D3D_OK)
  {
    CLog::Log(LOGDEBUG, "CDSVideoClock: GetDeviceCaps returned %s: %s",
                         DXGetErrorString(ReturnV), DXGetErrorDescription(ReturnV));
    return false;
  }

  if (DevCaps.Caps != D3DCAPS_READ_SCANLINE)
  {
    CLog::Log(LOGDEBUG, "CDSVideoClock: Hardware does not support GetRasterStatus");
    return false;
  }

  D3DRASTER_STATUS RasterStatus;
  ReturnV = m_D3dDev->GetRasterStatus(0, &RasterStatus);
  if (ReturnV != D3D_OK)
  {
    CLog::Log(LOGDEBUG, "CDSVideoClock: GetRasterStatus returned returned %s: %s",
              DXGetErrorString(ReturnV), DXGetErrorDescription(ReturnV));
    return false;
  }

  D3DDISPLAYMODE DisplayMode;
  ReturnV = m_D3dDev->GetDisplayMode(0, &DisplayMode);
  if (ReturnV != D3D_OK)
  {
    CLog::Log(LOGDEBUG, "CDSVideoClock: GetDisplayMode returned returned %s: %s",
              DXGetErrorString(ReturnV), DXGetErrorDescription(ReturnV));
    return false;
  }

  //forced update of windows refreshrate
  UpdateRefreshrate(true);

  //measure the refreshrate a couple times
  list<double> Measures;
  for (int i = 0; i < NRMEASURES; i++)
    Measures.push_back(MeasureRefreshrate(MEASURETIME));

  //build up a string of measured rates
  CStdString StrRates;
  for (list<double>::iterator it = Measures.begin(); it != Measures.end(); it++)
    StrRates.AppendFormat("%.2f ", *it);

  //get the top half of the measured rates
  Measures.sort();
  double RefreshRate = 0.0;
  int    NrMeasurements = 0;
  while (NrMeasurements < NRMEASURES / 2 && !Measures.empty())
  {
    if (Measures.back() > 0.0)
    {
      RefreshRate += Measures.back();
      NrMeasurements++;
    }
    Measures.pop_back();
  }

  if (NrMeasurements < NRMEASURES / 2)
  {
    CLog::Log(LOGDEBUG, "CDSVideoClock: refreshrate measurements: %s, unable to get a good measurement",
      StrRates.c_str(), m_RefreshRate);
    return false;
  }

  RefreshRate /= NrMeasurements;
  m_RefreshRate = MathUtils::round_int(RefreshRate);

  CLog::Log(LOGDEBUG, "CDSVideoClock: refreshrate measurements: %s, assuming %i hertz", StrRates.c_str(), m_RefreshRate);

  m_MissedVblanks = 0;

  return true;
}

double CDSVideoClock::MeasureRefreshrate(int MSecs)
{
  D3DRASTER_STATUS RasterStatus;
  int64_t          Now;
  int64_t          Target;
  int64_t          Prev;
  int64_t          AvgInterval;
  int64_t          MeasureCount;
  unsigned int     LastLine;
  int              ReturnV;

  Now = CurrentHostCounter();
  Target = Now + (m_SystemFrequency * MSecs / 1000);
  Prev = -1;
  AvgInterval = 0;
  MeasureCount = 0;

  //start measuring vblanks
  LastLine = 0;
  while(Now <= Target)
  {
    ReturnV = m_D3dDev->GetRasterStatus(0, &RasterStatus);
    Now = CurrentHostCounter();
    if (ReturnV != D3D_OK)
    {
      CLog::Log(LOGDEBUG, "CDSVideoClock: GetRasterStatus returned returned %s: %s",
                DXGetErrorString(ReturnV), DXGetErrorDescription(ReturnV));
      return -1.0;
    }

    if ((RasterStatus.InVBlank && LastLine != 0) || (!RasterStatus.InVBlank && RasterStatus.ScanLine < LastLine))
    { //we got a vblank
      if (Prev != -1) //need two for a measurement
      {
        AvgInterval += Now - Prev; //save how long this vblank lasted
        MeasureCount++;
      }
      Prev = Now; //save this time for the next measurement
    }

    //save the current scanline
    if (RasterStatus.InVBlank)
      LastLine = 0;
    else
      LastLine = RasterStatus.ScanLine;

    ::Sleep(1);
  }

  if (MeasureCount < 1)
  {
    CLog::Log(LOGDEBUG, "CDSVideoClock: Didn't measure any vblanks");
    return -1.0;
  }

  double fRefreshRate = 1.0 / ((double)AvgInterval / (double)MeasureCount / (double)m_SystemFrequency);

  return fRefreshRate;
}

void CDSVideoClock::CleanupD3D()
{
  CLog::Log(LOGDEBUG, "CDSVideoClock: Cleaning up Direct3d");
  m_DsVideoCallback.Release();
}

//this is called from the vblank run function and from CDSVideoClock::Wait in case of a late update
void CDSVideoClock::UpdateClock(int NrVBlanks, bool CheckMissed)
{
  if (CheckMissed) //set to true from the vblank run function, set to false from Wait and GetTime
  {
    if (NrVBlanks < m_MissedVblanks) //if this is true the vblank detection in the run function is wrong
      CLog::Log(LOGDEBUG, "CDSVideoClock: detected %i vblanks, missed %i", NrVBlanks, m_MissedVblanks);
    
    NrVBlanks -= m_MissedVblanks; //subtract the vblanks we missed
    m_MissedVblanks = 0;
  }
  else
  {
    m_MissedVblanks += NrVBlanks;      //tell the vblank clock how many vblanks it missed
    m_TotalMissedVblanks += NrVBlanks; //for the codec information screen
    m_VblankTime += m_SystemFrequency * (int64_t)NrVBlanks / m_RefreshRate; //set the vblank time forward
  }

  if (NrVBlanks > 0) //update the clock with the adjusted frequency if we have any vblanks
    m_CurrTime += (int64_t)NrVBlanks * m_AdjustedFrequency / m_RefreshRate;
}

//called from dvdclock to get the time
int64_t CDSVideoClock::GetTime()
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
    
    return m_CurrTime;
  }
  else
  {
    int64_t ClockOffset = m_ClockOffset; //get offset of clock
    SingleLock.Leave();
    
    return CurrentHostCounter() + ClockOffset;
  }
}

//called from dvdclock to get the clock frequency
int64_t CDSVideoClock::GetFrequency()
{
  return m_SystemFrequency;
}

void CDSVideoClock::SetSpeed(double Speed)
{
  CSingleLock SingleLock(m_CritSection);
  //dvdplayer can change the speed to fit the rereshrate
  if (m_UseVblank)
  {
    int64_t Frequency = (int64_t)((double)m_SystemFrequency * Speed);
    if (Frequency != m_AdjustedFrequency)
    {
      m_AdjustedFrequency = Frequency;
      CLog::Log(LOGDEBUG, "CDSVideoClock: Clock speed %f%%", GetSpeed() * 100);
    }
  }
}

double CDSVideoClock::GetSpeed()
{
  double Speed = 1.0;
  CSingleLock SingleLock(m_CritSection);

  //dvdplayer needs to know the speed for the resampler
  if (m_UseVblank) Speed = (double)m_AdjustedFrequency / (double)m_SystemFrequency;

  return Speed;
}

bool CDSVideoClock::UpdateRefreshrate(bool Forced /*= false*/)
{
  //update the refreshrate about once a second, or update immediately if a forced update is required
  if (m_CurrTime - m_LastRefreshTime < m_SystemFrequency && !Forced)
    return false;

  if (Forced)
    m_LastRefreshTime = 0;
  else
    m_LastRefreshTime = m_CurrTime;

  D3DDISPLAYMODE DisplayMode;
  m_D3dDev->GetDisplayMode(0, &DisplayMode);

  //0 indicates adapter default
  if (DisplayMode.RefreshRate == 0)
    DisplayMode.RefreshRate = 60;

  if (m_PrevRefreshRate != DisplayMode.RefreshRate  || m_Width != DisplayMode.Width || m_Height != DisplayMode.Height || Forced)
  {
    m_PrevRefreshRate = DisplayMode.RefreshRate;
    m_Width = DisplayMode.Width;
    m_Height = DisplayMode.Height;
    return true;
  }
   
  return false;
}

//dvdplayer needs to know the refreshrate for matching the fps of the video playing to it
int CDSVideoClock::GetRefreshRate()
{
  CSingleLock SingleLock(m_CritSection);

  if (m_UseVblank)
    return (int)m_RefreshRate;
  else
    return -1;
}


//this is called from CDVDClock::WaitAbsoluteClock, which is called from CXBMCRenderManager::WaitPresentTime
//it waits until a certain timestamp has passed, used for displaying videoframes at the correct moment
int64_t CDSVideoClock::Wait(int64_t Target)
{
  int64_t       Now;
  int           SleepTime;
  int64_t       NextVblank;
  bool          Late;

  CSingleLock SingleLock(m_CritSection);
  
  if (m_UseVblank) //when true the vblank is used as clock source
  {
    while (m_CurrTime < Target)
    {
      //calculate how long to sleep before we should have gotten a signal that a vblank happened
      Now = CurrentHostCounter();
      NextVblank = TimeOfNextVblank();
      SleepTime = (int)((NextVblank - Now) * 1000 / m_SystemFrequency);

      int64_t CurrTime = m_CurrTime; //save current value of the clock
      
      Late = false;
      if (SleepTime <= 0) //if sleeptime is 0 or lower, the vblank clock is already late in updating
      {
        Late = true;
      }
      else
      {
        m_VblankEvent.Reset();
        SingleLock.Leave();
        if (!m_VblankEvent.WaitMSec(SleepTime)) //if this returns false, it means the vblank event was not set within
          Late = true;                          //the required time
        SingleLock.Enter();
      }
      
      //if the vblank clock was late with its update, we update the clock ourselves
      if (Late && CurrTime == m_CurrTime)
      {
        CLog::Log(LOGDEBUG, "CDSVideoClock: vblank clock was late: SleepTime %i", SleepTime);
        UpdateClock(1, false); //update the clock by 1 vblank
      }
    }
    return m_CurrTime;
  }
  else
  {
    int64_t ClockOffset = m_ClockOffset;
    SingleLock.Leave();
    Now = CurrentHostCounter();
    //sleep until the timestamp has passed
    SleepTime = (int)((Target - (Now + ClockOffset)) * 1000 / m_SystemFrequency);
    if (SleepTime > 0)
      ::Sleep(SleepTime);
    
    Now = CurrentHostCounter();
    return Now + ClockOffset;
  }
}


void CDSVideoClock::SendVblankSignal()
{
  m_VblankEvent.Set();
}

#define MAXVBLANKDELAY 1200
//guess when the next vblank should happen,
//based on the refreshrate and when the previous one happened
//increase that by 20% to allow for errors
int64_t CDSVideoClock::TimeOfNextVblank()
{
  return m_VblankTime + (m_SystemFrequency / m_RefreshRate * MAXVBLANKDELAY / 1000);
}

//for the codec information screen
bool CDSVideoClock::GetClockInfo(int& MissedVblanks, double& ClockSpeed, int& RefreshRate)
{
  if (m_UseVblank)
  {
    MissedVblanks = m_TotalMissedVblanks;
    ClockSpeed = (double)m_AdjustedFrequency / (double)m_SystemFrequency * 100.0;
    RefreshRate = (int)m_RefreshRate;
    return true;
  }
  return false;
}