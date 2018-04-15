/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#include "WinSystemX11.h"

#include "EGL/egl.h"
#include "rendering/gl/RenderSystemGL.h"
#include "platform/linux/OptionalsReg.h"
#include <memory>

class CGLContext;
class CVaapiProxy;

class CWinSystemX11GLContext : public CWinSystemX11, public CRenderSystemGL
{
public:
  CWinSystemX11GLContext();
  ~CWinSystemX11GLContext() override;

  // Implementation of CWinSystem via CWinSystemX11
  CRenderSystemBase *GetRenderSystem() override { return this; }
  bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) override;
  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  void FinishWindowResize(int newWidth, int newHeight) override;
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;
  bool DestroyWindowSystem() override;
  bool DestroyWindow() override;

  bool IsExtSupported(const char* extension) const override;

  // videosync
  std::unique_ptr<CVideoSync> GetVideoSync(void *clock) override;

  XID GetWindow() const;
  void* GetGlxContext() const;
  EGLDisplay GetEGLDisplay() const;
  EGLSurface GetEGLSurface() const;
  EGLContext GetEGLContext() const;
  EGLConfig GetEGLConfig() const;

protected:
  bool SetWindow(int width, int height, bool fullscreen, const std::string &output, int *winstate = NULL) override;
  void PresentRenderImpl(bool rendered) override;
  void SetVSyncImpl(bool enable) override;
  bool RefreshGLContext(bool force);
  XVisualInfo* GetVisual() override;

  CGLContext *m_pGLContext = nullptr;
  bool m_newGlContext;

  struct delete_CVaapiProxy
  {
    void operator()(CVaapiProxy *p) const;
  };
  std::unique_ptr<CVaapiProxy, delete_CVaapiProxy> m_vaapiProxy;

  std::unique_ptr<OPTIONALS::CLircContainer, OPTIONALS::delete_CLircContainer> m_lirc;
};
