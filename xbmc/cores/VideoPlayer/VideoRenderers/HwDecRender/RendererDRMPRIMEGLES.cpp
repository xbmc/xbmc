/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererDRMPRIMEGLES.h"

#include "ServiceBroker.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/DRMPRIMEEGL.h"
#include "cores/VideoPlayer/VideoRenderers/RenderCapture.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"
#include "cores/VideoPlayer/VideoRenderers/VideoShaders/YUV2RGBShaderGLES.h"
#include "rendering/MatrixGL.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "utils/EGLFence.h"
#include "utils/EGLImage.h"
#include "utils/GLUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"
#include "windowing/linux/WinSystemEGL.h"

using namespace KODI::UTILS::EGL;

CRendererDRMPRIMEGLES::~CRendererDRMPRIMEGLES()
{
  Flush(false);
}

CBaseRenderer* CRendererDRMPRIMEGLES::Create(CVideoBuffer* buffer)
{
  if (!buffer)
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

  m_srcPrimaries = GetSrcPrimaries(static_cast<AVColorPrimaries>(picture.color_primaries),
                                   picture.iWidth, picture.iHeight);
  m_toneMap = false;

  m_fullRange = !CServiceBroker::GetWinSystem()->UseLimitedColor();

  // Calculate the input frame aspect ratio.
  CalculateFrameAspectRatio(picture.iDisplayWidth, picture.iDisplayHeight);
  SetViewMode(m_videoSettings.m_ViewMode);
  ManageRenderArea();

  Flush(false);

  auto winSystem = CServiceBroker::GetWinSystem();

  if (!winSystem)
    return false;

  auto winSystemEGL = dynamic_cast<KODI::WINDOWING::LINUX::CWinSystemEGL*>(winSystem);

  if (!winSystemEGL)
    return false;

  EGLDisplay eglDisplay = winSystemEGL->GetEGLDisplay();

  for (auto&& buf : m_buffers)
  {
    if (!buf.fence)
    {
      buf.texture = std::make_unique<CDRMPRIMETexture>(eglDisplay);
      buf.fence = std::make_unique<CEGLFence>(eglDisplay);
    }
  }

  m_clearColour = winSystem->UseLimitedColor() ? (16.0f / 0xff) : 0.0f;

  m_configured = true;
  return true;
}

void CRendererDRMPRIMEGLES::AddVideoPicture(const VideoPicture& picture, int index)
{
  BUFFER& buf = m_buffers[index];
  if (buf.videoBuffer)
  {
    CLog::LogF(LOGERROR, "unreleased video buffer");
    if (buf.fence)
      buf.fence->DestroyFence();
    buf.texture->Unmap();
    buf.videoBuffer->Release();
  }
  buf.videoBuffer = picture.videoBuffer;
  buf.videoBuffer->Acquire();

  buf.m_srcPrimaries = static_cast<AVColorPrimaries>(picture.color_primaries);
  buf.m_srcColSpace = static_cast<AVColorSpace>(picture.color_space);
  buf.m_srcFullRange = picture.color_range == 1;
  buf.m_srcBits = picture.colorBits;

  buf.hasDisplayMetadata = picture.hasDisplayMetadata;
  buf.displayMetadata = picture.displayMetadata;
  buf.lightMetadata = picture.lightMetadata;
  if (picture.hasLightMetadata && picture.lightMetadata.MaxCLL)
  {
    buf.hasLightMetadata = picture.hasLightMetadata;
  }
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

  if (buf.texture)
    buf.texture->Unmap();

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

  struct Svertex
  {
    float x, y;
  };

  std::vector<Svertex> vertices(6 * quads.size());

  GLubyte count = 0;
  for (const auto& quad : quads)
  {
    vertices[count + 1].x = quad.x1;
    vertices[count + 1].y = quad.y1;

    vertices[count + 0].x = vertices[count + 5].x = quad.x1;
    vertices[count + 0].y = vertices[count + 5].y = quad.y2;

    vertices[count + 2].x = vertices[count + 3].x = quad.x2;
    vertices[count + 2].y = vertices[count + 3].y = quad.y1;

    vertices[count + 4].x = quad.x2;
    vertices[count + 4].y = quad.y2;

    count += 6;
  }

  glDisable(GL_BLEND);

  CRenderSystemGLES* renderSystem =
      dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem());
  if (!renderSystem)
    return;

  renderSystem->EnableGUIShader(SM_DEFAULT);
  GLint posLoc = renderSystem->GUIShaderGetPos();
  GLint uniCol = renderSystem->GUIShaderGetUniCol();

  glUniform4f(uniCol, m_clearColour / 255.0f, m_clearColour / 255.0f, m_clearColour / 255.0f, 1.0f);

  GLuint vertexVBO;
  glGenBuffers(1, &vertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Svertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, sizeof(Svertex), 0);
  glEnableVertexAttribArray(posLoc);

  glDrawArrays(GL_TRIANGLES, 0, vertices.size());

  glDisableVertexAttribArray(posLoc);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &vertexVBO);

  renderSystem->DisableGUIShader();
}

