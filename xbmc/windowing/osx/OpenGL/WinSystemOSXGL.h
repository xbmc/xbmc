/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "rendering/gl/RenderSystemGL.h"
#include "windowing/osx/WinSystemOSX.h"

class CWinSystemOSXGL : public CWinSystemOSX, public CRenderSystemGL
{
public:
  CWinSystemOSXGL() = default;
  ~CWinSystemOSXGL() override = default;

  static void Register();
  static std::unique_ptr<CWinSystemBase> CreateWinSystem();

  // Implementation of CWinSystemBase via CWinSystemOSX
  CRenderSystemBase *GetRenderSystem() override { return this; }
  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;

protected:
  void PresentRenderImpl(bool rendered) override;
  void SetVSyncImpl(bool enable) override;
};
