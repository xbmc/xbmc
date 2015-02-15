/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#if defined(TARGET_DARWIN)
#include "utils/log.h"
#include "VideoSyncCocoa.h"
#include "utils/MathUtils.h"
#include "video/VideoReferenceClock.h"
#include "utils/TimeUtils.h"

#include "windowing/WindowingFactory.h"
#include "guilib/GraphicContext.h"

//osx specifics
#if defined(TARGET_DARWIN_OSX)
#include <QuartzCore/CVDisplayLink.h>
#include "osx/CocoaInterface.h"
// Called by the Core Video Display Link whenever it's appropriate to render a frame.
static CVReturn DisplayLinkCallBack(CVDisplayLinkRef displayLink, const CVTimeStamp* inNow, const CVTimeStamp* inOutputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
  double fps = 60.0;
  
  if (inNow->videoRefreshPeriod > 0)
    fps = (double)inOutputTime->videoTimeScale / (double)inOutputTime->videoRefreshPeriod;
  
  // Create an autorelease pool (necessary to call into non-Obj-C code from Obj-C code)
  void* pool = Cocoa_Create_AutoReleasePool();
  
  CVideoSyncCocoa *VideoSyncCocoa = reinterpret_cast<CVideoSyncCocoa*>(displayLinkContext);
  VideoSyncCocoa->VblankHandler(inOutputTime->hostTime, fps);
  
  // Destroy the autorelease pool
  Cocoa_Destroy_AutoReleasePool(pool);
  
  return kCVReturnSuccess;
}
#endif

void CVideoSyncCocoa::VblankHandler(int64_t nowtime, double fps)
{
  int           NrVBlanks;
  double        VBlankTime;
  
  //calculate how many vblanks happened
  VBlankTime = (double)(nowtime - m_LastVBlankTime) / (double)g_VideoReferenceClock.GetFrequency();
  NrVBlanks = MathUtils::round_int(VBlankTime * m_fps);
  
  //save the timestamp of this vblank so we can calculate how many happened next time
  m_LastVBlankTime = nowtime;
  
  //update the vblank timestamp, update the clock and send a signal that we got a vblank
  UpdateClock(NrVBlanks, nowtime);
}

bool CVideoSyncCocoa::Setup(PUPDATECLOCK func)
{
  CLog::Log(LOGDEBUG, "CVideoSyncCocoa::%s - setting up Cocoa", __FUNCTION__);
  bool setupOk = false;

  //init the vblank timestamp
  m_LastVBlankTime = CurrentHostCounter();
  UpdateClock = func;
  m_abort = false;
  
#if defined(TARGET_DARWIN_IOS)
  g_Windowing.InitDisplayLink(this);
  setupOk = true;
#else
  setupOk = Cocoa_CVDisplayLinkCreate((void*)DisplayLinkCallBack, reinterpret_cast<void*>(this));
  if (setupOk)
    g_Windowing.Register(this);
  else
    CLog::Log(LOGDEBUG, "CVideoSyncCocoa::%s Cocoa_CVDisplayLinkCreate failed", __FUNCTION__);
#endif

  if (setupOk)
    GetFps();

  return setupOk;
}

void CVideoSyncCocoa::Run(volatile bool& stop)
{
  //because cocoa has a vblank callback, we just keep sleeping until we're asked to stop the thread
  while(!stop && !m_abort)
  {
    Sleep(1000);
  }
}

void CVideoSyncCocoa::Cleanup()
{
  CLog::Log(LOGDEBUG, "CVideoSyncCocoa::%s cleaning up Cocoa", __FUNCTION__);
#if defined(TARGET_DARWIN_IOS)
  g_Windowing.DeinitDisplayLink();
#else
  Cocoa_CVDisplayLinkRelease();
  g_Windowing.Unregister(this);
#endif
}

void CVideoSyncCocoa::OnResetDevice()
{
  m_abort = true;
}

float CVideoSyncCocoa::GetFps()
{
#if defined(TARGET_DARWIN_IOS)
  m_fps = g_Windowing.GetDisplayLinkFPS();
#else
  m_fps = g_graphicsContext.GetFPS();
#endif
  CLog::Log(LOGDEBUG, "CVideoSyncCocoa::%s Detected refreshrate: %f hertz", __FUNCTION__, m_fps);
  return m_fps;
}

#endif//TARGET_DARWIN
