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

#include "platform/linux/TimeUtils.h"

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <functional>

using namespace KODI::WINDOWING::WAYLAND;
using namespace std::placeholders;

namespace
{
// The presented event's refresh is a prediction and may jitter slightly
constexpr float MAX_REFRESH_RATE_DIFFERENCE{0.01f};
} // namespace

CVideoSyncWpPresentation::CVideoSyncWpPresentation(CVideoReferenceClock* clock,
                                                   CWinSystemWayland& winSystem)
  : CVideoSync(clock), m_winSystem(winSystem)
{
}

bool CVideoSyncWpPresentation::Setup()
{
  m_stopEvent.Reset();
  m_fps = m_winSystem.GetPresentationRefreshRate();
  if (m_fps <= 0.0f)
  {
    CLog::Log(LOGDEBUG, "VideoSyncWpPresentation: refresh rate unknown");
    return false;
  }

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
  std::uint64_t vblanks{1};
  if (msc != 0 && m_lastMsc != 0)
  {
    vblanks = msc - m_lastMsc;
  }
  else if (syncOutputRefreshRate > 0.0f && m_lastPresentationTime)
  {
    // Without MSC, derive the number of vblanks from the presentation timestamps
    const auto elapsed = KODI::LINUX::TimespecDifference(*m_lastPresentationTime, tv);
    vblanks = std::max<std::int64_t>(
        1, std::llround(elapsed * 1.0e-9 * static_cast<double>(syncOutputRefreshRate)));
  }

  CLog::Log(LOGDEBUG, LOGAVTIMING,
            "VideoSyncWpPresentation: tv {}.{:09} s next refresh in +{} ns (fps {:f}) sync output "
            "id {} msc {} vblanks {}",
            static_cast<std::uint64_t>(tv.tv_sec), static_cast<std::uint64_t>(tv.tv_nsec), refresh,
            syncOutputRefreshRate, syncOutputID, msc, vblanks);

  if (std::abs(m_fps - syncOutputRefreshRate) > MAX_REFRESH_RATE_DIFFERENCE ||
      (m_syncOutputID != 0 && m_syncOutputID != syncOutputID))
  {
    // Restart if fps changes or sync output changes (which means that the msc jumps)
    CLog::Log(LOGDEBUG, "fps or sync output changed, restarting Wayland video sync");
    m_stopEvent.Set();
  }
  m_syncOutputID = syncOutputID;
  m_lastMsc = msc;
  m_lastPresentationTime = tv;

  // FIXME use timespec instead of currenthostcounter()? Possibly difficult
  // due to different clock base
  m_refClock->UpdateClock(static_cast<int>(vblanks), CurrentHostCounter());
}
