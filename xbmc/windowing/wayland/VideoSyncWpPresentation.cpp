/*
 *      Copyright (C) 2017 Team XBMC
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

#include "VideoSyncWpPresentation.h"

#include <cinttypes>
#include <functional>

#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "settings/AdvancedSettings.h"
#include "windowing/wayland/WinSystemWayland.h"

using namespace KODI::WINDOWING::WAYLAND;
using namespace std::placeholders;

CVideoSyncWpPresentation::CVideoSyncWpPresentation(void* clock, CWinSystemWayland& winSystem)
: CVideoSync(clock), m_winSystem(winSystem)
{
}

bool CVideoSyncWpPresentation::Setup(PUPDATECLOCK func)
{
  UpdateClock = func;
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

  CLog::Log(LOGDEBUG, LOGAVTIMING, "VideoSyncWpPresentation: tv %" PRIu64 ".%09" PRIu64 " s next refresh in +%" PRIu32 " ns (fps %f) sync output id %" PRIu32 " fps %f msc %" PRIu64 " mscdiff %" PRIu64, static_cast<std::uint64_t> (tv.tv_sec), static_cast<std::uint64_t> (tv.tv_nsec), refresh, 1.0e9 / refresh, syncOutputID, syncOutputRefreshRate, msc, mscDiff);

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
  UpdateClock(mscDiff, CurrentHostCounter(), m_refClock);
}
