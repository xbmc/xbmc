/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/DispResource.h"
#include "windowing/VideoSync.h"

#include <atomic>

class CWinSystemBase;

class CVideoSyncGbm : public CVideoSync, IDispResource
{
public:
  explicit CVideoSyncGbm(CVideoReferenceClock* clock);
  CVideoSyncGbm() = delete;
  ~CVideoSyncGbm() override = default;

  // CVideoSync overrides
  bool Setup() override;
  void Run(CEvent& stopEvent) override;
  void Cleanup() override;
  float GetFps() override;
  void RefreshChanged() override;

  // IDispResource overrides
  void OnResetDisplay() override;

private:
  int m_fd = -1;
  uint32_t m_crtcId = 0;
  uint64_t m_sequence = 0;
  uint64_t m_offset = 0;
  std::atomic<bool> m_abort{false};

  CWinSystemBase* m_winSystem;
};
