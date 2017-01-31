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

#if defined(TARGET_ANDROID)
#include "utils/log.h"
#include "VideoSyncAndroid.h"
#include "video/VideoReferenceClock.h"
#include "utils/TimeUtils.h"
#include "platform/android/activity/XBMCApp.h"
#include "windowing/WindowingFactory.h"
#include "guilib/GraphicContext.h"
#include "utils/MathUtils.h"
#include "linux/XTimeUtils.h"


bool CVideoSyncAndroid::Setup(PUPDATECLOCK func)
{
  CLog::Log(LOGDEBUG, "CVideoSyncAndroid::%s setting up", __FUNCTION__);
  
  //init the vblank timestamp
  m_LastVBlankTime = CurrentHostCounter();
  UpdateClock = func;
  m_abort = false;
  
  CXBMCApp::InitFrameCallback(this);
  g_Windowing.Register(this);

  return true;
}

void CVideoSyncAndroid::Run(std::atomic<bool>& stop)
{
  while(!stop && !m_abort)
  {
    Sleep(100);
  }
}

void CVideoSyncAndroid::Cleanup()
{
  CLog::Log(LOGDEBUG, "CVideoSyncAndroid::%s cleaning up", __FUNCTION__);
  CXBMCApp::DeinitFrameCallback();
  g_Windowing.Unregister(this);
}

float CVideoSyncAndroid::GetFps()
{
  m_fps = g_graphicsContext.GetFPS();
  CLog::Log(LOGDEBUG, "CVideoSyncAndroid::%s Detected refreshrate: %f hertz", __FUNCTION__, m_fps);
  return m_fps;
}

void CVideoSyncAndroid::OnResetDisplay()
{
  m_abort = true;
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

#endif //TARGET_ANDROID
