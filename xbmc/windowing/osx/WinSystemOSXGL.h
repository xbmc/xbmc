/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "WinSystemOSX.h"
#include "rendering/gl/RenderSystemGL.h"

class CWinSystemOSXGL : public CWinSystemOSX, public CRenderSystemGL
{
public:
  CWinSystemOSXGL();
  virtual ~CWinSystemOSXGL();

  // Implementation of CWinSystemBase via CWinSystemOSX
  CRenderSystemBase *GetRenderSystem() override { return this; }
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  virtual bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;

protected:
  virtual void PresentRenderImpl(bool rendered) override;
  virtual void SetVSyncImpl(bool enable) override;
};
