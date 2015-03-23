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

#include "system.h"

#if defined(TARGET_DARWIN_IOS)
#include "utils/log.h"
#include "VideoSyncIos.h"
#include "utils/MathUtils.h"
#include "video/VideoReferenceClock.h"
#include "guilib/GraphicContext.h"
#include "windowing/WindowingFactory.h"
#include "utils/TimeUtils.h"

bool CVideoSyncIos::Setup(PUPDATECLOCK func)
{
  CLog::Log(LOGDEBUG, "CVideoSyncIos::%s setting up OSX", __FUNCTION__);
  
  //init the vblank timestamp
  m_LastVBlankTime = CurrentHostCounter();
  UpdateClock = func;
  m_abort = false;
  
  bool setupOk = InitDisplayLink();
  if (setupOk)
  {
    g_Windowing.Register(this);
  }
  
  return setupOk;
}

void CVideoSyncIos::Run(volatile bool& stop)
{
  //because cocoa has a vblank callback, we just keep sleeping until we're asked to stop the thread
  while(!stop && !m_abort)
  {
    Sleep(100);
  }
}

void CVideoSyncIos::Cleanup()
{
  CLog::Log(LOGDEBUG, "CVideoSyncIos::%s cleaning up OSX", __FUNCTION__);
  DeinitDisplayLink();
  g_Windowing.Unregister(this);
}

float CVideoSyncIos::GetFps()
{
  m_fps = g_graphicsContext.GetFPS();
  CLog::Log(LOGDEBUG, "CVideoSyncIos::%s Detected refreshrate: %f hertz", __FUNCTION__, m_fps);
  return m_fps;
}

void CVideoSyncIos::OnResetDevice()
{
  m_abort = true;
}

void CVideoSyncIos::IosVblankHandler()
{
  int           NrVBlanks;
  double        VBlankTime;
  int64_t       nowtime = CurrentHostCounter();
  
  //calculate how many vblanks happened
  VBlankTime = (double)(nowtime - m_LastVBlankTime) / (double)g_VideoReferenceClock.GetFrequency();
  NrVBlanks = MathUtils::round_int(VBlankTime * m_fps);
  
  //save the timestamp of this vblank so we can calculate how many happened next time
  m_LastVBlankTime = nowtime;
  
  //update the vblank timestamp, update the clock and send a signal that we got a vblank
  UpdateClock(NrVBlanks, nowtime);
}

bool CVideoSyncIos::InitDisplayLink()
{
  bool ret = true;
  CLog::Log(LOGDEBUG, "CVideoSyncIos: setting up displaylink");
  if (!g_Windowing.InitDisplayLink(this))
  {
    CLog::Log(LOGDEBUG, "CVideoSyncIos: InitDisplayLink failed");
    ret = false;
  }
  return ret;
}

void CVideoSyncIos::DeinitDisplayLink()
{
  g_Windowing.DeinitDisplayLink();
}

#endif//TARGET_DARWIN_IOS
