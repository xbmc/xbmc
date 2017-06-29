/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

//#define DEBUG_VERBOSE 1

#include "system.h"

#if HAS_GLES == 2
#include "system_gl.h"

#include <locale.h>
#include "guilib/MatrixGLES.h"
#include "LinuxRendererGLES.h"
#include "ServiceBroker.h"
#include "utils/MathUtils.h"
#include "utils/GLUtils.h"
#include "utils/log.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "VideoShaders/YUV2RGBShader.h"
#include "VideoShaders/VideoFilterShader.h"
#include "windowing/WindowingFactory.h"
#include "guilib/Texture.h"
#include "threads/SingleLock.h"
#include "RenderCapture.h"
#include "Application.h"
#include "RenderFactory.h"
#include "cores/IPlayer.h"

#if defined(__ARM_NEON__) && !defined(__LP64__)
#include "yuv2rgb.neon.h"
#include "utils/CPUInfo.h"
#endif

#if defined(EGL_KHR_reusable_sync) && !defined(EGL_EGLEXT_PROTOTYPES)
static PFNEGLCREATESYNCKHRPROC eglCreateSyncKHR;
static PFNEGLDESTROYSYNCKHRPROC eglDestroySyncKHR;
static PFNEGLCLIENTWAITSYNCKHRPROC eglClientWaitSyncKHR;
#endif

using namespace Shaders;

CLinuxRendererGLES::YUVBUFFER::YUVBUFFER()
{
  memset(&fields, 0, sizeof(fields));
  memset(&image , 0, sizeof(image));
  videoBuffer = nullptr;
  loaded = false;
}

CLinuxRendererGLES::YUVBUFFER::~YUVBUFFER()
{
}

CLinuxRendererGLES::CLinuxRendererGLES()
{
  m_textureTarget = GL_TEXTURE_2D;

  m_renderMethod = RENDER_GLSL;
  m_oldRenderMethod = m_renderMethod;
  m_renderQuality = RQ_SINGLEPASS;
  m_iFlags = 0;
  m_format = AV_PIX_FMT_NONE;

  m_iYV12RenderBuffer = 0;
  m_currentField = FIELD_FULL;
  m_reloadShaders = 0;
  m_pYUVProgShader = NULL;
  m_pYUVBobShader = NULL;
  m_pVideoFilterShader = NULL;
  m_scalingMethod = VS_SCALINGMETHOD_LINEAR;
  m_scalingMethodGui = (ESCALINGMETHOD)-1;

  m_NumYV12Buffers = 0;
  m_iLastRenderBuffer = 0;
  m_bConfigured = false;
  m_bValidated = false;
  m_StrictBinding = false;
  m_clearColour = 0.0f;

#if defined(EGL_KHR_reusable_sync) && !defined(EGL_EGLEXT_PROTOTYPES)
  if (!eglCreateSyncKHR) {
    eglCreateSyncKHR = (PFNEGLCREATESYNCKHRPROC) eglGetProcAddress("eglCreateSyncKHR");
  }
  if (!eglDestroySyncKHR) {
    eglDestroySyncKHR = (PFNEGLDESTROYSYNCKHRPROC) eglGetProcAddress("eglDestroySyncKHR");
  }
  if (!eglClientWaitSyncKHR) {
    eglClientWaitSyncKHR = (PFNEGLCLIENTWAITSYNCKHRPROC) eglGetProcAddress("eglClientWaitSyncKHR");
  }
#endif
}

CLinuxRendererGLES::~CLinuxRendererGLES()
{
  UnInit();

  ReleaseShaders();
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
    CLog::Log(LOGNOTICE,"Using GL_TEXTURE_2D");

    // function pointer for texture might change in
    // call to LoadShaders
    glFinish();
    for (int i = 0 ; i < NUM_BUFFERS ; i++)
      DeleteTexture(i);

     // create the yuv textures
    UpdateVideoFilter();
    LoadShaders();
    if (m_renderMethod < 0)
      return false;

    for (int i = 0 ; i < m_NumYV12Buffers ; i++)
      CreateTexture(i);

    m_bValidated = true;
    return true;
  }
  return false;
}

