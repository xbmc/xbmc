/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemGbmEGLContext.h"

#include "OptionalsReg.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "utils/log.h"

using namespace KODI::WINDOWING::GBM;
using namespace KODI::WINDOWING::LINUX;

bool CWinSystemGbmEGLContext::InitWindowSystemEGL(EGLint renderableType, EGLint apiType)
{
  if (!m_DRM && !CWinSystemGbm::InitWindowSystem())
  {
    return false;
  }

  if (!m_eglContext.CreatePlatformDisplay(m_GBM->GetDevice().Get(), m_GBM->GetDevice().Get()))
  {
    return false;
  }

  if (!m_eglContext.InitializeDisplay(apiType))
  {
    m_eglContext.Destroy();
    return false;
  }

  m_renderableType = renderableType;

  auto plane = m_DRM->GetGuiPlane();
  // TODO: GetGuiPlane() should never be null here (FindPlanes would have failed).
  // The DRM_FORMAT_XRGB2101010 fallback is suspect — consider DRM_FORMAT_XRGB8888.
  uint32_t visualId = plane != nullptr ? plane->GetFormat() : DRM_FORMAT_XRGB2101010;

  // Determine alpha size based on plane format. 10-bit formats (ARGB2101010) only
  // have 2-bit alpha; 8-bit formats (ARGB8888) have full 8-bit alpha.
  bool is10bit = (visualId == DRM_FORMAT_XRGB2101010 || visualId == DRM_FORMAT_ARGB2101010);
  int alphaWithAlpha = is10bit ? 2 : 8;

  // prefer alpha visual id, fallback to non-alpha visual id
  if (!m_eglContext.ChooseConfig(renderableType, CDRMUtils::FourCCWithAlpha(visualId), false,
                                 alphaWithAlpha) &&
      !m_eglContext.ChooseConfig(renderableType, CDRMUtils::FourCCWithoutAlpha(visualId), false, 0))
  {
    // fallback to 8bit format if no EGL config was found for 10bit
    if (plane)
      plane->SetFormat(DRM_FORMAT_XRGB8888);

    visualId = plane != nullptr ? plane->GetFormat() : DRM_FORMAT_XRGB8888;

    if (!m_eglContext.ChooseConfig(renderableType, CDRMUtils::FourCCWithAlpha(visualId)) &&
        !m_eglContext.ChooseConfig(renderableType, CDRMUtils::FourCCWithoutAlpha(visualId), false,
                                   0))
    {
      m_eglContext.Destroy();
      return false;
    }
  }

  if (!CreateContext())
  {
    m_eglContext.Destroy();
    return false;
  }

  if (CEGLUtils::HasExtension(m_eglContext.GetEGLDisplay(), "EGL_ANDROID_native_fence_sync") &&
      CEGLUtils::HasExtension(m_eglContext.GetEGLDisplay(), "EGL_KHR_fence_sync"))
  {
    if (m_DRM->SupportsFencing())
    {
      m_eglFence = std::make_unique<KODI::UTILS::EGL::CEGLFence>(m_eglContext.GetEGLDisplay());
    }
    else
    {
      CLog::Log(LOGWARNING, "[GBM] EGL_KHR_fence_sync and EGL_ANDROID_native_fence_sync supported"
                            ", but DRM backend doesn't support fencing");
    }
  }
  else
  {
    CLog::Log(LOGWARNING, "[GBM] missing support for EGL_KHR_fence_sync and "
                          "EGL_ANDROID_native_fence_sync - performance may be impacted");
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

  std::vector<uint64_t> modifiers;

  auto plane = m_DRM->GetGuiPlane();
  if (plane)
    modifiers = plane->GetModifiersForFormat(format);

  if (!m_GBM->GetDevice().CreateSurface(res.iWidth, res.iHeight, format, modifiers.data(),
                                        modifiers.size()))
  {
    CLog::Log(LOGERROR, "CWinSystemGbmEGLContext::{} - failed to initialize GBM", __FUNCTION__);
    return false;
  }

  // This check + the reinterpret cast is for security reason, if the user has outdated platform header files which often is the case
  static_assert(sizeof(EGLNativeWindowType) == sizeof(gbm_surface*),
                "Declaration specifier differs in size");

  if (!m_eglContext.CreatePlatformSurface(
          m_GBM->GetDevice().GetSurface().Get(),
          reinterpret_cast<khronos_uintptr_t>(m_GBM->GetDevice().GetSurface().Get())))
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

void CWinSystemGbmEGLContext::delete_CVaapiProxy::operator()(CVaapiProxy* p) const
{
  VaapiProxyDelete(p);
}

bool CWinSystemGbmEGLContext::BindTextureUploadContext()
{
  return m_eglContext.BindTextureUploadContext();
}

bool CWinSystemGbmEGLContext::UnbindTextureUploadContext()
{
  return m_eglContext.UnbindTextureUploadContext();
}

bool CWinSystemGbmEGLContext::HasContext()
{
  return m_eglContext.HasContext();
}
