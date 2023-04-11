/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoSyncWpPresentation.h"

#include "cores/VideoPlayer/VideoReferenceClock.h"
#include "settings/AdvancedSettings.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "windowing/wayland/WinSystemWayland.h"

#include <cinttypes>
#include <functional>

using namespace KODI::WINDOWING::WAYLAND;
using namespace std::placeholders;

CVideoSyncWpPresentation::CVideoSyncWpPresentation(CVideoReferenceClock* clock,
                                                   CWinSystemWayland& winSystem)
  : CVideoSync(clock), m_winSystem(winSystem)
{
}

bool CVideoSyncWpPresentation::Setup()
{
  m_stopEvent.Reset();
  m_fps = m_winSystem.GetSyncOutputRefreshRate();

  return true;
}

void CVideoSyncWpPresentation::Run(CEvent& stopEvent)
{
  m_presentationHandler = m_winSystem.RegisterOnPresentationFeedback(std::bind(&CVideoSyncWpPresentation::HandlePresentation, this, _1, _2, _3, _4, _5));

  XbmcThreads::CEventGroup waitGroup{&stopEvent, &m_stopEvent};
  waitGroup.wait();

  m_presentationHandler.Unregister();
}

void CVideoSyncWpPresentation::Cleanup()
{
}

float CVideoSyncWpPresentation::GetFps()
{
  return m_fps;
}

void CVideoSyncWpPresentation::HandlePresentation(timespec tv, std::uint32_t refresh, std::uint32_t syncOutputID, float syncOutputRefreshRate, std::uint64_t msc)
{
  auto mscDiff = msc - m_lastMsc;

  CLog::Log(LOGDEBUG, LOGAVTIMING,
            "VideoSyncWpPresentation: tv {}.{:09} s next refresh in +{} ns (fps {:f}) sync output "
            "id {} fps {:f} msc {} mscdiff {}",
            static_cast<std::uint64_t>(tv.tv_sec), static_cast<std::uint64_t>(tv.tv_nsec), refresh,
            1.0e9 / refresh, syncOutputID, syncOutputRefreshRate, msc, mscDiff);

  if (m_fps != syncOutputRefreshRate || (m_syncOutputID != 0 && m_syncOutputID != syncOutputID))
  {
    // Restart if fps changes or sync output changes (which means that the msc jumps)
    CLog::Log(LOGDEBUG, "fps or sync output changed, restarting Wayland video sync");
    m_stopEvent.Set();
  }
  m_syncOutputID = syncOutputID;

  if (m_lastMsc == 0)
  {
    // If this is the first time or MSC is not supported, assume we moved one frame
    mscDiff = 1;
  }
  m_lastMsc = msc;

  // FIXME use timespec instead of currenthostcounter()? Possibly difficult
  // due to different clock base
  m_refClock->UpdateClock(mscDiff, CurrentHostCounter());
}
