/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LinuxRendererGLES.h"

#include "RenderCapture.h"
#include "RenderCaptureGLES.h"
#include "RenderFactory.h"
#include "ServiceBroker.h"
#include "VideoShaders/VideoFilterShaderGLES.h"
#include "VideoShaders/YUV2RGBShaderGLES.h"
#include "application/Application.h"
#include "cores/IPlayer.h"
#include "guilib/Texture.h"
#include "rendering/MatrixGL.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/GLUtils.h"
#include "utils/MathUtils.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"

#include <mutex>

using namespace Shaders;
using namespace Shaders::GLES;

CLinuxRendererGLES::CLinuxRendererGLES()
{
  m_format = AV_PIX_FMT_NONE;

  m_fullRange = !CServiceBroker::GetWinSystem()->UseLimitedColor();

  m_renderSystem = dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem());

#if defined (GL_UNPACK_ROW_LENGTH_EXT)
  if (m_renderSystem->IsExtSupported("GL_EXT_unpack_subimage"))
  {
    m_pixelStoreKey = GL_UNPACK_ROW_LENGTH_EXT;
  }
#endif
}

CLinuxRendererGLES::~CLinuxRendererGLES()
{
  UnInit();

  ReleaseShaders();

  free(m_planeBuffer);
  m_planeBuffer = nullptr;
}

CBaseRenderer* CLinuxRendererGLES::Create(CVideoBuffer *buffer)
{
  return new CLinuxRendererGLES();
}

bool CLinuxRendererGLES::Register()
{
  VIDEOPLAYER::CRendererFactory::RegisterRenderer("default", CLinuxRendererGLES::Create);
  return true;
}

bool CLinuxRendererGLES::ValidateRenderTarget()
{
  if (!m_bValidated)
  {
    // function pointer for texture might change in
    // call to LoadShaders
    glFinish();

    for (int i = 0 ; i < NUM_BUFFERS ; i++)
    {
      DeleteTexture(i);
    }

     // create the yuv textures
    UpdateVideoFilter();
    LoadShaders();

    if (m_renderMethod < 0)
    {
      return false;
    }

    for (int i = 0 ; i < m_NumYV12Buffers ; i++)
    {
      CreateTexture(i);
    }

    m_bValidated = true;

    return true;
  }

  return false;
}

bool CLinuxRendererGLES::Configure(const VideoPicture &picture, float fps, unsigned int orientation)
{
  CLog::Log(LOGDEBUG, "LinuxRendererGLES::Configure: fps: {:0.3f}", fps);
  m_format = picture.videoBuffer->GetFormat();
  m_sourceWidth = picture.iWidth;
  m_sourceHeight = picture.iHeight;
  m_renderOrientation = orientation;

  m_srcPrimaries = picture.color_primaries;
  m_toneMap = false;

  // Calculate the input frame aspect ratio.
  CalculateFrameAspectRatio(picture.iDisplayWidth, picture.iDisplayHeight);
  SetViewMode(m_videoSettings.m_ViewMode);
  ManageRenderArea();

  m_bConfigured = true;
  m_scalingMethodGui = (ESCALINGMETHOD)-1;

  // Ensure that textures are recreated and rendering starts only after the 1st
  // frame is loaded after every call to Configure().
  m_bValidated = false;

  // setup the background colour
  m_clearColour = CServiceBroker::GetWinSystem()->UseLimitedColor() ? (16.0f / 0xff) : 0.0f;

  if (picture.hasDisplayMetadata && picture.hasLightMetadata)
  {
    m_passthroughHDR = CServiceBroker::GetWinSystem()->SetHDR(&picture);
    CLog::Log(LOGDEBUG, "LinuxRendererGLES::Configure: HDR passthrough: {}",
              m_passthroughHDR ? "on" : "off");
  }

  return true;
}

bool CLinuxRendererGLES::ConfigChanged(const VideoPicture &picture)
{
  if (picture.videoBuffer->GetFormat() != m_format)
  {
    return true;
  }

  return false;
}

int CLinuxRendererGLES::NextYV12Texture()
{
  return (m_iYV12RenderBuffer + 1) % m_NumYV12Buffers;
}

