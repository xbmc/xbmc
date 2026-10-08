/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererDRMPRIMEGLES.h"

#include "ServiceBroker.h"
#include "cores/VideoPlayer/Buffers/VideoBufferDRMPRIME.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"
#include "cores/VideoPlayer/VideoRenderers/VideoShaders/YUV2RGBShaderGLES.h"
#include "rendering/MatrixGL.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/EGLFence.h"
#include "utils/EGLImage.h"
#include "utils/GLUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"
#include "windowing/linux/WinSystemEGL.h"

#include <memory>

using namespace KODI::UTILS::EGL;

CRendererDRMPRIMEGLES::~CRendererDRMPRIMEGLES()
{
  UnInit();
}

CBaseRenderer* CRendererDRMPRIMEGLES::Create(CVideoBuffer* buffer)
{
  if (!buffer)
    return nullptr;

  auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (!settings->GetBool(CSettings::SETTING_VIDEOPLAYER_USEPRIMEDECODER))
    return nullptr;

  if (settings->GetInt(CSettings::SETTING_VIDEOPLAYER_USEPRIMERENDERER) != 1)
    return nullptr;

  auto buf = dynamic_cast<CVideoBufferDRMPRIME*>(buffer);
  if (!buf)
    return nullptr;

#if defined(EGL_EXT_image_dma_buf_import_modifiers)
  if (!buf->AcquireDescriptor())
    return nullptr;

  auto desc = buf->GetDescriptor();
  if (!desc)
  {
    buf->ReleaseDescriptor();
    return nullptr;
  }

  uint64_t modifier = desc->objects[0].format_modifier;
  uint32_t format = desc->layers[0].format;

  buf->ReleaseDescriptor();

  auto winSystemEGL =
      dynamic_cast<KODI::WINDOWING::LINUX::CWinSystemEGL*>(CServiceBroker::GetWinSystem());
  if (!winSystemEGL)
    return nullptr;

  CEGLImage image{winSystemEGL->GetEGLDisplay()};
  if (!image.SupportsFormatAndModifier(format, modifier))
    return nullptr;
#endif

  return new CRendererDRMPRIMEGLES();
}

void CRendererDRMPRIMEGLES::Register()
{
  VIDEOPLAYER::CRendererFactory::RegisterRenderer("drm_prime_gles", CRendererDRMPRIMEGLES::Create);
}

