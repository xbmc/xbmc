/*
 *      Copyright (C) 2005-2014 Team Kodi
 *      http://kodi.tv
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

#if defined(TARGET_DARWIN_OSX)
#include "utils/log.h"
#include "VideoSyncOsx.h"
#include "utils/MathUtils.h"
#include "video/VideoReferenceClock.h"
#include "guilib/GraphicContext.h"
#include "utils/TimeUtils.h"
#include "windowing/WindowingFactory.h"
#include <QuartzCore/CVDisplayLink.h>
#include <CoreVideo/CVHostTime.h>
#include "platform/darwin/osx/CocoaInterface.h"
#include <unistd.h>

bool CVideoSyncOsx::Setup(PUPDATECLOCK func)
{
  CLog::Log(LOGDEBUG, "CVideoSyncOsx::%s setting up OSX", __FUNCTION__);
  
  //init the vblank timestamp
  m_LastVBlankTime = 0;
  UpdateClock = func;
  m_displayLost = false;
  m_displayReset = false;
  m_lostEvent.Reset();

  g_Windowing.Register(this);
  
  return true;
}

void CVideoSyncOsx::Run(std::atomic<bool>& stop)
{
  InitDisplayLink();

  //because cocoa has a vblank callback, we just keep sleeping until we're asked to stop the thread
  while(!stop && !m_displayLost && !m_displayReset)
  {
    usleep(100000);
  }

  m_lostEvent.Set();

  while(!stop && m_displayLost && !m_displayReset)
  {
    usleep(10000);
  }

  DeinitDisplayLink();
}

void CVideoSyncOsx::Cleanup()
{
  CLog::Log(LOGDEBUG, "CVideoSyncOsx::%s cleaning up OSX", __FUNCTION__);
  m_lostEvent.Set();
  m_LastVBlankTime = 0;
  g_Windowing.Unregister(this);
}

float CVideoSyncOsx::GetFps()
{
  m_fps = g_graphicsContext.GetFPS();
  CLog::Log(LOGDEBUG, "CVideoSyncOsx::%s Detected refreshrate: %f hertz", __FUNCTION__, m_fps);
  return m_fps;
}

void CVideoSyncOsx::RefreshChanged()
{
  m_displayReset = true;
}

void CVideoSyncOsx::OnLostDevice()
{
  if (!m_displayLost)
  {
    m_displayLost = true;
    m_lostEvent.WaitMSec(1000);
  }
}

void CVideoSyncOsx::OnResetDevice()
{
  m_displayReset = true;
}

void CVideoSyncOsx::VblankHandler(int64_t nowtime, uint32_t timebase)
{
  int           NrVBlanks;
  double        VBlankTime;
  int64_t       Now = CurrentHostCounter();
  
  if (m_LastVBlankTime != 0)
  {
    VBlankTime = (double)(nowtime - m_LastVBlankTime) / (double)timebase;
    NrVBlanks = MathUtils::round_int(VBlankTime * m_fps);

    //update the vblank timestamp, update the clock and send a signal that we got a vblank
    UpdateClock(NrVBlanks, Now, m_refClock);
  }

  //save the timestamp of this vblank so we can calculate how many happened next time
  m_LastVBlankTime = nowtime;
}

// Called by the Core Video Display Link whenever it's appropriate to render a frame.
static CVReturn DisplayLinkCallBack(CVDisplayLinkRef displayLink, const CVTimeStamp* inNow, const CVTimeStamp* inOutputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
  // Create an autorelease pool (necessary to call into non-Obj-C code from Obj-C code)
  void* pool = Cocoa_Create_AutoReleasePool();
  
  CVideoSyncOsx *VideoSyncOsx = reinterpret_cast<CVideoSyncOsx*>(displayLinkContext);

  if (inOutputTime->flags & kCVTimeStampHostTimeValid)
    VideoSyncOsx->VblankHandler(inOutputTime->hostTime, CVGetHostClockFrequency());
  else
    VideoSyncOsx->VblankHandler(CVGetCurrentHostTime(), CVGetHostClockFrequency());
  
  // Destroy the autorelease pool
  Cocoa_Destroy_AutoReleasePool(pool);
  
  return kCVReturnSuccess;
}

bool CVideoSyncOsx::InitDisplayLink()
{
  bool ret = true;
  CLog::Log(LOGDEBUG, "CVideoSyncOsx::%s setting up displaylink", __FUNCTION__);

  if (!Cocoa_CVDisplayLinkCreate((void*)DisplayLinkCallBack, reinterpret_cast<void*>(this)))
  {
    CLog::Log(LOGDEBUG, "CVideoSyncOsx::%s Cocoa_CVDisplayLinkCreate failed", __FUNCTION__);
    ret = false;
  }
  return ret;
}

void CVideoSyncOsx::DeinitDisplayLink()
{
  Cocoa_CVDisplayLinkRelease();
}

#endif//TARGET_DARWIN_OSX
