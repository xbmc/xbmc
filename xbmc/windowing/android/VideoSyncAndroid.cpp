/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoSyncAndroid.h"

#include "ServiceBroker.h"
#include "cores/VideoPlayer/VideoReferenceClock.h"
#include "utils/MathUtils.h"
#include "utils/TimeUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include "platform/android/activity/XBMCApp.h"

bool CVideoSyncAndroid::Setup()
{
  CLog::Log(LOGDEBUG, "CVideoSyncAndroid::{} setting up", __FUNCTION__);

  //init the vblank timestamp
  m_LastVBlankTime = CurrentHostCounter();
  m_abortEvent.Reset();

  CXBMCApp::Get().InitFrameCallback(this);
  CServiceBroker::GetWinSystem()->Register(this);

  return true;
}

void CVideoSyncAndroid::Run(CEvent& stopEvent)
{
  XbmcThreads::CEventGroup waitGroup{&stopEvent, &m_abortEvent};
  waitGroup.wait();
}

void CVideoSyncAndroid::Cleanup()
{
  CLog::Log(LOGDEBUG, "CVideoSyncAndroid::{} cleaning up", __FUNCTION__);
  CXBMCApp::Get().DeinitFrameCallback();
  CServiceBroker::GetWinSystem()->Unregister(this);
}

float CVideoSyncAndroid::GetFps()
{
  m_fps = CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS();
  CLog::Log(LOGDEBUG, "CVideoSyncAndroid::{} Detected refreshrate: {:f} hertz", __FUNCTION__,
            m_fps);
  return m_fps;
}

void CVideoSyncAndroid::OnResetDisplay()
{
  m_abortEvent.Set();
}

void CVideoSyncAndroid::FrameCallback(int64_t frameTimeNanos)
{
  int           NrVBlanks;
  double        VBlankTime;

  //calculate how many vblanks happened
  VBlankTime = (double)(frameTimeNanos - m_LastVBlankTime) / (double)CurrentHostFrequency();
  NrVBlanks = MathUtils::round_int(VBlankTime * static_cast<double>(m_fps));

  //save the timestamp of this vblank so we can calculate how many happened next time
  m_LastVBlankTime = frameTimeNanos;

  //update the vblank timestamp, update the clock and send a signal that we got a vblank
  m_refClock->UpdateClock(NrVBlanks, frameTimeNanos);
}
