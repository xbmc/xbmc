/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "WinSystemWayland.h"
#include "utils/EGLUtils.h"

#include <wayland-egl.hpp>

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

class CWinSystemWaylandEGLContext : public CWinSystemWayland
{
public:
  CWinSystemWaylandEGLContext();
  ~CWinSystemWaylandEGLContext() override = default;

  bool CreateNewWindow(const std::string& name,
                       bool fullScreen,
                       RESOLUTION_INFO& res) override;
  bool DestroyWindow() override;
  bool DestroyWindowSystem() override;

  EGLDisplay GetEGLDisplay() const;

protected:
  /**
   * Inheriting classes should override InitWindowSystem() without parameters
   * and call this function there with appropriate parameters
   */
  bool InitWindowSystemEGL(EGLint renderableType, EGLint apiType);

  CSizeInt GetNativeWindowAttachedSize();
  void PresentFrame(bool rendered);
  void SetContextSize(CSizeInt size) override;

  virtual bool CreateContext() = 0;

  CEGLContextUtils m_eglContext;
  wayland::egl_window_t m_nativeWindow;
};

}
}
}
