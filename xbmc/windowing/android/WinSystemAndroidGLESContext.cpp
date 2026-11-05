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
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "windowing/WindowSystemFactory.h"

#include "platform/android/activity/XBMCApp.h"

#include <memory>

#include <unistd.h>

#include "PlatformDefs.h"

#include <EGL/eglext.h>

extern "C"
{
#include <libavutil/csp.h>
}

CWinSystemAndroidGLESContext::~CWinSystemAndroidGLESContext() = default;

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
  m_nWidth = newWidth;
  m_nHeight = newHeight;
  CRenderSystemGLES::ResetRenderSystem(newWidth, newHeight);
  return true;
}

bool CWinSystemAndroidGLESContext::DestroyRenderSystem()
{
  SetGuiCompositing(0);
  return CRenderSystemGLES::DestroyRenderSystem();
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
  if (videoPicture && !IsHDRDisplaySettingEnabled())
    return false;

  EGLint HDRColorSpace = EGL_NONE;

#if EGL_EXT_gl_colorspace_bt2020_pq
  if (m_hasHDRConfig && m_hasEGL_BT2020_PQ_Colorspace_Extension && m_hasEGL_ST2086_Extension)
  {
    // Mastering display / content light level metadata is optional and does not describe the
    // transfer function, so the tagging is decided from the transfer characteristic. SDR and
    // HLG (ARIB STD-B67 or BT.2020 10-bit) are never PQ, which also covers Dolby Vision with
    // an SDR or HLG base layer. FFmpeg can report HDR10 with an unspecified transfer, in that
    // case use the HDR type inferred by the demuxer (see CDVDDemuxFFmpeg::DetermineHdrType).
    const bool isPQ =
        videoPicture && (videoPicture->color_transfer == AVCOL_TRC_SMPTE2084 ||
                         (videoPicture->color_transfer == AVCOL_TRC_UNSPECIFIED &&
                          (videoPicture->hdrType == StreamHdrType::HDR_TYPE_HDR10 ||
                           videoPicture->hdrType == StreamHdrType::HDR_TYPE_HDR10PLUS)));
    HDRColorSpace = isPQ ? EGL_GL_COLORSPACE_BT2020_PQ_EXT : EGL_NONE;

    const bool hasDisplayMetadata = isPQ && videoPicture->hasDisplayMetadata;
    // The metadata is applied at surface creation, so a change in its presence needs a new
    // surface just like a colourspace change does
    const bool displayMetadataChanged = static_cast<bool>(m_displayMetadata) != hasDisplayMetadata;

    if (!hasDisplayMetadata)
    {
      m_displayMetadata = nullptr;
      m_lightMetadata = nullptr;
    }

    if (HDRColorSpace != m_HDRColorSpace || displayMetadataChanged)
    {
      if (!m_nativeWindow)
      {
        if (HDRColorSpace != EGL_NONE)
          return false;
        m_HDRColorSpace = EGL_NONE;
        m_displayMetadata.reset();
        m_lightMetadata.reset();
        return true;
      }

      CLog::Log(LOGDEBUG, "CWinSystemAndroidGLESContext::SetHDR: ColorSpace: {}", HDRColorSpace);

      m_HDRColorSpace = HDRColorSpace;
      m_displayMetadata = nullptr;
      if (hasDisplayMetadata)
        m_displayMetadata =
            std::make_unique<AVMasteringDisplayMetadata>(videoPicture->displayMetadata);
      // TODO: discuss with NVIDIA why this prevent turning HDR display off
      //m_lightMetadata = !videoPicture || m_HDRColorSpace == EGL_NONE ? nullptr : std::unique_ptr<AVContentLightMetadata>(new AVContentLightMetadata(videoPicture->lightMetadata));
      m_pGLContext.DestroySurface();
      if (!CreateSurface() || !m_pGLContext.BindContext())
      {
        m_HDRColorSpace = EGL_NONE;
        m_displayMetadata.reset();
        m_lightMetadata.reset();
        m_pGLContext.DestroySurface();
        if (!CreateSurface() || !m_pGLContext.BindContext())
          CLog::Log(LOGERROR, "CWinSystemAndroidGLESContext: failed to restore SDR surface");
        return false;
      }

      EGLint colorSpace = EGL_NONE;
      if (eglQuerySurface(GetEGLDisplay(), GetEGLSurface(), EGL_GL_COLORSPACE_KHR, &colorSpace))
        CLog::Log(LOGDEBUG, "CWinSystemAndroidGLESContext::SetHDR: EGL surface colorspace: {:#x}",
                  colorSpace);
    }
  }
#endif

  return m_HDRColorSpace == HDRColorSpace && (!videoPicture || HDRColorSpace != EGL_NONE);
}