bool CLinuxRendererGLES::Configure(const VideoPicture &picture, float fps, unsigned flags, unsigned int orientation)
{
  m_format = picture.videoBuffer->GetFormat();
  m_sourceWidth = picture.iWidth;
  m_sourceHeight = picture.iHeight;
  m_renderOrientation = orientation;

  // Save the flags.
  m_iFlags = flags;

  // Calculate the input frame aspect ratio.
  CalculateFrameAspectRatio(picture.iDisplayWidth, picture.iDisplayHeight);
  SetViewMode(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ViewMode);
  ManageRenderArea();

  m_bConfigured = true;
  m_scalingMethodGui = (ESCALINGMETHOD)-1;

  // Ensure that textures are recreated and rendering starts only after the 1st
  // frame is loaded after every call to Configure().
  m_bValidated = false;

  m_iLastRenderBuffer = -1;

  return true;
}

bool CLinuxRendererGLES::ConfigChanged(const VideoPicture &picture)
{
  if (picture.videoBuffer->GetFormat() != m_format)
    return true;

  return false;
}

int CLinuxRendererGLES::NextYV12Texture()
{
  return (m_iYV12RenderBuffer + 1) % m_NumYV12Buffers;
}

void CLinuxRendererGLES::AddVideoPicture(const VideoPicture &picture, int index)
{
  YUVBUFFER &buf = m_buffers[index];
  buf.videoBuffer = picture.videoBuffer;
  buf.videoBuffer->Acquire();
  buf.loaded = false;
}

void CLinuxRendererGLES::ReleaseBuffer(int idx)
{
  YUVBUFFER &buf = m_buffers[idx];
  if (buf.videoBuffer)
  {
    buf.videoBuffer->Release();
    buf.videoBuffer = nullptr;
  }
}

