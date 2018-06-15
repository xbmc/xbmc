/*
 *  Copyright (C) 2005-2014 Team XBMC
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "windowing/VideoSync.h"
#include "guilib/DispResource.h"

class CWinSystemX11GLContext;

class CVideoSyncDRM : public CVideoSync, IDispResource
{
public:
  explicit CVideoSyncDRM(void *clock, CWinSystemX11GLContext& winSystem) :
    CVideoSync(clock), m_winSystem(winSystem) {};
  bool Setup(PUPDATECLOCK func) override;
  void Run(CEvent& stopEvent) override;
  void Cleanup() override;
  float GetFps() override;
  void OnResetDisplay() override;
  void RefreshChanged() override;
private:
  static void EventHandler(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data);
  int m_fd;
  volatile bool m_abort;
  struct VblInfo
  {
    uint64_t start;
    CVideoSyncDRM *videoSync;
  };
  CWinSystemX11GLContext &m_winSystem;
};

