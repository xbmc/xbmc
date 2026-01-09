/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemGbmEGLContext.h"

#include "OptionalsReg.h"
#include "ServiceBroker.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/log.h"

using namespace KODI::WINDOWING::GBM;
using namespace KODI::WINDOWING::LINUX;

bool CWinSystemGbmEGLContext::InitWindowSystemEGL(EGLint renderableType, EGLint apiType)
{
  if (!CWinSystemGbm::InitWindowSystem())
  {
    return false;
  }

  if (!m_eglContext.CreatePlatformDisplay(m_GBM->GetDevice().Get(), m_GBM->GetDevice().Get()))
  {
    return false;
  }

  if (!m_eglContext.InitializeDisplay(apiType))
  {
    return false;
  }

  auto guiformats = m_DRM->GetGuiFormats();
  if (!std::ranges::any_of(guiformats,
                           [&](struct guiformat format)
                           {
                             return format.active &&
                                    m_eglContext.ChooseConfig(renderableType, format.drm, false,
                                                              format.alpha);
                           }))
  {
    return false;
  }

  CServiceBroker::GetSettingsComponent()
      ->GetSettings()
      ->GetSetting(CSettings::SETTING_VIDEOSCREEN_USEMODIFIERS)
      ->SetVisible(true);

  if (!CreateContext())
  {
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
  bool useModifiers = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
      CSettings::SETTING_VIDEOSCREEN_USEMODIFIERS);

  std::vector<uint64_t> fallbackModifiers = {DRM_FORMAT_MOD_LINEAR};
  std::vector<uint64_t>* modifiers = &fallbackModifiers;

  if (useModifiers)
  {
    for (auto& fmt : m_DRM->GetGuiFormats())
    {
      if (fmt.drm == format && fmt.active)
      {
        modifiers = &fmt.modifiers;
        break;
      }
    }
  }

  if (!m_GBM->GetDevice().CreateSurface(res.iWidth, res.iHeight, format, modifiers->data(),
                                        modifiers->size()))
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

  if (!m_eglContext.TrySwapBuffers())
  {
    return false;
  }

  struct gbm_bo* bo = m_GBM->GetDevice().GetSurface().LockFrontBuffer().Get();

#if defined(HAS_GBM_MODIFIERS)
  uint64_t modifier = gbm_bo_get_modifier(bo);
#else
  uint64_t modifier = DRM_FORMAT_MOD_LINEAR;
#endif
  if (!m_DRM->FindGuiPlane(gbm_bo_get_format(bo), modifier))
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