void CLinuxRendererGLES::AddVideoPicture(const VideoPicture &picture, int index)
{
  CPictureBuffer &buf = m_buffers[index];
  if (buf.videoBuffer)
  {
    CLog::LogF(LOGERROR, "unreleased video buffer");
    buf.videoBuffer->Release();
  }
  buf.videoBuffer = picture.videoBuffer;
  buf.videoBuffer->Acquire();
  buf.loaded = false;
  buf.m_srcPrimaries = picture.color_primaries;
  buf.m_srcColSpace = picture.color_space;
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

void CLinuxRendererGLES::ReleaseBuffer(int idx)
{
  CPictureBuffer &buf = m_buffers[idx];
  if (buf.videoBuffer)
  {
    buf.videoBuffer->Release();
    buf.videoBuffer = nullptr;
  }
}

void CLinuxRendererGLES::CalculateTextureSourceRects(int source, int num_planes)
{
  CPictureBuffer& buf = m_buffers[source];
  YuvImage* im = &buf.image;

  // calculate the source rectangle
  for(int field = 0; field < 3; field++)
  {
    for(int plane = 0; plane < num_planes; plane++)
    {
      CYuvPlane& p = buf.fields[field][plane];

      p.rect = m_sourceRect;
      p.width  = im->width;
      p.height = im->height;

      if(field != FIELD_FULL)
      {
        // correct for field offsets and chroma offsets
        float offset_y = 0.5;
        if(plane != 0)
        {
          offset_y += 0.5f;
        }

        if(field == FIELD_BOT)
        {
          offset_y *= -1;
        }

        p.rect.y1 += offset_y;
        p.rect.y2 += offset_y;

        // half the height if this is a field
        p.height  *= 0.5f;
        p.rect.y1 *= 0.5f;
        p.rect.y2 *= 0.5f;
      }

      if(plane != 0)
      {
        p.width   /= 1 << im->cshift_x;
        p.height  /= 1 << im->cshift_y;

        p.rect.x1 /= 1 << im->cshift_x;
        p.rect.x2 /= 1 << im->cshift_x;
        p.rect.y1 /= 1 << im->cshift_y;
        p.rect.y2 /= 1 << im->cshift_y;
      }

      // protect against division by zero
      if (p.texheight == 0 || p.texwidth == 0 ||
          p.pixpertex_x == 0 || p.pixpertex_y == 0)
      {
        continue;
      }

      p.height  /= p.pixpertex_y;
      p.rect.y1 /= p.pixpertex_y;
      p.rect.y2 /= p.pixpertex_y;
      p.width   /= p.pixpertex_x;
      p.rect.x1 /= p.pixpertex_x;
      p.rect.x2 /= p.pixpertex_x;

      if (m_textureTarget == GL_TEXTURE_2D)
      {
        p.height  /= p.texheight;
        p.rect.y1 /= p.texheight;
        p.rect.y2 /= p.texheight;
        p.width   /= p.texwidth;
        p.rect.x1 /= p.texwidth;
        p.rect.x2 /= p.texwidth;
      }
    }
  }
}

void CLinuxRendererGLES::LoadPlane(CYuvPlane& plane, int type,
                                   unsigned width, unsigned height,
                                   int stride, int bpp, void* data)
{
  const GLvoid *pixelData = data;
  int bps = bpp * KODI::UTILS::GL::glFormatElementByteCount(type);

  glBindTexture(m_textureTarget, plane.id);

  bool pixelStoreChanged = false;
  if (stride != static_cast<int>(width * bps))
  {
    if (m_pixelStoreKey > 0)
    {
      pixelStoreChanged = true;
      glPixelStorei(m_pixelStoreKey, stride);
    }
    else
    {
      size_t planeSize = width * height * bps;
      if (m_planeBufferSize < planeSize)
      {
        m_planeBuffer = static_cast<unsigned char*>(realloc(m_planeBuffer, planeSize));
        m_planeBufferSize = planeSize;
      }

      unsigned char *src(static_cast<unsigned char*>(data)),
                    *dst(m_planeBuffer);

      for (unsigned int y = 0; y < height; ++y, src += stride, dst += width * bps)
        memcpy(dst, src, width * bps);

      pixelData = m_planeBuffer;
    }
  }
  glTexSubImage2D(m_textureTarget, 0, 0, 0, width, height, type, GL_UNSIGNED_BYTE, pixelData);

  if (m_pixelStoreKey > 0 && pixelStoreChanged)
    glPixelStorei(m_pixelStoreKey, 0);

  // check if we need to load any border pixels
  if (height < plane.texheight)
  {
    glTexSubImage2D(m_textureTarget, 0,
                    0, height, width, 1,
                    type, GL_UNSIGNED_BYTE,
                    static_cast<const unsigned char*>(pixelData) + stride * (height - 1));
  }

  if (width  < plane.texwidth)
  {
    glTexSubImage2D(m_textureTarget, 0,
                    width, 0, 1, height,
                    type, GL_UNSIGNED_BYTE,
                    static_cast<const unsigned char*>(pixelData) + bps * (width - 1));
  }

  glBindTexture(m_textureTarget, 0);
}

bool CLinuxRendererGLES::Flush(bool saveBuffers)
{
  glFinish();

  for (int i = 0 ; i < m_NumYV12Buffers ; i++)
  {
    DeleteTexture(i);
  }

  glFinish();
  m_bValidated = false;
  m_fbo.fbo.Cleanup();
  m_iYV12RenderBuffer = 0;

  return false;
}

void CLinuxRendererGLES::Update()
{
  if (!m_bConfigured)
  {
    return;
  }

  ManageRenderArea();
  ValidateRenderTarget();
}

void CLinuxRendererGLES::ClearBackBuffer()
{
  //set the entire backbuffer to black
  //if we do a two pass render, we have to draw a quad. else we might occlude OSD elements.
  if (CServiceBroker::GetWinSystem()->GetGfxContext().GetRenderOrder() ==
      RENDER_ORDER_ALL_BACK_TO_FRONT)
  {
    CServiceBroker::GetWinSystem()->GetGfxContext().Clear(0xff000000);
  }
  else
  {
    ClearBackBufferQuad();
  }
}

void CLinuxRendererGLES::ClearBackBufferQuad()
{
  CRect windowRect(0, 0, CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth(),
                   CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight());
  struct Svertex
  {
    float x, y;
  };

  std::vector<Svertex> vertices{
      {windowRect.x1, windowRect.y2 * 2},
      {windowRect.x1, windowRect.y1},
      {windowRect.x2 * 2, windowRect.y1},
  };

  glDisable(GL_BLEND);

  m_renderSystem->EnableGUIShader(ShaderMethodGLES::SM_DEFAULT);
  GLint posLoc = m_renderSystem->GUIShaderGetPos();
  GLint uniCol = m_renderSystem->GUIShaderGetUniCol();
  GLint depthLoc = m_renderSystem->GUIShaderGetDepth();

  glUniform4f(uniCol, m_clearColour / 255.0f, m_clearColour / 255.0f, m_clearColour / 255.0f, 1.0f);
  glUniform1f(depthLoc, -1);

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

  m_renderSystem->DisableGUIShader();
}

void CLinuxRendererGLES::DrawBlackBars()
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

  renderSystem->EnableGUIShader(ShaderMethodGLES::SM_DEFAULT);
  GLint posLoc = renderSystem->GUIShaderGetPos();
  GLint uniCol = renderSystem->GUIShaderGetUniCol();
  GLint depthLoc = m_renderSystem->GUIShaderGetDepth();

  glUniform4f(uniCol, m_clearColour / 255.0f, m_clearColour / 255.0f, m_clearColour / 255.0f, 1.0f);
  glUniform1f(depthLoc, -1);

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

void CLinuxRendererGLES::RenderUpdate(int index, int index2, bool clear, unsigned int flags, unsigned int alpha)
{
  m_iYV12RenderBuffer = index;

  if (!m_bConfigured)
  {
    return;
  }

  // if its first pass, just init textures and return
  if (ValidateRenderTarget())
  {
    if (clear) //if clear is set, we're expected to overwrite all backbuffer pixels, even if we have nothing to render
      ClearBackBuffer();

    return;
  }

  if (!IsGuiLayer())
  {
    RenderUpdateVideo(clear, flags, alpha);
    return;
  }

  CPictureBuffer& buf = m_buffers[index];

  if (!buf.fields[FIELD_FULL][0].id)
  {
    return;
  }

  ManageRenderArea();

  if (clear)
  {
    if (alpha == 255)
      DrawBlackBars();
    else
    {
      ClearBackBuffer();
    }
  }

  if (alpha < 255)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (m_pYUVProgShader)
    {
      m_pYUVProgShader->SetAlpha(alpha / 255.0f);
    }

    if (m_pYUVBobShader)
    {
      m_pYUVBobShader->SetAlpha(alpha / 255.0f);
    }
  }
  else
  {
    glDisable(GL_BLEND);
    if (m_pYUVProgShader)
    {
      m_pYUVProgShader->SetAlpha(1.0f);
    }

    if (m_pYUVBobShader)
    {
      m_pYUVBobShader->SetAlpha(1.0f);
    }
  }

  if (!Render(flags, index) && clear)
    ClearBackBuffer();

  VerifyGLState();
  glEnable(GL_BLEND);
}

