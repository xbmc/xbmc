/*
 *  Copyright (c) 2007 Frodo/jcmarshall/vulkanr/d4rk
 *      Based on XBoxRenderer by Frodo/jcmarshall
 *      Portions Copyright (c) by the authors of ffmpeg / xvid /mplayer
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LinuxRendererGL.h"

#include "RenderCapture.h"
#include "RenderCaptureGL.h"
#include "RenderFactory.h"
#include "ServiceBroker.h"
#include "VideoShaders/VideoFilterShaderGL.h"
#include "VideoShaders/YUV2RGBShaderGL.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "cores/IPlayer.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"
#include "rendering/MatrixGL.h"
#include "rendering/gl/RenderSystemGL.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/GLUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#ifdef TARGET_DARWIN_OSX
#include "platform/darwin/osx/CocoaInterface.h"
#endif

#include <locale.h>
#include <memory>
#include <mutex>

#if defined(TARGET_DARWIN_OSX)
#include <CoreVideo/CoreVideo.h>
#include <OpenGL/CGLIOSurface.h>
#endif

//! @bug
//! due to a bug on osx nvidia, using gltexsubimage2d with a pbo bound and a null pointer
//! screws up the alpha, an offset fixes this, there might still be a problem if stride + PBO_OFFSET
//! is a multiple of 128 and deinterlacing is on
#define PBO_OFFSET 16

using namespace Shaders;
using namespace Shaders::GL;

static const GLubyte stipple_weave[] = {
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
};

CLinuxRendererGL::CPictureBuffer::CPictureBuffer()
{
  memset(&fields, 0, sizeof(fields));
  memset(&image , 0, sizeof(image));
  memset(&pbo   , 0, sizeof(pbo));
  videoBuffer = nullptr;
  loaded = false;
}

CBaseRenderer* CLinuxRendererGL::Create(CVideoBuffer *buffer)
{
  return new CLinuxRendererGL();
}

bool CLinuxRendererGL::Register()
{
  VIDEOPLAYER::CRendererFactory::RegisterRenderer("default", CLinuxRendererGL::Create);
  return true;
}

CLinuxRendererGL::CLinuxRendererGL()
{
  m_iFlags = 0;
  m_format = AV_PIX_FMT_NONE;

  std::tie(m_useDithering, m_ditherDepth) = CServiceBroker::GetWinSystem()->GetDitherSettings();

  m_fullRange = !CServiceBroker::GetWinSystem()->UseLimitedColor();

  m_fbo.width = 0.0;
  m_fbo.height = 0.0;

  m_ColorManager = std::make_unique<CColorManager>();
  m_tCLUTTex = 0;
  m_CLUT = NULL;
  m_CLUTsize = 0;
  m_cmsToken = -1;
  m_cmsOn = false;

  m_renderSystem = dynamic_cast<CRenderSystemGL*>(CServiceBroker::GetRenderSystem());
}

CLinuxRendererGL::~CLinuxRendererGL()
{
  UnInit();

  if (m_pYUVShader)
  {
    delete m_pYUVShader;
    m_pYUVShader = nullptr;
  }

  if (m_pVideoFilterShader)
  {
    delete m_pVideoFilterShader;
    m_pVideoFilterShader = nullptr;
  }
}

bool CLinuxRendererGL::ValidateRenderer()
{
  if (!m_bConfigured)
    return false;

  // if its first pass, just init textures and return
  if (ValidateRenderTarget())
    return false;

  int index = m_iYV12RenderBuffer;
  const CPictureBuffer& buf = m_buffers[index];

  if (!buf.fields[FIELD_FULL][0].id)
    return false;

  return true;
}

bool CLinuxRendererGL::ValidateRenderTarget()
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

    // trigger update of video filters
    m_scalingMethodGui = (ESCALINGMETHOD)-1;

     // create the yuv textures
    UpdateVideoFilter();
    LoadShaders();
    if (m_renderMethod < 0)
      return false;

    if (m_textureTarget == GL_TEXTURE_RECTANGLE)
      CLog::Log(LOGINFO, "Using GL_TEXTURE_RECTANGLE");
    else
      CLog::Log(LOGINFO, "Using GL_TEXTURE_2D");

    for (int i = 0 ; i < m_NumYV12Buffers ; i++)
      CreateTexture(i);

    m_bValidated = true;
    return true;
  }
  return false;
}

bool CLinuxRendererGL::Configure(const VideoPicture &picture, float fps, unsigned int orientation)
{
  m_format = picture.videoBuffer->GetFormat();
  m_sourceWidth = picture.iWidth;
  m_sourceHeight = picture.iHeight;
  m_renderOrientation = orientation;
  m_fps = fps;

  m_iFlags = GetFlagsChromaPosition(picture.chroma_position) |
             GetFlagsStereoMode(picture.stereoMode);

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

  m_nonLinStretch = false;
  m_nonLinStretchGui = false;
  m_pixelRatio = 1.0;

  m_pboSupported = CServiceBroker::GetRenderSystem()->IsExtSupported("GL_ARB_pixel_buffer_object");

  // setup the background colour
  m_clearColour = CServiceBroker::GetWinSystem()->UseLimitedColor() ? (16.0f / 0xff) : 0.0f;

  // load 3DLUT
  if (m_ColorManager->IsEnabled())
  {
    if (!m_ColorManager->CheckConfiguration(m_cmsToken, m_srcPrimaries))
    {
      CLog::Log(LOGDEBUG, "CMS configuration changed, reload LUT");
      if (!LoadCLUT())
        return false;
    }
    m_cmsOn = true;
  }
  else
  {
    m_cmsOn = false;
  }

  return true;
}

bool CLinuxRendererGL::ConfigChanged(const VideoPicture &picture)
{
  if (picture.videoBuffer->GetFormat() != m_format)
    return true;

  return false;
}

void CLinuxRendererGL::AddVideoPicture(const VideoPicture &picture, int index)
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
    buf.hasLightMetadata = picture.hasLightMetadata;
}

void CLinuxRendererGL::ReleaseBuffer(int idx)
{
  CPictureBuffer &buf = m_buffers[idx];
  if (buf.videoBuffer)
  {
    buf.videoBuffer->Release();
    buf.videoBuffer = nullptr;
  }
}

void CLinuxRendererGL::GetPlaneTextureSize(CYuvPlane& plane)
{
  /* texture is assumed to be bound */
  GLint width  = 0
      , height = 0
      , border = 0;
  glGetTexLevelParameteriv(m_textureTarget, 0, GL_TEXTURE_WIDTH , &width);
  glGetTexLevelParameteriv(m_textureTarget, 0, GL_TEXTURE_HEIGHT, &height);
  glGetTexLevelParameteriv(m_textureTarget, 0, GL_TEXTURE_BORDER, &border);
  plane.texwidth  = width  - 2 * border;
  plane.texheight = height - 2 * border;
  if(plane.texwidth <= 0 || plane.texheight <= 0)
  {
    CLog::Log(LOGDEBUG, "CLinuxRendererGL::GetPlaneTextureSize - invalid size {}x{} - {}", width,
              height, border);
    /* to something that avoid division by zero */
    plane.texwidth  = 1;
    plane.texheight = 1;
  }
}

void CLinuxRendererGL::CalculateTextureSourceRects(int source, int num_planes)
{
  CPictureBuffer& buf = m_buffers[source];
  YuvImage* im  = &buf.image;

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
        /* correct for field offsets and chroma offsets */
        float offset_y = 0.5f;
        if(plane != 0)
          offset_y += 0.5f;
        if(field == FIELD_BOT)
          offset_y *= -1;

        p.rect.y1 += offset_y;
        p.rect.y2 += offset_y;

        /* half the height if this is a field */
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

void CLinuxRendererGL::LoadPlane(CYuvPlane& plane, int type,
                                 unsigned width, unsigned height,
                                 int stride, int bpp, void* data)
{

  if (plane.pbo)
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, plane.pbo);

  int bps = bpp * KODI::UTILS::GL::glFormatElementByteCount(type);

  unsigned datatype;
  if (bpp == 2)
    datatype = GL_UNSIGNED_SHORT;
  else
    datatype = GL_UNSIGNED_BYTE;

  glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / bps);
  glBindTexture(m_textureTarget, plane.id);
  glTexSubImage2D(m_textureTarget, 0, 0, 0, width, height, type, datatype, data);

  /* check if we need to load any border pixels */
  if (height < plane.texheight)
    glTexSubImage2D( m_textureTarget, 0
                   , 0, height, width, 1
                   , type, datatype
                   , (unsigned char*)data + stride * (height-1));

  if (width  < plane.texwidth)
    glTexSubImage2D( m_textureTarget, 0
                   , width, 0, 1, height
                   , type, datatype
                   , (unsigned char*)data + bps * (width-1));

  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glBindTexture(m_textureTarget, 0);
  if (plane.pbo)
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

bool CLinuxRendererGL::Flush(bool saveBuffers)
{
  bool safe = saveBuffers && CanSaveBuffers();
  glFinish();

  for (int i = 0 ; i < m_NumYV12Buffers ; i++)
  {
    if (!safe)
      ReleaseBuffer(i);
    DeleteTexture(i);
  }

  delete m_pYUVShader;
  m_pYUVShader = nullptr;
  delete m_pVideoFilterShader;
  m_pVideoFilterShader = nullptr;

  glFinish();
  m_bValidated = false;
  m_fbo.fbo.Cleanup();
  m_iYV12RenderBuffer = 0;

  return safe;
}

void CLinuxRendererGL::Update()
{
  if (!m_bConfigured)
    return;
  ManageRenderArea();
  m_scalingMethodGui = (ESCALINGMETHOD)-1;

  ValidateRenderTarget();
}

