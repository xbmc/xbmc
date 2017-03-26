/*
 *      Copyright (C) 2016 Canonical Ltd.
 *      brandon.schaefer@canonical.com
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

#include "utils/log.h"

CGLContextEGL::CGLContextEGL() :
  m_eglDisplay(EGL_NO_DISPLAY),
  m_eglSurface(EGL_NO_SURFACE),
  m_eglContext(EGL_NO_CONTEXT),
  m_eglConfig (0)
{
}

CGLContextEGL::~CGLContextEGL()
{
  Destroy();
}

bool CGLContextEGL::CreateDisplay(MirConnection* connection,
                                  EGLint renderable_type,
                                  EGLint rendering_api)
{
  EGLint neglconfigs = 0;
  int major, minor;

  EGLint attribs[] =
  {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, renderable_type,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_NONE,
  };

  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    m_eglDisplay = eglGetDisplay(static_cast<EGLNativeDisplayType>(
                     mir_connection_get_egl_native_display(connection)));
  }

  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    CLog::Log(LOGERROR, "failed to get EGL display\n");
    return false;
  }

  if (!eglInitialize(m_eglDisplay, &major, &minor))
  {
    CLog::Log(LOGERROR, "failed to initialize EGL display\n");
    return false;
  }

  eglBindAPI(rendering_api);

  if (!eglChooseConfig(m_eglDisplay, attribs,
                       &m_eglConfig, 1, &neglconfigs))
  {
    CLog::Log(LOGERROR, "Failed to query number of EGL configs");
    return false;
  }

  if (neglconfigs <= 0)
  {
    CLog::Log(LOGERROR, "No suitable EGL configs found");
    return false;
  }

  return true;
}

bool CGLContextEGL::CreateContext()
{
  int client_version = 2;

  const EGLint context_atrribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, client_version, EGL_NONE
  };

  m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig,
                                  EGL_NO_CONTEXT, context_atrribs);

  if (m_eglContext == EGL_NO_CONTEXT)
  {
    CLog::Log(LOGERROR, "failed to create EGL context\n");
    return false;
  }

  if (!eglMakeCurrent(m_eglDisplay, m_eglSurface,
                      m_eglSurface, m_eglContext))
  {
    CLog::Log(LOGERROR, "Failed to make context current %p %p %p\n",
                         m_eglDisplay, m_eglSurface, m_eglContext);
    return false;
  }

  return true;
}

bool CGLContextEGL::CreateSurface(MirWindow* window)
{
  EGLNativeWindowType egl_nwin = (EGLNativeWindowType)
                                 mir_buffer_stream_get_egl_native_window(
                                 mir_window_get_buffer_stream(window));

  m_eglSurface = eglCreateWindowSurface(m_eglDisplay,
                                        m_eglConfig,
                                        egl_nwin, nullptr);

  if (m_eglSurface == EGL_NO_SURFACE)
  {
    CLog::Log(LOGERROR, "failed to create EGL window surface %d\n", eglGetError());
    return false;
  }

  return true;
}

void CGLContextEGL::Destroy()
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

  if (m_eglDisplay != EGL_NO_DISPLAY)
  {
    eglTerminate(m_eglDisplay);
    m_eglDisplay = EGL_NO_DISPLAY;
  }
}

void CGLContextEGL::Detach()
{
  if (m_eglContext != EGL_NO_CONTEXT)
  {
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    m_eglContext = EGL_NO_CONTEXT;
  }

  if (m_eglSurface != EGL_NO_SURFACE)
  {
    eglDestroySurface(m_eglDisplay, m_eglSurface);
    m_eglSurface = EGL_NO_SURFACE;
  }
}

void CGLContextEGL::SetVSync(bool enable)
{
    eglSwapInterval(m_eglDisplay, enable);
}

void CGLContextEGL::SwapBuffers()
{
  if (m_eglDisplay == EGL_NO_DISPLAY || m_eglSurface == EGL_NO_SURFACE)
    return;

  eglSwapBuffers(m_eglDisplay, m_eglSurface);
}

// TODO
void CGLContextEGL::QueryExtensions()
{
}