void CLinuxRendererGLES::CalculateTextureSourceRects(int source, int num_planes)
{
  YUVBUFFER& buf = m_buffers[source];
  YuvImage* im  = &buf.image;

  // calculate the source rectangle
  for(int field = 0; field < 3; field++)
  {
    for(int plane = 0; plane < num_planes; plane++)
    {
      YUVPLANE& p = buf.fields[field][plane];

      p.rect = m_sourceRect;
      p.width  = im->width;
      p.height = im->height;

      if(field != FIELD_FULL)
      {
        /* correct for field offsets and chroma offsets */
        float offset_y = 0.5;
        if(plane != 0)
          offset_y += 0.5;
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

void CLinuxRendererGLES::LoadPlane(YUVPLANE& plane, int type,
                                 unsigned width, unsigned height,
                                 int stride, int bpp, void* data)
{
  const GLvoid *pixelData = data;

  int bps = bpp * glFormatElementByteCount(type);

  unsigned datatype;
  if (bpp == 2)
    datatype = GL_UNSIGNED_SHORT;
  else
    datatype = GL_UNSIGNED_BYTE;

  glBindTexture(m_textureTarget, plane.id);

  // OpenGL ES does not support strided texture input.
  if (stride != width * bps)
  {
    unsigned char* src = (unsigned char*)data;
    for (unsigned int y = 0; y < height;++y, src += stride)
      glTexSubImage2D(m_textureTarget, 0, 0, y, width, 1, type, datatype, src);
  } else {
    glTexSubImage2D(m_textureTarget, 0, 0, 0, width, height, type, datatype, pixelData);
  }

  /* check if we need to load any border pixels */
  if (height < plane.texheight)
    glTexSubImage2D( m_textureTarget, 0
                   , 0, height, width, 1
                   , type, datatype
                   , (unsigned char*)pixelData + stride * (height-1));

  if (width  < plane.texwidth)
    glTexSubImage2D( m_textureTarget, 0
                   , width, 0, 1, height
                   , type, datatype
                   , (unsigned char*)pixelData + bps * (width-1));

  glBindTexture(m_textureTarget, 0);
}

void CLinuxRendererGLES::Reset()
{
  for(int i=0; i<m_NumYV12Buffers; i++)
  {
    ReleaseBuffer(i);
  }
}

void CLinuxRendererGLES::Flush()
{
  if (!m_bValidated)
    return;

  glFinish();

  for (int i = 0 ; i < m_NumYV12Buffers ; i++)
    DeleteTexture(i);

  glFinish();
  m_bValidated = false;
  m_fbo.Cleanup();
  m_iYV12RenderBuffer = 0;
}

void CLinuxRendererGLES::Update()
{
  if (!m_bConfigured)
    return;
  ManageRenderArea();
  ValidateRenderTarget();
}

void CLinuxRendererGLES::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  if (!m_bConfigured)
    return;

  // if its first pass, just init textures and return
  if (ValidateRenderTarget())
    return;

  if (!IsGuiLayer())
  {
    RenderUpdateVideo(clear, flags, alpha);
    return;
  }

  int index = m_iYV12RenderBuffer;
  YUVBUFFER& buf =  m_buffers[index];

  if (!buf.fields[FIELD_FULL][0].id)
    return;

  ManageRenderArea();

  m_iLastRenderBuffer = index;

  if (clear)
  {
    glClearColor(m_clearColour, m_clearColour, m_clearColour, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0,0,0,0);
  }

  if (alpha<255)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (m_pYUVProgShader)
      m_pYUVProgShader->SetAlpha(alpha / 255.0f);
    if (m_pYUVBobShader)
      m_pYUVBobShader->SetAlpha(alpha / 255.0f);
  }
  else
  {
    glDisable(GL_BLEND);
    if (m_pYUVProgShader)
      m_pYUVProgShader->SetAlpha(1.0f);
    if (m_pYUVBobShader)
      m_pYUVBobShader->SetAlpha(1.0f);
  }

  if ((flags & RENDER_FLAG_TOP) && (flags & RENDER_FLAG_BOT))
    CLog::Log(LOGERROR, "GLES: Cannot render stipple!");
  else
    Render(flags, index);

  VerifyGLState();
  glEnable(GL_BLEND);
}

void CLinuxRendererGLES::RenderUpdateVideo(bool clear, DWORD flags, DWORD alpha)
{
  if (!m_bConfigured)
    return;

  if (IsGuiLayer())
    return;
}

void CLinuxRendererGLES::FlipPage(int source)
{
  if( source >= 0 && source < m_NumYV12Buffers )
    m_iYV12RenderBuffer = source;
  else
    m_iYV12RenderBuffer = NextYV12Texture();

  return;
}

void CLinuxRendererGLES::UpdateVideoFilter()
{
  if (m_scalingMethodGui == CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ScalingMethod)
    return;
  m_scalingMethodGui = CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ScalingMethod;
  m_scalingMethod    = m_scalingMethodGui;

  if(!Supports(m_scalingMethod))
  {
    CLog::Log(LOGWARNING, "CLinuxRendererGLES::UpdateVideoFilter - chosen scaling method %d, is not supported by renderer", (int)m_scalingMethod);
    m_scalingMethod = VS_SCALINGMETHOD_LINEAR;
  }

  if (m_pVideoFilterShader)
  {
    m_pVideoFilterShader->Free();
    delete m_pVideoFilterShader;
    m_pVideoFilterShader = NULL;
  }
  m_fbo.Cleanup();

  VerifyGLState();

  switch (m_scalingMethod)
  {
  case VS_SCALINGMETHOD_NEAREST:
    SetTextureFilter(GL_NEAREST);
    m_renderQuality = RQ_SINGLEPASS;
    return;

  case VS_SCALINGMETHOD_LINEAR:
    SetTextureFilter(GL_LINEAR);
    m_renderQuality = RQ_SINGLEPASS;
    return;

  case VS_SCALINGMETHOD_CUBIC:
    CLog::Log(LOGERROR, "GLES: CUBIC not supported!");
    break;

  case VS_SCALINGMETHOD_LANCZOS2:
  case VS_SCALINGMETHOD_LANCZOS3:
  case VS_SCALINGMETHOD_SINC8:
  case VS_SCALINGMETHOD_NEDI:
    CLog::Log(LOGERROR, "GL: TODO: This scaler has not yet been implemented");
    break;

  default:
    break;
  }

  CLog::Log(LOGERROR, "GL: Falling back to bilinear due to failure to init scaler");
  if (m_pVideoFilterShader)
  {
    m_pVideoFilterShader->Free();
    delete m_pVideoFilterShader;
    m_pVideoFilterShader = NULL;
  }
  m_fbo.Cleanup();

  SetTextureFilter(GL_LINEAR);
  m_renderQuality = RQ_SINGLEPASS;
}

void CLinuxRendererGLES::LoadShaders(int field)
{
  m_reloadShaders = 0;

  if (!LoadShadersHook())
  {
    int requestedMethod = CServiceBroker::GetSettings().GetInt(CSettings::SETTING_VIDEOPLAYER_RENDERMETHOD);
    CLog::Log(LOGDEBUG, "GL: Requested render method: %d", requestedMethod);

    ReleaseShaders();

    switch(requestedMethod)
    {
      case RENDER_METHOD_AUTO:
      case RENDER_METHOD_GLSL:
        // Try GLSL shaders if supported and user requested auto or GLSL.
        if (glCreateProgram)
        {
          // create regular scan shader
          CLog::Log(LOGNOTICE, "GL: Selecting Single Pass YUV 2 RGB shader");

          EShaderFormat shaderFormat = GetShaderFormat();
          m_pYUVProgShader = new YUV2RGBProgressiveShader(false, m_iFlags, shaderFormat);
          m_pYUVBobShader = new YUV2RGBBobShader(false, m_iFlags, shaderFormat);
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
            CLog::Log(LOGERROR, "GL: Error enabling YUV2RGB GLSL shader");
            m_renderMethod = -1;
            break;
          }
        }
      default:
        {
          m_renderMethod = -1 ;
          CLog::Log(LOGERROR, "GL: render method not supported");
        }
    }
  }

  // determine whether GPU supports NPOT textures
  if (!g_Windowing.IsExtSupported("GL_TEXTURE_NPOT"))
  {
    CLog::Log(LOGNOTICE, "GL: GL_ARB_texture_rectangle not supported and OpenGL version is not 2.x");
    CLog::Log(LOGNOTICE, "GL: Reverting to POT textures");
    m_renderMethod |= RENDER_POT;
  }
  else
    CLog::Log(LOGNOTICE, "GL: NPOT texture support detected");

  if (m_oldRenderMethod != m_renderMethod)
  {
    CLog::Log(LOGDEBUG, "CLinuxRendererGLES: Reorder drawpoints due to method change from %i to %i", m_oldRenderMethod, m_renderMethod);
    ReorderDrawPoints();
    m_oldRenderMethod = m_renderMethod;
  }
}

void CLinuxRendererGLES::ReleaseShaders()
{
  if (m_pYUVProgShader)
  {
    m_pYUVProgShader->Free();
    delete m_pYUVProgShader;
    m_pYUVProgShader = NULL;
  }
  if (m_pYUVBobShader)
  {
    m_pYUVBobShader->Free();
    delete m_pYUVBobShader;
    m_pYUVBobShader = NULL;
  }
}

void CLinuxRendererGLES::UnInit()
{
  CLog::Log(LOGDEBUG, "LinuxRendererGL: Cleaning up GL resources");
  CSingleLock lock(g_graphicsContext);

  // YV12 textures
  for (int i = 0; i < NUM_BUFFERS; ++i)
    DeleteTexture(i);

  // cleanup framebuffer object if it was in use
  m_fbo.Cleanup();
  m_bValidated = false;
  m_bConfigured = false;
}

inline void CLinuxRendererGLES::ReorderDrawPoints()
{
  CBaseRenderer::ReorderDrawPoints();//call base impl. for rotating the points
}

bool CLinuxRendererGLES::CreateTexture(int index)
{
  if (m_format == AV_PIX_FMT_NV12)
    return CreateNV12Texture(index);
  else
    return CreateYV12Texture(index);
}

void CLinuxRendererGLES::DeleteTexture(int index)
{
  ReleaseBuffer(index);

  if (m_format == AV_PIX_FMT_NV12)
    DeleteNV12Texture(index);
  else
    DeleteYV12Texture(index);
}

bool CLinuxRendererGLES::UploadTexture(int index)
{
  if (!m_buffers[index].videoBuffer)
    return false;

  if (m_buffers[index].loaded)
    return true;

  bool ret = false;

  YuvImage &dst = m_buffers[index].image;
  m_buffers[index].videoBuffer->GetPlanes(dst.plane);
  m_buffers[index].videoBuffer->GetStrides(dst.stride);

  if (m_format == AV_PIX_FMT_NV12)
  {
    return UploadNV12Texture(index);
  }
  else
  {
    // default to YV12 texture handlers
    return UploadYV12Texture(index);
  }
  return ret;
}

void CLinuxRendererGLES::Render(DWORD flags, int index)
{
  // obtain current field, if interlaced
  if( flags & RENDER_FLAG_TOP)
    m_currentField = FIELD_TOP;

  else if (flags & RENDER_FLAG_BOT)
    m_currentField = FIELD_BOT;

  else
    m_currentField = FIELD_FULL;

  // call texture load function
  if (!UploadTexture(index))
    return;
  
  if (RenderHook(index))
    ;
  else if (m_renderMethod & RENDER_GLSL)
  {
    UpdateVideoFilter();
    switch(m_renderQuality)
    {
    case RQ_LOW:
    case RQ_SINGLEPASS:
      RenderSinglePass(index, m_currentField);
      VerifyGLState();
      break;

    case RQ_MULTIPASS:
      RenderMultiPass(index, m_currentField);
      VerifyGLState();
      break;

    default:
      break;
    }
  }
  
  AfterRenderHook(index);
}

void CLinuxRendererGLES::RenderSinglePass(int index, int field)
{
  YUVPLANE (&planes)[YuvImage::MAX_PLANES] = m_buffers[index].fields[field];

  if (m_reloadShaders)
  {
    LoadShaders(field);
  }

  glDisable(GL_DEPTH_TEST);

  // Y
  glActiveTexture(GL_TEXTURE0);
  glEnable(m_textureTarget);
  glBindTexture(m_textureTarget, planes[0].id);

  // U
  glActiveTexture(GL_TEXTURE1);
  glEnable(m_textureTarget);
  glBindTexture(m_textureTarget, planes[1].id);

  // V
  glActiveTexture(GL_TEXTURE2);
  glEnable(m_textureTarget);
  glBindTexture(m_textureTarget, planes[2].id);

  glActiveTexture(GL_TEXTURE0);
  VerifyGLState();

  Shaders::BaseYUV2RGBShader *pYUVShader;
  if (field != FIELD_FULL)
    pYUVShader = m_pYUVBobShader;
  else
    pYUVShader = m_pYUVProgShader;

  pYUVShader->SetBlack(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_Brightness * 0.01f - 0.5f);
  pYUVShader->SetContrast(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_Contrast * 0.02f);
  pYUVShader->SetWidth(planes[0].texwidth);
  pYUVShader->SetHeight(planes[0].texheight);
  if     (field == FIELD_TOP)
    pYUVShader->SetField(1);
  else if(field == FIELD_BOT)
    pYUVShader->SetField(0);

  pYUVShader->SetMatrices(glMatrixProject.Get(), glMatrixModview.Get());
  pYUVShader->SetConvertFullColorRange(!g_Windowing.UseLimitedColor());
  pYUVShader->Enable();

  GLubyte idx[4] = {0, 1, 3, 2};        //determines order of triangle strip
  GLfloat m_vert[4][3];
  GLfloat m_tex[3][4][2];

  GLint vertLoc = pYUVShader->GetVertexLoc();
  GLint Yloc    = pYUVShader->GetYcoordLoc();
  GLint Uloc    = pYUVShader->GetUcoordLoc();
  GLint Vloc    = pYUVShader->GetVcoordLoc();

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
  for (int i=0; i<3; i++)
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

  glActiveTexture(GL_TEXTURE1);
  glDisable(m_textureTarget);

  glActiveTexture(GL_TEXTURE2);
  glDisable(m_textureTarget);

  glActiveTexture(GL_TEXTURE0);
  glDisable(m_textureTarget);

  VerifyGLState();
}

void CLinuxRendererGLES::RenderMultiPass(int index, int field)
{
  //! @todo Multipass rendering does not currently work! FIX!
  CLog::Log(LOGERROR, "GLES: MULTIPASS rendering was called! But it doesnt work!!!");
  return;
}

bool CLinuxRendererGLES::RenderCapture(CRenderCapture* capture)
{
  if (!m_bValidated)
    return false;

  // save current video rect
  CRect saveSize = m_destRect;
  saveRotatedCoords();//backup current m_rotatedDestCoords

  // new video rect is thumbnail size
  m_destRect.SetRect(0, 0, (float)capture->GetWidth(), (float)capture->GetHeight());
  MarkDirty();
  syncDestRectToRotatedPoints();//syncs the changed destRect to m_rotatedDestCoords
  // clear framebuffer and invert Y axis to get non-inverted image
  glDisable(GL_BLEND);

  glMatrixModview.Push();
  // fixme - we know that cvref  & eglimg are already flipped in y direction
  // but somehow this also effects the rendercapture here
  // therefore we have to skip the flip here or we get upside down
  // images
  if (m_renderMethod != RENDER_CVREF)
  {
    glMatrixModview->Translatef(0.0f, capture->GetHeight(), 0.0f);
    glMatrixModview->Scalef(1.0f, -1.0f, 1.0f);
  }
  glMatrixModview.Load();

  capture->BeginRender();

  Render(RENDER_FLAG_NOOSD, m_iYV12RenderBuffer);
  // read pixels
  glReadPixels(0, g_graphicsContext.GetHeight() - capture->GetHeight(), capture->GetWidth(), capture->GetHeight(),
               GL_RGBA, GL_UNSIGNED_BYTE, capture->GetRenderBuffer());

  // OpenGLES returns in RGBA order but CRenderCapture needs BGRA order
  // XOR Swap RGBA -> BGRA
  unsigned char* pixels = (unsigned char*)capture->GetRenderBuffer();
  for (unsigned int i = 0; i < capture->GetWidth() * capture->GetHeight(); i++, pixels+=4)
  {
    std::swap(pixels[0], pixels[2]);
  }

  capture->EndRender();

  // revert model view matrix
  glMatrixModview.PopLoad();

  // restore original video rect
  m_destRect = saveSize;
  restoreRotatedCoords();//restores the previous state of the rotated dest coords

  return true;
}

//********************************************************************************************************
// YV12 Texture creation, deletion, copying + clearing
//********************************************************************************************************
bool CLinuxRendererGLES::UploadYV12Texture(int source)
{
  YUVBUFFER& buf = m_buffers[source];
  YuvImage* im = &buf.image;

  glEnable(m_textureTarget);
  VerifyGLState();

  glPixelStorei(GL_UNPACK_ALIGNMENT,1);

  //Load Y plane
  LoadPlane(buf.fields[FIELD_FULL][0], GL_LUMINANCE,
            im->width, im->height,
            im->stride[0], im->bpp, im->plane[0]);

  //load U plane
  LoadPlane(buf.fields[FIELD_FULL][1], GL_LUMINANCE,
            im->width >> im->cshift_x, im->height >> im->cshift_y,
            im->stride[1], im->bpp, im->plane[1]);

  //load V plane
  LoadPlane(buf.fields[FIELD_FULL][2], GL_ALPHA,
            im->width >> im->cshift_x, im->height >> im->cshift_y,
            im->stride[2], im->bpp, im->plane[2]);

  VerifyGLState();

  CalculateTextureSourceRects(source, 3);

  glDisable(m_textureTarget);
  return true;
}

void CLinuxRendererGLES::DeleteYV12Texture(int index)
{
  YuvImage &im = m_buffers[index].image;

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
    if (im.plane[p])
    {
      delete[] im.plane[p];
      im.plane[p] = NULL;
    }
  }
}