void CLinuxRendererGL::RenderUpdate(int index, int index2, bool clear, unsigned int flags, unsigned int alpha)
{
  if (index2 >= 0)
    m_iYV12RenderBuffer = index2;
  else
    m_iYV12RenderBuffer = index;

  if (!ValidateRenderer())
  {
    if (clear) //if clear is set, we're expected to overwrite all backbuffer pixels, even if we have nothing to render
      ClearBackBuffer();

    return;
  }

  ManageRenderArea();

  if (clear)
  {
    //draw black bars when video is not transparent, clear the entire backbuffer when it is
    if (alpha == 255)
      DrawBlackBars();
    else
      ClearBackBuffer();
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

  if (m_pYUVShader)
    m_pYUVShader->SetAlpha(alpha/255);
  if (m_pVideoFilterShader)
    m_pVideoFilterShader->SetAlpha(alpha/255);

  if (!Render(flags, m_iYV12RenderBuffer) && clear)
    ClearBackBuffer();

  if (index2 >= 0)
  {
    m_iYV12RenderBuffer = index;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (m_pYUVShader)
      m_pYUVShader->SetAlpha(alpha/255/2);
    if (m_pVideoFilterShader)
      m_pVideoFilterShader->SetAlpha(alpha/255/2);

    Render(flags, m_iYV12RenderBuffer);
  }

  VerifyGLState();
  glEnable(GL_BLEND);
  glFlush();
}

void CLinuxRendererGL::ClearBackBuffer()
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

void CLinuxRendererGL::ClearBackBufferQuad()
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

  m_renderSystem->EnableShader(ShaderMethodGL::SM_DEFAULT);
  GLint posLoc = m_renderSystem->ShaderGetPos();
  GLint uniCol = m_renderSystem->ShaderGetUniCol();

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

  m_renderSystem->DisableShader();
}

//draw black bars around the video quad, this is more efficient than glClear()
//since it only sets pixels to black that aren't going to be overwritten by the video
void CLinuxRendererGL::DrawBlackBars()
{
  glDisable(GL_BLEND);

  struct Svertex
  {
    float x,y,z;
  };
  Svertex vertices[24];
  GLubyte count = 0;

  m_renderSystem->EnableShader(ShaderMethodGL::SM_DEFAULT);
  GLint posLoc = m_renderSystem->ShaderGetPos();
  GLint uniCol = m_renderSystem->ShaderGetUniCol();

  glUniform4f(uniCol, m_clearColour / 255.0f, m_clearColour / 255.0f, m_clearColour / 255.0f, 1.0f);

  int osWindowWidth = CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth();
  int osWindowHeight = CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight();

  //top quad
  if (m_destRect.y1 > 0.0f)
  {
    GLubyte quad = count;
    vertices[quad].x = 0.0;
    vertices[quad].y = 0.0;
    vertices[quad].z = 0;
    vertices[quad+1].x = osWindowWidth;
    vertices[quad+1].y = 0;
    vertices[quad+1].z = 0;
    vertices[quad+2].x = osWindowWidth;
    vertices[quad+2].y = m_destRect.y1;
    vertices[quad+2].z = 0;
    vertices[quad+3] = vertices[quad+2];
    vertices[quad+4].x = 0;
    vertices[quad+4].y = m_destRect.y1;
    vertices[quad+4].z = 0;
    vertices[quad+5] = vertices[quad];
    count += 6;
  }

  // bottom quad
  if (m_destRect.y2 < osWindowHeight)
  {
    GLubyte quad = count;
    vertices[quad].x = 0.0;
    vertices[quad].y = m_destRect.y2;
    vertices[quad].z = 0;
    vertices[quad+1].x = osWindowWidth;
    vertices[quad+1].y = m_destRect.y2;
    vertices[quad+1].z = 0;
    vertices[quad+2].x = osWindowWidth;
    vertices[quad+2].y = osWindowHeight;
    vertices[quad+2].z = 0;
    vertices[quad+3] = vertices[quad+2];
    vertices[quad+4].x = 0;
    vertices[quad+4].y = osWindowHeight;
    vertices[quad+4].z = 0;
    vertices[quad+5] = vertices[quad];
    count += 6;
  }

  // left quad
  if (m_destRect.x1 > 0.0f)
  {
    GLubyte quad = count;
    vertices[quad].x = 0.0;
    vertices[quad].y = m_destRect.y1;
    vertices[quad].z = 0;
    vertices[quad+1].x = m_destRect.x1;
    vertices[quad+1].y = m_destRect.y1;
    vertices[quad+1].z = 0;
    vertices[quad+2].x = m_destRect.x1;
    vertices[quad+2].y = m_destRect.y2;
    vertices[quad+2].z = 0;
    vertices[quad+3] = vertices[quad+2];
    vertices[quad+4].x = 0;
    vertices[quad+4].y = m_destRect.y2;
    vertices[quad+4].z = 0;
    vertices[quad+5] = vertices[quad];
    count += 6;
  }

  // right quad
  if (m_destRect.x2 < osWindowWidth)
  {
    GLubyte quad = count;
    vertices[quad].x = m_destRect.x2;
    vertices[quad].y = m_destRect.y1;
    vertices[quad].z = 0;
    vertices[quad+1].x = osWindowWidth;
    vertices[quad+1].y = m_destRect.y1;
    vertices[quad+1].z = 0;
    vertices[quad+2].x = osWindowWidth;
    vertices[quad+2].y = m_destRect.y2;
    vertices[quad+2].z = 0;
    vertices[quad+3] = vertices[quad+2];
    vertices[quad+4].x = m_destRect.x2;
    vertices[quad+4].y = m_destRect.y2;
    vertices[quad+4].z = 0;
    vertices[quad+5] = vertices[quad];
    count += 6;
  }

  GLuint vertexVBO;

  glGenBuffers(1, &vertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Svertex)*count, &vertices[0], GL_STATIC_DRAW);

  glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Svertex), 0);
  glEnableVertexAttribArray(posLoc);

  glDrawArrays(GL_TRIANGLES, 0, count);

  glDisableVertexAttribArray(posLoc);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &vertexVBO);

  m_renderSystem->DisableShader();
}

