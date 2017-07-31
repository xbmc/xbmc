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


#include "WinSystemMirGLContext.h"

#if defined(HAS_GL)

bool CWinSystemMirGLContext::CreateNewWindow(const std::string& name,
                                               bool fullScreen,
                                               RESOLUTION_INFO& res)
{
  if (!m_pGLContext.CreateDisplay(m_connection,
                                  EGL_OPENGL_BIT,
                                  EGL_OPENGL_API))
  {
    return false;
  }

  m_pixel_format = mir_connection_get_egl_pixel_format(m_connection,
                                                       m_pGLContext.m_eglDisplay,
                                                       m_pGLContext.m_eglConfig);

  CWinSystemMir::CreateNewWindow(name, fullScreen, res);

  if (!m_pGLContext.CreateSurface(m_window))
  {
    return false;
  }

  if (!m_pGLContext.CreateContext())
  {
    return false;
  }

  return SetFullScreen(fullScreen, res, false);
}

bool CWinSystemMirGLContext::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  auto ret = CWinSystemMir::SetFullScreen(fullScreen, res, blankOtherDisplays);
  if (ret)
  {
    CRenderSystemGL::ResetRenderSystem(res.iWidth, res.iHeight);
  }

  return ret;
}

void CWinSystemMirGLContext::SetVSyncImpl(bool enable)
{
  m_pGLContext.SetVSync(enable);
}

void CWinSystemMirGLContext::PresentRenderImpl(bool rendered)
{
  if (rendered)
  {
    m_pGLContext.SwapBuffers();
  }
}

EGLDisplay CWinSystemMirGLContext::GetEGLDisplay() const
{
  return m_pGLContext.m_eglDisplay;
}

EGLSurface CWinSystemMirGLContext::GetEGLSurface() const
{
  return m_pGLContext.m_eglSurface;
}

EGLContext CWinSystemMirGLContext::GetEGLContext() const
{
  return m_pGLContext.m_eglContext;
}

EGLConfig  CWinSystemMirGLContext::GetEGLConfig() const
{
  return m_pGLContext.m_eglConfig;
}

// FIXME Implement
bool CWinSystemMirGLContext::IsExtSupported(const char* extension)
{
  return false;
}

#endif
