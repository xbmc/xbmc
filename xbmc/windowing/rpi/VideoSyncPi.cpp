/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoSyncPi.h"

#include "ServiceBroker.h"
#include "threads/Thread.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include "platform/linux/RBP.h"

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
  CThread* thread = CThread::GetCurrentThread();
  if (thread != nullptr)
  {
    /* This shouldn't be very busy and timing is important so increase priority */
    thread->SetPriority(thread->GetPriority() + 1);
  }

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
