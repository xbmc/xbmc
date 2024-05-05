/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "EGL/egl.h"
#include "WinSystemX11.h"
#include "rendering/gles/RenderSystemGLES.h"

class CGLContextEGL;

namespace KODI
{
namespace WINDOWING
{
namespace X11
{

class CWinSystemX11GLESContext : public CWinSystemX11, public CRenderSystemGLES
{
public:
  CWinSystemX11GLESContext() = default;
  virtual ~CWinSystemX11GLESContext() override;

  static void Register();
  static std::unique_ptr<CWinSystemBase> CreateWinSystem();

  // Implementation of CWinSystem via CWinSystemX11
  CRenderSystemBase* GetRenderSystem() override { return this; }
  bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) override;
  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  void FinishWindowResize(int newWidth, int newHeight) override;
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;
  bool DestroyWindowSystem() override;
  bool DestroyWindow() override;

  bool IsExtSupported(const char* extension) const override;

  EGLDisplay GetEGLDisplay() const;
  EGLSurface GetEGLSurface() const;
  EGLContext GetEGLContext() const;
  EGLConfig GetEGLConfig() const;

  bool BindTextureUploadContext() override;
  bool UnbindTextureUploadContext() override;
  bool HasContext() override;

protected:
  bool SetWindow(int width, int height, bool fullscreen, const std::string& output, int* winstate = nullptr) override;
  void PresentRenderImpl(bool rendered) override;
  void SetVSyncImpl(bool enable) override;
  bool RefreshGLContext(bool force);
  XVisualInfo* GetVisual() override;

  CGLContextEGL* m_pGLContext = nullptr;
  bool m_newGlContext;
};

} // namespace X11
} // namespace WINDOWING
} // namespace KODI
