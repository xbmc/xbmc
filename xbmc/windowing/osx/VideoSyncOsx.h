/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/DispResource.h"
#include "threads/Event.h"
#include "windowing/VideoSync.h"

class CVideoSyncOsx : public CVideoSync, IDispResource
{
public:
  CVideoSyncOsx(CVideoReferenceClock* clock) : CVideoSync(clock) {}

  // CVideoSync interface
  bool Setup() override;
  void Run(CEvent& stopEvent) override;
  void Cleanup() override;
  float GetFps() override;
  void RefreshChanged() override;

  // IDispResource interface
  void OnLostDisplay() override;
  void OnResetDisplay() override;

  // used in the displaylink callback
  void VblankHandler(int64_t nowtime, uint32_t timebase);

private:
  virtual bool InitDisplayLink();
  virtual void DeinitDisplayLink();

  int64_t m_LastVBlankTime =
      0; //timestamp of the last vblank, used for calculating how many vblanks happened
  volatile bool m_displayLost = false;
  volatile bool m_displayReset = false;
  CEvent m_lostEvent;
};

