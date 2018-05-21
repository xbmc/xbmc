/*
 *      Copyright (C) 2015-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "utils/log.h"
#include "ServiceBroker.h"
#include "VideoSyncAndroid.h"
#include "cores/VideoPlayer/VideoReferenceClock.h"
#include "utils/TimeUtils.h"
#include "platform/android/activity/XBMCApp.h"
#include "windowing/WinSystem.h"
#include "windowing/GraphicContext.h"
#include "utils/MathUtils.h"
#include "platform/linux/XTimeUtils.h"


bool CVideoSyncAndroid::Setup(PUPDATECLOCK func)
{
  CLog::Log(LOGDEBUG, "CVideoSyncAndroid::%s setting up", __FUNCTION__);

  //init the vblank timestamp
  m_LastVBlankTime = CurrentHostCounter();
  UpdateClock = func;
  m_abortEvent.Reset();

  CXBMCApp::InitFrameCallback(this);
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
  CLog::Log(LOGDEBUG, "CVideoSyncAndroid::%s cleaning up", __FUNCTION__);
  CXBMCApp::DeinitFrameCallback();
  CServiceBroker::GetWinSystem()->Unregister(this);
}

float CVideoSyncAndroid::GetFps()
{
  m_fps = CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS();
  CLog::Log(LOGDEBUG, "CVideoSyncAndroid::%s Detected refreshrate: %f hertz", __FUNCTION__, m_fps);
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
  int64_t       nowtime = CurrentHostCounter();

  //calculate how many vblanks happened
  VBlankTime = (double)(nowtime - m_LastVBlankTime) / (double)CurrentHostFrequency();
  NrVBlanks = MathUtils::round_int(VBlankTime * m_fps);

  //save the timestamp of this vblank so we can calculate how many happened next time
  m_LastVBlankTime = nowtime;

  //update the vblank timestamp, update the clock and send a signal that we got a vblank
  UpdateClock(NrVBlanks, nowtime, m_refClock);
}