void CLinuxRendererGL::UpdateVideoFilter()
{
  if (!m_pVideoFilterShader)
  {
    m_pVideoFilterShader = new DefaultFilterShader();
    if (!m_pVideoFilterShader->CompileAndLink())
    {
      CLog::Log(LOGERROR, "CLinuxRendererGL::UpdateVideoFilter: Error compiling and linking video filter shader");
      return;
    }
  }

  bool pixelRatioChanged = (CDisplaySettings::GetInstance().GetPixelRatio() > 1.001f ||
                            CDisplaySettings::GetInstance().GetPixelRatio() < 0.999f) !=
                            (m_pixelRatio > 1.001f || m_pixelRatio < 0.999f);
  bool nonLinStretchChanged = false;
  bool cmsChanged = (m_cmsOn != m_ColorManager->IsEnabled()) ||
                    (m_cmsOn && !m_ColorManager->CheckConfiguration(m_cmsToken, m_srcPrimaries));
  if (m_nonLinStretchGui != CDisplaySettings::GetInstance().IsNonLinearStretched() || pixelRatioChanged)
  {
    m_nonLinStretchGui = CDisplaySettings::GetInstance().IsNonLinearStretched();
    m_pixelRatio = CDisplaySettings::GetInstance().GetPixelRatio();
    m_reloadShaders = 1;
    nonLinStretchChanged = true;

    if (m_nonLinStretchGui && (m_pixelRatio < 0.999f || m_pixelRatio > 1.001f) && Supports(RENDERFEATURE_NONLINSTRETCH))
    {
      m_nonLinStretch = true;
      CLog::Log(LOGDEBUG, "GL: Enabling non-linear stretch");
    }
    else
    {
      m_nonLinStretch = false;
      CLog::Log(LOGDEBUG, "GL: Disabling non-linear stretch");
    }
  }

  CRect srcRect, dstRect, viewRect;
  GetVideoRect(srcRect, dstRect, viewRect);

  if (m_scalingMethodGui == m_videoSettings.m_ScalingMethod &&
      viewRect.Height() == m_viewRect.Height() &&
      viewRect.Width() == m_viewRect.Width() &&
      !nonLinStretchChanged && !cmsChanged)
    return;
  else
    m_reloadShaders = 1;

  // recompile YUV shader when non-linear stretch is turned on/off
  // or when it's on and the scaling method changed
  if (m_nonLinStretch || nonLinStretchChanged)
    m_reloadShaders = 1;

  if (cmsChanged)
  {
    if (m_ColorManager->IsEnabled())
    {
      if (!m_ColorManager->CheckConfiguration(m_cmsToken, m_srcPrimaries))
      {
        CLog::Log(LOGDEBUG, "CMS configuration changed, reload LUT");
        LoadCLUT();
      }
      m_cmsOn = true;
    }
    else
    {
      m_cmsOn = false;
    }
  }

  m_scalingMethodGui = m_videoSettings.m_ScalingMethod;
  m_scalingMethod = m_scalingMethodGui;
  m_viewRect = viewRect;

  if (!Supports(m_scalingMethod))
  {
    CLog::Log(LOGWARNING,
              "CLinuxRendererGL::UpdateVideoFilter - chosen scaling method {}, is not supported by "
              "renderer",
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

  if (m_scalingMethod == VS_SCALINGMETHOD_AUTO)
  {
    bool scaleSD = m_sourceHeight < 720 && m_sourceWidth < 1280;
    bool scaleUp = (int)m_sourceHeight < m_viewRect.Height() && (int)m_sourceWidth < m_viewRect.Width();
    bool scaleFps = m_fps < CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoAutoScaleMaxFps + 0.01f;

    if (Supports(VS_SCALINGMETHOD_LANCZOS3_FAST) && scaleSD && scaleUp && scaleFps)
      m_scalingMethod = VS_SCALINGMETHOD_LANCZOS3_FAST;
    else
      m_scalingMethod = VS_SCALINGMETHOD_LINEAR;
  }

  switch (m_scalingMethod)
  {
  case VS_SCALINGMETHOD_NEAREST:
  case VS_SCALINGMETHOD_LINEAR:
    SetTextureFilter(m_scalingMethod == VS_SCALINGMETHOD_NEAREST ? GL_NEAREST : GL_LINEAR);
    m_renderQuality = RQ_SINGLEPASS;
    if (m_nonLinStretch)
    {
      m_pVideoFilterShader = new StretchFilterShader();
      if (!m_pVideoFilterShader->CompileAndLink())
      {
        CLog::Log(LOGERROR, "GL: Error compiling and linking video filter shader");
        break;
      }
    }
    else
    {
      m_pVideoFilterShader = new DefaultFilterShader();
      if (!m_pVideoFilterShader->CompileAndLink())
      {
        CLog::Log(LOGERROR, "GL: Error compiling and linking video filter shader");
        break;
      }
    }
    return;

  case VS_SCALINGMETHOD_LANCZOS3_FAST:
  case VS_SCALINGMETHOD_SPLINE36_FAST:
    {
      EShaderFormat fmt = GetShaderFormat();
      if (fmt == SHADER_NV12 || (fmt >= SHADER_YV12 && fmt <= SHADER_YV12_16))
      {
        unsigned int major, minor;
        m_renderSystem->GetRenderVersion(major, minor);
        if (major >= 4)
        {
          SetTextureFilter(GL_LINEAR);
          m_renderQuality = RQ_SINGLEPASS;
          return;
        }
      }

      [[fallthrough]];
    }

  case VS_SCALINGMETHOD_LANCZOS2:
  case VS_SCALINGMETHOD_SPLINE36:
  case VS_SCALINGMETHOD_LANCZOS3:
  case VS_SCALINGMETHOD_CUBIC_B_SPLINE:
  case VS_SCALINGMETHOD_CUBIC_MITCHELL:
  case VS_SCALINGMETHOD_CUBIC_CATMULL:
  case VS_SCALINGMETHOD_CUBIC_0_075:
  case VS_SCALINGMETHOD_CUBIC_0_1:
    if (m_renderMethod & RENDER_GLSL)
    {
      if (!m_fbo.fbo.Initialize())
      {
        CLog::Log(LOGERROR, "GL: Error initializing FBO");
        break;
      }

      if (!m_fbo.fbo.CreateAndBindToTexture(GL_TEXTURE_2D, m_sourceWidth, m_sourceHeight, GL_RGBA16, GL_SHORT))
      {
        CLog::Log(LOGERROR, "GL: Error creating texture and binding to FBO");
        break;
      }
    }

    GLSLOutput *out;
    out = new GLSLOutput(3,
        m_useDithering,
        m_ditherDepth,
        m_cmsOn ? m_fullRange : false,
        m_cmsOn ? m_tCLUTTex : 0,
        m_CLUTsize);
    m_pVideoFilterShader = new ConvolutionFilterShader(m_scalingMethod, m_nonLinStretch, out);
    if (!m_pVideoFilterShader->CompileAndLink())
    {
      CLog::Log(LOGERROR, "GL: Error compiling and linking video filter shader");
      break;
    }

    SetTextureFilter(GL_LINEAR);
    m_renderQuality = RQ_MULTIPASS;
    return;

  case VS_SCALINGMETHOD_BICUBIC_SOFTWARE:
  case VS_SCALINGMETHOD_LANCZOS_SOFTWARE:
  case VS_SCALINGMETHOD_SINC_SOFTWARE:
  case VS_SCALINGMETHOD_SINC8:
    CLog::Log(LOGERROR, "GL: TODO: This scaler has not yet been implemented");
    break;

  default:
    break;
  }

  CLog::Log(LOGERROR, "GL: Falling back to bilinear due to failure to init scaler");
  if (m_pVideoFilterShader)
  {
    delete m_pVideoFilterShader;
    m_pVideoFilterShader = nullptr;
  }
  m_fbo.fbo.Cleanup();

  m_pVideoFilterShader = new DefaultFilterShader();
  if (!m_pVideoFilterShader->CompileAndLink())
  {
    CLog::Log(LOGERROR, "CLinuxRendererGL::UpdateVideoFilter: Error compiling and linking defauilt video filter shader");
  }

  SetTextureFilter(GL_LINEAR);
  m_renderQuality = RQ_SINGLEPASS;
}

void CLinuxRendererGL::LoadShaders(int field)
{
  m_reloadShaders = 0;

  if (!LoadShadersHook())
  {
    int requestedMethod = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_RENDERMETHOD);
    CLog::Log(LOGDEBUG, "GL: Requested render method: {}", requestedMethod);

    if (m_pYUVShader)
    {
      delete m_pYUVShader;
      m_pYUVShader = NULL;
    }

    // create regular progressive scan shader
    // if single pass, create GLSLOutput helper and pass it to YUV2RGB shader
    EShaderFormat shaderFormat = GetShaderFormat();
    std::shared_ptr<GLSLOutput> out;
    m_toneMapMethod = m_videoSettings.m_ToneMapMethod;
    if (m_renderQuality == RQ_SINGLEPASS)
    {
      out = std::make_shared<GLSLOutput>(GLSLOutput(4, m_useDithering, m_ditherDepth,
                                                    m_cmsOn ? m_fullRange : false,
                                                    m_cmsOn ? m_tCLUTTex : 0,
                                                    m_CLUTsize));

      if (m_scalingMethod == VS_SCALINGMETHOD_LANCZOS3_FAST || m_scalingMethod == VS_SCALINGMETHOD_SPLINE36_FAST)
      {
        m_pYUVShader = new YUV2RGBFilterShader4(m_textureTarget == GL_TEXTURE_RECTANGLE,
                                                shaderFormat, m_nonLinStretch,
                                                AVColorPrimaries::AVCOL_PRI_BT709, m_srcPrimaries,
                                                m_toneMap,
                                                m_toneMapMethod,
                                                m_scalingMethod, out);
        if (!m_cmsOn)
          m_pYUVShader->SetConvertFullColorRange(m_fullRange);

        CLog::Log(LOGINFO, "GL: Selecting YUV 2 RGB shader with filter");

        if (m_pYUVShader && m_pYUVShader->CompileAndLink())
        {
          m_renderMethod = RENDER_GLSL;
          UpdateVideoFilter();
        }
        else
        {
          CLog::Log(LOGERROR, "GL: Error enabling YUV2RGB GLSL shader");
          delete m_pYUVShader;
          m_pYUVShader = nullptr;
        }
      }
    }

    if (!m_pYUVShader)
    {
      m_pYUVShader = new YUV2RGBProgressiveShader(m_textureTarget == GL_TEXTURE_RECTANGLE, shaderFormat,
                                                  m_nonLinStretch && m_renderQuality == RQ_SINGLEPASS,
                                                  AVColorPrimaries::AVCOL_PRI_BT709, m_srcPrimaries, m_toneMap, m_toneMapMethod, out);

      if (!m_cmsOn)
        m_pYUVShader->SetConvertFullColorRange(m_fullRange);

      CLog::Log(LOGINFO, "GL: Selecting YUV 2 RGB shader");

      if (m_pYUVShader && m_pYUVShader->CompileAndLink())
      {
        m_renderMethod = RENDER_GLSL;
        UpdateVideoFilter();
      }
      else
      {
        CLog::Log(LOGERROR, "GL: Error enabling YUV2RGB GLSL shader");
      }
    }
  }

  if (m_pboSupported)
  {
    CLog::Log(LOGINFO, "GL: Using GL_ARB_pixel_buffer_object");
    m_pboUsed = true;
  }
  else
    m_pboUsed = false;
}

void CLinuxRendererGL::UnInit()
{
  CLog::Log(LOGDEBUG, "LinuxRendererGL: Cleaning up GL resources");
  std::unique_lock<CCriticalSection> lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  glFinish();

  // YV12 textures
  for (int i = 0; i < NUM_BUFFERS; ++i)
  {
    ReleaseBuffer(i);
    DeleteTexture(i);
  }

  DeleteCLUT();

  // cleanup framebuffer object if it was in use
  m_fbo.fbo.Cleanup();
  m_bValidated = false;
  m_bConfigured = false;
}

bool CLinuxRendererGL::Render(unsigned int flags, int renderBuffer)
{
  // obtain current field, if interlaced
  if( flags & RENDER_FLAG_TOP)
    m_currentField = FIELD_TOP;

  else if (flags & RENDER_FLAG_BOT)
    m_currentField = FIELD_BOT;

  else
    m_currentField = FIELD_FULL;

  // call texture load function
  if (!UploadTexture(renderBuffer))
  {
    return false;
  }

  if (RenderHook(renderBuffer))
    ;
  else if (m_renderMethod & RENDER_GLSL)
  {
    UpdateVideoFilter();
    switch(m_renderQuality)
    {
    case RQ_LOW:
    case RQ_SINGLEPASS:
      RenderSinglePass(renderBuffer, m_currentField);
      VerifyGLState();
      break;

    case RQ_MULTIPASS:
      RenderToFBO(renderBuffer, m_currentField);
      RenderFromFBO();
      VerifyGLState();
      break;
    }
  }
  else
  {
    return false;
  }

  AfterRenderHook(renderBuffer);
  return true;
}

void CLinuxRendererGL::RenderSinglePass(int index, int field)
{
  CPictureBuffer &buf = m_buffers[index];
  CYuvPlane (&planes)[YuvImage::MAX_PLANES] = m_buffers[index].fields[field];

  CheckVideoParameters(index);

  if (m_reloadShaders)
  {
    m_reloadShaders = 0;
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

  m_pYUVShader->SetBlack(m_videoSettings.m_Brightness * 0.01f - 0.5f);
  m_pYUVShader->SetContrast(m_videoSettings.m_Contrast * 0.02f);
  m_pYUVShader->SetWidth(planes[0].texwidth);
  m_pYUVShader->SetHeight(planes[0].texheight);
  m_pYUVShader->SetColParams(buf.m_srcColSpace, buf.m_srcBits, !buf.m_srcFullRange, buf.m_srcTextureBits);
  m_pYUVShader->SetDisplayMetadata(buf.hasDisplayMetadata, buf.displayMetadata,
                                   buf.hasLightMetadata, buf.lightMetadata);
  m_pYUVShader->SetToneMapParam(m_toneMapMethod, m_videoSettings.m_ToneMapParam);

  //disable non-linear stretch when a dvd menu is shown, parts of the menu are rendered through the overlay renderer
  //having non-linear stretch on breaks the alignment
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer->IsInMenu())
    m_pYUVShader->SetNonLinStretch(1.0);
  else
    m_pYUVShader->SetNonLinStretch(pow(CDisplaySettings::GetInstance().GetPixelRatio(), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoNonLinStretchRatio));

  if (field == FIELD_TOP)
    m_pYUVShader->SetField(1);
  else if(field == FIELD_BOT)
    m_pYUVShader->SetField(0);

  m_pYUVShader->SetMatrices(glMatrixProject.Get(), glMatrixModview.Get());
  m_pYUVShader->Enable();

  GLubyte idx[4] = {0, 1, 3, 2};  //determines order of the vertices
  GLuint vertexVBO;
  GLuint indexVBO;

  struct PackedVertex
  {
    float x, y, z;
    float u1, v1;
    float u2, v2;
    float u3, v3;
  }vertex[4];

  GLint vertLoc = m_pYUVShader->GetVertexLoc();
  GLint Yloc = m_pYUVShader->GetYcoordLoc();
  GLint Uloc = m_pYUVShader->GetUcoordLoc();
  GLint Vloc = m_pYUVShader->GetVcoordLoc();

  // Setup vertex position values
  for(int i = 0; i < 4; i++)
  {
    vertex[i].x = m_rotatedDestCoords[i].x;
    vertex[i].y = m_rotatedDestCoords[i].y;
    vertex[i].z = 0.0f;// set z to 0
  }

  // bottom left
  vertex[0].u1 = planes[0].rect.x1;
  vertex[0].v1 = planes[0].rect.y1;
  vertex[0].u2 = planes[1].rect.x1;
  vertex[0].v2 = planes[1].rect.y1;
  vertex[0].u3 = planes[2].rect.x1;
  vertex[0].v3 = planes[2].rect.y1;

  // bottom right
  vertex[1].u1 = planes[0].rect.x2;
  vertex[1].v1 = planes[0].rect.y1;
  vertex[1].u2 = planes[1].rect.x2;
  vertex[1].v2 = planes[1].rect.y1;
  vertex[1].u3 = planes[2].rect.x2;
  vertex[1].v3 = planes[2].rect.y1;

  // top right
  vertex[2].u1 = planes[0].rect.x2;
  vertex[2].v1 = planes[0].rect.y2;
  vertex[2].u2 = planes[1].rect.x2;
  vertex[2].v2 = planes[1].rect.y2;
  vertex[2].u3 = planes[2].rect.x2;
  vertex[2].v3 = planes[2].rect.y2;

  // top left
  vertex[3].u1 = planes[0].rect.x1;
  vertex[3].v1 = planes[0].rect.y2;
  vertex[3].u2 = planes[1].rect.x1;
  vertex[3].v2 = planes[1].rect.y2;
  vertex[3].u3 = planes[2].rect.x1;
  vertex[3].v3 = planes[2].rect.y2;

  glGenBuffers(1, &vertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(PackedVertex)*4, &vertex[0], GL_STATIC_DRAW);

  glVertexAttribPointer(vertLoc, 3, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, x)));
  glVertexAttribPointer(Yloc, 2, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, u1)));
  glVertexAttribPointer(Uloc, 2, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, u2)));
  if (Vloc != -1)
    glVertexAttribPointer(Vloc, 2, GL_FLOAT, 0, sizeof(PackedVertex),
                          reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, u3)));

  glEnableVertexAttribArray(vertLoc);
  glEnableVertexAttribArray(Yloc);
  glEnableVertexAttribArray(Uloc);
  if (Vloc != -1)
    glEnableVertexAttribArray(Vloc);

  glGenBuffers(1, &indexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte)*4, idx, GL_STATIC_DRAW);

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, 0);
  VerifyGLState();

  glDisableVertexAttribArray(vertLoc);
  glDisableVertexAttribArray(Yloc);
  glDisableVertexAttribArray(Uloc);
  if (Vloc != -1)
    glDisableVertexAttribArray(Vloc);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &vertexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &indexVBO);

  m_pYUVShader->Disable();
  VerifyGLState();

  VerifyGLState();
}

