/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "VideoSyncAML.h"
#include "WinSystemAmlogicGLESContext.h"
#include "utils/log.h"
#include "threads/SingleLock.h"

bool CWinSystemAmlogicGLESContext::InitWindowSystem()
{
  if (!CWinSystemAmlogic::InitWindowSystem())
  {
    return false;
  }

  if (!m_pGLContext.CreateDisplay(m_nativeDisplay,
                                  EGL_OPENGL_ES2_BIT,
                                  EGL_OPENGL_ES_API))
  {
    return false;
  }

  return true;
}

bool CWinSystemAmlogicGLESContext::CreateNewWindow(const std::string& name,
                                               bool fullScreen,
                                               RESOLUTION_INFO& res)
{
  m_pGLContext.Detach();

  if (!CWinSystemAmlogic::DestroyWindow())
  {
    return false;
  }

  if (!CWinSystemAmlogic::CreateNewWindow(name, fullScreen, res))
  {
    return false;
  }

  if (!m_pGLContext.CreateSurface(m_nativeWindow))
  {
    return false;
  }

  if (!m_pGLContext.CreateContext())
  {
    return false;
  }

  if (!m_pGLContext.BindContext())
  {
    return false;
  }

  if (!m_pGLContext.SurfaceAttrib())
  {
    return false;
  }

  if (!m_delayDispReset)
  {
    CSingleLock lock(m_resourceSection);
    // tell any shared resources
    for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
      (*i)->OnResetDisplay();
  }

  return true;
}

bool CWinSystemAmlogicGLESContext::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  CRenderSystemGLES::ResetRenderSystem(newWidth, newHeight);
  return true;
}

bool CWinSystemAmlogicGLESContext::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  CreateNewWindow("", fullScreen, res);
  CRenderSystemGLES::ResetRenderSystem(res.iWidth, res.iHeight);
  return true;
}

void CWinSystemAmlogicGLESContext::SetVSyncImpl(bool enable)
{
  m_iVSyncMode = enable ? 10:0;
  if (!m_pGLContext.SetVSync(enable))
  {
    m_iVSyncMode = 0;
    CLog::Log(LOGERROR, "%s,Could not set egl vsync", __FUNCTION__);
  }
}

void CWinSystemAmlogicGLESContext::PresentRenderImpl(bool rendered)
{
  if (m_delayDispReset && m_dispResetTimer.IsTimePast())
  {
    m_delayDispReset = false;
    CSingleLock lock(m_resourceSection);
    // tell any shared resources
    for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
      (*i)->OnResetDisplay();
  }
  if (!rendered)
    return;

  m_pGLContext.SwapBuffers();
}

EGLDisplay CWinSystemAmlogicGLESContext::GetEGLDisplay() const
{
  return m_pGLContext.m_eglDisplay;
}

EGLSurface CWinSystemAmlogicGLESContext::GetEGLSurface() const
{
  return m_pGLContext.m_eglSurface;
}

EGLContext CWinSystemAmlogicGLESContext::GetEGLContext() const
{
  return m_pGLContext.m_eglContext;
}

EGLConfig  CWinSystemAmlogicGLESContext::GetEGLConfig() const
{
  return m_pGLContext.m_eglConfig;
}

std::unique_ptr<CVideoSync> CWinSystemAmlogicGLESContext::GetVideoSync(void *clock)
{
  std::unique_ptr<CVideoSync> pVSync(new CVideoSyncAML(clock));
  return pVSync;
}

