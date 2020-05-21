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

class CWinSystemTVOS;

class CVideoSyncTVos : public CVideoSync, IDispResource
{
public:
  CVideoSyncTVos(void* clock, CWinSystemTVOS& winSystem)
    : CVideoSync(clock), m_winSystem(winSystem){};

  // CVideoSync interface
  bool Setup(PUPDATECLOCK func) override;
  void Run(CEvent& stopEvent) override;
  void Cleanup() override;
  float GetFps() override;

  // IDispResource interface
  void OnResetDisplay() override;

  // used in the displaylink callback
  void TVosVblankHandler();

private:
  // CVideoSyncDarwin interface
  virtual bool InitDisplayLink();
  virtual void DeinitDisplayLink();

  //timestamp of the last vblank, used for calculating how many vblanks happened
  int64_t m_LastVBlankTime = 0;
  CEvent m_abortEvent;
  CWinSystemTVOS& m_winSystem;
};