void CLinuxRendererGLES::RenderUpdateVideo(bool clear, unsigned int flags, unsigned int alpha)
{
  if (!m_bConfigured)
  {
    return;
  }

  if (IsGuiLayer())
  {
    return;
  }
}

void CLinuxRendererGLES::UpdateVideoFilter()
{
  CRect srcRect;
  CRect dstRect;
  CRect viewRect;
  GetVideoRect(srcRect, dstRect, viewRect);

  if (m_scalingMethodGui == m_videoSettings.m_ScalingMethod &&
      viewRect.Height() == m_viewRect.Height() &&
      viewRect.Width() == m_viewRect.Width())
  {
    return;
  }

  m_scalingMethodGui = m_videoSettings.m_ScalingMethod;
  m_scalingMethod = m_scalingMethodGui;
  m_viewRect = viewRect;

  if(!Supports(m_scalingMethod))
  {
    CLog::Log(LOGWARNING,
              "CLinuxRendererGLES::UpdateVideoFilter - chosen scaling method {}, is not supported "
              "by renderer",
              m_scalingMethod);
    m_scalingMethod = VS_SCALINGMETHOD_LINEAR;
  }

  if (m_pVideoFilterShader)
  {
    delete m_pVideoFilterShader;
    m_pVideoFilterShader = nullptr;
  }

  m_fbo.fbo.Cleanup();

  VerifyGLState();

  switch (m_scalingMethod)
  {
  case VS_SCALINGMETHOD_NEAREST:
  {
    CLog::Log(LOGINFO, "GLES: Selecting single pass rendering");
    SetTextureFilter(GL_NEAREST);
    m_renderQuality = RQ_SINGLEPASS;
    return;
  }
  case VS_SCALINGMETHOD_LINEAR:
  {
    CLog::Log(LOGINFO, "GLES: Selecting single pass rendering");
    SetTextureFilter(GL_LINEAR);
    m_renderQuality = RQ_SINGLEPASS;
    return;
  }
  case VS_SCALINGMETHOD_LANCZOS2:
  case VS_SCALINGMETHOD_SPLINE36_FAST:
  case VS_SCALINGMETHOD_LANCZOS3_FAST:
  case VS_SCALINGMETHOD_SPLINE36:
  case VS_SCALINGMETHOD_LANCZOS3:
  case VS_SCALINGMETHOD_CUBIC_B_SPLINE:
  case VS_SCALINGMETHOD_CUBIC_MITCHELL:
  case VS_SCALINGMETHOD_CUBIC_CATMULL:
  case VS_SCALINGMETHOD_CUBIC_0_075:
  case VS_SCALINGMETHOD_CUBIC_0_1:
  {
    if (m_renderMethod & RENDER_GLSL)
    {
      if (!m_fbo.fbo.Initialize())
      {
        CLog::Log(LOGERROR, "GLES: Error initializing FBO");
        break;
      }

      if (!m_fbo.fbo.CreateAndBindToTexture(GL_TEXTURE_2D, m_sourceWidth, m_sourceHeight, GL_RGBA))
      {
        CLog::Log(LOGERROR, "GLES: Error creating texture and binding to FBO");
        break;
      }
    }

    m_pVideoFilterShader = new ConvolutionFilterShader(m_scalingMethod);
    if (!m_pVideoFilterShader->CompileAndLink())
    {
      CLog::Log(LOGERROR, "GLES: Error compiling and linking video filter shader");
      break;
    }

    CLog::Log(LOGINFO, "GLES: Selecting multi pass rendering");
    SetTextureFilter(GL_LINEAR);
    m_renderQuality = RQ_MULTIPASS;
      return;
  }
  case VS_SCALINGMETHOD_BICUBIC_SOFTWARE:
  case VS_SCALINGMETHOD_LANCZOS_SOFTWARE:
  case VS_SCALINGMETHOD_SINC_SOFTWARE:
  case VS_SCALINGMETHOD_SINC8:
  {
    CLog::Log(LOGERROR, "GLES: TODO: This scaler has not yet been implemented");
    break;
  }
  default:
    break;
  }

  CLog::Log(LOGERROR, "GLES: Falling back to bilinear due to failure to init scaler");
  if (m_pVideoFilterShader)
  {
    delete m_pVideoFilterShader;
    m_pVideoFilterShader = nullptr;
  }

  m_fbo.fbo.Cleanup();

  SetTextureFilter(GL_LINEAR);
  m_renderQuality = RQ_SINGLEPASS;
}

void CLinuxRendererGLES::LoadShaders(int field)
{
  m_reloadShaders = 0;

  if (!LoadShadersHook())
  {
    int requestedMethod = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_RENDERMETHOD);
    CLog::Log(LOGDEBUG, "GLES: Requested render method: {}", requestedMethod);

    ReleaseShaders();

    switch(requestedMethod)
    {
      case RENDER_METHOD_AUTO:
      case RENDER_METHOD_GLSL:
      {
        // Try GLSL shaders if supported and user requested auto or GLSL.
        if (glCreateProgram())
        {
          // create regular scan shader
          CLog::Log(LOGINFO, "GLES: Selecting YUV 2 RGB shader");

          EShaderFormat shaderFormat = GetShaderFormat();
          m_toneMapMethod = m_videoSettings.m_ToneMapMethod;
          m_pYUVProgShader = new YUV2RGBProgressiveShader(
              shaderFormat, m_passthroughHDR ? m_srcPrimaries : AVColorPrimaries::AVCOL_PRI_BT709,
              m_srcPrimaries, m_toneMap, m_toneMapMethod);
          m_pYUVProgShader->SetConvertFullColorRange(m_fullRange);
          m_pYUVBobShader = new YUV2RGBBobShader(
              shaderFormat, m_passthroughHDR ? m_srcPrimaries : AVColorPrimaries::AVCOL_PRI_BT709,
              m_srcPrimaries, m_toneMap, m_toneMapMethod);
          m_pYUVBobShader->SetConvertFullColorRange(m_fullRange);

          if ((m_pYUVProgShader && m_pYUVProgShader->CompileAndLink())
              && (m_pYUVBobShader && m_pYUVBobShader->CompileAndLink()))
          {
            m_renderMethod = RENDER_GLSL;
            UpdateVideoFilter();
            break;
          }
          else
          {
            ReleaseShaders();
            CLog::Log(LOGERROR, "GLES: Error enabling YUV2RGB GLSL shader");
            m_renderMethod = -1;
            break;
          }
        }

        break;
      }
      default:
      {
        m_renderMethod = -1 ;
        CLog::Log(LOGERROR, "GLES: render method not supported");
      }
    }
  }
}

void CLinuxRendererGLES::ReleaseShaders()
{
  if (m_pYUVProgShader)
  {
    delete m_pYUVProgShader;
    m_pYUVProgShader = nullptr;
  }

  if (m_pYUVBobShader)
  {
    delete m_pYUVBobShader;
    m_pYUVBobShader = nullptr;
  }
}

