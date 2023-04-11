/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoSyncIos.h"

#include "cores/VideoPlayer/VideoReferenceClock.h"
#include "utils/MathUtils.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/ios/WinSystemIOS.h"

bool CVideoSyncIos::Setup()
{
  CLog::Log(LOGDEBUG, "CVideoSyncIos::{} setting up OSX", __FUNCTION__);

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

void CVideoSyncIos::Run(CEvent& stopEvent)
{
  //because cocoa has a vblank callback, we just keep sleeping until we're asked to stop the thread
  XbmcThreads::CEventGroup waitGroup{&stopEvent, &m_abortEvent};
  waitGroup.wait();
}

void CVideoSyncIos::Cleanup()
{
  CLog::Log(LOGDEBUG, "CVideoSyncIos::{} cleaning up OSX", __FUNCTION__);
  DeinitDisplayLink();
  m_winSystem.Unregister(this);
}

float CVideoSyncIos::GetFps()
{
  m_fps = CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS();
  CLog::Log(LOGDEBUG, "CVideoSyncIos::{} Detected refreshrate: {:f} hertz", __FUNCTION__, m_fps);
  return m_fps;
}

void CVideoSyncIos::OnResetDisplay()
{
  m_abortEvent.Set();
}

void CVideoSyncIos::IosVblankHandler()
{
  int           NrVBlanks;
  double        VBlankTime;
  int64_t       nowtime = CurrentHostCounter();

  //calculate how many vblanks happened
  VBlankTime = (double)(nowtime - m_LastVBlankTime) / (double)CurrentHostFrequency();
  NrVBlanks = MathUtils::round_int(VBlankTime * static_cast<double>(m_fps));

  //save the timestamp of this vblank so we can calculate how many happened next time
  m_LastVBlankTime = nowtime;

  //update the vblank timestamp, update the clock and send a signal that we got a vblank
  m_refClock->UpdateClock(NrVBlanks, nowtime);
}

bool CVideoSyncIos::InitDisplayLink()
{
  bool ret = true;
  CLog::Log(LOGDEBUG, "CVideoSyncIos: setting up displaylink");
  if (!m_winSystem.InitDisplayLink(this))
  {
    CLog::Log(LOGDEBUG, "CVideoSyncIos: InitDisplayLink failed");
    ret = false;
  }
  return ret;
}

void CVideoSyncIos::DeinitDisplayLink()
{
  m_winSystem.DeinitDisplayLink();
}