void CLinuxRendererGL::RenderToFBO(int index, int field, bool weave /*= false*/)
{
  CPictureBuffer &buf = m_buffers[index];
  CYuvPlane (&planes)[YuvImage::MAX_PLANES] = m_buffers[index].fields[field];

  CheckVideoParameters(index);

  if (m_reloadShaders)
  {
    m_reloadShaders = 0;
    LoadShaders(m_currentField);
  }

  if (!m_fbo.fbo.IsValid())
  {
    if (!m_fbo.fbo.Initialize())
    {
      CLog::Log(LOGERROR, "GL: Error initializing FBO");
      return;
    }

    if (!m_fbo.fbo.CreateAndBindToTexture(GL_TEXTURE_2D, m_sourceWidth, m_sourceHeight, GL_RGBA, GL_SHORT))
    {
      CLog::Log(LOGERROR, "GL: Error creating texture and binding to FBO");
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

  // make sure the yuv shader is loaded and ready to go
  if (!m_pYUVShader || (!m_pYUVShader->OK()))
  {
    CLog::Log(LOGERROR, "GL: YUV shader not active, cannot do multipass render");
    return;
  }

  m_fbo.fbo.BeginRender();
  VerifyGLState();

  m_pYUVShader->SetBlack(m_videoSettings.m_Brightness * 0.01f - 0.5f);
  m_pYUVShader->SetContrast(m_videoSettings.m_Contrast * 0.02f);
  m_pYUVShader->SetWidth(planes[0].texwidth);
  m_pYUVShader->SetHeight(planes[0].texheight);
  m_pYUVShader->SetNonLinStretch(1.0);
  m_pYUVShader->SetColParams(buf.m_srcColSpace, buf.m_srcBits, !buf.m_srcFullRange, buf.m_srcTextureBits);
  m_pYUVShader->SetDisplayMetadata(buf.hasDisplayMetadata, buf.displayMetadata,
                                   buf.hasLightMetadata, buf.lightMetadata);
  m_pYUVShader->SetToneMapParam(m_toneMapMethod, m_videoSettings.m_ToneMapParam);

  if (field == FIELD_TOP)
    m_pYUVShader->SetField(1);
  else if (field == FIELD_BOT)
    m_pYUVShader->SetField(0);

  VerifyGLState();

  glMatrixModview.Push();
  glMatrixModview->LoadIdentity();
  glMatrixModview.Load();

  glMatrixProject.Push();
  glMatrixProject->LoadIdentity();
  glMatrixProject->Ortho2D(0, m_sourceWidth, 0, m_sourceHeight);
  glMatrixProject.Load();

  CRect viewport;
  m_renderSystem->GetViewPort(viewport);
  glViewport(0, 0, m_sourceWidth, m_sourceHeight);
  glScissor (0, 0, m_sourceWidth, m_sourceHeight);

  m_pYUVShader->SetMatrices(glMatrixProject.Get(), glMatrixModview.Get());
  if (!m_pYUVShader->Enable())
  {
    CLog::Log(LOGERROR, "GL: Error enabling YUV shader");
  }

  m_fbo.width = planes[0].rect.x2 - planes[0].rect.x1;
  m_fbo.height = planes[0].rect.y2 - planes[0].rect.y1;

  if (m_textureTarget == GL_TEXTURE_2D)
  {
    m_fbo.width  *= planes[0].texwidth;
    m_fbo.height *= planes[0].texheight;
  }
  m_fbo.width  *= planes[0].pixpertex_x;
  m_fbo.height *= planes[0].pixpertex_y;
  if (weave)
    m_fbo.height *= 2;

  // 1st Pass to video frame size
  GLubyte idx[4] = {0, 1, 3, 2};  //determines order of the vertices
  GLuint vertexVBO;
  GLuint indexVBO;
  struct PackedVertex
  {
    float x, y, z;
    float u1, v1;
    float u2, v2;
    float u3, v3;
  } vertex[4];

  GLint vertLoc = m_pYUVShader->GetVertexLoc();
  GLint Yloc = m_pYUVShader->GetYcoordLoc();
  GLint Uloc = m_pYUVShader->GetUcoordLoc();
  GLint Vloc = m_pYUVShader->GetVcoordLoc();

  // top left
  vertex[0].x = 0.0f;
  vertex[0].y = 0.0f;
  vertex[0].z = 0.0f;
  vertex[0].u1 = planes[0].rect.x1;
  vertex[0].v1 = planes[0].rect.y1;
  vertex[0].u2 = planes[1].rect.x1;
  vertex[0].v2 = planes[1].rect.y1;
  vertex[0].u3 = planes[2].rect.x1;
  vertex[0].v3 = planes[2].rect.y1;

  // top right
  vertex[1].x = m_fbo.width;
  vertex[1].y = 0.0f;
  vertex[1].z = 0.0f;
  vertex[1].u1 = planes[0].rect.x2;
  vertex[1].v1 = planes[0].rect.y1;
  vertex[1].u2 = planes[1].rect.x2;
  vertex[1].v2 = planes[1].rect.y1;
  vertex[1].u3 = planes[2].rect.x2;
  vertex[1].v3 = planes[2].rect.y1;

  // bottom right
  vertex[2].x = m_fbo.width;
  vertex[2].y = m_fbo.height;
  vertex[2].z = 0.0f;
  vertex[2].u1 = planes[0].rect.x2;
  vertex[2].v1 = planes[0].rect.y2;
  vertex[2].u2 = planes[1].rect.x2;
  vertex[2].v2 = planes[1].rect.y2;
  vertex[2].u3 = planes[2].rect.x2;
  vertex[2].v3 = planes[2].rect.y2;

  // bottom left
  vertex[3].x = 0.0f;
  vertex[3].y = m_fbo.height;
  vertex[3].z = 0.0f;
  vertex[3].u1 = planes[0].rect.x1;
  vertex[3].v1 = planes[0].rect.y2;
  vertex[3].u2 = planes[1].rect.x1;
  vertex[3].v2 = planes[1].rect.y2;
  vertex[3].u3 = planes[2].rect.x1;
  vertex[3].v3 = planes[2].rect.y2;

  glGenBuffers(1, &vertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(PackedVertex)*4, &vertex[0], GL_STATIC_DRAW);

  glVertexAttribPointer(vertLoc, 3, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, x)));
  glVertexAttribPointer(Yloc, 2, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, u1)));
  glVertexAttribPointer(Uloc, 2, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, u2)));
  if (Vloc != -1)
    glVertexAttribPointer(Vloc, 2, GL_FLOAT, 0, sizeof(PackedVertex),
                          reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, u3)));

  glEnableVertexAttribArray(vertLoc);
  glEnableVertexAttribArray(Yloc);
  glEnableVertexAttribArray(Uloc);
  if (Vloc != -1)
    glEnableVertexAttribArray(Vloc);

  glGenBuffers(1, &indexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte)*4, idx, GL_STATIC_DRAW);

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, 0);
  VerifyGLState();

  glDisableVertexAttribArray(vertLoc);
  glDisableVertexAttribArray(Yloc);
  glDisableVertexAttribArray(Uloc);
  if (Vloc != -1)
    glDisableVertexAttribArray(Vloc);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &vertexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &indexVBO);

  m_pYUVShader->Disable();

  glMatrixModview.PopLoad();
  glMatrixProject.PopLoad();

  m_renderSystem->SetViewPort(viewport);

  m_fbo.fbo.EndRender();

  VerifyGLState();
}

