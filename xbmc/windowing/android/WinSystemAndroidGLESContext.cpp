/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemAndroidGLESContext.h"

#include "ServiceBroker.h"
#include "VideoSyncAndroid.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "windowing/WindowSystemFactory.h"

#include "platform/android/activity/XBMCApp.h"

#include <memory>

#include <unistd.h>

#include "PlatformDefs.h"

#include <EGL/eglext.h>

// Resolve ANativeWindow_setBuffersDataSpace() at runtime from
// libnativewindow.so. The symbol is API 28+, while Kodi targets a lower
// Android API, so dynamic lookup lets the same binary run on older
// devices and use the API only when available.
#include <android/native_window.h>
#include <dlfcn.h>

namespace
{
// Hardcode ADATASPACE_UNKNOWN and ADATASPACE_BT2020_PQ because
// <android/data_space.h> is not guaranteed at the current NDK target.
// Values are from AOSP libsystem/include/system/graphics-base-v1.0.h.
constexpr int32_t KODI_ADATASPACE_UNKNOWN = 0;
constexpr int32_t KODI_ADATASPACE_BT2020_PQ = 163971072;

using PFN_ANativeWindow_setBuffersDataSpace = int32_t (*)(ANativeWindow*, int32_t);

PFN_ANativeWindow_setBuffersDataSpace GetSetBuffersDataSpaceFn()
{
  static const PFN_ANativeWindow_setBuffersDataSpace fn = []()
  {
    void* handle = dlopen("libnativewindow.so", RTLD_NOW);
    void* sym = handle ? dlsym(handle, "ANativeWindow_setBuffersDataSpace") : nullptr;
    if (!sym)
      CLog::Log(LOGDEBUG,
                "CWinSystemAndroidGLESContext: ANativeWindow_setBuffersDataSpace not resolvable");
    return reinterpret_cast<PFN_ANativeWindow_setBuffersDataSpace>(sym);
  }();
  return fn;
}
} // namespace

void CWinSystemAndroidGLESContext::Register()
{
  KODI::WINDOWING::CWindowSystemFactory::RegisterWindowSystem(CreateWinSystem);
}

std::unique_ptr<CWinSystemBase> CWinSystemAndroidGLESContext::CreateWinSystem()
{
  return std::make_unique<CWinSystemAndroidGLESContext>();
}

bool CWinSystemAndroidGLESContext::InitWindowSystem()
{
  if (!CWinSystemAndroid::InitWindowSystem())
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

  m_hasHDRConfig = m_pGLContext.ChooseConfig(EGL_OPENGL_ES2_BIT, 0, true);

  m_hasEGL_BT2020_PQ_Colorspace_Extension =
      CEGLUtils::HasExtension(m_pGLContext.GetEGLDisplay(), "EGL_EXT_gl_colorspace_bt2020_pq");
  m_hasEGL_ST2086_Extension =
      CEGLUtils::HasExtension(m_pGLContext.GetEGLDisplay(), "EGL_EXT_surface_SMPTE2086_metadata");

  bool hasEGLHDRExtensions = m_hasEGL_BT2020_PQ_Colorspace_Extension && m_hasEGL_ST2086_Extension;

  CLog::Log(LOGDEBUG,
            "CWinSystemAndroidGLESContext::InitWindowSystem: HDRConfig: {}, HDRExtensions: {}",
            static_cast<int>(m_hasHDRConfig), static_cast<int>(hasEGLHDRExtensions));

  CEGLAttributesVec contextAttribs;
  contextAttribs.Add({{EGL_CONTEXT_CLIENT_VERSION, 2}});

  if (!m_pGLContext.CreateContext(contextAttribs))
  {
    return false;
  }

  return true;
}

bool CWinSystemAndroidGLESContext::CreateNewWindow(const std::string& name,
                                               bool fullScreen,
                                               RESOLUTION_INFO& res)
{
  m_pGLContext.DestroySurface();

  if (!CWinSystemAndroid::CreateNewWindow(name, fullScreen, res))
  {
    return false;
  }

  if (!CreateSurface())
  {
    return false;
  }

  if (!m_pGLContext.BindContext())
  {
    return false;
  }

  return true;
}

