/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoSyncTVos.h"

#include "cores/VideoPlayer/VideoReferenceClock.h"
#include "utils/MathUtils.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#import "windowing/tvos/WinSystemTVOS.h"

bool CVideoSyncTVos::Setup()
{
  CLog::Log(LOGDEBUG, "CVideoSyncTVos::{} setting up TVOS", __FUNCTION__);

  //init the vblank timestamp
  m_LastVBlankTime = CurrentHostCounter();
  m_abortEvent.Reset();

  bool setupOk = InitDisplayLink();
  if (setupOk)
  {
    m_winSystem.Register(this);
  }

  return setupOk;
}

void CVideoSyncTVos::Run(CEvent& stopEvent)
{
  //because cocoa has a vblank callback, we just keep sleeping until we're asked to stop the thread
  XbmcThreads::CEventGroup waitGroup{&stopEvent, &m_abortEvent};
  waitGroup.wait();
}

void CVideoSyncTVos::Cleanup()
{
  CLog::Log(LOGDEBUG, "CVideoSyncTVos::{} cleaning up TVOS", __FUNCTION__);
  DeinitDisplayLink();
  m_winSystem.Unregister(this);
}

float CVideoSyncTVos::GetFps()
{
  m_fps = CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS();
  CLog::Log(LOGDEBUG, "CVideoSyncTVos::{} Detected refreshrate: {} hertz", __FUNCTION__, m_fps);
  return m_fps;
}

void CVideoSyncTVos::OnResetDisplay()
{
  m_abortEvent.Set();
}

void CVideoSyncTVos::TVosVblankHandler()
{
  int64_t nowtime = CurrentHostCounter();

  //calculate how many vblanks happened
  double VBlankTime =
      static_cast<double>(nowtime - m_LastVBlankTime) / static_cast<double>(CurrentHostFrequency());
  int NrVBlanks = MathUtils::round_int(VBlankTime * static_cast<double>(m_fps));

  //save the timestamp of this vblank so we can calculate how many happened next time
  m_LastVBlankTime = nowtime;

  //update the vblank timestamp, update the clock and send a signal that we got a vblank
  m_refClock->UpdateClock(NrVBlanks, nowtime);
}

bool CVideoSyncTVos::InitDisplayLink()
{
  bool ret = true;
  CLog::Log(LOGDEBUG, "CVideoSyncTVos: setting up displaylink");
  if (!m_winSystem.InitDisplayLink(this))
  {
    CLog::Log(LOGDEBUG, "CVideoSyncTVos: InitDisplayLink failed");
    ret = false;
  }
  return ret;
}

void CVideoSyncTVos::DeinitDisplayLink()
{
  m_winSystem.DeinitDisplayLink();
}