bool CRendererDRMPRIMEGLES::Configure(const VideoPicture& picture,
                                      float fps,
                                      unsigned int orientation)
{
  m_format = picture.videoBuffer->GetFormat();
  m_sourceWidth = picture.iWidth;
  m_sourceHeight = picture.iHeight;
  m_renderOrientation = orientation;

  m_iFlags = GetFlagsChromaPosition(picture.chroma_position) |
             GetFlagsColorMatrix(picture.color_space, picture.iWidth, picture.iHeight) |
             GetFlagsColorPrimaries(picture.color_primaries) |
             GetFlagsStereoMode(picture.stereoMode);

  // Calculate the input frame aspect ratio.
  CalculateFrameAspectRatio(picture.iDisplayWidth, picture.iDisplayHeight);
  SetViewMode(m_videoSettings.m_ViewMode);
  ManageRenderArea();

  Flush(false);

  auto winSystem = CServiceBroker::GetWinSystem();

  if (!winSystem)
    return false;

  m_configured = true;

  if (!winSystem->SetVideoOutput(&picture))
    CLog::Log(LOGWARNING, "RendererDRMPRIMEGLES::Configure: SetVideoOutput failed");

  auto winSystemEGL = dynamic_cast<KODI::WINDOWING::LINUX::CWinSystemEGL*>(winSystem);

  if (!winSystemEGL)
    return false;

  EGLDisplay eglDisplay = winSystemEGL->GetEGLDisplay();

  m_texturePool.ReleaseAll();
  m_texturePool.Init(eglDisplay);

  for (auto&& buf : m_buffers)
  {
    if (!buf.fence)
      buf.fence = std::make_unique<CEGLFence>(eglDisplay);
  }

  m_configured = true;

  // Two render paths, selected per-frame in Render():
  //  - Full-range: OES samplerExternalOES (always available).
  //  - Limited-range: per-plane sampler2D + BaseYUV2RGBGLSLShader, built
  //    here when the source fourcc is one we can per-plane import.
  m_yuvShader.reset();

  uint32_t sourceFourcc = 0;
  auto* drmBuf = dynamic_cast<CVideoBufferDRMPRIME*>(picture.videoBuffer);
  if (drmBuf && drmBuf->AcquireDescriptor())
  {
    auto* desc = drmBuf->GetDescriptor();
    if (desc && desc->nb_layers == 1)
      sourceFourcc = desc->layers[0].format;
    drmBuf->ReleaseDescriptor();
  }

  if (CDRMPRIMETextureYUV::SupportsFormat(sourceFourcc))
  {
    using namespace Shaders::GLES;
    // Both src and dst primaries equal disables the in-shader primary-matrix
    // path (`if (dstPrimaries != srcPrimaries)`); we want pass-through, same
    // as the OES path does. No tone mapping either.
    //! @todo SHADER_YV12_10 is the WRONG token here for 8-bit YUV420 content.
    //! The EShaderFormat enum conflates two things: the shader sampling
    //! pattern (.r/.g/.a legacy CPU-upload vs .r/.r/.r per-plane R8/R16) and
    //! the source bit depth. Per-plane EGL import always wants the
    //! single-channel sampling pattern (XBMC_YV12_HI define) regardless of
    //! bit depth, but the only way to get that today is to pick one of
    //! SHADER_YV12_9 / _10 / _12 / _14 / _16. Picking _10 here just routes
    //! through the right shader define; the actual bit depth is handled by
    //! SetColParams below. Clean up by either (a) adding a
    //! SHADER_YV12_PLANAR enum value that activates XBMC_YV12_HI without
    //! implying a bit depth, or (b) dropping 3-plane YUV420 from this path
    //! entirely (NV12/P010/P012/P016 via SHADER_NV12_RRG cover the realistic
    //! DRMPRIME cases).
    //! @todo SHADER_YV12_10 is the WRONG token here for 8-bit YUV420 content.
    //! The EShaderFormat enum conflates two things: the shader sampling
    //! pattern (.r/.g/.a legacy CPU-upload vs .r/.r/.r per-plane R8/R16) and
    //! the source bit depth. Per-plane EGL import always wants the
    //! single-channel sampling pattern (XBMC_YV12_HI define) regardless of
    //! bit depth, but the only way to get that today is to pick one of
    //! SHADER_YV12_9 / _10 / _12 / _14 / _16. Picking _10 here just routes
    //! through the right shader define; the actual bit depth is handled by
    //! SetColParams below. Clean up by either (a) adding a
    //! SHADER_YV12_PLANAR enum value that activates XBMC_YV12_HI without
    //! implying a bit depth, or (b) dropping 3-plane YUV420 from this path
    //! entirely (NV12/P010/P012/P016 via SHADER_NV12_RRG cover the realistic
    //! DRMPRIME cases).
    const bool planar3 = (sourceFourcc == DRM_FORMAT_YUV420
#if defined(DRM_FORMAT_S010)
                          || sourceFourcc == DRM_FORMAT_S010
#endif
#if defined(DRM_FORMAT_S012)
                          || sourceFourcc == DRM_FORMAT_S012
#endif
#if defined(DRM_FORMAT_S016)
                          || sourceFourcc == DRM_FORMAT_S016
#endif
    );
    EShaderFormat fmt = planar3 ? SHADER_YV12_10 : SHADER_NV12_RRG;
    auto shader = std::make_unique<YUV2RGBProgressiveShader>(
        fmt, picture.color_primaries, picture.color_primaries,
        /*toneMap*/ false, VS_TONEMAPMETHOD_OFF);
    if (shader->CompileAndLink())
    {
      // P010/P012/P016 are MSB-aligned -> textureBits=8 (no rescale).
      // S010/S012/S016 are LSB-aligned -> textureBits=colorBits (matrix rescales).
      const bool lsbAligned =
#if defined(DRM_FORMAT_S010)
          sourceFourcc == DRM_FORMAT_S010 ||
#endif
#if defined(DRM_FORMAT_S012)
          sourceFourcc == DRM_FORMAT_S012 ||
#endif
#if defined(DRM_FORMAT_S016)
          sourceFourcc == DRM_FORMAT_S016 ||
#endif
          false;
      const int textureBits = lsbAligned ? picture.colorBits : 8;
      shader->SetColParams(picture.color_space, picture.colorBits, picture.color_range != 1,
                           textureBits);
      m_yuvShader = std::move(shader);
      CLog::Log(LOGINFO,
                "RendererDRMPRIMEGLES::Configure: limited-range YUV path "
                "available (src bits {}, src {} range, fourcc {:#x})",
                picture.colorBits, picture.color_range ? "full" : "limited", sourceFourcc);
    }
    else
    {
      CLog::Log(LOGERROR, "RendererDRMPRIMEGLES::Configure: limited-range "
                          "YUV shader compile/link failed");
    }
  }

  if (!m_yuvShader && winSystem->UseLimitedColor())
  {
    CLog::Log(LOGERROR,
              "RendererDRMPRIMEGLES::Configure: fourcc {:#x} cannot be converted to limited "
              "range; switching videoscreen.limitedrange off so the GUI matches the video",
              sourceFourcc);
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool(
        CSettings::SETTING_VIDEOSCREEN_LIMITEDRANGE, false);
  }

  winSystem->SetColorimetry(&picture);
  m_passthroughHDR = winSystem->SetHDR(&picture);
  CLog::Log(LOGDEBUG, "RendererDRMPRIMEGLES::Configure: HDR passthrough: {}",
            m_passthroughHDR ? "on" : "off");

  m_hdrFboActive = m_passthroughHDR && winSystem->SetGuiCompositing(picture.color_transfer);
  if (m_passthroughHDR && !m_hdrFboActive)
    CLog::Log(LOGWARNING, "RendererDRMPRIMEGLES::Configure: HDR passthrough active but GUI "
                          "compositing not supported by windowing system");

  return true;
}

