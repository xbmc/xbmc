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

class CWinSystemTVOS;

class CVideoSyncTVos : public CVideoSync, IDispResource
{
public:
  CVideoSyncTVos(void* clock, CWinSystemTVOS& winSystem) :
    CVideoSync(clock), m_winSystem(winSystem) {}

  // CVideoSync interface
  virtual bool Setup(PUPDATECLOCK func) override;
  virtual void Run(CEvent& stopEvent) override;
  virtual void Cleanup() override;
  virtual float GetFps() override;

  // IDispResource interface
  virtual void OnResetDisplay() override;

  // used in the displaylink callback
  void TVosVblankHandler();

private:
  // CVideoSyncDarwin interface
  virtual bool InitDisplayLink();
  virtual void DeinitDisplayLink();

  int64_t m_LastVBlankTime = 0;  //timestamp of the last vblank, used for calculating how many vblanks happened
  CEvent m_abortEvent;
  CWinSystemTVOS &m_winSystem;
};

