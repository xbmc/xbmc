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
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
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

  if (!ChooseEGLConfig(renderableType))
  {
    m_eglContext.Destroy();
    return false;
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

bool CWinSystemGbmEGLContext::ChooseEGLConfig(EGLint renderableType, int bitDepth)
{
  // Pick the highest active output format whose bpp is in range for the
  // requested bit depth. Policy: when bitDepth > 8, accept only entries
  // with bpp >= 10 and bpp <= bitDepth (do not silently fall back to 8).
  // When bitDepth <= 8, accept entries with bpp <= 8.
  auto outputformats = m_DRM->GetOutputFormats();

  std::vector<outputformat> candidates;
  std::ranges::copy_if(outputformats, std::back_inserter(candidates),
                       [bitDepth](const outputformat& format)
                       {
                         if (!format.active)
                           return false;
                         if (bitDepth > 8)
                           return format.bpp >= 10 && format.bpp <= bitDepth;
                         return format.bpp <= 8;
                       });
  std::ranges::sort(candidates, std::greater<>{}, &outputformat::bpp);

  for (const auto& format : candidates)
  {
    if (m_eglContext.ChooseConfig(renderableType, format.drm, false, format.alpha))
      return true;
  }

  return false;
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
  std::vector<uint64_t> fallbackModifiers = {DRM_FORMAT_MOD_LINEAR};
  std::vector<uint64_t>* modifiers = &fallbackModifiers;

  for (auto& fmt : m_DRM->GetOutputFormats())
  {
    if (fmt.drm == format && fmt.active)
    {
      modifiers = &fmt.modifiers;
      break;
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
  if (!bo)
  {
    CLog::Log(LOGERROR, "CWinSystemGbmEGLContext::{} - failed to lock front buffer", __FUNCTION__);
    return false;
  }

#if defined(HAS_GBM_MODIFIERS)
  uint64_t modifier = gbm_bo_get_modifier(bo);
#else
  uint64_t modifier = DRM_FORMAT_MOD_LINEAR;
#endif
  // Offscreen has no CRTC and no scanout planes; FindGuiPlane has nothing to
  // assign, so skip the call rather than violate its post-condition that
  // m_gui_plane is set on success.
  if (m_DRM->GetCrtc() && !m_DRM->FindGuiPlane(gbm_bo_get_format(bo), modifier))
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

bool CWinSystemGbmEGLContext::SetVideoOutput(const VideoPicture* videoPicture)
{
  // Override: the base only flips the gui/video plane role. On EGL
  // backends we additionally rebuild the GBM and EGL output surface
  // to a wider format (e.g. AR24 -> AR30) when the video needs more
  // than 8-bit through the single output plane, then chain to the
  // base so plane-role tracking sees the new format. EGL context survives
  // across the rebuild, no shader-rebuild. The next flip forces a modeset
  // when needed (amdgpu), on other drivers it is a fast flip."
  const int bitDepth = videoPicture ? videoPicture->colorBits : 8;

  // Pick an EGL config matching the target bit depth, then rebuild
  // the surface only if that pick changed the native visual ID.
  uint32_t currentFormat = m_eglContext.GetConfigAttrib(EGL_NATIVE_VISUAL_ID);

  if (!ChooseEGLConfig(m_renderableType, bitDepth))
  {
    CLog::LogF(LOGERROR, "failed to choose EGL config for {}-bit", bitDepth);
    return false;
  }

  uint32_t format = m_eglContext.GetConfigAttrib(EGL_NATIVE_VISUAL_ID);

  if (format != currentFormat)
  {
    m_eglContext.DestroySurface();

    std::vector<uint64_t> fallbackModifiers = {DRM_FORMAT_MOD_LINEAR};
    std::vector<uint64_t>* modifiers = &fallbackModifiers;

    for (auto& fmt : m_DRM->GetOutputFormats())
    {
      if (fmt.drm == format && fmt.active)
      {
        modifiers = &fmt.modifiers;
        break;
      }
    }

    if (!m_GBM->GetDevice().CreateSurface(m_nWidth, m_nHeight, format, modifiers->data(),
                                          modifiers->size()))
    {
      CLog::LogF(LOGERROR, "failed to create GBM surface");
      return false;
    }

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

    CLog::LogF(LOGINFO, "Output surface recreated at {}-bit", bitDepth);

    // amdgpu disables the CRTC when RMFB removes a still-bound FB during
    // surface destroy (primary-plane invariant). Force a full modeset on
    // the next commit to re-enable the CRTC. No-op on drivers without the
    // quirk (Intel, RPi, etc.) so they keep the fast-flip path.
    if (m_DRM->HasQuirk(KODI::WINDOWING::GBM::QUIRK_NEEDSPRIMARY))
      m_DRM->SetActive(true);

    // The chained base reads the active plane's cached format and
    // modifier to decide which DRM plane satisfies the new role.
    // After a surface rebuild that cache is stale, so refresh it from
    // the new GBM front buffer (made available by TrySwapBuffers above)
    // before chaining.
    if (auto* plane = m_DRM->GetGuiPlane() ? m_DRM->GetGuiPlane() : m_DRM->GetVideoPlane())
    {
      if (struct gbm_bo* bo = m_GBM->GetDevice().GetSurface().LockFrontBuffer().Get())
      {
        plane->SetFormat(gbm_bo_get_format(bo));
#if defined(HAS_GBM_MODIFIERS)
        plane->SetModifier(gbm_bo_get_modifier(bo));
#else
        plane->SetModifier(DRM_FORMAT_MOD_LINEAR);
#endif
      }
    }
  }

  // Chain to base for the fine-plane part: routes through FindVideoPlane
  // if videoPicture set or FindGuiPlane if videoPicture was nullptr, using
  // the now-current cached format/modifier on the active plane.
  return CWinSystemGbm::SetVideoOutput(videoPicture);
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
