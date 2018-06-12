/*
 *      Copyright (C) 2017 Team XBMC
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

#include <set>
#include <string>

#include <wayland-client.hpp>
#include <wayland-egl.hpp>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "utils/Geometry.h"

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

/**
 * Class that handles EGL setup/teardown with Wayland native surfaces
 */
class CEGLContext
{
public:
  CEGLContext();
  ~CEGLContext() noexcept;

  bool CreateDisplay(wayland::display_t& display, EGLint renderableType, EGLenum renderingApi);
  /**
   * Create EGL context. Call after \ref CreateDisplay.
   */
  bool CreateContext(const EGLint* contextAttribs);
  /**
   * Create EGL surface. Call after \ref CreateDisplay.
   */
  bool CreateSurface(wayland::surface_t const& surface, CSizeInt size);
  /**
   * Activate EGL context. Call after \ref CreateDisplay, \ref CreateContext,
   * and \ref CreateSurface.
   */
  bool MakeCurrent();

  CSizeInt GetAttachedSize();
  void Resize(CSizeInt size);
  void DestroyContext();
  void DestroySurface();
  void Destroy();
  void SetVSync(bool enable);
  void SwapBuffers();

  EGLDisplay GetEGLDisplay() const
  {
    return m_eglDisplay;
  }

private:
  CEGLContext(CEGLContext const& other) = delete;
  CEGLContext& operator=(CEGLContext const& other) = delete;

  wayland::egl_window_t m_nativeWindow;
  EGLDisplay m_eglDisplay{EGL_NO_DISPLAY};
  EGLSurface m_eglSurface{EGL_NO_SURFACE};
  EGLContext m_eglContext{EGL_NO_CONTEXT};
  EGLConfig m_eglConfig{};

  std::set<std::string> m_clientExtensions;

  PFNEGLGETPLATFORMDISPLAYEXTPROC m_eglGetPlatformDisplayEXT = nullptr;
  PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC m_eglCreatePlatformWindowSurfaceEXT = nullptr;
};

}
}
}
