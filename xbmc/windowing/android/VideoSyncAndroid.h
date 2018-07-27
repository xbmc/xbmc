/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "windowing/VideoSync.h"
#include "guilib/DispResource.h"

class CVideoSyncAndroid : public CVideoSync, IDispResource
{
public:
  CVideoSyncAndroid(void *clock) : CVideoSync(clock), m_LastVBlankTime(0) {}

  // CVideoSync interface
  virtual bool Setup(PUPDATECLOCK func) override;
  virtual void Run(CEvent& stop) override;
  virtual void Cleanup() override;
  virtual float GetFps() override;

  // IDispResource interface
  virtual void OnResetDisplay() override;

  // Choreographer callback
  void FrameCallback(int64_t frameTimeNanos);

private:
  int64_t m_LastVBlankTime;  //timestamp of the last vblank, used for calculating how many vblanks happened
  CEvent m_abortEvent;
};