void CLinuxRendererGLES::UnInit()
{
  CLog::Log(LOGDEBUG, "LinuxRendererGLES: Cleaning up GLES resources");
  std::unique_lock<CCriticalSection> lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  glFinish();

  // YV12 textures
  for (int i = 0; i < NUM_BUFFERS; ++i)
  {
    DeleteTexture(i);
  }

  // cleanup framebuffer object if it was in use
  m_fbo.fbo.Cleanup();
  m_bValidated = false;
  m_bConfigured = false;

  CServiceBroker::GetWinSystem()->SetHDR(nullptr);
  m_passthroughHDR = false;
}

bool CLinuxRendererGLES::CreateTexture(int index)
{
  if (m_format == AV_PIX_FMT_NV12)
  {
    return CreateNV12Texture(index);
  }
  else
  {
    return CreateYV12Texture(index);
  }
}

void CLinuxRendererGLES::DeleteTexture(int index)
{
  ReleaseBuffer(index);

  if (m_format == AV_PIX_FMT_NV12)
  {
    DeleteNV12Texture(index);
  }
  else
  {
    DeleteYV12Texture(index);
  }
}

bool CLinuxRendererGLES::UploadTexture(int index)
{
  if (!m_buffers[index].videoBuffer)
  {
    return false;
  }

  if (m_buffers[index].loaded)
  {
    return true;
  }

  bool ret{false};

  YuvImage &dst = m_buffers[index].image;
  m_buffers[index].videoBuffer->GetPlanes(dst.plane);
  m_buffers[index].videoBuffer->GetStrides(dst.stride);

  if (m_format == AV_PIX_FMT_NV12)
  {
    ret = UploadNV12Texture(index);
  }
  else
  {
    // default to YV12 texture handlers
    ret = UploadYV12Texture(index);
  }

  if (ret)
  {
    m_buffers[index].loaded = true;
  }

  return ret;
}

bool CLinuxRendererGLES::Render(unsigned int flags, int index)
{
  // obtain current field, if interlaced
  if( flags & RENDER_FLAG_TOP)
  {
    m_currentField = FIELD_TOP;
  }
  else if (flags & RENDER_FLAG_BOT)
  {
    m_currentField = FIELD_BOT;
  }
  else
  {
    m_currentField = FIELD_FULL;
  }

  // call texture load function
  if (!UploadTexture(index))
  {
    return false;
  }

  if (RenderHook(index))
  {
    ;
  }
  else if (m_renderMethod & RENDER_GLSL)
  {
    UpdateVideoFilter();
    switch(m_renderQuality)
    {
    case RQ_LOW:
    case RQ_SINGLEPASS:
    {
      RenderSinglePass(index, m_currentField);
      VerifyGLState();
      break;
    }
    case RQ_MULTIPASS:
    {
      RenderToFBO(index, m_currentField);
      RenderFromFBO();
      VerifyGLState();
      break;
    }
    default:
      break;
    }
  }
  else
  {
    return false;
  }

  AfterRenderHook(index);
  return true;
}

void CLinuxRendererGLES::RenderSinglePass(int index, int field)
{
  CPictureBuffer &buf = m_buffers[index];
  CYuvPlane (&planes)[YuvImage::MAX_PLANES] = m_buffers[index].fields[field];

  if (buf.m_srcPrimaries != m_srcPrimaries)
  {
    m_srcPrimaries = buf.m_srcPrimaries;
    m_reloadShaders = true;
  }

  bool toneMap = false;
  ETONEMAPMETHOD toneMapMethod = m_videoSettings.m_ToneMapMethod;

  if (!m_passthroughHDR && toneMapMethod != VS_TONEMAPMETHOD_OFF)
  {
    if (buf.hasLightMetadata || (buf.hasDisplayMetadata && buf.displayMetadata.has_luminance))
    {
      toneMap = true;
    }
  }

  if (toneMap != m_toneMap || toneMapMethod != m_toneMapMethod)
  {
    m_reloadShaders = true;
  }

  m_toneMap = toneMap;
  m_toneMapMethod = toneMapMethod;

  if (m_reloadShaders)
  {
    LoadShaders(field);
  }

  // Y
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(m_textureTarget, planes[0].id);

  // U
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(m_textureTarget, planes[1].id);

  // V
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(m_textureTarget, planes[2].id);

  glActiveTexture(GL_TEXTURE0);
  VerifyGLState();

  Shaders::GLES::BaseYUV2RGBGLSLShader* pYUVShader;
  if (field != FIELD_FULL)
  {
    pYUVShader = m_pYUVBobShader;
  }
  else
  {
    pYUVShader = m_pYUVProgShader;
  }

  pYUVShader->SetBlack(m_videoSettings.m_Brightness * 0.01f - 0.5f);
  pYUVShader->SetContrast(m_videoSettings.m_Contrast * 0.02f);
  pYUVShader->SetWidth(planes[0].texwidth);
  pYUVShader->SetHeight(planes[0].texheight);
  pYUVShader->SetColParams(buf.m_srcColSpace, buf.m_srcBits, !buf.m_srcFullRange, buf.m_srcTextureBits);
  pYUVShader->SetDisplayMetadata(buf.hasDisplayMetadata, buf.displayMetadata,
                                 buf.hasLightMetadata, buf.lightMetadata);
  pYUVShader->SetToneMapParam(m_videoSettings.m_ToneMapParam);

  if (field == FIELD_TOP)
  {
    pYUVShader->SetField(1);
  }
  else if(field == FIELD_BOT)
  {
    pYUVShader->SetField(0);
  }

  pYUVShader->SetMatrices(glMatrixProject.Get(), glMatrixModview.Get());
  pYUVShader->Enable();

  GLubyte idx[4] = {0, 1, 3, 2}; // determines order of triangle strip
  GLfloat m_vert[4][3];
  GLfloat m_tex[3][4][2];

  GLint vertLoc = pYUVShader->GetVertexLoc();
  GLint Yloc = pYUVShader->GetYcoordLoc();
  GLint Uloc = pYUVShader->GetUcoordLoc();
  GLint Vloc = pYUVShader->GetVcoordLoc();

  glVertexAttribPointer(vertLoc, 3, GL_FLOAT, 0, 0, m_vert);
  glVertexAttribPointer(Yloc, 2, GL_FLOAT, 0, 0, m_tex[0]);
  glVertexAttribPointer(Uloc, 2, GL_FLOAT, 0, 0, m_tex[1]);
  glVertexAttribPointer(Vloc, 2, GL_FLOAT, 0, 0, m_tex[2]);

  glEnableVertexAttribArray(vertLoc);
  glEnableVertexAttribArray(Yloc);
  glEnableVertexAttribArray(Uloc);
  glEnableVertexAttribArray(Vloc);

  // Setup vertex position values
  for(int i = 0; i < 4; i++)
  {
    m_vert[i][0] = m_rotatedDestCoords[i].x;
    m_vert[i][1] = m_rotatedDestCoords[i].y;
    m_vert[i][2] = 0.0f;// set z to 0
  }

  // Setup texture coordinates
  for (int i = 0; i < 3; i++)
  {
    m_tex[i][0][0] = m_tex[i][3][0] = planes[i].rect.x1;
    m_tex[i][0][1] = m_tex[i][1][1] = planes[i].rect.y1;
    m_tex[i][1][0] = m_tex[i][2][0] = planes[i].rect.x2;
    m_tex[i][2][1] = m_tex[i][3][1] = planes[i].rect.y2;
  }

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);

  VerifyGLState();

  pYUVShader->Disable();
  VerifyGLState();

  glDisableVertexAttribArray(vertLoc);
  glDisableVertexAttribArray(Yloc);
  glDisableVertexAttribArray(Uloc);
  glDisableVertexAttribArray(Vloc);

  VerifyGLState();
}