bool CWinSystemAndroidGLESContext::SupportsHDRGameOutput()
{
  const auto player = CServiceBroker::GetAppComponents().GetComponent<CApplicationPlayer>();
  if (!player->IsPlayingGame() || !IsHDRGameOutputValid() || !m_hasHDRConfig ||
      !m_hasEGL_BT2020_PQ_Colorspace_Extension || !m_hasEGL_ST2086_Extension ||
      !IsHDRDisplaySettingEnabled() || !GetDisplayHDRCapabilities().SupportsHDR10())
    return false;

  return m_pGLContext.GetConfigAttrib(EGL_COLOR_COMPONENT_TYPE_EXT, true) ==
             EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT &&
         m_pGLContext.GetConfigAttrib(EGL_RED_SIZE, true) == 16 &&
         m_pGLContext.GetConfigAttrib(EGL_GREEN_SIZE, true) == 16 &&
         m_pGLContext.GetConfigAttrib(EGL_BLUE_SIZE, true) == 16 &&
         m_pGLContext.GetConfigAttrib(EGL_ALPHA_SIZE, true) == 16;
}

bool CWinSystemAndroidGLESContext::IsHDRGameOutputValid() const
{
  // Larger GUI surfaces are not yet validated for Android HDR compositing.
  return m_nativeWindow && GetEGLSurface() != EGL_NO_SURFACE && m_nWidth == 1920 && m_nHeight == 1080 &&
         (!m_guiComposite.IsActive() || m_HDRColorSpace == EGL_GL_COLORSPACE_BT2020_PQ_EXT);
}

bool CWinSystemAndroidGLESContext::SetVideoOutput(const VideoPicture* videoPicture)
{
  if (!videoPicture)
    return true;

  const auto player = CServiceBroker::GetAppComponents().GetComponent<CApplicationPlayer>();
  if (player->IsPlayingGame())
    return videoPicture->color_transfer == AVCOL_TRC_SMPTE2084 && SupportsHDRGameOutput();

  // Android switches its single EGL surface in SetHDR, including the output format.
  return videoPicture->color_transfer == AVCOL_TRC_SMPTE2084 && m_hasHDRConfig &&
         m_hasEGL_BT2020_PQ_Colorspace_Extension && m_hasEGL_ST2086_Extension &&
         IsHDRDisplaySettingEnabled() && GetDisplayHDRCapabilities().SupportsHDR10();
}

bool CWinSystemAndroidGLESContext::SetGuiCompositing(int colorTransfer)
{
  if (colorTransfer == 0)
  {
    m_guiComposite.Disable();
    return false;
  }

  if (colorTransfer != AVCOL_TRC_SMPTE2084 || !SupportsHDRGameOutput() ||
      m_HDRColorSpace != EGL_GL_COLORSPACE_BT2020_PQ_EXT)
    return false;

  // Android's GUI setting supplies PQ white; the LUT uses normalized linear light.
  const float sdrPeak = static_cast<float>(
      av_csp_trc_func_inv_from_id(AVCOL_TRC_SMPTE2084)(
          static_cast<double>(GetGuiSdrPeakLuminance())) /
      10000.0);
  if (!m_guiComposite.Enable(colorTransfer, false, sdrPeak) ||
      !m_guiComposite.Prepare(m_nWidth, m_nHeight, GetEnabledFrontToBackRendering()))
  {
    CLog::Log(LOGERROR, "CWinSystemAndroidGLESContext: failed to initialize HDR GUI compositing");
    m_guiComposite.Disable();
    return false;
  }

  return true;
}

bool CWinSystemAndroidGLESContext::BeginGuiComposite(bool guiWillRender)
{
  if (!guiWillRender || !m_guiComposite.IsActive() || !IsHDRGameOutputValid())
    return false;

  m_guiComposite.SetSdrPeak(static_cast<float>(
      av_csp_trc_func_inv_from_id(AVCOL_TRC_SMPTE2084)(
          static_cast<double>(GetGuiSdrPeakLuminance())) /
      10000.0));
  return m_guiComposite.Begin(guiWillRender, m_nWidth, m_nHeight, GetEnabledFrontToBackRendering());
}

void CWinSystemAndroidGLESContext::EndGuiComposite()
{
  m_guiComposite.End(false);
}

void CWinSystemAndroidGLESContext::CompositeGui()
{
  m_guiComposite.Composite(*this, false, GetGUIElementCount());
}

int CWinSystemAndroidGLESContext::GetOutputBitDepth() const
{
  return m_pGLContext.GetConfigAttrib(EGL_RED_SIZE, m_HDRColorSpace != EGL_NONE);
}

KODI::UTILS::Eotf CWinSystemAndroidGLESContext::GetEotf() const
{
  return m_HDRColorSpace == EGL_GL_COLORSPACE_BT2020_PQ_EXT ? KODI::UTILS::Eotf::PQ
                                                            : KODI::UTILS::Eotf::TRADITIONAL_SDR;
}

KODI::UTILS::Colorimetry CWinSystemAndroidGLESContext::GetColorimetry() const
{
  return m_HDRColorSpace == EGL_GL_COLORSPACE_BT2020_PQ_EXT ? KODI::UTILS::Colorimetry::BT2020_RGB
                                                            : KODI::UTILS::Colorimetry::DEFAULT;
}