bool CWinSystemAndroidGLESContext::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  CRenderSystemGLES::ResetRenderSystem(newWidth, newHeight);
  return true;
}

bool CWinSystemAndroidGLESContext::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  CreateNewWindow("", fullScreen, res);
  CRenderSystemGLES::ResetRenderSystem(res.iWidth, res.iHeight);
  return true;
}

void CWinSystemAndroidGLESContext::SetVSyncImpl(bool enable)
{
  // We use Choreographer for timing
  m_pGLContext.SetVSync(false);
}

void CWinSystemAndroidGLESContext::PresentRenderImpl(bool rendered)
{
  if (!m_nativeWindow)
  {
    usleep(10000);
    return;
  }

  // Mode change finalization was triggered by timer
  if (IsHdmiModeTriggered())
    SetHdmiState(true);

  // Ignore EGL_BAD_SURFACE: It seems to happen during/after mode changes, but
  // we can't actually do anything about it
  if (rendered && !m_pGLContext.TrySwapBuffers())
    CEGLUtils::Log(LOGERROR, "eglSwapBuffers failed");

  CXBMCApp::Get().WaitVSync(1000);
}

float CWinSystemAndroidGLESContext::GetFrameLatencyAdjustment()
{
  return CXBMCApp::Get().GetFrameLatencyMs();
}

EGLDisplay CWinSystemAndroidGLESContext::GetEGLDisplay() const
{
  return m_pGLContext.GetEGLDisplay();
}

EGLSurface CWinSystemAndroidGLESContext::GetEGLSurface() const
{
  return m_pGLContext.GetEGLSurface();
}

EGLContext CWinSystemAndroidGLESContext::GetEGLContext() const
{
  return m_pGLContext.GetEGLContext();
}

EGLConfig  CWinSystemAndroidGLESContext::GetEGLConfig() const
{
  return m_pGLContext.GetEGLConfig();
}

std::unique_ptr<CVideoSync> CWinSystemAndroidGLESContext::GetVideoSync(CVideoReferenceClock* clock)
{
  std::unique_ptr<CVideoSync> pVSync(new CVideoSyncAndroid(clock));
  return pVSync;
}