void CLinuxRendererGLES::RenderToFBO(int index, int field)
{
  CPictureBuffer &buf = m_buffers[index];
  CYuvPlane (&planes)[YuvImage::MAX_PLANES] = m_buffers[index].fields[field];

  if (buf.m_srcPrimaries != m_srcPrimaries)
  {
    m_srcPrimaries = buf.m_srcPrimaries;
    m_reloadShaders = true;
  }

  bool toneMap = false;
  ETONEMAPMETHOD toneMapMethod = m_videoSettings.m_ToneMapMethod;

  if (toneMapMethod != VS_TONEMAPMETHOD_OFF)
  {
    if (buf.hasLightMetadata || (buf.hasDisplayMetadata && buf.displayMetadata.has_luminance))
    {
      toneMap = true;
    }
  }

  if (toneMap != m_toneMap || m_toneMapMethod != toneMapMethod)
  {
    m_reloadShaders = true;
  }

  m_toneMap = toneMap;
  m_toneMapMethod = toneMapMethod;

  if (m_reloadShaders)
  {
    m_reloadShaders = 0;
    LoadShaders(m_currentField);
  }

  if (!m_fbo.fbo.IsValid())
  {
    if (!m_fbo.fbo.Initialize())
    {
      CLog::Log(LOGERROR, "GLES: Error initializing FBO");
      return;
    }

    if (!m_fbo.fbo.CreateAndBindToTexture(GL_TEXTURE_2D, m_sourceWidth, m_sourceHeight, GL_RGBA))
    {
      CLog::Log(LOGERROR, "GLES: Error creating texture and binding to FBO");
      return;
    }
  }

  // Y
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(m_textureTarget, planes[0].id);
  VerifyGLState();

  // U
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(m_textureTarget, planes[1].id);
  VerifyGLState();

  // V
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(m_textureTarget, planes[2].id);
  VerifyGLState();

  glActiveTexture(GL_TEXTURE0);
  VerifyGLState();

  Shaders::GLES::BaseYUV2RGBGLSLShader* pYUVShader = m_pYUVProgShader;
  // make sure the yuv shader is loaded and ready to go
  if (!pYUVShader || (!pYUVShader->OK()))
  {
    CLog::Log(LOGERROR, "GLES: YUV shader not active, cannot do multipass render");
    return;
  }

  m_fbo.fbo.BeginRender();
  VerifyGLState();

  pYUVShader->SetBlack(m_videoSettings.m_Brightness * 0.01f - 0.5f);
  pYUVShader->SetContrast(m_videoSettings.m_Contrast * 0.02f);
  pYUVShader->SetWidth(planes[0].texwidth);
  pYUVShader->SetHeight(planes[0].texheight);
  pYUVShader->SetColParams(buf.m_srcColSpace, buf.m_srcBits, !buf.m_srcFullRange, buf.m_srcTextureBits);
  pYUVShader->SetDisplayMetadata(buf.hasDisplayMetadata, buf.displayMetadata,
                                 buf.hasLightMetadata, buf.lightMetadata);
  pYUVShader->SetToneMapParam(m_videoSettings.m_ToneMapParam);

  if (field == FIELD_TOP)
  {
    pYUVShader->SetField(1);
  }
  else if(field == FIELD_BOT)
  {
    pYUVShader->SetField(0);
  }

  VerifyGLState();

  glMatrixModview.Push();
  glMatrixModview->LoadIdentity();
  glMatrixModview.Load();

  glMatrixProject.Push();
  glMatrixProject->LoadIdentity();
  glMatrixProject->Ortho2D(0, m_sourceWidth, 0, m_sourceHeight);
  glMatrixProject.Load();

  pYUVShader->SetMatrices(glMatrixProject.Get(), glMatrixModview.Get());

  CRect viewport;
  m_renderSystem->GetViewPort(viewport);
  glViewport(0, 0, m_sourceWidth, m_sourceHeight);
  glScissor(0, 0, m_sourceWidth, m_sourceHeight);

  if (!pYUVShader->Enable())
  {
    CLog::Log(LOGERROR, "GLES: Error enabling YUV shader");
  }

  m_fbo.width  = planes[0].rect.x2 - planes[0].rect.x1;
  m_fbo.height = planes[0].rect.y2 - planes[0].rect.y1;

  if (m_textureTarget == GL_TEXTURE_2D)
  {
    m_fbo.width  *= planes[0].texwidth;
    m_fbo.height *= planes[0].texheight;
  }

  m_fbo.width  *= planes[0].pixpertex_x;
  m_fbo.height *= planes[0].pixpertex_y;

  // 1st Pass to video frame size
  GLubyte idx[4] = {0, 1, 3, 2}; // determines order of triangle strip
  GLfloat vert[4][3];
  GLfloat tex[3][4][2];

  GLint vertLoc = pYUVShader->GetVertexLoc();
  GLint Yloc = pYUVShader->GetYcoordLoc();
  GLint Uloc = pYUVShader->GetUcoordLoc();
  GLint Vloc = pYUVShader->GetVcoordLoc();

  glVertexAttribPointer(vertLoc, 3, GL_FLOAT, 0, 0, vert);
  glVertexAttribPointer(Yloc, 2, GL_FLOAT, 0, 0, tex[0]);
  glVertexAttribPointer(Uloc, 2, GL_FLOAT, 0, 0, tex[1]);
  glVertexAttribPointer(Vloc, 2, GL_FLOAT, 0, 0, tex[2]);

  glEnableVertexAttribArray(vertLoc);
  glEnableVertexAttribArray(Yloc);
  glEnableVertexAttribArray(Uloc);
  glEnableVertexAttribArray(Vloc);

  // Setup vertex position values
  // Set vertex coordinates
  vert[0][0] = vert[3][0] = 0.0f;
  vert[0][1] = vert[1][1] = 0.0f;
  vert[1][0] = vert[2][0] = m_fbo.width;
  vert[2][1] = vert[3][1] = m_fbo.height;
  vert[0][2] = vert[1][2] = vert[2][2] = vert[3][2] = 0.0f;

  // Setup texture coordinates
  for (int i = 0; i < 3; i++)
  {
    tex[i][0][0] = tex[i][3][0] = planes[i].rect.x1;
    tex[i][0][1] = tex[i][1][1] = planes[i].rect.y1;
    tex[i][1][0] = tex[i][2][0] = planes[i].rect.x2;
    tex[i][2][1] = tex[i][3][1] = planes[i].rect.y2;
  }

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);

  VerifyGLState();

  pYUVShader->Disable();

  glMatrixModview.PopLoad();
  glMatrixProject.PopLoad();

  VerifyGLState();

  glDisableVertexAttribArray(vertLoc);
  glDisableVertexAttribArray(Yloc);
  glDisableVertexAttribArray(Uloc);
  glDisableVertexAttribArray(Vloc);

  m_renderSystem->SetViewPort(viewport);

  m_fbo.fbo.EndRender();

  VerifyGLState();
}

