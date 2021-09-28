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
  CVideoSyncOsx(void* clock)
    : CVideoSync(clock), m_LastVBlankTime(0), m_displayLost(false), m_displayReset(false)
  {
  }

  // CVideoSync interface
  bool Setup(PUPDATECLOCK func) override;
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

  int64_t m_LastVBlankTime;  //timestamp of the last vblank, used for calculating how many vblanks happened
  volatile bool m_displayLost;
  volatile bool m_displayReset;
  CEvent m_lostEvent;
};

