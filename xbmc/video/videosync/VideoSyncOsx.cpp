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
#include "osx/CocoaInterface.h"

bool CVideoSyncOsx::Setup(PUPDATECLOCK func)
{
  CLog::Log(LOGDEBUG, "CVideoSyncOsx::%s setting up OSX", __FUNCTION__);
  bool setupOk = false;
  
  //init the vblank timestamp
  m_LastVBlankTime = CurrentHostCounter();
  UpdateClock = func;
  m_abort = false;
  
  setupOk = InitDisplayLink();
  if (setupOk)
  {
    g_Windowing.Register(this);
  }
  
  return setupOk;
}

void CVideoSyncOsx::Run(volatile bool& stop)
{
  //because cocoa has a vblank callback, we just keep sleeping until we're asked to stop the thread
  while(!stop && !m_abort)
  {
    Sleep(100);
  }
}

void CVideoSyncOsx::Cleanup()
{
  CLog::Log(LOGDEBUG, "CVideoSyncOsx::%s cleaning up OSX", __FUNCTION__);
  DeinitDisplayLink();
  g_Windowing.Unregister(this);
}

float CVideoSyncOsx::GetFps()
{
  m_fps = g_graphicsContext.GetFPS();
  CLog::Log(LOGDEBUG, "CVideoSyncOsx::%s Detected refreshrate: %f hertz", __FUNCTION__, m_fps);
  return m_fps;
}

void CVideoSyncOsx::OnResetDevice()
{
  m_abort = true;
}

void CVideoSyncOsx::VblankHandler(int64_t nowtime)
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

// Called by the Core Video Display Link whenever it's appropriate to render a frame.
static CVReturn DisplayLinkCallBack(CVDisplayLinkRef displayLink, const CVTimeStamp* inNow, const CVTimeStamp* inOutputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
  // Create an autorelease pool (necessary to call into non-Obj-C code from Obj-C code)
  void* pool = Cocoa_Create_AutoReleasePool();
  
  CVideoSyncOsx *VideoSyncOsx = reinterpret_cast<CVideoSyncOsx*>(displayLinkContext);

  VideoSyncOsx->VblankHandler(inOutputTime->hostTime);
  
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
