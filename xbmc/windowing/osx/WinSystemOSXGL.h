/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#if !defined(__arm__) && !defined(__aarch64__)
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

#endif

