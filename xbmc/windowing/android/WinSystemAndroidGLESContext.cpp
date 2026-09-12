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
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodecAndroidMediaCodec.h"
#include "cores/VideoPlayer/Interface/StreamInfo.h"
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

  // GLES 3.0 required for sized texture formats (GL_R16F, GL_RGB10_A2, etc.)
  // Fall back to GLES 2.0 for devices that don't support GLES 3.0
  if (!InitWindowSystemEGL(EGL_OPENGL_ES3_BIT) && !InitWindowSystemEGL(EGL_OPENGL_ES2_BIT))
  {
    return false;
  }

  m_hasHDRConfig = m_pGLContext.ChooseConfig(m_renderableType, 0, true);

  m_hasEGL_BT2020_PQ_Colorspace_Extension =
      CEGLUtils::HasExtension(m_pGLContext.GetEGLDisplay(), "EGL_EXT_gl_colorspace_bt2020_pq");
  m_hasEGL_ST2086_Extension =
      CEGLUtils::HasExtension(m_pGLContext.GetEGLDisplay(), "EGL_EXT_surface_SMPTE2086_metadata");

  bool hasEGLHDRExtensions = m_hasEGL_BT2020_PQ_Colorspace_Extension && m_hasEGL_ST2086_Extension;

  CLog::Log(LOGDEBUG,
            "CWinSystemAndroidGLESContext::InitWindowSystem: HDRConfig: {}, HDRExtensions: {}",
            static_cast<int>(m_hasHDRConfig), static_cast<int>(hasEGLHDRExtensions));

  return true;
}

bool CWinSystemAndroidGLESContext::InitWindowSystemEGL(EGLint renderableType)
{
  if (!m_pGLContext.CreateDisplay(m_nativeDisplay))
  {
    return false;
  }

  if (!m_pGLContext.InitializeDisplay(EGL_OPENGL_ES_API))
  {
    return false;
  }

  // ChooseConfig destroys the display when no config matches (or on an EGL
  // error), so the retry with the other renderable type starts again from
  // CreateDisplay.
  if (!m_pGLContext.ChooseConfig(renderableType))
  {
    return false;
  }

  const EGLint version = (renderableType == EGL_OPENGL_ES3_BIT) ? 3 : 2;

  CEGLAttributesVec contextAttribs;
  contextAttribs.Add({{EGL_CONTEXT_CLIENT_VERSION, version}});

  if (!m_pGLContext.CreateContext(contextAttribs))
  {
    m_pGLContext.Destroy();
    return false;
  }

  m_renderableType = renderableType;
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
  // The float config is for a surface the video is drawn into; a GUI surface
  // over a separate video surface keeps the RGBA8 config.
  const bool hdrConfig = m_HDRColorSpace != EGL_NONE && !m_videoOnSeparateSurface;

  if (!m_pGLContext.CreateSurface(static_cast<EGLNativeWindowType>(m_nativeWindow->GetWindow()),
                                  m_HDRColorSpace, hdrConfig))
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

bool CWinSystemAndroidGLESContext::SetVideoOutput(const VideoPicture* videoPicture)
{
  // The MediaCodec surface renderer posts frames to its own SurfaceView, which
  // the system compositor blends the GUI surface over; every other renderer
  // draws the video into the GUI surface.
  const CMediaCodecVideoBuffer* buffer =
      videoPicture ? dynamic_cast<CMediaCodecVideoBuffer*>(videoPicture->videoBuffer) : nullptr;
  m_videoOnSeparateSurface = buffer && !buffer->HasSurfaceTexture();
  return true;
}

bool CWinSystemAndroidGLESContext::SetHDR(const VideoPicture* videoPicture)
{
  EGLint colorSpace = EGL_NONE;

#if EGL_EXT_gl_colorspace_bt2020_pq
  if (videoPicture && m_hasEGL_BT2020_PQ_Colorspace_Extension)
  {
    const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    const bool hdrDisplay = settings && settings->GetBool(SETTING_WINSYSTEM_IS_HDR_DISPLAY) &&
                            CWinSystemAndroid::IsHDRDisplay();

    // PQ and HLG (see SetGuiCompositing) get a PQ GUI surface; Dolby Vision
    // too when the display supports it
    const bool hdr = videoPicture->color_transfer == AVCOL_TRC_SMPTE2084 ||
                     videoPicture->color_transfer == AVCOL_TRC_ARIB_STD_B67 ||
                     (videoPicture->hdrType == StreamHdrType::HDR_TYPE_DOLBYVISION &&
                      GetDisplayHDRCapabilities().SupportsDolbyVision());

    // A GUI surface over a separate video surface only declares its colorspace;
    // the video's mastering metadata reached MediaCodec as KEY_HDR_STATIC_INFO.
    // A surface the video is drawn into needs the float config and also carries
    // the SMPTE2086 attributes.
    const bool supported =
        m_videoOnSeparateSurface || (m_hasHDRConfig && m_hasEGL_ST2086_Extension);

    if (hdrDisplay && hdr && supported)
      colorSpace = EGL_GL_COLORSPACE_BT2020_PQ_EXT;
  }
#endif

  if (colorSpace != m_HDRColorSpace)
  {
    CLog::Log(LOGDEBUG, "CWinSystemAndroidGLESContext::SetHDR: ColorSpace: {}", colorSpace);

    const bool copyMetadata =
        colorSpace != EGL_NONE && !m_videoOnSeparateSurface && videoPicture->hasDisplayMetadata;

    m_HDRColorSpace = colorSpace;
    m_displayMetadata =
        copyMetadata ? std::make_unique<AVMasteringDisplayMetadata>(videoPicture->displayMetadata)
                     : nullptr;
    //! @todo Light metadata is not passed: with the CTA861.3 attributes set,
    //! NVIDIA devices did not turn HDR off again.
    m_pGLContext.DestroySurface();
    CreateSurface();
    m_pGLContext.BindContext();
  }

  return m_HDRColorSpace != EGL_NONE;
}

bool CWinSystemAndroidGLESContext::SetGuiCompositing(int colorTransfer)
{
  //! @todo The EGL headers define no HLG colorspace, so SetHDR declares the GUI
  //! surface PQ for HLG video too; composite the GUI to PQ to match it.
  if (colorTransfer == AVCOL_TRC_ARIB_STD_B67)
    colorTransfer = AVCOL_TRC_SMPTE2084;

  return m_guiComposite.Enable(colorTransfer, UseLimitedColor());
}

bool CWinSystemAndroidGLESContext::BeginGuiComposite(bool guiWillRender)
{
  return m_guiComposite.Begin(guiWillRender, m_nWidth, m_nHeight,
                              GetEnabledFrontToBackRendering());
}

void CWinSystemAndroidGLESContext::EndGuiComposite()
{
  m_guiComposite.End(m_videoOnSeparateSurface);
}

void CWinSystemAndroidGLESContext::CompositeGui()
{
  m_guiComposite.Composite(m_videoOnSeparateSurface, GetGUIElementCount());
}
