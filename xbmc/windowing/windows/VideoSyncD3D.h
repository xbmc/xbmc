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

#include <dxgi1_5.h>

class CVideoSyncD3D : public CVideoSync, IDispResource
{
public:
  CVideoSyncD3D(CVideoReferenceClock* clock)
    : CVideoSync(clock), m_displayLost(false), m_displayReset(false)
  {
  }
  bool Setup() override;
  void Run(CEvent& stopEvent) override;
  void Cleanup() override;
  float GetFps() override;
  void RefreshChanged() override;
  // IDispResource overrides
  void OnLostDisplay() override;
  void OnResetDisplay() override;

private:
  volatile bool m_displayLost;
  volatile bool m_displayReset;
  CEvent m_lostEvent;
  DXGI_OUTPUT_DESC m_outputDesc{};
  Microsoft::WRL::ComPtr<IDXGIFactory2> m_factory;
};

