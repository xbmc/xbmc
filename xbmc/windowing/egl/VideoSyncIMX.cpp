/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://xbmc.org
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

#if defined(HAS_IMXVPU)

#include "video/videosync/VideoSyncIMX.h"
#include "guilib/GraphicContext.h"
#include "windowing/WindowingFactory.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "linux/imx/IMX.h"

CVideoSyncIMX::CVideoSyncIMX(CVideoReferenceClock *clock) : CVideoSync(clock)
{
  g_IMX.Initialize();
}

CVideoSyncIMX::~CVideoSyncIMX()
{
  g_IMX.Deinitialize();
}

bool CVideoSyncIMX::Setup(PUPDATECLOCK func)
{
  UpdateClock = func;

  m_abort = false;

  g_Windowing.Register(this);
  CLog::Log(LOGDEBUG, "CVideoReferenceClock: setting up IMX");
  return true;
}

void CVideoSyncIMX::Run(std::atomic<bool>& stop)
{
  int counter;

  while (!stop && !m_abort)
  {
    counter = g_IMX.WaitVsync();
    uint64_t now = CurrentHostCounter();

    UpdateClock(counter, now, m_refClock);
  }
}

void CVideoSyncIMX::Cleanup()
{
  CLog::Log(LOGDEBUG, "CVideoReferenceClock: cleaning up IMX");
  g_Windowing.Unregister(this);
}

float CVideoSyncIMX::GetFps()
{
  m_fps = g_graphicsContext.GetFPS();
  CLog::Log(LOGDEBUG, "CVideoReferenceClock: fps: %.3f", m_fps);
  return m_fps;
}

void CVideoSyncIMX::OnResetDisplay()
{
  m_abort = true;
}

#endif