void CLinuxRendererGL::RenderFromFBO()
{
  glActiveTexture(GL_TEXTURE0);
  VerifyGLState();

  // Use regular normalized texture coordinates
  // 2nd Pass to screen size with optional video filter

  if (!m_pVideoFilterShader)
  {
    CLog::Log(LOGERROR, "CLinuxRendererGL::RenderFromFBO - no videofilter shader");
    return;
  }

  GLint filter;
  if (!m_pVideoFilterShader->GetTextureFilter(filter))
    filter = m_scalingMethod == VS_SCALINGMETHOD_NEAREST ? GL_NEAREST : GL_LINEAR;

  m_fbo.fbo.SetFiltering(GL_TEXTURE_2D, filter);
  m_pVideoFilterShader->SetSourceTexture(0);
  m_pVideoFilterShader->SetWidth(m_sourceWidth);
  m_pVideoFilterShader->SetHeight(m_sourceHeight);

  //disable non-linear stretch when a dvd menu is shown, parts of the menu are rendered through the overlay renderer
  //having non-linear stretch on breaks the alignment
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer->IsInMenu())
    m_pVideoFilterShader->SetNonLinStretch(1.0);
  else
    m_pVideoFilterShader->SetNonLinStretch(pow(CDisplaySettings::GetInstance().GetPixelRatio(), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoNonLinStretchRatio));

  m_pVideoFilterShader->SetMatrices(glMatrixProject.Get(), glMatrixModview.Get());
  m_pVideoFilterShader->Enable();

  VerifyGLState();

  float imgwidth = m_fbo.width / m_sourceWidth;
  float imgheight = m_fbo.height / m_sourceHeight;

  GLubyte idx[4] = {0, 1, 3, 2};  //determines order of the vertices
  GLuint vertexVBO;
  GLuint indexVBO;
  struct PackedVertex
  {
    float x, y, z;
    float u1, v1;
  } vertex[4];

  GLint vertLoc = m_pVideoFilterShader->GetVertexLoc();
  GLint loc = m_pVideoFilterShader->GetCoordLoc();

  // Setup vertex position values
  // top left
  vertex[0].x = m_rotatedDestCoords[0].x;
  vertex[0].y = m_rotatedDestCoords[0].y;
  vertex[0].z = 0.0f;
  vertex[0].u1 = 0.0;
  vertex[0].v1 = 0.0;

  // top right
  vertex[1].x = m_rotatedDestCoords[1].x;
  vertex[1].y = m_rotatedDestCoords[1].y;
  vertex[1].z = 0.0f;
  vertex[1].u1 = imgwidth;
  vertex[1].v1 = 0.0f;

  // bottom right
  vertex[2].x = m_rotatedDestCoords[2].x;
  vertex[2].y = m_rotatedDestCoords[2].y;
  vertex[2].z = 0.0f;
  vertex[2].u1 = imgwidth;
  vertex[2].v1 = imgheight;

  // bottom left
  vertex[3].x = m_rotatedDestCoords[3].x;
  vertex[3].y = m_rotatedDestCoords[3].y;
  vertex[3].z = 0.0f;
  vertex[3].u1 = 0.0f;
  vertex[3].v1 = imgheight;

  glGenBuffers(1, &vertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(PackedVertex)*4, &vertex[0], GL_STATIC_DRAW);

  glVertexAttribPointer(vertLoc, 3, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, x)));
  glVertexAttribPointer(loc, 2, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, u1)));

  glEnableVertexAttribArray(vertLoc);
  glEnableVertexAttribArray(loc);

  glGenBuffers(1, &indexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte)*4, idx, GL_STATIC_DRAW);

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, 0);
  VerifyGLState();

  glDisableVertexAttribArray(loc);
  glDisableVertexAttribArray(vertLoc);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &vertexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &indexVBO);

  m_pVideoFilterShader->Disable();

  VerifyGLState();

  glBindTexture(GL_TEXTURE_2D, 0);
  VerifyGLState();
}

void CLinuxRendererGL::RenderProgressiveWeave(int index, int field)
{
  bool scale = (int)m_sourceHeight != m_destRect.Height() ||
               (int)m_sourceWidth != m_destRect.Width();

  if (m_fbo.fbo.IsSupported() && (scale || m_renderQuality == RQ_MULTIPASS))
  {
    glEnable(GL_POLYGON_STIPPLE);
    glPolygonStipple(stipple_weave);
    RenderToFBO(index, FIELD_TOP, true);
    glPolygonStipple(stipple_weave+4);
    RenderToFBO(index, FIELD_BOT, true);
    glDisable(GL_POLYGON_STIPPLE);
    RenderFromFBO();
  }
  else
  {
    glEnable(GL_POLYGON_STIPPLE);
    glPolygonStipple(stipple_weave);
    RenderSinglePass(index, FIELD_TOP);
    glPolygonStipple(stipple_weave+4);
    RenderSinglePass(index, FIELD_BOT);
    glDisable(GL_POLYGON_STIPPLE);
  }
}

void CLinuxRendererGL::RenderRGB(int index, int field)
{
  CYuvPlane &plane = m_buffers[index].fields[FIELD_FULL][0];

  glActiveTexture(GL_TEXTURE0);

  glBindTexture(m_textureTarget, plane.id);

  // make sure we know the correct texture size
  GetPlaneTextureSize(plane);

  if (!m_pVideoFilterShader)
  {
    CLog::Log(LOGERROR, "CLinuxRendererGL::RenderRGB - no videofilter shader");
    return;
  }

  GLint filter;
  if (!m_pVideoFilterShader->GetTextureFilter(filter))
    filter = m_scalingMethod == VS_SCALINGMETHOD_NEAREST ? GL_NEAREST : GL_LINEAR;

  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, filter);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, filter);
  m_pVideoFilterShader->SetSourceTexture(0);
  m_pVideoFilterShader->SetWidth(m_sourceWidth);
  m_pVideoFilterShader->SetHeight(m_sourceHeight);

  //disable non-linear stretch when a dvd menu is shown, parts of the menu are rendered through the overlay renderer
  //having non-linear stretch on breaks the alignment
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer->IsInMenu())
    m_pVideoFilterShader->SetNonLinStretch(1.0);
  else
    m_pVideoFilterShader->SetNonLinStretch(pow(CDisplaySettings::GetInstance().GetPixelRatio(), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoNonLinStretchRatio));

  m_pVideoFilterShader->SetMatrices(glMatrixProject.Get(), glMatrixModview.Get());
  m_pVideoFilterShader->Enable();

  GLubyte idx[4] = {0, 1, 3, 2};  //determines order of the vertices
  GLuint vertexVBO;
  GLuint indexVBO;
  struct PackedVertex
  {
    float x, y, z;
    float u1, v1;
  } vertex[4];

  GLint vertLoc = m_pVideoFilterShader->GetVertexLoc();
  GLint loc = m_pVideoFilterShader->GetCoordLoc();

  // Setup vertex position values
  // top left
  vertex[0].x = m_rotatedDestCoords[0].x;
  vertex[0].y = m_rotatedDestCoords[0].y;
  vertex[0].z = 0.0f;
  vertex[0].u1 = plane.rect.x1;
  vertex[0].v1 = plane.rect.y1;

  // top right
  vertex[1].x = m_rotatedDestCoords[1].x;
  vertex[1].y = m_rotatedDestCoords[1].y;
  vertex[1].z = 0.0f;
  vertex[1].u1 = plane.rect.x2;
  vertex[1].v1 = plane.rect.y1;

  // bottom right
  vertex[2].x = m_rotatedDestCoords[2].x;
  vertex[2].y = m_rotatedDestCoords[2].y;
  vertex[2].z = 0.0f;
  vertex[2].u1 = plane.rect.x2;
  vertex[2].v1 = plane.rect.y2;

  // bottom left
  vertex[3].x = m_rotatedDestCoords[3].x;
  vertex[3].y = m_rotatedDestCoords[3].y;
  vertex[3].z = 0.0f;
  vertex[3].u1 = plane.rect.x1;
  vertex[3].v1 = plane.rect.y2;

  glGenBuffers(1, &vertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(PackedVertex)*4, &vertex[0], GL_STATIC_DRAW);

  glVertexAttribPointer(vertLoc, 3, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, x)));
  glVertexAttribPointer(loc, 2, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, u1)));

  glEnableVertexAttribArray(vertLoc);
  glEnableVertexAttribArray(loc);

  glGenBuffers(1, &indexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte)*4, idx, GL_STATIC_DRAW);

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, 0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &vertexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &indexVBO);

  VerifyGLState();

  m_pVideoFilterShader->Disable();

  glBindTexture(m_textureTarget, 0);
}