bool CWinSystemAndroidGLESContext::CreateSurface()
{
  if (!m_pGLContext.CreateSurface(static_cast<EGLNativeWindowType>(m_nativeWindow->GetWindow()),
                                  m_HDRColorSpace))
  {
    if (m_HDRColorSpace != EGL_NONE)
    {
      m_HDRColorSpace = EGL_NONE;
      m_displayMetadata = nullptr;
      m_lightMetadata = nullptr;
      if (!m_pGLContext.CreateSurface(
              static_cast<EGLNativeWindowType>(m_nativeWindow->GetWindow())))
        return false;
    }
    else
      return false;
  }

#if EGL_EXT_surface_SMPTE2086_metadata
  if (m_displayMetadata)
  {
    m_pGLContext.SurfaceAttrib(EGL_SMPTE2086_DISPLAY_PRIMARY_RX_EXT, static_cast<int>(av_q2d(m_displayMetadata->display_primaries[0][0]) * EGL_METADATA_SCALING_EXT + 0.5));
    m_pGLContext.SurfaceAttrib(EGL_SMPTE2086_DISPLAY_PRIMARY_RY_EXT, static_cast<int>(av_q2d(m_displayMetadata->display_primaries[0][1]) * EGL_METADATA_SCALING_EXT + 0.5));
    m_pGLContext.SurfaceAttrib(EGL_SMPTE2086_DISPLAY_PRIMARY_GX_EXT, static_cast<int>(av_q2d(m_displayMetadata->display_primaries[1][0]) * EGL_METADATA_SCALING_EXT + 0.5));
    m_pGLContext.SurfaceAttrib(EGL_SMPTE2086_DISPLAY_PRIMARY_GY_EXT, static_cast<int>(av_q2d(m_displayMetadata->display_primaries[1][1]) * EGL_METADATA_SCALING_EXT + 0.5));
    m_pGLContext.SurfaceAttrib(EGL_SMPTE2086_DISPLAY_PRIMARY_BX_EXT, static_cast<int>(av_q2d(m_displayMetadata->display_primaries[2][0]) * EGL_METADATA_SCALING_EXT + 0.5));
    m_pGLContext.SurfaceAttrib(EGL_SMPTE2086_DISPLAY_PRIMARY_BY_EXT, static_cast<int>(av_q2d(m_displayMetadata->display_primaries[2][1]) * EGL_METADATA_SCALING_EXT + 0.5));
    m_pGLContext.SurfaceAttrib(EGL_SMPTE2086_WHITE_POINT_X_EXT, static_cast<int>(av_q2d(m_displayMetadata->white_point[0]) * EGL_METADATA_SCALING_EXT + 0.5));
    m_pGLContext.SurfaceAttrib(EGL_SMPTE2086_WHITE_POINT_Y_EXT, static_cast<int>(av_q2d(m_displayMetadata->white_point[1]) * EGL_METADATA_SCALING_EXT + 0.5));
    m_pGLContext.SurfaceAttrib(EGL_SMPTE2086_MAX_LUMINANCE_EXT, static_cast<int>(av_q2d(m_displayMetadata->max_luminance) * EGL_METADATA_SCALING_EXT + 0.5));
    m_pGLContext.SurfaceAttrib(EGL_SMPTE2086_MIN_LUMINANCE_EXT, static_cast<int>(av_q2d(m_displayMetadata->min_luminance) * EGL_METADATA_SCALING_EXT + 0.5));
  }
  if (m_lightMetadata)
  {
    m_pGLContext.SurfaceAttrib(EGL_CTA861_3_MAX_CONTENT_LIGHT_LEVEL_EXT, static_cast<int>(m_lightMetadata->MaxCLL * EGL_METADATA_SCALING_EXT));
    m_pGLContext.SurfaceAttrib(EGL_CTA861_3_MAX_FRAME_AVERAGE_LEVEL_EXT, static_cast<int>(m_lightMetadata->MaxFALL * EGL_METADATA_SCALING_EXT));
  }
#endif
  return true;
}

bool CWinSystemAndroidGLESContext::IsHDRDisplay()
{
  return m_hasHDRConfig && (m_hasEGL_BT2020_PQ_Colorspace_Extension || m_hasEGL_ST2086_Extension) &&
         CWinSystemAndroid::IsHDRDisplay();
}