bool CRendererDRMPRIMEGLES::IsGuiLayer()
{
  return !m_hdrFboActive;
}

void CRendererDRMPRIMEGLES::UnInit()
{
  Flush(false);

  if (m_configured)
  {
    m_hdrFboActive = false;
    CServiceBroker::GetWinSystem()->SetGuiCompositing(false);
    CServiceBroker::GetWinSystem()->SetHDR(nullptr);
    m_passthroughHDR = false;
    CServiceBroker::GetWinSystem()->SetColorimetry(nullptr);
    CServiceBroker::GetWinSystem()->SetVideoOutput(nullptr);
  }

  m_texturePool.ReleaseAll();
  m_yuvShader.reset();
  m_configured = false;
}

void CRendererDRMPRIMEGLES::AddVideoPicture(const VideoPicture& picture, int index)
{
  BUFFER& buf = m_buffers[index];
  if (buf.videoBuffer)
  {
    CLog::LogF(LOGERROR, "unreleased video buffer");
    if (buf.fence)
      buf.fence->DestroyFence();
    buf.videoBuffer->Release();
  }
  buf.videoBuffer = picture.videoBuffer;
  buf.videoBuffer->Acquire();

  // CDVDVideoCodecDRMPRIME fills its buffers at decode; CVideoBufferDMA arrives unfilled
  auto* drmBuffer = dynamic_cast<CVideoBufferDRMPRIME*>(picture.videoBuffer);
  if (drmBuffer && !dynamic_cast<CVideoBufferDRMPRIMEFFmpeg*>(drmBuffer))
    drmBuffer->SetPictureParams(picture);
}

bool CRendererDRMPRIMEGLES::Flush(bool saveBuffers)
{
  if (!saveBuffers)
    for (int i = 0; i < NUM_BUFFERS; i++)
      ReleaseBuffer(i);

  return saveBuffers;
}