bool CLinuxRendererGL::RenderCapture(int index, CRenderCapture* capture)
{
  if (!m_bValidated)
    return false;

  // save current video rect
  CRect saveSize = m_destRect;

  saveRotatedCoords();//backup current m_rotatedDestCoords

  // new video rect is capture size
  m_destRect.SetRect(0, 0, (float)capture->GetWidth(), (float)capture->GetHeight());
  MarkDirty();
  syncDestRectToRotatedPoints();//syncs the changed destRect to m_rotatedDestCoords

  //invert Y axis to get non-inverted image
  glDisable(GL_BLEND);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

  glMatrixModview.Push();
  glMatrixModview->Translatef(0.0f, capture->GetHeight(), 0.0f);
  glMatrixModview->Scalef(1.0f, -1.0f, 1.0f);
  glMatrixModview.Load();

  capture->BeginRender();

  Render(RENDER_FLAG_NOOSD, index);
  // read pixels
  glReadPixels(0, CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight() - capture->GetHeight(), capture->GetWidth(), capture->GetHeight(),
               GL_BGRA, GL_UNSIGNED_BYTE, capture->GetRenderBuffer());

  capture->EndRender();

  // revert model view matrix
  glMatrixModview.PopLoad();

  // restore original video rect
  m_destRect = saveSize;
  restoreRotatedCoords();//restores the previous state of the rotated dest coords

  return true;
}


GLint CLinuxRendererGL::GetInternalFormat(GLint format, int bpp)
{
  unsigned int major, minor;
  m_renderSystem->GetRenderVersion(major, minor);
  if (bpp == 2)
  {
    if (format == GL_RED)
    {
      if (major > 2)
        return GL_R16;
      else
        return GL_LUMINANCE16;
    }
  }
  else
  {
    if (format == GL_RED)
    {
      if (major > 2)
        return GL_RED;
      else
        return GL_LUMINANCE;
    }
  }

  return format;
}

//-----------------------------------------------------------------------------
// Textures
//-----------------------------------------------------------------------------

bool CLinuxRendererGL::CreateTexture(int index)
{
  if (m_format == AV_PIX_FMT_NV12)
    return CreateNV12Texture(index);
  else if (m_format == AV_PIX_FMT_YUYV422 ||
           m_format == AV_PIX_FMT_UYVY422)
    return CreateYUV422PackedTexture(index);
  else
    return CreateYV12Texture(index);
}

void CLinuxRendererGL::DeleteTexture(int index)
{
  CPictureBuffer& buf = m_buffers[index];
  buf.loaded = false;

  if (m_format == AV_PIX_FMT_NV12)
    DeleteNV12Texture(index);
  else if (m_format == AV_PIX_FMT_YUYV422 ||
           m_format == AV_PIX_FMT_UYVY422)
    DeleteYUV422PackedTexture(index);
  else
    DeleteYV12Texture(index);
}

bool CLinuxRendererGL::UploadTexture(int index)
{
  if (!m_buffers[index].videoBuffer)
    return false;

  bool ret = true;

  if (!m_buffers[index].loaded)
  {
    YuvImage &dst = m_buffers[index].image;
    YuvImage src;
    m_buffers[index].videoBuffer->GetPlanes(src.plane);
    m_buffers[index].videoBuffer->GetStrides(src.stride);

    UnBindPbo(m_buffers[index]);

    if (m_format == AV_PIX_FMT_NV12)
    {
      CVideoBuffer::CopyNV12Picture(&dst, &src);
      BindPbo(m_buffers[index]);
      ret = UploadNV12Texture(index);
    }
    else if (m_format == AV_PIX_FMT_YUYV422 ||
             m_format == AV_PIX_FMT_UYVY422)
    {
      CVideoBuffer::CopyYUV422PackedPicture(&dst, &src);
      BindPbo(m_buffers[index]);
      ret = UploadYUV422PackedTexture(index);
    }
    else
    {
      CVideoBuffer::CopyPicture(&dst, &src);
      BindPbo(m_buffers[index]);
      ret = UploadYV12Texture(index);
    }

    if (ret)
      m_buffers[index].loaded = true;
  }

  if (ret)
    CalculateTextureSourceRects(index, 3);

  return ret;
}

//********************************************************************************************************
// YV12 Texture creation, deletion, copying + clearing
//********************************************************************************************************

bool CLinuxRendererGL::CreateYV12Texture(int index)
{
  /* since we also want the field textures, pitch must be texture aligned */
  unsigned p;

  CPictureBuffer &buf = m_buffers[index];
  YuvImage &im = m_buffers[index].image;
  GLuint *pbo = m_buffers[index].pbo;

  DeleteYV12Texture(index);

  im.height = m_sourceHeight;
  im.width = m_sourceWidth;
  im.cshift_x = 1;
  im.cshift_y = 1;

  switch (m_format)
  {
    case AV_PIX_FMT_YUV420P16:
      buf.m_srcTextureBits = 16;
      break;
    case AV_PIX_FMT_YUV420P14:
      buf.m_srcTextureBits = 14;
      break;
    case AV_PIX_FMT_YUV420P12:
      buf.m_srcTextureBits = 12;
      break;
    case AV_PIX_FMT_YUV420P10:
      buf.m_srcTextureBits = 10;
      break;
    case AV_PIX_FMT_YUV420P9:
      buf.m_srcTextureBits = 9;
      break;
    default:
      break;
  }
  if (buf.m_srcTextureBits > 8)
    im.bpp = 2;
  else
    im.bpp = 1;

  im.stride[0] = im.bpp * im.width;
  im.stride[1] = im.bpp * (im.width >> im.cshift_x);
  im.stride[2] = im.bpp * (im.width >> im.cshift_x);

  im.planesize[0] = im.stride[0] * im.height;
  im.planesize[1] = im.stride[1] * (im.height >> im.cshift_y);
  im.planesize[2] = im.stride[2] * (im.height >> im.cshift_y);

  bool pboSetup = false;
  if (m_pboUsed)
  {
    pboSetup = true;
    glGenBuffers(3, pbo);

    for (int i = 0; i < 3; i++)
    {
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[i]);
      glBufferData(GL_PIXEL_UNPACK_BUFFER, im.planesize[i] + PBO_OFFSET, 0, GL_STREAM_DRAW);
      void* pboPtr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
      if (pboPtr)
      {
        im.plane[i] = (uint8_t*) pboPtr + PBO_OFFSET;
        memset(im.plane[i], 0, im.planesize[i]);
      }
      else
      {
        CLog::Log(LOGWARNING,"GL: failed to set up pixel buffer object");
        pboSetup = false;
        break;
      }
    }

    if (!pboSetup)
    {
      for (int i = 0; i < 3; i++)
      {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[i]);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
      }
      glDeleteBuffers(3, pbo);
      memset(m_buffers[index].pbo, 0, sizeof(m_buffers[index].pbo));
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  }

  if (!pboSetup)
  {
    for (int i = 0; i < 3; i++)
      im.plane[i] = new uint8_t[im.planesize[i]];
  }

  for(int f = 0;f<MAX_FIELDS;f++)
  {
    for(p = 0;p<YuvImage::MAX_PLANES;p++)
    {
      if (!glIsTexture(m_buffers[index].fields[f][p].id))
      {
        glGenTextures(1, &m_buffers[index].fields[f][p].id);
        VerifyGLState();
      }
      m_buffers[index].fields[f][p].pbo = pbo[p];
    }
  }

  // YUV
  for (int f = FIELD_FULL; f<=FIELD_BOT ; f++)
  {
    int fieldshift = (f==FIELD_FULL) ? 0 : 1;
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

    for (int p = 0; p < 3; p++)
    {
      CYuvPlane &plane = planes[p];
      if (plane.texwidth * plane.texheight == 0)
        continue;

      glBindTexture(m_textureTarget, plane.id);
      GLint internalformat;
      internalformat = GetInternalFormat(GL_RED, im.bpp);
      if (im.bpp == 2)
        glTexImage2D(m_textureTarget, 0, internalformat, plane.texwidth, plane.texheight, 0, GL_RED, GL_UNSIGNED_SHORT, NULL);
      else
        glTexImage2D(m_textureTarget, 0, internalformat, plane.texwidth, plane.texheight, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);

      glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      VerifyGLState();
    }
  }
  return true;
}

bool CLinuxRendererGL::UploadYV12Texture(int source)
{
  CPictureBuffer& buf = m_buffers[source];
  YuvImage* im = &buf.image;

  bool deinterlacing;
  if (m_currentField == FIELD_FULL)
    deinterlacing = false;
  else
    deinterlacing = true;

  VerifyGLState();

  glPixelStorei(GL_UNPACK_ALIGNMENT,1);

  if (deinterlacing)
  {
    // Load Even Y Field
    LoadPlane(buf.fields[FIELD_TOP][0] , GL_RED,
              im->width, im->height >> 1,
              im->stride[0]*2, im->bpp, im->plane[0] );

    //load Odd Y Field
    LoadPlane(buf.fields[FIELD_BOT][0], GL_RED,
              im->width, im->height >> 1,
              im->stride[0]*2, im->bpp, im->plane[0] + im->stride[0]);

    // Load Even U & V Fields
    LoadPlane(buf.fields[FIELD_TOP][1], GL_RED,
              im->width >> im->cshift_x, im->height >> (im->cshift_y + 1),
              im->stride[1]*2, im->bpp, im->plane[1]);

    LoadPlane(buf.fields[FIELD_TOP][2], GL_RED,
              im->width >> im->cshift_x, im->height >> (im->cshift_y + 1),
              im->stride[2]*2, im->bpp, im->plane[2]);

    // Load Odd U & V Fields
    LoadPlane(buf.fields[FIELD_BOT][1], GL_RED,
              im->width >> im->cshift_x, im->height >> (im->cshift_y + 1),
              im->stride[1]*2, im->bpp, im->plane[1] + im->stride[1]);

    LoadPlane(buf.fields[FIELD_BOT][2], GL_RED,
              im->width >> im->cshift_x, im->height >> (im->cshift_y + 1),
              im->stride[2]*2, im->bpp, im->plane[2] + im->stride[2]);
  }
  else
  {
    //Load Y plane
    LoadPlane(buf.fields[FIELD_FULL][0], GL_RED,
              im->width, im->height,
              im->stride[0], im->bpp, im->plane[0]);

    //load U plane
    LoadPlane(buf.fields[FIELD_FULL][1], GL_RED,
              im->width >> im->cshift_x, im->height >> im->cshift_y,
              im->stride[1], im->bpp, im->plane[1]);

    //load V plane
    LoadPlane(buf.fields[FIELD_FULL][2], GL_RED,
              im->width >> im->cshift_x, im->height >> im->cshift_y,
              im->stride[2], im->bpp, im->plane[2]);
  }

  VerifyGLState();

  return true;
}

