/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "windowing/VideoSync.h"

#include <cstdint>

namespace KODI::WINDOWING::WASM
{
class CVideoSyncWasm : public CVideoSync
{
public:
  explicit CVideoSyncWasm(CVideoReferenceClock* clock) : CVideoSync(clock) {}

  bool Setup() override;
  void Run(CEvent& stop) override;
  void Cleanup() override {}
  float GetFps() override;

private:
  int64_t m_lastVBlankTime{0};
};
} // namespace KODI::WINDOWING::WASM