static GLint GetInternalFormat(GLint format, int bpp)
{
  if(bpp == 2)
  {
    switch (format)
    {
#ifdef GL_ALPHA16
      case GL_ALPHA:     return GL_ALPHA16;
#endif
#ifdef GL_LUMINANCE16
      case GL_LUMINANCE: return GL_LUMINANCE16;
#endif
      default:           return format;
    }
  }
  else
    return format;
}

bool CLinuxRendererGLES::CreateYV12Texture(int index)
{
  /* since we also want the field textures, pitch must be texture aligned */
  unsigned p;
  YuvImage &im = m_buffers[index].image;

  DeleteYV12Texture(index);

  im.height = m_sourceHeight;
  im.width  = m_sourceWidth;
  im.cshift_x = 1;
  im.cshift_y = 1;

  if(m_format == AV_PIX_FMT_YUV420P16 ||
     m_format == AV_PIX_FMT_YUV420P10)
    im.bpp = 2;
  else
    im.bpp = 1;

  im.stride[0] = im.bpp *   im.width;
  im.stride[1] = im.bpp * ( im.width >> im.cshift_x );
  im.stride[2] = im.bpp * ( im.width >> im.cshift_x );

  im.planesize[0] = im.stride[0] *   im.height;
  im.planesize[1] = im.stride[1] * ( im.height >> im.cshift_y );
  im.planesize[2] = im.stride[2] * ( im.height >> im.cshift_y );

  for (int i = 0; i < 3; i++)
    im.plane[i] = new BYTE[im.planesize[i]];

  glEnable(m_textureTarget);
  for(int f = 0;f<MAX_FIELDS;f++)
  {
    for(p = 0;p<YuvImage::MAX_PLANES;p++)
    {
      if (!glIsTexture(m_buffers[index].fields[f][p].id))
      {
        glGenTextures(1, &m_buffers[index].fields[f][p].id);
        VerifyGLState();
      }
    }
  }

  // YUV
  for (int f = FIELD_FULL; f<=FIELD_BOT ; f++)
  {
    int fieldshift = (f==FIELD_FULL) ? 0 : 1;
    YUVPLANE (&planes)[YuvImage::MAX_PLANES] = m_buffers[index].fields[f];

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

    if(m_renderMethod & RENDER_POT)
    {
      for(int p = 0; p < 3; p++)
      {
        planes[p].texwidth  = NP2(planes[p].texwidth);
        planes[p].texheight = NP2(planes[p].texheight);
      }
    }

    for(int p = 0; p < 3; p++)
    {
      YUVPLANE &plane = planes[p];
      if (plane.texwidth * plane.texheight == 0)
        continue;

      glBindTexture(m_textureTarget, plane.id);

      GLenum format;
      GLint internalformat;
      if (p == 2) //V plane needs an alpha texture
        format = GL_ALPHA;
      else
        format = GL_LUMINANCE;
      internalformat = GetInternalFormat(format, im.bpp);

      if(m_renderMethod & RENDER_POT)
        CLog::Log(LOGDEBUG, "GL: Creating YUV POT texture of size %d x %d",  plane.texwidth, plane.texheight);
      else
        CLog::Log(LOGDEBUG,  "GL: Creating YUV NPOT texture of size %d x %d", plane.texwidth, plane.texheight);

      glTexImage2D(m_textureTarget, 0, internalformat, plane.texwidth, plane.texheight, 0, format, GL_UNSIGNED_BYTE, NULL);

      glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      VerifyGLState();
    }
  }
  glDisable(m_textureTarget);
  return true;
}