void CLinuxRendererGLES::RenderFromFBO()
{
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_fbo.fbo.Texture());
  VerifyGLState();

  // Use regular normalized texture coordinates

  // 2nd Pass to screen size with optional video filter

  if (m_pVideoFilterShader)
  {
    GLint filter;
    if (!m_pVideoFilterShader->GetTextureFilter(filter))
    {
      filter = m_scalingMethod == VS_SCALINGMETHOD_NEAREST ? GL_NEAREST : GL_LINEAR;
    }

    m_fbo.fbo.SetFiltering(GL_TEXTURE_2D, filter);
    m_pVideoFilterShader->SetSourceTexture(0);
    m_pVideoFilterShader->SetWidth(m_sourceWidth);
    m_pVideoFilterShader->SetHeight(m_sourceHeight);
    m_pVideoFilterShader->SetAlpha(1.0f);
    m_pVideoFilterShader->SetMatrices(glMatrixProject.Get(), glMatrixModview.Get());
    m_pVideoFilterShader->Enable();
  }
  else
  {
    GLint filter = m_scalingMethod == VS_SCALINGMETHOD_NEAREST ? GL_NEAREST : GL_LINEAR;
    m_fbo.fbo.SetFiltering(GL_TEXTURE_2D, filter);
  }

  VerifyGLState();

  float imgwidth = m_fbo.width / m_sourceWidth;
  float imgheight = m_fbo.height / m_sourceHeight;

  GLubyte idx[4] = {0, 1, 3, 2}; // determines order of triangle strip
  GLfloat vert[4][3];
  GLfloat tex[4][2];

  GLint vertLoc = m_pVideoFilterShader->GetVertexLoc();
  GLint loc = m_pVideoFilterShader->GetcoordLoc();

  glVertexAttribPointer(vertLoc, 3, GL_FLOAT, 0, 0, vert);
  glVertexAttribPointer(loc, 2, GL_FLOAT, 0, 0, tex);

  glEnableVertexAttribArray(vertLoc);
  glEnableVertexAttribArray(loc);

  // Setup vertex position values
  for(int i = 0; i < 4; i++)
  {
    vert[i][0] = m_rotatedDestCoords[i].x;
    vert[i][1] = m_rotatedDestCoords[i].y;
    vert[i][2] = 0.0f; // set z to 0
  }

  // Setup texture coordinates
  tex[0][0] = tex[3][0] = 0.0f;
  tex[0][1] = tex[1][1] = 0.0f;
  tex[1][0] = tex[2][0] = imgwidth;
  tex[2][1] = tex[3][1] = imgheight;

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);

  glDisableVertexAttribArray(loc);
  glDisableVertexAttribArray(vertLoc);

  VerifyGLState();

  if (m_pVideoFilterShader)
  {
    m_pVideoFilterShader->Disable();
  }

  VerifyGLState();

  glBindTexture(GL_TEXTURE_2D, 0);
  VerifyGLState();
}

bool CLinuxRendererGLES::RenderCapture(int index, CRenderCapture* capture)
{
  if (!m_bValidated)
  {
    return false;
  }

  // save current video rect
  CRect saveSize = m_destRect;
  saveRotatedCoords(); // backup current m_rotatedDestCoords

  // new video rect is thumbnail size
  m_destRect.SetRect(0, 0, static_cast<float>(capture->GetWidth()), static_cast<float>(capture->GetHeight()));
  MarkDirty();
  syncDestRectToRotatedPoints(); // syncs the changed destRect to m_rotatedDestCoords

  // clear framebuffer and invert Y axis to get non-inverted image
  glDisable(GL_BLEND);

  glMatrixModview.Push();
  glMatrixModview->Translatef(0.0f, capture->GetHeight(), 0.0f);
  glMatrixModview->Scalef(1.0f, -1.0f, 1.0f);
  glMatrixModview.Load();

  capture->BeginRender();

  Render(RENDER_FLAG_NOOSD, index);
  // read pixels
  glReadPixels(0, CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight() - capture->GetHeight(), capture->GetWidth(), capture->GetHeight(),
               GL_RGBA, GL_UNSIGNED_BYTE, capture->GetRenderBuffer());

  // OpenGLES returns in RGBA order but CRenderCapture needs BGRA order
  // XOR Swap RGBA -> BGRA
  unsigned char* pixels = static_cast<unsigned char*>(capture->GetRenderBuffer());
  for (unsigned int i = 0; i < capture->GetWidth() * capture->GetHeight(); i++, pixels += 4)
  {
    std::swap(pixels[0], pixels[2]);
  }

  capture->EndRender();

  // revert model view matrix
  glMatrixModview.PopLoad();

  // restore original video rect
  m_destRect = saveSize;
  restoreRotatedCoords(); // restores the previous state of the rotated dest coords

  return true;
}

//********************************************************************************************************/
// YV12 Texture creation, deletion, copying + clearing
//********************************************************************************************************/
bool CLinuxRendererGLES::UploadYV12Texture(int source)
{
  CPictureBuffer& buf = m_buffers[source];
  YuvImage* im = &buf.image;

  VerifyGLState();

  glPixelStorei(GL_UNPACK_ALIGNMENT,1);

  // load Y plane
  LoadPlane(buf.fields[FIELD_FULL][0], GL_LUMINANCE,
            im->width, im->height,
            im->stride[0], im->bpp, im->plane[0]);

  // load U plane
  LoadPlane(buf.fields[FIELD_FULL][1], GL_LUMINANCE,
            im->width >> im->cshift_x, im->height >> im->cshift_y,
            im->stride[1], im->bpp, im->plane[1]);

  // load V plane
  LoadPlane(buf.fields[FIELD_FULL][2], GL_ALPHA,
            im->width >> im->cshift_x, im->height >> im->cshift_y,
            im->stride[2], im->bpp, im->plane[2]);

  VerifyGLState();

  CalculateTextureSourceRects(source, 3);

  return true;
}

