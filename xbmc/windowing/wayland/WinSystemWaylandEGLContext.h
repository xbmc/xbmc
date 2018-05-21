/*
 *      Copyright (C) 2017-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "EGLContext.h"
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

  virtual bool CreateContext() = 0;

  CEGLContext m_eglContext;
};

}
}
}