//********************************************************************************************************
// NV12 Texture loading, creation and deletion
//********************************************************************************************************
bool CLinuxRendererGLES::UploadNV12Texture(int source)
{
  YUVBUFFER& buf = m_buffers[source];
  YuvImage* im = &buf.image;

  bool deinterlacing;
  if (m_currentField == FIELD_FULL)
    deinterlacing = false;
  else
    deinterlacing = true;

  glEnable(m_textureTarget);
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

  glDisable(m_textureTarget);
  return true;
}

bool CLinuxRendererGLES::CreateNV12Texture(int index)
{
  // since we also want the field textures, pitch must be texture aligned
  YUVBUFFER& buf = m_buffers[index];
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

  im.plane[0] = NULL;
  im.plane[1] = NULL;
  im.plane[2] = NULL;

  // Y plane
  im.planesize[0] = im.stride[0] * im.height;
  // packed UV plane
  im.planesize[1] = im.stride[1] * im.height / 2;
  // third plane is not used
  im.planesize[2] = 0;

  for (int i = 0; i < 2; i++)
    im.plane[i] = new BYTE[im.planesize[i]];

  glEnable(m_textureTarget);
  for(int f = 0;f<MAX_FIELDS;f++)
  {
    for(int p = 0;p<2;p++)
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
  for (int f = FIELD_FULL; f<=FIELD_BOT ; f++)
  {
    int fieldshift = (f==FIELD_FULL) ? 0 : 1;
    YUVPLANE (&planes)[YuvImage::MAX_PLANES] = buf.fields[f];

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

    if(m_renderMethod & RENDER_POT)
    {
      for(int p = 0; p < 3; p++)
      {
        planes[p].texwidth  = NP2(planes[p].texwidth);
        planes[p].texheight = NP2(planes[p].texheight);
      }
    }

    for(int p = 0; p < 2; p++)
    {
      YUVPLANE &plane = planes[p];
      if (plane.texwidth * plane.texheight == 0)
        continue;

      glBindTexture(m_textureTarget, plane.id);

      if (p == 1)
        glTexImage2D(m_textureTarget, 0, GL_LUMINANCE_ALPHA, plane.texwidth, plane.texheight, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, NULL);
      else
        glTexImage2D(m_textureTarget, 0, GL_LUMINANCE, plane.texwidth, plane.texheight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

      glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      VerifyGLState();
    }
  }
  glDisable(m_textureTarget);

  return true;
}
void CLinuxRendererGLES::DeleteNV12Texture(int index)
{
  YUVBUFFER& buf = m_buffers[index];
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
    if (im.plane[p])
    {
      delete[] im.plane[p];
      im.plane[p] = NULL;
    }
  }
}

//********************************************************************************************************
// SurfaceTexture creation, deletion, copying + clearing
//********************************************************************************************************
void CLinuxRendererGLES::SetTextureFilter(GLenum method)
{
  for (int i = 0 ; i<m_NumYV12Buffers ; i++)
  {
    YUVBUFFER& buf = m_buffers[i];

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

bool CLinuxRendererGLES::Supports(ERENDERFEATURE feature)
{
  if(feature == RENDERFEATURE_BRIGHTNESS)
    return true;

  if(feature == RENDERFEATURE_CONTRAST)
    return true;

  if(feature == RENDERFEATURE_GAMMA)
    return false;

  if(feature == RENDERFEATURE_NOISE)
    return false;

  if(feature == RENDERFEATURE_SHARPNESS)
    return false;

  if (feature == RENDERFEATURE_NONLINSTRETCH)
    return false;

  if (feature == RENDERFEATURE_STRETCH         ||
      feature == RENDERFEATURE_ZOOM            ||
      feature == RENDERFEATURE_VERTICAL_SHIFT  ||
      feature == RENDERFEATURE_PIXEL_RATIO     ||
      feature == RENDERFEATURE_POSTPROCESS     ||
      feature == RENDERFEATURE_ROTATION)
    return true;


  return false;
}

bool CLinuxRendererGLES::SupportsMultiPassRendering()
{
  return false;
}

bool CLinuxRendererGLES::Supports(ESCALINGMETHOD method)
{
  if(method == VS_SCALINGMETHOD_NEAREST
  || method == VS_SCALINGMETHOD_LINEAR)
    return true;

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

#endif

