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

  // we need to provide an alpha format to egl to workaround a mesa bug
  int visualId = CDRMUtils::FourCCWithAlpha(CWinSystemGbm::GetDrm()->GetOverlayPlane()->format);

  if (!m_eglContext.CreatePlatformDisplay(m_GBM->GetDevice(), m_GBM->GetDevice()))
  {
    return false;
  }

  if (!m_eglContext.InitializeDisplay(apiType))
  {
    return false;
  }

  if (!m_eglContext.ChooseConfig(renderableType, visualId))
  {
    // fallback to 8bit format if no EGL config was found for 10bit
    CWinSystemGbm::GetDrm()->GetOverlayPlane()->useFallbackFormat = true;
    visualId = CDRMUtils::FourCCWithAlpha(CWinSystemGbm::GetDrm()->GetOverlayPlane()->GetFormat());

    if (!m_eglContext.ChooseConfig(renderableType, visualId))
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
  m_eglContext.DestroySurface();

  if (!CWinSystemGbm::DestroyWindow())
  {
    return false;
  }

  if (!CWinSystemGbm::CreateNewWindow(name, fullScreen, res))
  {
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
