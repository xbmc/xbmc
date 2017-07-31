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

#if defined (HAVE_LIBVA)
#include <va/va_drm.h>
#include "cores/VideoPlayer/DVDCodecs/Video/VAAPI.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererVAAPIGLES.h"
#endif

#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGLES.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"

#include "WinSystemGbmGLESContext.h"
#include "utils/log.h"

bool CWinSystemGbmGLESContext::InitWindowSystem()
{
  CLinuxRendererGLES::Register();

  if (!CWinSystemGbm::InitWindowSystem())
  {
    return false;
  }

  if (!m_pGLContext.CreateDisplay(m_nativeDisplay,
                                  EGL_OPENGL_ES2_BIT,
                                  EGL_OPENGL_ES_API))
  {
    return false;
  }

#if defined (HAVE_LIBVA)
  VADisplay vaDpy = static_cast<VADisplay>(CWinSystemGbm::GetVaDisplay());
  bool general, hevc;
  CRendererVAAPI::Register(vaDpy, m_pGLContext.m_eglDisplay, general, hevc);
  if (general)
  {
    VAAPI::CDecoder::Register(hevc);
  }
#endif

  return true;
}

bool CWinSystemGbmGLESContext::DestroyWindowSystem()
{
  CDVDFactoryCodec::ClearHWAccels();
  VIDEOPLAYER::CRendererFactory::ClearRenderer();

  if (!CWinSystemGbm::DestroyWindowSystem())
  {
    return false;
  }

  return true;
}

bool CWinSystemGbmGLESContext::CreateNewWindow(const std::string& name,
                                               bool fullScreen,
                                               RESOLUTION_INFO& res)
{
  m_pGLContext.Detach();

  if (!CWinSystemGbm::DestroyWindow())
  {
    return false;
  }

  if (!CWinSystemGbm::CreateNewWindow(name, fullScreen, res))
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

  return true;
}

bool CWinSystemGbmGLESContext::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  if (res.iWidth != m_drm->mode->hdisplay ||
      res.iHeight != m_drm->mode->vdisplay)
  {
    CLog::Log(LOGDEBUG, "CWinSystemGbmGLESContext::%s - resolution changed, creating a new window", __FUNCTION__);
    CreateNewWindow("", fullScreen, res);
  }

  m_pGLContext.SwapBuffers();

  CWinSystemGbm::SetFullScreen(fullScreen, res, blankOtherDisplays);
  CRenderSystemGLES::ResetRenderSystem(res.iWidth, res.iHeight);

  return true;
}

void CWinSystemGbmGLESContext::PresentRenderImpl(bool rendered)
{
  if (rendered)
  {
    m_pGLContext.SwapBuffers();
    CGBMUtils::FlipPage();
  }
}

EGLDisplay CWinSystemGbmGLESContext::GetEGLDisplay() const
{
  return m_pGLContext.m_eglDisplay;
}

EGLSurface CWinSystemGbmGLESContext::GetEGLSurface() const
{
  return m_pGLContext.m_eglSurface;
}

EGLContext CWinSystemGbmGLESContext::GetEGLContext() const
{
  return m_pGLContext.m_eglContext;
}

EGLConfig  CWinSystemGbmGLESContext::GetEGLConfig() const
{
  return m_pGLContext.m_eglConfig;
}
