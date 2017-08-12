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

#include "GLContextEGL.h"

#include "utils/EGLUtils.h"
#include "utils/log.h"

using namespace KODI::WINDOWING::WAYLAND;

CGLContextEGL::CGLContextEGL()
{
  // EGL_EXT_platform_wayland requires EGL_EXT_client_extensions, so this should
  // be safe to use
  m_clientExtensions = CEGLUtils::GetClientExtensions();

  if (m_clientExtensions.find("EGL_EXT_platform_base") == m_clientExtensions.end())
  {
    throw std::runtime_error("EGL implementation does not support EGL_EXT_platform_base, cannot continue");
  }
  if (m_clientExtensions.find("EGL_EXT_platform_wayland") == m_clientExtensions.end())
  {
    throw std::runtime_error("EGL implementation does not support EGL_EXT_platform_wayland, cannot continue");
  }

  // Theoretically it is possible to use eglGetDisplay() and eglCreateWindowSurface,
  // but then the EGL library basically has to guess which platform we want
  // if it supports multiple which is usually the case -
  // it's better and safer to make it explicit
  m_eglGetPlatformDisplayEXT = CEGLUtils::GetRequiredProcAddress<PFNEGLGETPLATFORMDISPLAYEXTPROC>("eglGetPlatformDisplayEXT");
  m_eglCreatePlatformWindowSurfaceEXT = CEGLUtils::GetRequiredProcAddress<PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC>("eglCreatePlatformWindowSurfaceEXT");
}

CGLContextEGL::~CGLContextEGL() noexcept
{
  Destroy();
}

bool CGLContextEGL::CreateDisplay(wayland::display_t& display,
                                  EGLint renderableType,
                                  EGLenum renderingApi)
{
  EGLint neglconfigs = 0;
  int major, minor;

  EGLint attribs[] ={
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, renderableType,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_DEPTH_SIZE, 16,
    EGL_NONE,
  };

  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    m_eglDisplay = m_eglGetPlatformDisplayEXT(EGL_PLATFORM_WAYLAND_EXT, display, nullptr);
  }

  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    CEGLUtils::LogError("Failed to get EGL Wayland platform display");
    return false;
  }

  if (eglInitialize(m_eglDisplay, &major, &minor) != EGL_TRUE)
  {
    CEGLUtils::LogError("Failed to initialize EGL display");
    return false;
  }
  CLog::Log(LOGINFO, "Got EGL v%d.%d", major, minor);

  if (eglBindAPI(renderingApi) != EGL_TRUE)
  {
    CEGLUtils::LogError("Failed to bind EGL API");
    return false;
  }

  if (eglChooseConfig(m_eglDisplay, attribs, &m_eglConfig, 1, &neglconfigs) != EGL_TRUE)
  {
    CEGLUtils::LogError("Failed to query number of EGL configs");
    return false;
  }

  if (neglconfigs <= 0)
  {
    CLog::Log(LOGERROR, "No suitable EGL configs found");
    return false;
  }

  return true;
}

bool CGLContextEGL::CreateSurface(wayland::surface_t const& surface, CSizeInt size)
{
  m_nativeWindow = wayland::egl_window_t(surface, size.Width(), size.Height());

  m_eglSurface = m_eglCreatePlatformWindowSurfaceEXT(m_eglDisplay,
                                                     m_eglConfig,
                                                     m_nativeWindow, nullptr);

  if (m_eglSurface == EGL_NO_SURFACE)
  {
    CEGLUtils::LogError("Failed to create EGL platform window surface");
    return false;
  }

  const EGLint context_atrribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };

  m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig,
                                  EGL_NO_CONTEXT, context_atrribs);

  if (m_eglContext == EGL_NO_CONTEXT)
  {
    CEGLUtils::LogError("Failed to create EGL context");
    return false;
  }

  if (eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext) != EGL_TRUE)
  {
    CEGLUtils::LogError("Failed to make context current");
    return false;
  }

  return true;
}

CSizeInt CGLContextEGL::GetAttachedSize()
{
  int width, height;
  m_nativeWindow.get_attached_size(width, height);
  return {width, height};
}

void CGLContextEGL::Resize(CSizeInt size)
{
  m_nativeWindow.resize(size.Width(), size.Height(), 0, 0);
}

void CGLContextEGL::Destroy()
{
  DestroySurface();

  if (m_eglDisplay != EGL_NO_DISPLAY)
  {
    eglTerminate(m_eglDisplay);
    m_eglDisplay = EGL_NO_DISPLAY;
  }
}

void CGLContextEGL::DestroySurface()
{
  if (m_eglContext != EGL_NO_CONTEXT)
  {
    eglDestroyContext(m_eglDisplay, m_eglContext);
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    m_eglContext = EGL_NO_CONTEXT;
  }

  if (m_eglSurface != EGL_NO_SURFACE)
  {
    eglDestroySurface(m_eglDisplay, m_eglSurface);
    m_eglSurface = EGL_NO_SURFACE;
  }

  m_nativeWindow = wayland::egl_window_t();
}

void CGLContextEGL::SetVSync(bool enable)
{
  if (eglSwapInterval(m_eglDisplay, enable) != EGL_TRUE)
  {
    CEGLUtils::LogError("Failed to set egl swap interval");
  }
}

void CGLContextEGL::SwapBuffers()
{
  if (m_eglDisplay == EGL_NO_DISPLAY || m_eglSurface == EGL_NO_SURFACE)
    return;

  if (eglSwapBuffers(m_eglDisplay, m_eglSurface) != EGL_TRUE)
  {
    // For now we just hard fail if this fails
    // Theoretically, EGL_CONTEXT_LOST could be handled, but it needs to be checked
    // whether egl implementations actually use it (mesa does not)
    CEGLUtils::LogError("eglSwapBuffers failed");
    throw std::runtime_error("eglSwapBuffers failed");
  }
}