void CLinuxRendererGLES::DeleteYV12Texture(int index)
{
  YuvImage &im = m_buffers[index].image;

  if (m_buffers[index].fields[FIELD_FULL][0].id == 0)
  {
    return;
  }

  // finish up all textures, and delete them
  for(int f = 0; f < MAX_FIELDS; f++)
  {
    for(int p = 0; p < YuvImage::MAX_PLANES; p++)
    {
      if (m_buffers[index].fields[f][p].id)
      {
        if (glIsTexture(m_buffers[index].fields[f][p].id))
        {
          glDeleteTextures(1, &m_buffers[index].fields[f][p].id);
        }

        m_buffers[index].fields[f][p].id = 0;
      }
    }
  }

  for(int p = 0; p < YuvImage::MAX_PLANES; p++)
  {
    im.plane[p] = nullptr;
  }
}

bool CLinuxRendererGLES::CreateYV12Texture(int index)
{
  // since we also want the field textures, pitch must be texture aligned
  unsigned p;
  YuvImage &im = m_buffers[index].image;

  DeleteYV12Texture(index);

  im.height = m_sourceHeight;
  im.width = m_sourceWidth;
  im.cshift_x = 1;
  im.cshift_y = 1;
  im.bpp = 1;

  im.stride[0] = im.bpp * im.width;
  im.stride[1] = im.bpp * (im.width >> im.cshift_x);
  im.stride[2] = im.bpp * (im.width >> im.cshift_x);

  im.planesize[0] = im.stride[0] * im.height;
  im.planesize[1] = im.stride[1] * (im.height >> im.cshift_y);
  im.planesize[2] = im.stride[2] * (im.height >> im.cshift_y);

  for (int i = 0; i < 3; i++)
  {
    im.plane[i] = nullptr; // will be set in UploadTexture()
  }

  for(int f = 0; f < MAX_FIELDS; f++)
  {
    for(p = 0; p < YuvImage::MAX_PLANES; p++)
    {
      if (!glIsTexture(m_buffers[index].fields[f][p].id))
      {
        glGenTextures(1, &m_buffers[index].fields[f][p].id);
        VerifyGLState();
      }
    }
  }

  // YUV
  for (int f = FIELD_FULL; f <= FIELD_BOT ; f++)
  {
    int fieldshift = (f == FIELD_FULL) ? 0 : 1;
    CYuvPlane (&planes)[YuvImage::MAX_PLANES] = m_buffers[index].fields[f];

    planes[0].texwidth  = im.width;
    planes[0].texheight = im.height >> fieldshift;

    planes[1].texwidth  = planes[0].texwidth  >> im.cshift_x;
    planes[1].texheight = planes[0].texheight >> im.cshift_y;
    planes[2].texwidth  = planes[0].texwidth  >> im.cshift_x;
    planes[2].texheight = planes[0].texheight >> im.cshift_y;

    for (int p = 0; p < 3; p++)
    {
      planes[p].pixpertex_x = 1;
      planes[p].pixpertex_y = 1;
    }

    for(int p = 0; p < 3; p++)
    {
      CYuvPlane &plane = planes[p];
      if (plane.texwidth * plane.texheight == 0)
      {
        continue;
      }

      glBindTexture(m_textureTarget, plane.id);

      GLint format;
      if (p == 2) // V plane needs an alpha texture
      {
        format = GL_ALPHA;
      }
      else
      {
        format = GL_LUMINANCE;
      }

      glTexImage2D(m_textureTarget, 0, format, plane.texwidth, plane.texheight, 0, format, GL_UNSIGNED_BYTE, nullptr);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      VerifyGLState();
    }
  }
  return true;
}

//********************************************************************************************************
// NV12 Texture loading, creation and deletion
//********************************************************************************************************
bool CLinuxRendererGLES::UploadNV12Texture(int source)
{
  CPictureBuffer& buf = m_buffers[source];
  YuvImage* im = &buf.image;

  bool deinterlacing;
  if (m_currentField == FIELD_FULL)
  {
    deinterlacing = false;
  }
  else
  {
    deinterlacing = true;
  }

  VerifyGLState();

  glPixelStorei(GL_UNPACK_ALIGNMENT, im->bpp);

  if (deinterlacing)
  {
    // Load Odd Y field
    LoadPlane(buf.fields[FIELD_TOP][0] , GL_LUMINANCE,
              im->width, im->height >> 1,
              im->stride[0]*2, im->bpp, im->plane[0]);

    // Load Even Y field
    LoadPlane(buf.fields[FIELD_BOT][0], GL_LUMINANCE,
              im->width, im->height >> 1,
              im->stride[0]*2, im->bpp, im->plane[0] + im->stride[0]) ;

    // Load Odd UV Fields
    LoadPlane(buf.fields[FIELD_TOP][1], GL_LUMINANCE_ALPHA,
              im->width >> im->cshift_x, im->height >> (im->cshift_y + 1),
              im->stride[1]*2, im->bpp, im->plane[1]);

    // Load Even UV Fields
    LoadPlane(buf.fields[FIELD_BOT][1], GL_LUMINANCE_ALPHA,
              im->width >> im->cshift_x, im->height >> (im->cshift_y + 1),
              im->stride[1]*2, im->bpp, im->plane[1] + im->stride[1]);

  }
  else
  {
    // Load Y plane
    LoadPlane(buf. fields[FIELD_FULL][0], GL_LUMINANCE,
              im->width, im->height,
              im->stride[0], im->bpp, im->plane[0]);

    // Load UV plane
    LoadPlane(buf.fields[FIELD_FULL][1], GL_LUMINANCE_ALPHA,
              im->width >> im->cshift_x, im->height >> im->cshift_y,
              im->stride[1], im->bpp, im->plane[1]);
  }

  VerifyGLState();

  CalculateTextureSourceRects(source, 3);

  return true;
}

