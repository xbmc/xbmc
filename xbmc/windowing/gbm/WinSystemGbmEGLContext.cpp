/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"

#include "OptionalsReg.h"
#include "utils/log.h"
#include "WinSystemGbmEGLContext.h"

using namespace KODI::WINDOWING::GBM;

bool CWinSystemGbmEGLContext::InitWindowSystemEGL(EGLint renderableType, EGLint apiType)
{
  if (!CWinSystemGbm::InitWindowSystem())
  {
    return false;
  }

  if (!m_eglContext.CreatePlatformDisplay(m_GBM->GetDevice(), m_GBM->GetDevice()))
  {
    return false;
  }

  if (!m_eglContext.InitializeDisplay(apiType))
  {
    return false;
  }

  uint32_t visualId = m_DRM->GetGuiPlane()->GetFormat();

  // prefer alpha visual id, fallback to non-alpha visual id
  if (!m_eglContext.ChooseConfig(renderableType, CDRMUtils::FourCCWithAlpha(visualId)) &&
      !m_eglContext.ChooseConfig(renderableType, CDRMUtils::FourCCWithoutAlpha(visualId)))
  {
    // fallback to 8bit format if no EGL config was found for 10bit
    m_DRM->GetGuiPlane()->useFallbackFormat = true;
    visualId = m_DRM->GetGuiPlane()->GetFormat();

    if (!m_eglContext.ChooseConfig(renderableType, CDRMUtils::FourCCWithAlpha(visualId)) &&
        !m_eglContext.ChooseConfig(renderableType, CDRMUtils::FourCCWithoutAlpha(visualId)))
    {
      return false;
    }
  }

  if (!CreateContext())
  {
    return false;
  }

  return true;
}

bool CWinSystemGbmEGLContext::CreateNewWindow(const std::string& name,
                                              bool fullScreen,
                                              RESOLUTION_INFO& res)
{
  //Notify other subsystems that we change resolution
  OnLostDevice();

  if (!DestroyWindow())
  {
    return false;
  }

  if (!m_DRM->SetMode(res))
  {
    CLog::Log(LOGERROR, "CWinSystemGbmEGLContext::{} - failed to set DRM mode", __FUNCTION__);
    return false;
  }

  uint32_t format = m_eglContext.GetConfigAttrib(EGL_NATIVE_VISUAL_ID);
  std::vector<uint64_t> *modifiers = m_DRM->GetGuiPlaneModifiersForFormat(format);

  if (!m_GBM->CreateSurface(res.iWidth, res.iHeight, format, modifiers->data(), modifiers->size()))
  {
    CLog::Log(LOGERROR, "CWinSystemGbmEGLContext::{} - failed to initialize GBM", __FUNCTION__);
    return false;
  }

  // This check + the reinterpret cast is for security reason, if the user has outdated platform header files which often is the case
  static_assert(sizeof(EGLNativeWindowType) == sizeof(gbm_surface*), "Declaration specifier differs in size");

  if (!m_eglContext.CreatePlatformSurface(m_GBM->GetSurface(), reinterpret_cast<EGLNativeWindowType>(m_GBM->GetSurface())))
  {
    return false;
  }

  if (!m_eglContext.BindContext())
  {
    return false;
  }

  m_bFullScreen = fullScreen;
  m_nWidth = res.iWidth;
  m_nHeight = res.iHeight;
  m_fRefreshRate = res.fRefreshRate;

  CLog::Log(LOGDEBUG, "CWinSystemGbmEGLContext::{} - initialized GBM", __FUNCTION__);
  return true;
}

bool CWinSystemGbmEGLContext::DestroyWindow()
{
  m_eglContext.DestroySurface();
  m_GBM->DestroySurface();

  CLog::Log(LOGDEBUG, "CWinSystemGbmEGLContext::{} - deinitialized GBM", __FUNCTION__);
  return true;
}

bool CWinSystemGbmEGLContext::DestroyWindowSystem()
{
  CDVDFactoryCodec::ClearHWAccels();
  VIDEOPLAYER::CRendererFactory::ClearRenderer();
  m_eglContext.Destroy();

  return CWinSystemGbm::DestroyWindowSystem();
}

void CWinSystemGbmEGLContext::delete_CVaapiProxy::operator()(CVaapiProxy *p) const
{
  VaapiProxyDelete(p);
}

EGLDisplay CWinSystemGbmEGLContext::GetEGLDisplay() const
{
  return m_eglContext.GetEGLDisplay();
}

EGLSurface CWinSystemGbmEGLContext::GetEGLSurface() const
{
  return m_eglContext.GetEGLSurface();
}

EGLContext CWinSystemGbmEGLContext::GetEGLContext() const
{
  return m_eglContext.GetEGLContext();
}

EGLConfig  CWinSystemGbmEGLContext::GetEGLConfig() const
{
  return m_eglContext.GetEGLConfig();
}
