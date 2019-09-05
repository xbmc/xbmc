/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemRpiGLESContext.h"

#include "Application.h"
#include "ServiceBroker.h"
#include "VideoSyncPi.h"
#include "cores/RetroPlayer/process/rbpi/RPProcessInfoPi.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererOpenGLES.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/DVDCodecs/Video/MMALCodec.h"
#include "cores/VideoPlayer/DVDCodecs/Video/MMALFFmpeg.h"
#include "cores/VideoPlayer/Process/rbpi/ProcessInfoPi.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "rendering/gles/ScreenshotSurfaceGLES.h"
#include "utils/log.h"

#include "platform/linux/ScreenshotSurfaceRBP.h"

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

  if (!m_pGLContext.CreateDisplay(m_nativeDisplay))
  {
    return false;
  }

  if (!m_pGLContext.InitializeDisplay(EGL_OPENGL_ES_API))
  {
    return false;
  }

  if (!m_pGLContext.ChooseConfig(EGL_OPENGL_ES2_BIT))
  {
    return false;
  }

  CEGLAttributesVec contextAttribs;
  contextAttribs.Add({{EGL_CONTEXT_CLIENT_VERSION, 2}});

  if (!m_pGLContext.CreateContext(contextAttribs))
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
  CScreenshotSurfaceGLES::Register();
  CScreenshotSurfaceRBP::Register();

  return true;
}

bool CWinSystemRpiGLESContext::CreateNewWindow(const std::string& name,
                                               bool fullScreen,
                                               RESOLUTION_INFO& res)
{
  m_pGLContext.DestroySurface();

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

  if (!m_pGLContext.BindContext())
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
  if (!m_pGLContext.SetVSync(enable))
  {
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

  if (!m_pGLContext.TrySwapBuffers())
  {
    CEGLUtils::Log(LOGERROR, "eglSwapBuffers failed");
    throw std::runtime_error("eglSwapBuffers failed");
  }
}

EGLDisplay CWinSystemRpiGLESContext::GetEGLDisplay() const
{
  return m_pGLContext.GetEGLDisplay();
}

EGLSurface CWinSystemRpiGLESContext::GetEGLSurface() const
{
  return m_pGLContext.GetEGLSurface();
}

EGLContext CWinSystemRpiGLESContext::GetEGLContext() const
{
  return m_pGLContext.GetEGLContext();
}

EGLConfig  CWinSystemRpiGLESContext::GetEGLConfig() const
{
  return m_pGLContext.GetEGLConfig();
}

std::unique_ptr<CVideoSync> CWinSystemRpiGLESContext::GetVideoSync(void *clock)
{
  std::unique_ptr<CVideoSync> pVSync(new CVideoSyncPi(clock));
  return pVSync;
}

