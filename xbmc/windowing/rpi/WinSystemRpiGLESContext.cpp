/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "Application.h"
#include "VideoSyncPi.h"
#include "WinSystemRpiGLESContext.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "ServiceBroker.h"
#include "utils/log.h"
#include "cores/RetroPlayer/process/rbpi/RPProcessInfoPi.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererOpenGLES.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/DVDCodecs/Video/MMALFFmpeg.h"
#include "cores/VideoPlayer/DVDCodecs/Video/MMALCodec.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "cores/VideoPlayer/Process/rbpi/ProcessInfoPi.h"

using namespace KODI;


std::unique_ptr<CWinSystemBase> CWinSystemBase::CreateWinSystem()
{
  std::unique_ptr<CWinSystemBase> winSystem(new CWinSystemRpiGLESContext());
  return winSystem;
}

bool CWinSystemRpiGLESContext::InitWindowSystem()
{
  if (!CWinSystemRpi::InitWindowSystem())
  {
    return false;
  }

  if (!m_pGLContext.CreateDisplay(m_nativeDisplay,
                                  EGL_OPENGL_ES2_BIT,
                                  EGL_OPENGL_ES_API))
  {
    return false;
  }
  CProcessInfoPi::Register();
  RETRO::CRPProcessInfoPi::Register();
  RETRO::CRPProcessInfoPi::RegisterRendererFactory(new RETRO::CRendererFactoryOpenGLES);
  CDVDFactoryCodec::ClearHWAccels();
  MMAL::CDecoder::Register();
  CDVDFactoryCodec::ClearHWVideoCodecs();
  MMAL::CMMALVideo::Register();
  VIDEOPLAYER::CRendererFactory::ClearRenderer();
  MMAL::CMMALRenderer::Register();

  return true;
}

bool CWinSystemRpiGLESContext::CreateNewWindow(const std::string& name,
                                               bool fullScreen,
                                               RESOLUTION_INFO& res)
{
  m_pGLContext.Detach();

  if (!CWinSystemRpi::DestroyWindow())
  {
    return false;
  }

  if (!CWinSystemRpi::CreateNewWindow(name, fullScreen, res))
  {
    return false;
  }

  if (!m_pGLContext.CreateSurface(m_nativeWindow))
  {
    return false;
  }

  const EGLint contextAttribs[] =
  {
    EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
  };

  if (!m_pGLContext.CreateContext(contextAttribs))
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

bool CWinSystemRpiGLESContext::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  CRenderSystemGLES::ResetRenderSystem(newWidth, newHeight);
  return true;
}

bool CWinSystemRpiGLESContext::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  CreateNewWindow("", fullScreen, res);
  CRenderSystemGLES::ResetRenderSystem(res.iWidth, res.iHeight);
  return true;
}

void CWinSystemRpiGLESContext::SetVSyncImpl(bool enable)
{
  m_iVSyncMode = enable ? 10:0;
  if (!m_pGLContext.SetVSync(enable))
  {
    m_iVSyncMode = 0;
    CLog::Log(LOGERROR, "%s,Could not set egl vsync", __FUNCTION__);
  }
}

void CWinSystemRpiGLESContext::PresentRenderImpl(bool rendered)
{
  CGUIComponent *gui = CServiceBroker::GetGUI();
  if (gui)
    CWinSystemRpi::SetVisible(gui->GetWindowManager().HasVisibleControls() || g_application.GetAppPlayer().IsRenderingGuiLayer());

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

EGLDisplay CWinSystemRpiGLESContext::GetEGLDisplay() const
{
  return m_pGLContext.m_eglDisplay;
}

EGLSurface CWinSystemRpiGLESContext::GetEGLSurface() const
{
  return m_pGLContext.m_eglSurface;
}

EGLContext CWinSystemRpiGLESContext::GetEGLContext() const
{
  return m_pGLContext.m_eglContext;
}

EGLConfig  CWinSystemRpiGLESContext::GetEGLConfig() const
{
  return m_pGLContext.m_eglConfig;
}

std::unique_ptr<CVideoSync> CWinSystemRpiGLESContext::GetVideoSync(void *clock)
{
  std::unique_ptr<CVideoSync> pVSync(new CVideoSyncPi(clock));
  return pVSync;
}

