/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "WinSystemWayland.h"
#ifdef TARGET_WEBOS
#include "WinSystemWaylandWebOS.h"
#endif
#include "utils/EGLUtils.h"
#include "windowing/linux/WinSystemEGL.h"

#include <wayland-egl.hpp>

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

#ifdef TARGET_WEBOS
using CWinSystemWaylandImpl = CWinSystemWaylandWebOS;
#else
using CWinSystemWaylandImpl = CWinSystemWayland;
#endif

class CWinSystemWaylandEGLContext : public KODI::WINDOWING::LINUX::CWinSystemEGL,
                                    public CWinSystemWaylandImpl
{
public:
  CWinSystemWaylandEGLContext();
  ~CWinSystemWaylandEGLContext() override = default;

  bool CreateNewWindow(const std::string& name,
                       bool fullScreen,
                       RESOLUTION_INFO& res) override;
  bool DestroyWindow() override;
  bool DestroyWindowSystem() override;

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

  wayland::egl_window_t m_nativeWindow;
};

}
}
}