bool CLinuxRendererGLES::CreateNV12Texture(int index)
{
  // since we also want the field textures, pitch must be texture aligned
  CPictureBuffer& buf = m_buffers[index];
  YuvImage &im = buf.image;

  // Delete any old texture
  DeleteNV12Texture(index);

  im.height = m_sourceHeight;
  im.width  = m_sourceWidth;
  im.cshift_x = 1;
  im.cshift_y = 1;
  im.bpp = 1;

  im.stride[0] = im.width;
  im.stride[1] = im.width;
  im.stride[2] = 0;

  im.plane[0] = nullptr;
  im.plane[1] = nullptr;
  im.plane[2] = nullptr;

  // Y plane
  im.planesize[0] = im.stride[0] * im.height;
  // packed UV plane
  im.planesize[1] = im.stride[1] * im.height / 2;
  // third plane is not used
  im.planesize[2] = 0;

  for (int i = 0; i < 2; i++)
  {
    im.plane[i] = nullptr; // will be set in UploadTexture()
  }

  for(int f = 0; f < MAX_FIELDS; f++)
  {
    for(int p = 0; p < 2; p++)
    {
      if (!glIsTexture(buf.fields[f][p].id))
      {
        glGenTextures(1, &buf.fields[f][p].id);
        VerifyGLState();
      }
    }

    buf.fields[f][2].id = buf.fields[f][1].id;
  }

  // YUV
  for (int f = FIELD_FULL; f <= FIELD_BOT; f++)
  {
    int fieldshift = (f == FIELD_FULL) ? 0 : 1;
    CYuvPlane (&planes)[YuvImage::MAX_PLANES] = buf.fields[f];

    planes[0].texwidth  = im.width;
    planes[0].texheight = im.height >> fieldshift;

    planes[1].texwidth  = planes[0].texwidth  >> im.cshift_x;
    planes[1].texheight = planes[0].texheight >> im.cshift_y;
    planes[2].texwidth  = planes[1].texwidth;
    planes[2].texheight = planes[1].texheight;

    for (int p = 0; p < 3; p++)
    {
      planes[p].pixpertex_x = 1;
      planes[p].pixpertex_y = 1;
    }

    for(int p = 0; p < 2; p++)
    {
      CYuvPlane &plane = planes[p];
      if (plane.texwidth * plane.texheight == 0)
      {
        continue;
      }

      glBindTexture(m_textureTarget, plane.id);

      if (p == 1)
      {
        glTexImage2D(m_textureTarget, 0, GL_LUMINANCE_ALPHA, plane.texwidth, plane.texheight, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, nullptr);
      }
      else
      {
        glTexImage2D(m_textureTarget, 0, GL_LUMINANCE, plane.texwidth, plane.texheight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, nullptr);
      }

      glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      VerifyGLState();
    }
  }

  return true;
}

void CLinuxRendererGLES::DeleteNV12Texture(int index)
{
  CPictureBuffer& buf = m_buffers[index];
  YuvImage &im = buf.image;

  if (buf.fields[FIELD_FULL][0].id == 0)
  {
    return;
  }

  // finish up all textures, and delete them
  for(int f = 0; f < MAX_FIELDS; f++)
  {
    for(int p = 0; p < 2; p++)
    {
      if (buf.fields[f][p].id)
      {
        if (glIsTexture(buf.fields[f][p].id))
        {
          glDeleteTextures(1, &buf.fields[f][p].id);
        }

        buf.fields[f][p].id = 0;
      }
    }

    buf.fields[f][2].id = 0;
  }

  for(int p = 0; p < 2; p++)
  {
    im.plane[p] = nullptr;
  }
}

//********************************************************************************************************
// SurfaceTexture creation, deletion, copying + clearing
//********************************************************************************************************
void CLinuxRendererGLES::SetTextureFilter(GLenum method)
{
  for (int i = 0 ; i < m_NumYV12Buffers; i++)
  {
    CPictureBuffer& buf = m_buffers[i];

    for (int f = FIELD_FULL; f <= FIELD_BOT; f++)
    {
      for (int p = 0; p < 3; p++)
      {
        if(glIsTexture(buf.fields[f][p].id))
        {
          glBindTexture(m_textureTarget, buf.fields[f][p].id);
          glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, method);
          glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, method);
          VerifyGLState();
        }
      }
    }
  }
}

bool CLinuxRendererGLES::Supports(ERENDERFEATURE feature) const
{
  if (feature == RENDERFEATURE_GAMMA ||
      feature == RENDERFEATURE_NOISE ||
      feature == RENDERFEATURE_SHARPNESS ||
      feature == RENDERFEATURE_NONLINSTRETCH)
  {
    return false;
  }

  if (feature == RENDERFEATURE_STRETCH ||
      feature == RENDERFEATURE_ZOOM ||
      feature == RENDERFEATURE_VERTICAL_SHIFT ||
      feature == RENDERFEATURE_PIXEL_RATIO ||
      feature == RENDERFEATURE_POSTPROCESS ||
      feature == RENDERFEATURE_ROTATION ||
      feature == RENDERFEATURE_BRIGHTNESS ||
      feature == RENDERFEATURE_CONTRAST ||
      feature == RENDERFEATURE_TONEMAP)
  {
    return true;
  }

  return false;
}

bool CLinuxRendererGLES::SupportsMultiPassRendering()
{
  return true;
}

bool CLinuxRendererGLES::Supports(ESCALINGMETHOD method) const
{
  if(method == VS_SCALINGMETHOD_NEAREST ||
     method == VS_SCALINGMETHOD_LINEAR)
  {
    return true;
  }

  if (method == VS_SCALINGMETHOD_CUBIC_B_SPLINE ||
      method == VS_SCALINGMETHOD_CUBIC_MITCHELL ||
      method == VS_SCALINGMETHOD_CUBIC_CATMULL ||
      method == VS_SCALINGMETHOD_CUBIC_0_075 ||
      method == VS_SCALINGMETHOD_CUBIC_0_1 ||
      method == VS_SCALINGMETHOD_LANCZOS2 ||
      method == VS_SCALINGMETHOD_SPLINE36_FAST ||
      method == VS_SCALINGMETHOD_LANCZOS3_FAST ||
      method == VS_SCALINGMETHOD_SPLINE36 ||
      method == VS_SCALINGMETHOD_LANCZOS3)
  {
    // if scaling is below level, avoid hq scaling
    float scaleX = fabs((static_cast<float>(m_sourceWidth) - m_destRect.Width()) / m_sourceWidth) * 100;
    float scaleY = fabs((static_cast<float>(m_sourceHeight) - m_destRect.Height()) / m_sourceHeight) * 100;
    int minScale = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_HQSCALERS);
    if (scaleX < minScale && scaleY < minScale)
    {
      return false;
    }

    if (m_renderMethod & RENDER_GLSL)
    {
      return true;
    }
  }

  return false;
}

CRenderInfo CLinuxRendererGLES::GetRenderInfo()
{
  CRenderInfo info;
  info.max_buffer_size = NUM_BUFFERS;

  return info;
}

bool CLinuxRendererGLES::IsGuiLayer()
{
  return true;
}

CRenderCapture* CLinuxRendererGLES::GetRenderCapture()
{
  return new CRenderCaptureGLES;
}
