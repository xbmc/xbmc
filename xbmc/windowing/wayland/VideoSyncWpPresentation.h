/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Signals.h"
#include "windowing/VideoSync.h"

#include <cstdint>

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

class CWinSystemWayland;

class CVideoSyncWpPresentation : public CVideoSync
{
public:
  explicit CVideoSyncWpPresentation(CVideoReferenceClock* clock, CWinSystemWayland& winSystem);

  float GetFps() override;
  bool Setup() override;
  void Run(CEvent& stop) override;
  void Cleanup() override;

private:
  void HandlePresentation(timespec tv, std::uint32_t refresh, std::uint32_t syncOutputID, float syncOutputRefreshRate, std::uint64_t msc);

  CEvent m_stopEvent;
  CSignalRegistration m_presentationHandler;
  std::uint64_t m_lastMsc{};
  std::uint32_t m_syncOutputID{};
  CWinSystemWayland &m_winSystem;
};

}
}
}
