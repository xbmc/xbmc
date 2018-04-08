/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#include "VideoSyncPi.h"
#include "ServiceBroker.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "platform/linux/RBP.h"
#include "threads/Thread.h"

bool CVideoSyncPi::Setup(PUPDATECLOCK func)
{
  UpdateClock = func;
  m_abort = false;
  CServiceBroker::GetWinSystem()->Register(this);
  CLog::Log(LOGDEBUG, "CVideoReferenceClock: setting up RPi");
  return true;
}

void CVideoSyncPi::Run(CEvent& stopEvent)
{
  /* This shouldn't be very busy and timing is important so increase priority */
  CThread::GetCurrentThread()->SetPriority(CThread::GetCurrentThread()->GetPriority()+1);

  while (!stopEvent.Signaled() && !m_abort)
  {
    g_RBP.WaitVsync();
    uint64_t now = CurrentHostCounter();
    UpdateClock(1, now, m_refClock);
  }
}

void CVideoSyncPi::Cleanup()
{
  CLog::Log(LOGDEBUG, "CVideoReferenceClock: cleaning up RPi");
  CServiceBroker::GetWinSystem()->Unregister(this);
}

float CVideoSyncPi::GetFps()
{
  m_fps = CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS();
  CLog::Log(LOGDEBUG, "CVideoReferenceClock: fps: %.2f", m_fps);
  return m_fps;
}

void CVideoSyncPi::OnResetDisplay()
{
  m_abort = true;
}

void CVideoSyncPi::RefreshChanged()
{
  if (m_fps != CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS())
    m_abort = true;
}
