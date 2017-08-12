/*
 *      Copyright (C) 2017 Team XBMC
 *      http://xbmc.org
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

#include "GLContextEGL.h"
#include "WinSystemWayland.h"

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

class CWinSystemWaylandEGLContext : public CWinSystemWayland
{
public:
  CWinSystemWaylandEGLContext() = default;
  virtual ~CWinSystemWaylandEGLContext() = default;

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

  void PresentFrame(bool rendered);
  void SetContextSize(CSizeInt size) override;

  CGLContextEGL m_eglContext;
};

}
}
}