bool CWinSystemAndroidGLESContext::SetHDR(const VideoPicture* videoPicture)
{
  if (!CServiceBroker::GetWinSystem()->IsHDRDisplaySettingEnabled())
    return false;

  // Tag the GUI surface's ANativeWindow as BT.2020 PQ for genuine HDR
  // content, and reset it to UNKNOWN otherwise. This lets the compositor
  // interpret PGS overlays treated as PQ correctly. Unlike the EGL path
  // below, this applies to subsequently-queued buffers and needs no
  // surface recreation. A rejected dataspace leaves the surface untagged;
  // PGS rendering then falls back to conversion (see
  // OverlayRendererGLES.cpp).
  //
  // SetHDR() is called for every video, HDR or SDR, so gate on the video's
  // actual HDR-ness, not simply videoPicture != nullptr. Only HdrPgsMode::AUTO
  // attempts tagging at all: HdrPgsMode::FORCE_CONVERSION leaves the surface
  // untagged on purpose (a "successful" tag has been observed to cause
  // compositor problems independent of which PGS shader is later selected,
  // on at least one platform), and HdrPgsMode::OFF disables the feature
  // outright.
  const auto settingsComponent = CServiceBroker::GetSettingsComponent();
  const auto settings = settingsComponent ? settingsComponent->GetSettings() : nullptr;
  const auto hdrPgsMode =
      settings
          ? static_cast<HdrPgsMode>(settings->GetInt(CSettings::SETTING_VIDEOPLAYER_HDRPGSMODE))
          : HdrPgsMode::AUTO;

  // color_transfer is not always populated for genuine PQ content on Android,
  // so mirror CRendererMediaCodecSurface::Configure()'s two-way condition.
  // IsTransferPQ() is deliberately not used here: SetHDR() is also reachable
  // through CLinuxRendererGLES::Configure() for SurfaceTexture-backed
  // MediaCodec buffers, where IsTransferPQ() may not have been set yet.
  // Derive from videoPicture directly so tagging does not depend on which
  // renderer happened to run first.
  const bool isRealHdrMode =
      videoPicture && (videoPicture->color_transfer == AVCOL_TRC_SMPTE2084 ||
                       videoPicture->hdrType == StreamHdrType::HDR_TYPE_DOLBYVISION);

  // The ANativeWindow_setBuffersDataSpace() call itself reports whether this
  // dataspace is supported; no additional display-capability gate is needed.
  const bool attemptTagging = hdrPgsMode == HdrPgsMode::AUTO && isRealHdrMode;

  m_nativeWindowTaggedPQ = false;
  if (m_nativeWindow && m_nativeWindow->GetWindow())
  {
    if (const auto setBuffersDataSpace = GetSetBuffersDataSpaceFn())
    {
      const int32_t targetDataSpace =
          attemptTagging ? KODI_ADATASPACE_BT2020_PQ : KODI_ADATASPACE_UNKNOWN;
      const int32_t result = setBuffersDataSpace(m_nativeWindow->GetWindow(), targetDataSpace);
      m_nativeWindowTaggedPQ = attemptTagging && (result == 0);
      if (attemptTagging && result != 0)
        CLog::Log(LOGDEBUG,
                  "CWinSystemAndroidGLESContext::SetHDR: BT.2020 PQ dataspace rejected for the "
                  "GUI surface (error {}) - HDR PGS subtitles will use SDR conversion",
                  result);
    }
  }

  EGLint HDRColorSpace = 0;

#if EGL_EXT_gl_colorspace_bt2020_linear
  if (m_hasHDRConfig && m_hasEGL_BT2020_PQ_Colorspace_Extension && m_hasEGL_ST2086_Extension)
  {
    HDRColorSpace = EGL_NONE;
    if (videoPicture && videoPicture->hasDisplayMetadata)
    {
      switch (videoPicture->color_space)
      {
      case AVCOL_SPC_BT2020_NCL:
      case AVCOL_SPC_BT2020_CL:
      case AVCOL_SPC_BT709:
        HDRColorSpace = EGL_GL_COLORSPACE_BT2020_PQ_EXT;
        break;
      default:
        m_displayMetadata = nullptr;
        m_lightMetadata = nullptr;
      }
    }
    else
    {
      m_displayMetadata = nullptr;
      m_lightMetadata = nullptr;
    }

    if (HDRColorSpace != m_HDRColorSpace)
    {
      CLog::Log(LOGDEBUG, "CWinSystemAndroidGLESContext::SetHDR: ColorSpace: {}", HDRColorSpace);

      m_HDRColorSpace = HDRColorSpace;
      m_displayMetadata =
          m_HDRColorSpace == EGL_NONE
              ? nullptr
              : std::make_unique<AVMasteringDisplayMetadata>(videoPicture->displayMetadata);
      // TODO: discuss with NVIDIA why this prevent turning HDR display off
      //m_lightMetadata = !videoPicture || m_HDRColorSpace == EGL_NONE ? nullptr : std::unique_ptr<AVContentLightMetadata>(new AVContentLightMetadata(videoPicture->lightMetadata));
      m_pGLContext.DestroySurface();
      CreateSurface();
      m_pGLContext.BindContext();
    }
  }
#endif

  return m_HDRColorSpace == HDRColorSpace;
}

bool CWinSystemAndroidGLESContext::IsGuiHdrPQTagged() const
{
  return m_HDRColorSpace == EGL_GL_COLORSPACE_BT2020_PQ_EXT || m_nativeWindowTaggedPQ;
}
