/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/DispResource.h"
#include "windowing/VideoSync.h"

#include <atomic>


namespace KODI
{
namespace WINDOWING
{
namespace X11
{

class CWinSystemX11GLContext;

class CVideoSyncOML : public CVideoSync, IDispResource
{
public:
  explicit CVideoSyncOML(CVideoReferenceClock* clock, CWinSystemX11GLContext& winSystem)
    : CVideoSync(clock), m_winSystem(winSystem)
  {
  }
  bool Setup() override;
  void Run(CEvent& stopEvent) override;
  void Cleanup() override;
  float GetFps() override;
  void OnResetDisplay() override;

private:
  std::atomic_bool m_abort;
  CWinSystemX11GLContext &m_winSystem;
};

}
}
}