void CLinuxRendererGL::DeleteYV12Texture(int index)
{
  YuvImage &im = m_buffers[index].image;
  GLuint *pbo = m_buffers[index].pbo;

  if (m_buffers[index].fields[FIELD_FULL][0].id == 0)
    return;

  /* finish up all textures, and delete them */
  for(int f = 0;f<MAX_FIELDS;f++)
  {
    for(int p = 0;p<YuvImage::MAX_PLANES;p++)
    {
      if (m_buffers[index].fields[f][p].id)
      {
        if (glIsTexture(m_buffers[index].fields[f][p].id))
          glDeleteTextures(1, &m_buffers[index].fields[f][p].id);
        m_buffers[index].fields[f][p].id = 0;
      }
    }
  }

  for(int p = 0;p<YuvImage::MAX_PLANES;p++)
  {
    if (pbo[p])
    {
      if (im.plane[p])
      {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[p]);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        im.plane[p] = NULL;
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
      }
      glDeleteBuffers(1, pbo + p);
      pbo[p] = 0;
    }
    else
    {
      if (im.plane[p])
      {
        delete[] im.plane[p];
        im.plane[p] = NULL;
      }
    }
  }
}

//********************************************************************************************************
// NV12 Texture loading, creation and deletion
//********************************************************************************************************
bool CLinuxRendererGL::UploadNV12Texture(int source)
{
  CPictureBuffer& buf = m_buffers[source];
  YuvImage* im = &buf.image;

  bool deinterlacing;
  if (m_currentField == FIELD_FULL)
    deinterlacing = false;
  else
    deinterlacing = true;

  VerifyGLState();

  glPixelStorei(GL_UNPACK_ALIGNMENT, im->bpp);

  if (deinterlacing)
  {
    // Load Odd Y field
    LoadPlane(buf.fields[FIELD_TOP][0] , GL_RED,
              im->width, im->height >> 1,
              im->stride[0]*2, im->bpp, im->plane[0]);

    // Load Even Y field
    LoadPlane(buf.fields[FIELD_BOT][0], GL_RED,
              im->width, im->height >> 1,
              im->stride[0]*2, im->bpp, im->plane[0] + im->stride[0]) ;

    // Load Odd UV Fields
    LoadPlane(buf.fields[FIELD_TOP][1], GL_RG,
              im->width >> im->cshift_x, im->height >> (im->cshift_y + 1),
              im->stride[1]*2, im->bpp, im->plane[1]);

    // Load Even UV Fields
    LoadPlane(buf.fields[FIELD_BOT][1], GL_RG,
              im->width >> im->cshift_x, im->height >> (im->cshift_y + 1),
              im->stride[1]*2, im->bpp, im->plane[1] + im->stride[1]);

  }
  else
  {
    // Load Y plane
    LoadPlane(buf. fields[FIELD_FULL][0], GL_RED,
              im->width, im->height,
              im->stride[0], im->bpp, im->plane[0]);

    // Load UV plane
    LoadPlane(buf.fields[FIELD_FULL][1], GL_RG,
              im->width >> im->cshift_x, im->height >> im->cshift_y,
              im->stride[1], im->bpp, im->plane[1]);
  }

  VerifyGLState();

  return true;
}

bool CLinuxRendererGL::CreateNV12Texture(int index)
{
  // since we also want the field textures, pitch must be texture aligned
  CPictureBuffer& buf = m_buffers[index];
  YuvImage &im = buf.image;
  GLuint *pbo = buf.pbo;

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

  im.plane[0] = NULL;
  im.plane[1] = NULL;
  im.plane[2] = NULL;

  // Y plane
  im.planesize[0] = im.stride[0] * im.height;
  // packed UV plane
  im.planesize[1] = im.stride[1] * im.height / 2;
  // third plane is not used
  im.planesize[2] = 0;

  bool pboSetup = false;
  if (m_pboUsed)
  {
    pboSetup = true;
    glGenBuffers(2, pbo);

    for (int i = 0; i < 2; i++)
    {
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[i]);
      glBufferData(GL_PIXEL_UNPACK_BUFFER, im.planesize[i] + PBO_OFFSET, 0, GL_STREAM_DRAW);
      void* pboPtr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
      if (pboPtr)
      {
        im.plane[i] = (uint8_t*)pboPtr + PBO_OFFSET;
        memset(im.plane[i], 0, im.planesize[i]);
      }
      else
      {
        CLog::Log(LOGWARNING,"GL: failed to set up pixel buffer object");
        pboSetup = false;
        break;
      }
    }

    if (!pboSetup)
    {
      for (int i = 0; i < 2; i++)
      {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[i]);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
      }
      glDeleteBuffers(2, pbo);
      memset(m_buffers[index].pbo, 0, sizeof(m_buffers[index].pbo));
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  }

  if (!pboSetup)
  {
    for (int i = 0; i < 2; i++)
      im.plane[i] = new uint8_t[im.planesize[i]];
  }

  for(int f = 0;f<MAX_FIELDS;f++)
  {
    for(int p = 0;p<2;p++)
    {
      if (!glIsTexture(buf.fields[f][p].id))
      {
        glGenTextures(1, &buf.fields[f][p].id);
        VerifyGLState();
      }
      buf.fields[f][p].pbo = pbo[p];
    }
    buf.fields[f][2].id = buf.fields[f][1].id;
  }

  // YUV
  for (int f = FIELD_FULL; f<=FIELD_BOT ; f++)
  {
    int fieldshift = (f==FIELD_FULL) ? 0 : 1;
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
        continue;

      glBindTexture(m_textureTarget, plane.id);
      if (p == 1)
        glTexImage2D(m_textureTarget, 0, GL_RG, plane.texwidth, plane.texheight, 0, GL_RG, GL_UNSIGNED_BYTE, NULL);
      else
        glTexImage2D(m_textureTarget, 0, GL_RED, plane.texwidth, plane.texheight, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);

      glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      VerifyGLState();
    }
  }

  return true;
}

void CLinuxRendererGL::DeleteNV12Texture(int index)
{
  CPictureBuffer& buf = m_buffers[index];
  YuvImage &im = buf.image;
  GLuint *pbo = buf.pbo;

  if (buf.fields[FIELD_FULL][0].id == 0)
    return;

  // finish up all textures, and delete them
  for(int f = 0;f<MAX_FIELDS;f++)
  {
    for(int p = 0;p<2;p++)
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

  for(int p = 0;p<2;p++)
  {
    if (pbo[p])
    {
      if (im.plane[p])
      {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[p]);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        im.plane[p] = NULL;
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
      }
      glDeleteBuffers(1, pbo + p);
      pbo[p] = 0;
    }
    else
    {
      if (im.plane[p])
      {
        delete[] im.plane[p];
        im.plane[p] = NULL;
      }
    }
  }
}

bool CLinuxRendererGL::UploadYUV422PackedTexture(int source)
{
  CPictureBuffer& buf = m_buffers[source];
  YuvImage* im = &buf.image;

  bool deinterlacing;
  if (m_currentField == FIELD_FULL)
    deinterlacing = false;
  else
    deinterlacing = true;

  VerifyGLState();

  glPixelStorei(GL_UNPACK_ALIGNMENT,1);

  if (deinterlacing)
  {
    // Load YUYV fields
    LoadPlane(buf.fields[FIELD_TOP][0], GL_BGRA,
              im->width / 2, im->height >> 1,
              im->stride[0] * 2, im->bpp, im->plane[0]);

    LoadPlane(buf.fields[FIELD_BOT][0], GL_BGRA,
              im->width / 2, im->height >> 1,
              im->stride[0] * 2, im->bpp, im->plane[0] + im->stride[0]);
  }
  else
  {
    // Load YUYV plane
    LoadPlane(buf.fields[FIELD_FULL][0], GL_BGRA,
              im->width / 2, im->height,
              im->stride[0], im->bpp, im->plane[0]);
  }

  VerifyGLState();

  return true;
}

void CLinuxRendererGL::DeleteYUV422PackedTexture(int index)
{
  CPictureBuffer& buf = m_buffers[index];
  YuvImage &im = buf.image;
  GLuint *pbo = buf.pbo;

  if (buf.fields[FIELD_FULL][0].id == 0)
    return;

  // finish up all textures, and delete them
  for (int f = 0;f<MAX_FIELDS;f++)
  {
    if (buf.fields[f][0].id)
    {
      if (glIsTexture(buf.fields[f][0].id))
      {
        glDeleteTextures(1, &buf.fields[f][0].id);
      }
      buf.fields[f][0].id = 0;
    }
    buf.fields[f][1].id = 0;
    buf.fields[f][2].id = 0;
  }

  if (pbo[0])
  {
    if (im.plane[0])
    {
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[0]);
      glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
      im.plane[0] = NULL;
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }
    glDeleteBuffers(1, pbo);
    pbo[0] = 0;
  }
  else
  {
    if (im.plane[0])
    {
      delete[] im.plane[0];
      im.plane[0] = NULL;
    }
  }
}

