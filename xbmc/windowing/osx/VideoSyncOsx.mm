/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoSyncOsx.h"

#include "ServiceBroker.h"
#include "cores/VideoPlayer/VideoReferenceClock.h"
#include "utils/MathUtils.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include "platform/darwin/osx/CocoaInterface.h"

#include <CoreVideo/CVHostTime.h>
#include <QuartzCore/CVDisplayLink.h>
#include <unistd.h>

using namespace std::chrono_literals;

bool CVideoSyncOsx::Setup()
{
  CLog::Log(LOGDEBUG, "CVideoSyncOsx::{} setting up OSX", __FUNCTION__);

  //init the vblank timestamp
  m_LastVBlankTime = 0;
  m_displayLost = false;
  m_displayReset = false;
  m_lostEvent.Reset();

  CServiceBroker::GetWinSystem()->Register(this);

  return true;
}

void CVideoSyncOsx::Run(CEvent& stopEvent)
{
  InitDisplayLink();

  //because cocoa has a vblank callback, we just keep sleeping until we're asked to stop the thread
  while(!stopEvent.Signaled() && !m_displayLost && !m_displayReset)
  {
    usleep(100000);
  }

  m_lostEvent.Set();

  while(!stopEvent.Signaled() && m_displayLost && !m_displayReset)
  {
    usleep(10000);
  }

  DeinitDisplayLink();
}

void CVideoSyncOsx::Cleanup()
{
  CLog::Log(LOGDEBUG, "CVideoSyncOsx::{} cleaning up OSX", __FUNCTION__);
  m_lostEvent.Set();
  m_LastVBlankTime = 0;
  CServiceBroker::GetWinSystem()->Unregister(this);
}

float CVideoSyncOsx::GetFps()
{
  m_fps = CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS();
  CLog::Log(LOGDEBUG, "CVideoSyncOsx::{} Detected refreshrate: {:f} hertz", __FUNCTION__, m_fps);
  return m_fps;
}

void CVideoSyncOsx::RefreshChanged()
{
  m_displayReset = true;
}

void CVideoSyncOsx::OnLostDisplay()
{
  if (!m_displayLost)
  {
    m_displayLost = true;
    m_lostEvent.Wait(1000ms);
  }
}

void CVideoSyncOsx::OnResetDisplay()
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
    NrVBlanks = MathUtils::round_int(VBlankTime * static_cast<double>(m_fps));

    //update the vblank timestamp, update the clock and send a signal that we got a vblank
    m_refClock->UpdateClock(NrVBlanks, Now);
  }

  //save the timestamp of this vblank so we can calculate how many happened next time
  m_LastVBlankTime = nowtime;
}

// Called by the Core Video Display Link whenever it's appropriate to render a frame.
static CVReturn DisplayLinkCallBack(CVDisplayLinkRef displayLink, const CVTimeStamp* inNow, const CVTimeStamp* inOutputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
  @autoreleasepool
  {
    CVideoSyncOsx* VideoSyncOsx = reinterpret_cast<CVideoSyncOsx*>(displayLinkContext);

    if (inOutputTime->flags & kCVTimeStampHostTimeValid)
      VideoSyncOsx->VblankHandler(inOutputTime->hostTime, CVGetHostClockFrequency());
    else
      VideoSyncOsx->VblankHandler(CVGetCurrentHostTime(), CVGetHostClockFrequency());
  }

  return kCVReturnSuccess;
}

bool CVideoSyncOsx::InitDisplayLink()
{
  bool ret = true;
  CLog::Log(LOGDEBUG, "CVideoSyncOsx::{} setting up displaylink", __FUNCTION__);

  if (!Cocoa_CVDisplayLinkCreate((void*)DisplayLinkCallBack, reinterpret_cast<void*>(this)))
  {
    CLog::Log(LOGDEBUG, "CVideoSyncOsx::{} Cocoa_CVDisplayLinkCreate failed", __FUNCTION__);
    ret = false;
  }
  return ret;
}

void CVideoSyncOsx::DeinitDisplayLink()
{
  Cocoa_CVDisplayLinkRelease();
}