void CRendererDRMPRIMEGLES::ReleaseBuffer(int index)
{
  BUFFER& buf = m_buffers[index];

  if (buf.fence)
    buf.fence->DestroyFence();

  if (buf.videoBuffer)
  {
    buf.videoBuffer->Release();
    buf.videoBuffer = nullptr;
  }
}

bool CRendererDRMPRIMEGLES::NeedBuffer(int index)
{
  return !m_buffers[index].fence->IsSignaled();
}

CRenderInfo CRendererDRMPRIMEGLES::GetRenderInfo()
{
  CRenderInfo info;
  info.max_buffer_size = NUM_BUFFERS;
  return info;
}

void CRendererDRMPRIMEGLES::Update()
{
  if (!m_configured)
    return;

  ManageRenderArea();
}

void CRendererDRMPRIMEGLES::DrawBlackBars()
{
  CRect windowRect(0, 0, CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth(),
                   CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight());

  auto quads = windowRect.SubtractRect(m_destRect);

  glDisable(GL_BLEND);

  CRenderSystemGLES* renderSystem =
      dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem());
  if (!renderSystem)
    return;

  renderSystem->EnableGUIShader(ShaderMethodGLES::SM_DEFAULT);
  GLint uniCol = renderSystem->GUIShaderGetUniCol();
  GLint depthLoc = renderSystem->GUIShaderGetDepth();

  glUniform4f(uniCol, 0.0f, 0.0f, 0.0f, 1.0f);
  glUniform1f(depthLoc, -1.0f);

  for (const auto& quad : quads)
    renderSystem->DrawGUIQuad(quad);

  renderSystem->DisableGUIShader();
}

void CRendererDRMPRIMEGLES::RenderUpdate(
    int index, int index2, bool clear, unsigned int flags, unsigned int alpha)
{
  if (!m_configured)
    return;

  ManageRenderArea();

  if (clear)
  {
    if (alpha == 255)
      DrawBlackBars();
    else
    {
      const float bg = CServiceBroker::GetWinSystem()->UseLimitedColor() ? (16.0f / 0xff) : 0.0f;
      glClearColor(bg, bg, bg, 0);
      glClear(GL_COLOR_BUFFER_BIT);
      glClearColor(0, 0, 0, 0);
    }
  }

  if (alpha < 255)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  else
  {
    glDisable(GL_BLEND);
  }

  Render(flags, index);

  VerifyGLState();
  glEnable(GL_BLEND);
}

bool CRendererDRMPRIMEGLES::ConfigChanged(const VideoPicture& picture)
{
  if (picture.videoBuffer->GetFormat() != m_format)
    return true;

  return false;
}