void CRendererDRMPRIMEGLES::RenderUpdate(
    int index, int index2, bool clear, unsigned int flags, unsigned int alpha)
{
  if (!m_configured)
    return;

  ManageRenderArea();

  BUFFER& buf = m_buffers[index];

  CVideoBufferDRMPRIME* buffer = dynamic_cast<CVideoBufferDRMPRIME*>(buf.videoBuffer);
  if (!buffer || !buffer->IsValid())
    return;

  CRenderSystemGLES* renderSystem =
      dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem());
  if (!renderSystem)
    return;

  if (!buf.texture->Map(buffer))
    return;

  AVColorPrimaries srcPrim =
      GetSrcPrimaries(buf.m_srcPrimaries, buf.texture->GetTextureSize().Width(),
                      buf.texture->GetTextureSize().Height());
  if (srcPrim != m_srcPrimaries)
  {
    m_srcPrimaries = srcPrim;
    m_reloadShaders = true;
  }

  bool toneMap = false;

  if (m_videoSettings.m_ToneMapMethod != VS_TONEMAPMETHOD_OFF)
  {
    if (buf.hasLightMetadata || (buf.hasDisplayMetadata && buf.displayMetadata.has_luminance))
    {
      toneMap = true;
    }
  }

  if (toneMap != m_toneMap)
  {
    m_reloadShaders = true;
  }

  m_toneMap = toneMap;

  EShaderFormat shaderFormat = SHADER_NONE;

  if (buf.texture->GetTextureY() != 0)
  {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(buf.texture->GetTextureTarget(), buf.texture->GetTextureY());
    CLog::Log(LOGDEBUG, LOGVIDEO, "CRendererDRMPRIMEGLES::{} - Y={}", __FUNCTION__,
              buf.texture->GetTextureY());
    shaderFormat = SHADER_OES;
  }

  if (buf.texture->GetTextureU() != 0)
  {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(buf.texture->GetTextureTarget(), buf.texture->GetTextureU());
    CLog::Log(LOGDEBUG, LOGVIDEO, "CRendererDRMPRIMEGLES::{} - U={}", __FUNCTION__,
              buf.texture->GetTextureU());
    shaderFormat = SHADER_NV12;
  }

  if (buf.texture->GetTextureV() != 0)
  {
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(buf.texture->GetTextureTarget(), buf.texture->GetTextureV());
    CLog::Log(LOGDEBUG, LOGVIDEO, "CRendererDRMPRIMEGLES::{} - V={}", __FUNCTION__,
              buf.texture->GetTextureV());
    shaderFormat = SHADER_YV12;
  }

  glActiveTexture(GL_TEXTURE0);

  if (shaderFormat != m_shaderFormat)
  {
    m_reloadShaders = true;
  }

  m_shaderFormat = shaderFormat;

  if (m_reloadShaders)
  {
    m_progressiveShader = std::make_unique<Shaders::YUV2RGBProgressiveShader>(
        m_shaderFormat, AVColorPrimaries::AVCOL_PRI_BT709, m_srcPrimaries, m_toneMap);

    m_progressiveShader->CompileAndLink();

    m_reloadShaders = false;
  }

  if (clear)
  {
    if (alpha == 255)
      DrawBlackBars();
    else
    {
      glClearColor(m_clearColour, m_clearColour, m_clearColour, 0);
      glClear(GL_COLOR_BUFFER_BIT);
      glClearColor(0, 0, 0, 0);
    }
  }

  if (alpha < 255)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_progressiveShader->SetAlpha(alpha / 255.0f);
  }
  else
  {
    glDisable(GL_BLEND);
    m_progressiveShader->SetAlpha(1.0f);
  }

  buf.m_srcTextureBits = buf.texture->GetTextureBits();

  m_progressiveShader->SetBlack(m_videoSettings.m_Brightness * 0.01f - 0.5f);
  m_progressiveShader->SetContrast(m_videoSettings.m_Contrast * 0.02f);
  m_progressiveShader->SetWidth(buf.texture->GetTextureSize().Width());
  m_progressiveShader->SetHeight(buf.texture->GetTextureSize().Height());
  m_progressiveShader->SetColParams(buf.m_srcColSpace, buf.m_srcBits, !buf.m_srcFullRange,
                                    buf.m_srcTextureBits);
  m_progressiveShader->SetDisplayMetadata(buf.hasDisplayMetadata, buf.displayMetadata,
                                          buf.hasLightMetadata, buf.lightMetadata);
  m_progressiveShader->SetToneMapParam(m_videoSettings.m_ToneMapParam);
  m_progressiveShader->SetConvertFullColorRange(m_fullRange);

  m_progressiveShader->SetMatrices(glMatrixProject.Get(), glMatrixModview.Get());
  m_progressiveShader->Enable();

  GLubyte idx[4] = {0, 1, 3, 2};
  GLuint vertexVBO;
  GLuint indexVBO;
  struct PackedVertex
  {
    float x, y, z;
    float u1, v1;
    float u2, v2;
    float u3, v3;
  };

  std::array<PackedVertex, 4> vertex;

  GLint vertLoc = m_progressiveShader->GetVertexLoc();
  GLint yloc = m_progressiveShader->GetYcoordLoc();
  GLint uloc = m_progressiveShader->GetUcoordLoc();
  GLint vloc = m_progressiveShader->GetVcoordLoc();

  // top left
  vertex[0].x = m_rotatedDestCoords[0].x;
  vertex[0].y = m_rotatedDestCoords[0].y;
  vertex[0].z = 0.0f;
  vertex[0].u1 = 0.0f;
  vertex[0].v1 = 0.0f;
  vertex[0].u2 = 0.0f;
  vertex[0].v2 = 0.0f;
  vertex[0].u3 = 0.0f;
  vertex[0].v3 = 0.0f;

  // top right
  vertex[1].x = m_rotatedDestCoords[1].x;
  vertex[1].y = m_rotatedDestCoords[1].y;
  vertex[1].z = 0.0f;
  vertex[1].u1 = 1.0f;
  vertex[1].v1 = 0.0f;
  vertex[1].u2 = 1.0f;
  vertex[1].v2 = 0.0f;
  vertex[1].u3 = 1.0f;
  vertex[1].v3 = 0.0f;

  // bottom right
  vertex[2].x = m_rotatedDestCoords[2].x;
  vertex[2].y = m_rotatedDestCoords[2].y;
  vertex[2].z = 0.0f;
  vertex[2].u1 = 1.0f;
  vertex[2].v1 = 1.0f;
  vertex[2].u2 = 1.0f;
  vertex[2].v2 = 1.0f;
  vertex[2].u3 = 1.0f;
  vertex[2].v3 = 1.0f;

  // bottom left
  vertex[3].x = m_rotatedDestCoords[3].x;
  vertex[3].y = m_rotatedDestCoords[3].y;
  vertex[3].z = 0.0f;
  vertex[3].u1 = 0.0f;
  vertex[3].v1 = 1.0f;
  vertex[3].u2 = 0.0f;
  vertex[3].v2 = 1.0f;
  vertex[3].u3 = 0.0f;
  vertex[3].v3 = 1.0f;

  glGenBuffers(1, &vertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(PackedVertex) * vertex.size(), vertex.data(),
               GL_STATIC_DRAW);

  glVertexAttribPointer(vertLoc, 3, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, x)));
  glVertexAttribPointer(yloc, 2, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, u1)));

  if (buf.texture->GetTextureU() != 0)
    glVertexAttribPointer(uloc, 2, GL_FLOAT, 0, sizeof(PackedVertex),
                          reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, u2)));

  if (buf.texture->GetTextureV() != 0)
    glVertexAttribPointer(vloc, 2, GL_FLOAT, 0, sizeof(PackedVertex),
                          reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, u3)));

  glEnableVertexAttribArray(vertLoc);
  glEnableVertexAttribArray(yloc);

  if (buf.texture->GetTextureU() != 0)
    glEnableVertexAttribArray(uloc);

  if (buf.texture->GetTextureV() != 0)
    glEnableVertexAttribArray(vloc);

  glGenBuffers(1, &indexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * 4, idx, GL_STATIC_DRAW);

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, 0);

  glDisableVertexAttribArray(vertLoc);
  glDisableVertexAttribArray(yloc);

  if (buf.texture->GetTextureU() != 0)
    glDisableVertexAttribArray(uloc);

  if (buf.texture->GetTextureV() != 0)
    glDisableVertexAttribArray(vloc);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &vertexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &indexVBO);

  m_progressiveShader->Disable();

  buf.fence->CreateFence();

  VerifyGLState();
  glEnable(GL_BLEND);
}

