/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/log.h"
#include "VideoSyncTVos.h"
#include "utils/MathUtils.h"
#include "cores/VideoPlayer/VideoReferenceClock.h"
#include "windowing/GraphicContext.h"
#include "windowing/tvos/WinSystemTVOS.h"
#include "utils/TimeUtils.h"

bool CVideoSyncTVos::Setup(PUPDATECLOCK func)
{
  CLog::Log(LOGDEBUG, "CVideoSyncTVos::%s setting up TVOS", __FUNCTION__);

  //init the vblank timestamp
  m_LastVBlankTime = CurrentHostCounter();
  UpdateClock = func;
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
  CLog::Log(LOGDEBUG, "CVideoSyncTVos::%s cleaning up TVOS", __FUNCTION__);
  DeinitDisplayLink();
  m_winSystem.Unregister(this);
}

float CVideoSyncTVos::GetFps()
{
  m_fps = CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS();
  CLog::Log(LOGDEBUG, "CVideoSyncTVos::%s Detected refreshrate: %f hertz", __FUNCTION__, m_fps);
  return m_fps;
}

void CVideoSyncTVos::OnResetDisplay()
{
  m_abortEvent.Set();
}

void CVideoSyncTVos::TVosVblankHandler()
{
  int NrVBlanks;
  double VBlankTime;
  int64_t nowtime = CurrentHostCounter();

  //calculate how many vblanks happened
  VBlankTime = (double)(nowtime - m_LastVBlankTime) / (double)CurrentHostFrequency();
  NrVBlanks = MathUtils::round_int(VBlankTime * m_fps);

  //save the timestamp of this vblank so we can calculate how many happened next time
  m_LastVBlankTime = nowtime;

  //update the vblank timestamp, update the clock and send a signal that we got a vblank
  UpdateClock(NrVBlanks, nowtime, m_refClock);
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