void CRendererDRMPRIMEGLES::Render(unsigned int flags, int index)
{
  BUFFER& buf = m_buffers[index];

  CVideoBufferDRMPRIME* buffer = dynamic_cast<CVideoBufferDRMPRIME*>(buf.videoBuffer);
  if (!buffer || !buffer->IsValid())
    return;

  CRenderSystemGLES* renderSystem =
      dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem());
  if (!renderSystem)
    return;

  // Per-frame path selection. m_yuvShader is only alive if the source
  // fourcc was importable; UseLimitedColor() is checked every frame so
  // the user can toggle the setting at runtime and the next frame picks
  // up the new path without any rebuild.
  auto* winSystem = CServiceBroker::GetWinSystem();
  CDRMPRIMETextureYUV* yuvTexture = nullptr;
  if (m_yuvShader && winSystem && winSystem->UseLimitedColor())
    yuvTexture = m_texturePool.GetYUV(buffer);

  if (yuvTexture)
  {
    // Limited-range YUV path: per-plane sampler2D + BaseYUV2RGBGLSLShader.
    // Vertex/cord array setup is taken from CLinuxRendererGLES::RenderSinglePass
    // (LinuxRendererGLES.cpp:1100-1183). The texture-unit binding pattern is
    // taken from CRendererVAAPIGLES::UploadTexture (RendererVAAPIGLES.cpp:270-282)
    // -- specifically, the SHADER_NV12_RRG case binds the UV texture to BOTH
    // U and V samplers so .r returns U and .g returns V via the GR88 / GR1616
    // import.
    const int numPlanes = yuvTexture->GetNumPlanes();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, yuvTexture->GetTexture(0));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, yuvTexture->GetTexture(1));
    glActiveTexture(GL_TEXTURE2);
    // 3-plane (YV12_HI): plane 2 is V. 2-plane (NV12_RRG): rebind plane 1
    // (UV) so sampV samples the same texture as sampU.
    glBindTexture(GL_TEXTURE_2D, yuvTexture->GetTexture(numPlanes == 3 ? 2 : 1));
    glActiveTexture(GL_TEXTURE0);

    const CSizeInt texSize = yuvTexture->GetTextureSize();
    m_yuvShader->SetWidth(texSize.Width());
    m_yuvShader->SetHeight(texSize.Height());
    m_yuvShader->SetAlpha(1.0f);
    m_yuvShader->SetMatrices(glMatrixProject.Get(), glMatrixModview.Get());
    // Per-plane texcoords stay at their 0..1 default; each EGL-imported texture covers the
    // whole plane regardless of chroma subsampling.
    m_yuvShader->SetQuadTransform(KODI::UTILS::GL::QuadTransform(
        m_rotatedDestCoords[0], m_rotatedDestCoords[1], m_rotatedDestCoords[3]));
    // SetConvertFullColorRange(false) was set at Configure time -> matrix
    // produces limited-range RGB. No per-frame setter needed.
    m_yuvShader->Enable();

    GLint vertLoc = m_yuvShader->GetVertexLoc();
    GLint yLoc = m_yuvShader->GetYcoordLoc();
    GLint uLoc = m_yuvShader->GetUcoordLoc();
    GLint vLoc = m_yuvShader->GetVcoordLoc();

    renderSystem->BindGUIUnitQuad();
    for (GLint loc : {vertLoc, yLoc, uLoc, vLoc})
    {
      glVertexAttribPointer(loc, 2, GL_FLOAT, 0, 0, 0);
      glEnableVertexAttribArray(loc);
    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(vertLoc);
    glDisableVertexAttribArray(yLoc);
    glDisableVertexAttribArray(uLoc);
    glDisableVertexAttribArray(vLoc);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_yuvShader->Disable();

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    buf.fence->DestroyFence();
    buf.fence->CreateFence();
    return;
  }

  // Full-range OES path (unchanged).
  CDRMPRIMETexture* texture = m_texturePool.GetOES(buffer);
  if (!texture)
    return;

  DrawTexture(*renderSystem, texture->GetTexture(), m_rotatedDestCoords);

  buf.fence->DestroyFence();
  buf.fence->CreateFence();
}

void CRendererDRMPRIMEGLES::DrawTexture(CRenderSystemGLES& renderSystem,
                                        GLuint texture,
                                        const CPoint dest[4])
{
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);

  renderSystem.EnableGUIShader(ShaderMethodGLES::SM_TEXTURE_RGBA_OES);

  glUniform1f(renderSystem.GUIShaderGetDepth(), -1.0f);

  // dest holds the corners of a (possibly rotated) rectangle: top left, top right, bottom right,
  // bottom left.
  const CRect texCoords(0.0f, 0.0f, 1.0f, 1.0f);
  renderSystem.DrawGUIQuad(dest[0], dest[1], dest[3], &texCoords);

  renderSystem.DisableGUIShader();

  glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
}

bool CRendererDRMPRIMEGLES::Supports(ERENDERFEATURE feature) const
{
  switch (feature)
  {
    case RENDERFEATURE_STRETCH:
    case RENDERFEATURE_ZOOM:
    case RENDERFEATURE_VERTICAL_SHIFT:
    case RENDERFEATURE_PIXEL_RATIO:
    case RENDERFEATURE_ROTATION:
      return true;
    default:
      return false;
  }
}

bool CRendererDRMPRIMEGLES::Supports(ESCALINGMETHOD method) const
{
  switch (method)
  {
    case VS_SCALINGMETHOD_LINEAR:
      return true;
    default:
      return false;
  }
}
