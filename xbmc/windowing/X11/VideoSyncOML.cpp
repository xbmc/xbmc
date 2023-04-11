/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoSyncOML.h"

#include "cores/VideoPlayer/VideoReferenceClock.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/X11/WinSystemX11GLContext.h"

#include <unistd.h>

using namespace KODI::WINDOWING::X11;

bool CVideoSyncOML::Setup()
{
  CLog::Log(LOGDEBUG, "CVideoSyncOML::{} - setting up OML", __FUNCTION__);

  m_abort = false;

  static_cast<CWinSystemX11*>(&m_winSystem)->Register(this);

  return true;
}

void CVideoSyncOML::Run(CEvent& stopEvent)
{
  uint64_t interval, timeSinceVblank, msc;

  timeSinceVblank = m_winSystem.GetVblankTiming(msc, interval);

  while (!stopEvent.Signaled() && !m_abort)
  {
    if (interval == 0)
    {
      usleep(10000);
    }
    else
    {
      usleep(interval - timeSinceVblank + 1000);
    }
    uint64_t newMsc;
    timeSinceVblank = m_winSystem.GetVblankTiming(newMsc, interval);

    if (newMsc == msc)
    {
      newMsc++;
    }
    else if (newMsc < msc)
    {
      timeSinceVblank = interval;
      continue;
    }

    uint64_t now = CurrentHostCounter();
    m_refClock->UpdateClock(newMsc - msc, now);
    msc = newMsc;
  }
}

void CVideoSyncOML::Cleanup()
{
  m_winSystem.Unregister(this);
}

void CVideoSyncOML::OnResetDisplay()
{
  m_abort = true;
}

float CVideoSyncOML::GetFps()
{
  m_fps = m_winSystem.GetGfxContext().GetFPS();
  return m_fps;
}

