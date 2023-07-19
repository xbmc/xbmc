/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/DispResource.h"
#include "windowing/VideoSync.h"

class CVideoSyncAndroid : public CVideoSync, IDispResource
{
public:
  CVideoSyncAndroid(CVideoReferenceClock* clock) : CVideoSync(clock) {}

  // CVideoSync interface
  bool Setup() override;
  void Run(CEvent& stop) override;
  void Cleanup() override;
  float GetFps() override;

  // IDispResource interface
  void OnResetDisplay() override;

  // Choreographer callback
  void FrameCallback(int64_t frameTimeNanos);

private:
  int64_t m_LastVBlankTime =
      0; //timestamp of the last vblank, used for calculating how many vblanks happened
  CEvent m_abortEvent;
};
