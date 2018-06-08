/*
 *      Copyright (C) 2015 Team Kodi
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

#include "utils/log.h"
#include "VideoSyncIos.h"
#include "utils/MathUtils.h"
#include "cores/VideoPlayer/VideoReferenceClock.h"
#include "windowing/GraphicContext.h"
#include "windowing/osx/WinSystemIOS.h"
#include "utils/TimeUtils.h"

bool CVideoSyncIos::Setup(PUPDATECLOCK func)
{
  CLog::Log(LOGDEBUG, "CVideoSyncIos::%s setting up OSX", __FUNCTION__);

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

void CVideoSyncIos::Run(CEvent& stopEvent)
{
  //because cocoa has a vblank callback, we just keep sleeping until we're asked to stop the thread
  XbmcThreads::CEventGroup waitGroup{&stopEvent, &m_abortEvent};
  waitGroup.wait();
}

void CVideoSyncIos::Cleanup()
{
  CLog::Log(LOGDEBUG, "CVideoSyncIos::%s cleaning up OSX", __FUNCTION__);
  DeinitDisplayLink();
  m_winSystem.Unregister(this);
}

float CVideoSyncIos::GetFps()
{
  m_fps = CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS();
  CLog::Log(LOGDEBUG, "CVideoSyncIos::%s Detected refreshrate: %f hertz", __FUNCTION__, m_fps);
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
  NrVBlanks = MathUtils::round_int(VBlankTime * m_fps);

  //save the timestamp of this vblank so we can calculate how many happened next time
  m_LastVBlankTime = nowtime;

  //update the vblank timestamp, update the clock and send a signal that we got a vblank
  UpdateClock(NrVBlanks, nowtime, m_refClock);
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