bool CRendererDRMPRIMEGLES::RenderCapture(CRenderCapture* capture)
{
  capture->BeginRender();
  capture->EndRender();
  return true;
}

bool CRendererDRMPRIMEGLES::ConfigChanged(const VideoPicture& picture)
{
  if (picture.videoBuffer->GetFormat() != m_format)
    return true;

  return false;
}

bool CRendererDRMPRIMEGLES::Supports(ERENDERFEATURE feature)
{
  switch (feature)
  {
    case RENDERFEATURE_STRETCH:
    case RENDERFEATURE_ZOOM:
    case RENDERFEATURE_VERTICAL_SHIFT:
    case RENDERFEATURE_PIXEL_RATIO:
    case RENDERFEATURE_ROTATION:
    case RENDERFEATURE_BRIGHTNESS:
    case RENDERFEATURE_CONTRAST:
    case RENDERFEATURE_TONEMAP:
      return true;
    default:
      return false;
  }
}

bool CRendererDRMPRIMEGLES::Supports(ESCALINGMETHOD method)
{
  switch (method)
  {
    case VS_SCALINGMETHOD_LINEAR:
      return true;
    default:
      return false;
  }
}

AVColorPrimaries CRendererDRMPRIMEGLES::GetSrcPrimaries(AVColorPrimaries srcPrimaries,
                                                        unsigned int width,
                                                        unsigned int height)
{
  AVColorPrimaries ret = srcPrimaries;
  if (ret == AVCOL_PRI_UNSPECIFIED)
  {
    if (width > 1024 || height >= 600)
    {
      ret = AVCOL_PRI_BT709;
    }
    else
    {
      ret = AVCOL_PRI_BT470BG;
    }
  }

  return ret;
}
