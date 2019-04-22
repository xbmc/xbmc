/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodecDRMPRIME.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererDRMPRIME.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererDRMPRIMEGLES.h"

#include "cores/RetroPlayer/process/gbm/RPProcessInfoGbm.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererGBM.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererOpenGLES.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/Process/gbm/ProcessInfoGBM.h"
#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGLES.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"

#include "OptionalsReg.h"
#include "platform/linux/XTimeUtils.h"
#include "utils/log.h"
#include "WinSystemGbmGLESContext.h"

#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

using namespace KODI::WINDOWING::GBM;

CWinSystemGbmGLESContext::CWinSystemGbmGLESContext()
: CWinSystemGbmEGLContext(EGL_PLATFORM_GBM_MESA, "EGL_MESA_platform_gbm")
{}

std::unique_ptr<CWinSystemBase> CWinSystemBase::CreateWinSystem()
{
  std::unique_ptr<CWinSystemBase> winSystem(new CWinSystemGbmGLESContext());
  return winSystem;
}

bool CWinSystemGbmGLESContext::InitWindowSystem()
{
  VIDEOPLAYER::CRendererFactory::ClearRenderer();
  CDVDFactoryCodec::ClearHWAccels();
  CLinuxRendererGLES::Register();
  RETRO::CRPProcessInfoGbm::Register();
  RETRO::CRPProcessInfoGbm::RegisterRendererFactory(new RETRO::CRendererFactoryGBM);
  RETRO::CRPProcessInfoGbm::RegisterRendererFactory(new RETRO::CRendererFactoryOpenGLES);

  if (!CWinSystemGbmEGLContext::InitWindowSystemEGL(EGL_OPENGL_ES2_BIT, EGL_OPENGL_ES_API))
  {
    return false;
  }

  bool general, deepColor;
  m_vaapiProxy.reset(GBM::VaapiProxyCreate(m_DRM->GetRenderNodeFileDescriptor()));
  GBM::VaapiProxyConfig(m_vaapiProxy.get(), m_eglContext.GetEGLDisplay());
  GBM::VAAPIRegisterRender(m_vaapiProxy.get(), general, deepColor);

  if (general)
  {
    GBM::VAAPIRegister(m_vaapiProxy.get(), deepColor);
  }

  CRendererDRMPRIMEGLES::Register();
  CRendererDRMPRIME::Register();
  CDVDVideoCodecDRMPRIME::Register();
  VIDEOPLAYER::CProcessInfoGBM::Register();

  return true;
}

bool CWinSystemGbmGLESContext::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  if (res.iWidth != m_nWidth ||
      res.iHeight != m_nHeight)
  {
    CLog::Log(LOGDEBUG, "CWinSystemGbmGLESContext::%s - resolution changed, creating a new window", __FUNCTION__);
    CreateNewWindow("", fullScreen, res);
  }

  if (!m_eglContext.TrySwapBuffers())
  {
    CEGLUtils::LogError("eglSwapBuffers failed");
    throw std::runtime_error("eglSwapBuffers failed");
  }

  CWinSystemGbm::SetFullScreen(fullScreen, res, blankOtherDisplays);
  CRenderSystemGLES::ResetRenderSystem(res.iWidth, res.iHeight);

  if (!m_delayDispReset)
  {
    CSingleLock lock(m_resourceSection);

    for (auto resource : m_resources)
      resource->OnResetDisplay();
  }

  return true;
}

void CWinSystemGbmGLESContext::PresentRender(bool rendered, bool videoLayer)
{
  if (!m_bRenderCreated)
    return;

  if (rendered || videoLayer)
  {
    if (rendered)
    {
      if (!m_eglContext.TrySwapBuffers())
      {
        CEGLUtils::LogError("eglSwapBuffers failed");
        throw std::runtime_error("eglSwapBuffers failed");
      }
    }
    CWinSystemGbm::FlipPage(rendered, videoLayer);
  }
  else
  {
    Sleep(10);
  }

  if (m_delayDispReset && m_dispResetTimer.IsTimePast())
  {
    m_delayDispReset = false;
    CSingleLock lock(m_resourceSection);

    for (auto resource : m_resources)
      resource->OnResetDisplay();
  }
}

bool CWinSystemGbmGLESContext::CreateContext()
{
  CEGLAttributesVec contextAttribs;
  contextAttribs.Add({{EGL_CONTEXT_CLIENT_VERSION, 2}});

  if (!m_eglContext.CreateContext(contextAttribs))
  {
    CLog::Log(LOGERROR, "EGL context creation failed");
    return false;
  }
  return true;
}