bool CLinuxRendererGL::CreateYUV422PackedTexture(int index)
{
  // since we also want the field textures, pitch must be texture aligned
  CPictureBuffer& buf = m_buffers[index];
  YuvImage &im = buf.image;
  GLuint *pbo = buf.pbo;

  // Delete any old texture
  DeleteYUV422PackedTexture(index);

  im.height = m_sourceHeight;
  im.width  = m_sourceWidth;
  im.cshift_x = 0;
  im.cshift_y = 0;
  im.bpp = 1;

  im.stride[0] = im.width * 2;
  im.stride[1] = 0;
  im.stride[2] = 0;

  im.plane[0] = NULL;
  im.plane[1] = NULL;
  im.plane[2] = NULL;

  // packed YUYV plane
  im.planesize[0] = im.stride[0] * im.height;
  // second plane is not used
  im.planesize[1] = 0;
  // third plane is not used
  im.planesize[2] = 0;

  bool pboSetup = false;
  if (m_pboUsed)
  {
    pboSetup = true;
    glGenBuffers(1, pbo);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[0]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, im.planesize[0] + PBO_OFFSET, 0, GL_STREAM_DRAW);
    void* pboPtr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    if (pboPtr)
    {
      im.plane[0] = (uint8_t*)pboPtr + PBO_OFFSET;
      memset(im.plane[0], 0, im.planesize[0]);
    }
    else
    {
      CLog::Log(LOGWARNING,"GL: failed to set up pixel buffer object");
      pboSetup = false;
    }

    if (!pboSetup)
    {
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, *pbo);
      glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
      glDeleteBuffers(1, pbo);
      memset(m_buffers[index].pbo, 0, sizeof(m_buffers[index].pbo));
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  }

  if (!pboSetup)
  {
    im.plane[0] = new uint8_t[im.planesize[0]];
  }

  for(int f = 0;f<MAX_FIELDS;f++)
  {
    if (!glIsTexture(buf.fields[f][0].id))
    {
      glGenTextures(1, &buf.fields[f][0].id);
      VerifyGLState();
    }
    buf.fields[f][0].pbo = pbo[0];
    buf.fields[f][1].id = buf.fields[f][0].id;
    buf.fields[f][2].id = buf.fields[f][1].id;
  }

  // YUV
  for (int f = FIELD_FULL; f<=FIELD_BOT ; f++)
  {
    int fieldshift = (f==FIELD_FULL) ? 0 : 1;
    CYuvPlane (&planes)[YuvImage::MAX_PLANES] = buf.fields[f];

    planes[0].texwidth  = im.width / 2;
    planes[0].texheight = im.height >> fieldshift;
    planes[1].texwidth  = planes[0].texwidth;
    planes[1].texheight = planes[0].texheight;
    planes[2].texwidth  = planes[1].texwidth;
    planes[2].texheight = planes[1].texheight;

    for (int p = 0; p < 3; p++)
    {
      planes[p].pixpertex_x = 2;
      planes[p].pixpertex_y = 1;
    }

    CYuvPlane &plane = planes[0];
    if (plane.texwidth * plane.texheight == 0)
      continue;

    glBindTexture(m_textureTarget, plane.id);

    glTexImage2D(m_textureTarget, 0, GL_RGBA, plane.texwidth, plane.texheight, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    VerifyGLState();
  }

  return true;
}

void CLinuxRendererGL::SetTextureFilter(GLenum method)
{
  for (int i = 0 ; i<m_NumYV12Buffers ; i++)
  {
    CPictureBuffer& buf = m_buffers[i];

    for (int f = FIELD_FULL; f<=FIELD_BOT ; f++)
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

bool CLinuxRendererGL::Supports(ERENDERFEATURE feature) const
{
  if (feature == RENDERFEATURE_STRETCH ||
      feature == RENDERFEATURE_NONLINSTRETCH ||
      feature == RENDERFEATURE_ZOOM ||
      feature == RENDERFEATURE_VERTICAL_SHIFT ||
      feature == RENDERFEATURE_PIXEL_RATIO ||
      feature == RENDERFEATURE_POSTPROCESS ||
      feature == RENDERFEATURE_ROTATION ||
      feature == RENDERFEATURE_BRIGHTNESS ||
      feature == RENDERFEATURE_CONTRAST ||
      feature == RENDERFEATURE_TONEMAP)
    return true;

  return false;
}

bool CLinuxRendererGL::SupportsMultiPassRendering()
{
  return m_renderSystem->IsExtSupported("GL_EXT_framebuffer_object");
}

bool CLinuxRendererGL::Supports(ESCALINGMETHOD method) const
{
  //nearest neighbor doesn't work on YUY2 and UYVY
  if (method == VS_SCALINGMETHOD_NEAREST &&
      m_format != AV_PIX_FMT_YUYV422 &&
      m_format != AV_PIX_FMT_UYVY422)
    return true;

  if (method == VS_SCALINGMETHOD_LINEAR ||
      method == VS_SCALINGMETHOD_AUTO)
    return true;

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
    float scaleX = fabs(((float)m_sourceWidth - m_destRect.Width())/m_sourceWidth)*100;
    float scaleY = fabs(((float)m_sourceHeight - m_destRect.Height())/m_sourceHeight)*100;
    int minScale = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_HQSCALERS);
    if (scaleX < minScale && scaleY < minScale)
      return false;

    bool hasFramebuffer = false;
    unsigned int major, minor;
    m_renderSystem->GetRenderVersion(major, minor);
    if (major > 3 ||
        (major == 3 && minor >= 2))
      hasFramebuffer = true;
    if (m_renderSystem->IsExtSupported("GL_EXT_framebuffer_object"))
      hasFramebuffer = true;
    if (hasFramebuffer  && (m_renderMethod & RENDER_GLSL))
      return true;
  }

  return false;
}

void CLinuxRendererGL::BindPbo(CPictureBuffer& buff)
{
  bool pbo = false;
  for(int plane = 0; plane < YuvImage::MAX_PLANES; plane++)
  {
    if(!buff.pbo[plane] || buff.image.plane[plane] == (uint8_t*)PBO_OFFSET)
      continue;
    pbo = true;

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buff.pbo[plane]);
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    buff.image.plane[plane] = (uint8_t*)PBO_OFFSET;
  }
  if (pbo)
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void CLinuxRendererGL::UnBindPbo(CPictureBuffer& buff)
{
  bool pbo = false;
  for(int plane = 0; plane < YuvImage::MAX_PLANES; plane++)
  {
    if(!buff.pbo[plane] || buff.image.plane[plane] != (uint8_t*)PBO_OFFSET)
      continue;
    pbo = true;

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buff.pbo[plane]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, buff.image.planesize[plane] + PBO_OFFSET, NULL, GL_STREAM_DRAW);
    buff.image.plane[plane] = (uint8_t*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY) + PBO_OFFSET;
  }
  if (pbo)
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

CRenderInfo CLinuxRendererGL::GetRenderInfo()
{
  CRenderInfo info;
  info.max_buffer_size = NUM_BUFFERS;
  return info;
}

// Color management helpers

bool CLinuxRendererGL::LoadCLUT()
{
  DeleteCLUT();

  int clutSize, dataSize;
  if (!CColorManager::Get3dLutSize(CMS_DATA_FMT_RGB, &clutSize, &dataSize))
    return false;

  // allocate buffer
  m_CLUTsize = clutSize;
  m_CLUT = static_cast<uint16_t*>(malloc(dataSize));

  // load 3DLUT
  if (!m_ColorManager->GetVideo3dLut(m_srcPrimaries, &m_cmsToken, CMS_DATA_FMT_RGB, m_CLUTsize,
                                     m_CLUT))
  {
    free(m_CLUT);
    CLog::Log(LOGERROR, "Error loading the LUT");
    return false;
  }

  // create 3DLUT texture
  CLog::Log(LOGDEBUG, "LinuxRendererGL: creating 3DLUT");
  glGenTextures(1, &m_tCLUTTex);
  glActiveTexture(GL_TEXTURE4);
  if (m_tCLUTTex <= 0)
  {
    CLog::Log(LOGERROR, "Error creating 3DLUT texture");
    return false;
  }

  // bind and set 3DLUT texture parameters
  glBindTexture(GL_TEXTURE_3D, m_tCLUTTex);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  // load 3DLUT data
  glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16, m_CLUTsize, m_CLUTsize, m_CLUTsize, 0, GL_RGB,
               GL_UNSIGNED_SHORT, m_CLUT);
  free(m_CLUT);
  glActiveTexture(GL_TEXTURE0);
  return true;
}

void CLinuxRendererGL::DeleteCLUT()
{
  if (m_tCLUTTex)
  {
    CLog::Log(LOGDEBUG, "LinuxRendererGL: deleting 3DLUT");
    glDeleteTextures(1, &m_tCLUTTex);
    m_tCLUTTex = 0;
  }
}

void CLinuxRendererGL::CheckVideoParameters(int index)
{
  const CPictureBuffer& buf = m_buffers[index];
  ETONEMAPMETHOD method = m_videoSettings.m_ToneMapMethod;

  if (buf.m_srcPrimaries != m_srcPrimaries)
  {
    m_srcPrimaries = buf.m_srcPrimaries;
    m_reloadShaders = true;
  }

  bool toneMap = false;
  if (method != VS_TONEMAPMETHOD_OFF)
  {
    if (buf.hasLightMetadata || (buf.hasDisplayMetadata && buf.displayMetadata.has_luminance))
    {
      toneMap = true;
    }
  }

  if (toneMap != m_toneMap || (m_toneMapMethod != method))
  {
    m_reloadShaders = true;
  }
  m_toneMap = toneMap;
  m_toneMapMethod = method;
}

CRenderCapture* CLinuxRendererGL::GetRenderCapture()
{
  return new CRenderCaptureGL;
}
