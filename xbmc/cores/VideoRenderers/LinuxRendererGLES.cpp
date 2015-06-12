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
#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif

#if HAS_GLES == 2
#include "system_gl.h"

#include <locale.h>
#include "guilib/MatrixGLES.h"
#include "LinuxRendererGLES.h"
#include "utils/MathUtils.h"
#include "utils/GLUtils.h"
#include "utils/log.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "guilib/FrameBufferObject.h"
#include "VideoShaders/YUV2RGBShader.h"
#include "VideoShaders/VideoFilterShader.h"
#include "windowing/WindowingFactory.h"
#include "guilib/Texture.h"
#include "../dvdplayer/DVDCodecs/Video/OpenMaxVideo.h"
#include "threads/SingleLock.h"
#include "RenderCapture.h"
#include "RenderFormats.h"
#include "xbmc/Application.h"
#include "cores/IPlayer.h"

extern "C" {
#include "libswscale/swscale.h"
}

#if defined(__ARM_NEON__)
#include "yuv2rgb.neon.h"
#include "utils/CPUInfo.h"
#endif
#ifdef HAVE_VIDEOTOOLBOXDECODER
#include "DVDCodecs/Video/DVDVideoCodecVideoToolBox.h"
#include <CoreVideo/CoreVideo.h>
#endif
#ifdef TARGET_DARWIN_IOS
#include "osx/DarwinUtils.h"
#endif
#if defined(HAS_LIBSTAGEFRIGHT)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "windowing/egl/EGLWrapper.h"
#include "android/activity/XBMCApp.h"
#include "DVDCodecs/Video/DVDVideoCodecStageFright.h"

// EGL extension functions
static PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
static PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;
#endif

#if defined(EGL_KHR_reusable_sync) && !defined(EGL_EGLEXT_PROTOTYPES)
static PFNEGLCREATESYNCKHRPROC eglCreateSyncKHR;
static PFNEGLDESTROYSYNCKHRPROC eglDestroySyncKHR;
static PFNEGLCLIENTWAITSYNCKHRPROC eglClientWaitSyncKHR;
#endif

#ifdef HAS_IMXVPU
#include "windowing/egl/EGLWrapper.h"
#include "DVDCodecs/Video/DVDVideoCodecIMX.h"
#endif

#if defined(TARGET_ANDROID)
#include "DVDCodecs/Video/DVDVideoCodecAndroidMediaCodec.h"
#endif

using namespace Shaders;

CLinuxRendererGLES::YUVBUFFER::YUVBUFFER()
{
  memset(&fields, 0, sizeof(fields));
  memset(&image , 0, sizeof(image));
  flipindex = 0;
#ifdef HAVE_LIBOPENMAX
  openMaxBufferHolder = NULL;
#endif
#ifdef HAVE_VIDEOTOOLBOXDECODER
  cvBufferRef = NULL;
#endif
#ifdef HAS_LIBSTAGEFRIGHT
  stf = NULL;
  eglimg = EGL_NO_IMAGE_KHR;
#endif
#if defined(TARGET_ANDROID)
  mediacodec = NULL;
#endif
#ifdef HAS_IMXVPU
  IMXBuffer = NULL;
#endif
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
  m_format = RENDER_FMT_NONE;

  m_iYV12RenderBuffer = 0;
  m_flipindex = 0;
  m_currentField = FIELD_FULL;
  m_reloadShaders = 0;
  m_pYUVProgShader = NULL;
  m_pYUVBobShader = NULL;
  m_pVideoFilterShader = NULL;
  m_scalingMethod = VS_SCALINGMETHOD_LINEAR;
  m_scalingMethodGui = (ESCALINGMETHOD)-1;

  // default texture handlers to YUV
  m_textureUpload = &CLinuxRendererGLES::UploadYV12Texture;
  m_textureCreate = &CLinuxRendererGLES::CreateYV12Texture;
  m_textureDelete = &CLinuxRendererGLES::DeleteYV12Texture;

  m_rgbBuffer = NULL;
  m_rgbBufferSize = 0;

  m_sw_context = NULL;
  m_NumYV12Buffers = 0;
  m_iLastRenderBuffer = 0;
  m_bConfigured = false;
  m_bValidated = false;
  m_bImageReady = false;
  m_StrictBinding = false;
  m_clearColour = 0.0f;

#ifdef HAS_LIBSTAGEFRIGHT
  if (!eglCreateImageKHR)
    eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC) CEGLWrapper::GetProcAddress("eglCreateImageKHR");
  if (!eglDestroyImageKHR)
    eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC) CEGLWrapper::GetProcAddress("eglDestroyImageKHR");
  if (!glEGLImageTargetTexture2DOES)
    glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) CEGLWrapper::GetProcAddress("glEGLImageTargetTexture2DOES");
#endif

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

  if (m_rgbBuffer != NULL) {
    delete [] m_rgbBuffer;
    m_rgbBuffer = NULL;
  }

  ReleaseShaders();
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
      (this->*m_textureDelete)(i);

     // create the yuv textures
    LoadShaders();

    for (int i = 0 ; i < m_NumYV12Buffers ; i++)
      (this->*m_textureCreate)(i);

    m_bValidated = true;
    return true;
  }
  return false;
}

bool CLinuxRendererGLES::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, ERenderFormat format, unsigned extended_format, unsigned int orientation)
{
  m_sourceWidth = width;
  m_sourceHeight = height;
  m_renderOrientation = orientation;

  // Save the flags.
  m_iFlags = flags;
  m_format = format;

  // Calculate the input frame aspect ratio.
  CalculateFrameAspectRatio(d_width, d_height);
  ChooseBestResolution(fps);
  SetViewMode(CMediaSettings::Get().GetCurrentVideoSettings().m_ViewMode);
  ManageDisplay();

  m_bConfigured = true;
  m_bImageReady = false;
  m_scalingMethodGui = (ESCALINGMETHOD)-1;

  // Ensure that textures are recreated and rendering starts only after the 1st
  // frame is loaded after every call to Configure().
  m_bValidated = false;

  for (int i = 0 ; i<m_NumYV12Buffers ; i++)
    m_buffers[i].image.flags = 0;

  m_iLastRenderBuffer = -1;

  m_RenderUpdateCallBackFn = NULL;
  m_RenderUpdateCallBackCtx = NULL;
  if ((m_format == RENDER_FMT_BYPASS) && g_application.GetCurrentPlayer())
  {
    m_renderFeatures.clear();
    m_scalingMethods.clear();
    m_deinterlaceModes.clear();
    m_deinterlaceMethods.clear();

    if (m_RenderFeaturesCallBackFn)
    {
      (*m_RenderFeaturesCallBackFn)(m_RenderFeaturesCallBackCtx, m_renderFeatures);
      // after setting up m_renderFeatures, we are done with the callback
      m_RenderFeaturesCallBackFn = NULL;
      m_RenderFeaturesCallBackCtx = NULL;
    }
    g_application.m_pPlayer->GetRenderFeatures(m_renderFeatures);
    g_application.m_pPlayer->GetDeinterlaceMethods(m_deinterlaceMethods);
    g_application.m_pPlayer->GetDeinterlaceModes(m_deinterlaceModes);
    g_application.m_pPlayer->GetScalingMethods(m_scalingMethods);
  }

  return true;
}

int CLinuxRendererGLES::NextYV12Texture()
{
  return (m_iYV12RenderBuffer + 1) % m_NumYV12Buffers;
}

int CLinuxRendererGLES::GetImage(YV12Image *image, int source, bool readonly)
{
  if (!image) return -1;
  if (!m_bValidated) return -1;

  /* take next available buffer */
  if( source == AUTOSOURCE )
   source = NextYV12Texture();

  if ( m_renderMethod & RENDER_OMXEGL )
  {
    return source;
  }
#ifdef HAS_LIBSTAGEFRIGHT
  if ( m_renderMethod & RENDER_EGLIMG )
  {
    return source;
  }

#endif
  if ( m_renderMethod & RENDER_MEDIACODEC )
  {
    return source;
  }
  if ( m_renderMethod & RENDER_IMXMAP )
  {
    return source;
  }

#ifdef HAVE_VIDEOTOOLBOXDECODER
  if (m_renderMethod & RENDER_CVREF )
  {
    return source;
  }
#endif

  YV12Image &im = m_buffers[source].image;

  if ((im.flags&(~IMAGE_FLAG_READY)) != 0)
  {
     CLog::Log(LOGDEBUG, "CLinuxRenderer::GetImage - request image but none to give");
     return -1;
  }

  if( readonly )
    im.flags |= IMAGE_FLAG_READING;
  else
    im.flags |= IMAGE_FLAG_WRITING;

  // copy the image - should be operator of YV12Image
  for (int p=0;p<MAX_PLANES;p++)
  {
    image->plane[p]  = im.plane[p];
    image->stride[p] = im.stride[p];
  }
  image->width    = im.width;
  image->height   = im.height;
  image->flags    = im.flags;
  image->cshift_x = im.cshift_x;
  image->cshift_y = im.cshift_y;
  image->bpp      = 1;

  return source;
}

void CLinuxRendererGLES::ReleaseImage(int source, bool preserve)
{
  YV12Image &im = m_buffers[source].image;

  im.flags &= ~IMAGE_FLAG_INUSE;
  im.flags |= IMAGE_FLAG_READY;
  /* if image should be preserved reserve it so it's not auto seleceted */

  if( preserve )
    im.flags |= IMAGE_FLAG_RESERVED;

  m_bImageReady = true;
}

void CLinuxRendererGLES::CalculateTextureSourceRects(int source, int num_planes)
{
  YUVBUFFER& buf    =  m_buffers[source];
  YV12Image* im     = &buf.image;
  YUVFIELDS& fields =  buf.fields;

  // calculate the source rectangle
  for(int field = 0; field < 3; field++)
  {
    for(int plane = 0; plane < num_planes; plane++)
    {
      YUVPLANE& p = fields[field][plane];

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

void CLinuxRendererGLES::LoadPlane( YUVPLANE& plane, int type, unsigned flipindex
                                , unsigned width, unsigned height
                                , unsigned int stride, int bpp, void* data )
{
  if(plane.flipindex == flipindex)
    return;

  const GLvoid *pixelData = data;

  int bps = bpp * glFormatElementByteCount(type);

  unsigned datatype;
  if(bpp == 2)
    datatype = GL_UNSIGNED_SHORT;
  else
    datatype = GL_UNSIGNED_BYTE;

  glBindTexture(m_textureTarget, plane.id);

  // OpenGL ES does not support strided texture input.
  if(stride != width * bps)
  {
    unsigned char* src = (unsigned char*)data;
    for (unsigned int y = 0; y < height;++y, src += stride)
      glTexSubImage2D(m_textureTarget, 0, 0, y, width, 1, type, datatype, src);
  } else {
    glTexSubImage2D(m_textureTarget, 0, 0, 0, width, height, type, datatype, pixelData);
  }

  /* check if we need to load any border pixels */
  if(height < plane.texheight)
    glTexSubImage2D( m_textureTarget, 0
                   , 0, height, width, 1
                   , type, datatype
                   , (unsigned char*)pixelData + stride * (height-1));

  if(width  < plane.texwidth)
    glTexSubImage2D( m_textureTarget, 0
                   , width, 0, 1, height
                   , type, datatype
                   , (unsigned char*)pixelData + bps * (width-1));

  glBindTexture(m_textureTarget, 0);

  plane.flipindex = flipindex;
}

void CLinuxRendererGLES::Reset()
{
  for(int i=0; i<m_NumYV12Buffers; i++)
  {
    /* reset all image flags, this will cleanup textures later */
    m_buffers[i].image.flags = 0;
  }
}

void CLinuxRendererGLES::Flush()
{
  if (!m_bValidated)
    return;

  glFinish();

  for (int i = 0 ; i < m_NumYV12Buffers ; i++)
    (this->*m_textureDelete)(i);

  glFinish();
  m_bValidated = false;
  m_fbo.Cleanup();
  m_iYV12RenderBuffer = 0;
}

void CLinuxRendererGLES::Update()
{
  if (!m_bConfigured) return;
  ManageDisplay();
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

  // this needs to be checked after texture validation
  if (!m_bImageReady) return;

  int index = m_iYV12RenderBuffer;
  YUVBUFFER& buf =  m_buffers[index];

  if (m_format != RENDER_FMT_OMXEGL && m_format != RENDER_FMT_EGLIMG && m_format != RENDER_FMT_MEDIACODEC)
  {
    if (!buf.fields[FIELD_FULL][0].id) return;
  }
  if (buf.image.flags==0)
    return;

  ManageDisplay();

  g_graphicsContext.BeginPaint();

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

  g_graphicsContext.EndPaint();
}

void CLinuxRendererGLES::RenderUpdateVideo(bool clear, DWORD flags, DWORD alpha)
{
  if (!m_bConfigured)
    return;

  if (IsGuiLayer())
    return;

  if (m_renderMethod & RENDER_BYPASS)
  {
    ManageDisplay();
    // if running bypass, then the player might need the src/dst rects
    // for sizing video playback on a layer other than the gles layer.
    if (m_RenderUpdateCallBackFn)
      (*m_RenderUpdateCallBackFn)(m_RenderUpdateCallBackCtx, m_sourceRect, m_destRect);

    return;
  }
#ifdef HAS_IMXVPU
  else if (m_renderMethod & RENDER_IMXMAP)
  {
#if 0
    static unsigned long long previous = 0;
    unsigned long long current = XbmcThreads::SystemClockMillis();
    printf("r->r: %d\n", (int)(current-previous));
    previous = current;
#endif
    CDVDVideoCodecIMXBuffer *buffer = m_buffers[m_iYV12RenderBuffer].IMXBuffer;
    if (buffer != NULL && buffer->IsValid())
    {
      // this hack is needed to get the 2D mode of a 3D movie going
      RENDER_STEREO_MODE stereo_mode = g_graphicsContext.GetStereoMode();
      if (stereo_mode)
        g_graphicsContext.SetStereoView(RENDER_STEREO_VIEW_LEFT);

      ManageDisplay();

      if (stereo_mode)
        g_graphicsContext.SetStereoView(RENDER_STEREO_VIEW_OFF);

      CRect dstRect(m_destRect);
      CRect srcRect(m_sourceRect);
      switch (stereo_mode)
      {
        case RENDER_STEREO_MODE_SPLIT_HORIZONTAL:
          dstRect.y2 *= 2.0;
          srcRect.y2 *= 2.0;
        break;

        case RENDER_STEREO_MODE_SPLIT_VERTICAL:
          dstRect.x2 *= 2.0;
          srcRect.x2 *= 2.0;
        break;

        default:
        break;
      }

      //CLog::Log(LOGDEBUG, "BLIT RECTS: source x1 %f x2 %f y1 %f y2 %f dest x1 %f x2 %f y1 %f y2 %f", srcRect.x1, srcRect.x2, srcRect.y1, srcRect.y2, dstRect.x1, dstRect.x2, dstRect.y1, dstRect.y2);
      g_IMXContext.SetBlitRects(srcRect, dstRect);

      bool topFieldFirst = true;

      // Deinterlacing requested
      if (flags & RENDER_FLAG_FIELDMASK)
      {
        if ((buffer->GetFieldType() == VPU_FIELD_BOTTOM)
        ||  (buffer->GetFieldType() == VPU_FIELD_BT) )
          topFieldFirst = false;

        if (flags & RENDER_FLAG_FIELD0)
        {
          // Double rate first frame
          g_IMXContext.SetDeInterlacing(true);
          g_IMXContext.SetDoubleRate(true);
          g_IMXContext.SetInterpolatedFrame(true);
        }
        else if (flags & RENDER_FLAG_FIELD1)
        {
          // Double rate second frame
          g_IMXContext.SetDeInterlacing(true);
          g_IMXContext.SetDoubleRate(true);
          g_IMXContext.SetInterpolatedFrame(false);
        }
        else
        {
          // Fast motion
          g_IMXContext.SetDeInterlacing(true);
          g_IMXContext.SetDoubleRate(false);
        }
      }
      // Progressive
      else
        g_IMXContext.SetDeInterlacing(false);

      g_IMXContext.BlitAsync(NULL, buffer, topFieldFirst);
    }

#if 0
    unsigned long long current2 = XbmcThreads::SystemClockMillis();
    printf("r: %d  %d\n", m_iYV12RenderBuffer, (int)(current2-current));
#endif
  }
#endif
}

void CLinuxRendererGLES::FlipPage(int source)
{
  if( source >= 0 && source < m_NumYV12Buffers )
    m_iYV12RenderBuffer = source;
  else
    m_iYV12RenderBuffer = NextYV12Texture();

  m_buffers[m_iYV12RenderBuffer].flipindex = ++m_flipindex;

  return;
}

unsigned int CLinuxRendererGLES::PreInit()
{
  CSingleLock lock(g_graphicsContext);
  m_bConfigured = false;
  m_bValidated = false;
  UnInit();
  m_resolution = CDisplaySettings::Get().GetCurrentResolution();
  if ( m_resolution == RES_WINDOW )
    m_resolution = RES_DESKTOP;

  m_iYV12RenderBuffer = 0;
  m_NumYV12Buffers = 2;

  m_formats.clear();
  m_formats.push_back(RENDER_FMT_YUV420P);
  m_formats.push_back(RENDER_FMT_NV12);
  m_formats.push_back(RENDER_FMT_BYPASS);
#if defined(HAVE_LIBOPENMAX)
  m_formats.push_back(RENDER_FMT_OMXEGL);
#endif
#ifdef HAVE_VIDEOTOOLBOXDECODER
  m_formats.push_back(RENDER_FMT_CVBREF);
#endif
#ifdef HAS_LIBSTAGEFRIGHT
  m_formats.push_back(RENDER_FMT_EGLIMG);
#endif
#if defined(TARGET_ANDROID)
  m_formats.push_back(RENDER_FMT_MEDIACODEC);
#endif
#ifdef HAS_IMXVPU
  m_formats.push_back(RENDER_FMT_IMXMAP);
#endif

  // setup the background colour
  m_clearColour = (float)(g_advancedSettings.m_videoBlackBarColour & 0xff) / 0xff;

  return true;
}

void CLinuxRendererGLES::UpdateVideoFilter()
{
  if (m_scalingMethodGui == CMediaSettings::Get().GetCurrentVideoSettings().m_ScalingMethod)
    return;
  m_scalingMethodGui = CMediaSettings::Get().GetCurrentVideoSettings().m_ScalingMethod;
  m_scalingMethod    = m_scalingMethodGui;

  if(!Supports(m_scalingMethod))
  {
    CLog::Log(LOGWARNING, "CLinuxRendererGLES::UpdateVideoFilter - choosen scaling method %d, is not supported by renderer", (int)m_scalingMethod);
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
#ifdef TARGET_DARWIN_IOS
  float ios_version = CDarwinUtils::GetIOSVersion();
#endif
  int requestedMethod = CSettings::Get().GetInt("videoplayer.rendermethod");
  CLog::Log(LOGDEBUG, "GL: Requested render method: %d", requestedMethod);

  ReleaseShaders();

  switch(requestedMethod)
  {
    case RENDER_METHOD_AUTO:
    case RENDER_METHOD_GLSL:
      if (m_format == RENDER_FMT_OMXEGL)
      {
        CLog::Log(LOGNOTICE, "GL: Using OMXEGL RGBA render method");
        m_renderMethod = RENDER_OMXEGL;
        break;
      }
      else if (m_format == RENDER_FMT_EGLIMG)
      {
        CLog::Log(LOGNOTICE, "GL: Using EGL Image render method");
        m_renderMethod = RENDER_EGLIMG;
        break;
      }
      else if (m_format == RENDER_FMT_MEDIACODEC)
      {
        CLog::Log(LOGNOTICE, "GL: Using MediaCodec render method");
        m_renderMethod = RENDER_MEDIACODEC;
        break;
      }
      else if (m_format == RENDER_FMT_IMXMAP)
      {
        CLog::Log(LOGNOTICE, "GL: Using IMXMAP render method");
        m_renderMethod = RENDER_IMXMAP;
        break;
      }
      else if (m_format == RENDER_FMT_BYPASS)
      {
        CLog::Log(LOGNOTICE, "GL: Using BYPASS render method");
        m_renderMethod = RENDER_BYPASS;
        break;
      }
      else if (m_format == RENDER_FMT_CVBREF)
      {
        CLog::Log(LOGNOTICE, "GL: Using CoreVideoRef RGBA render method");
        m_renderMethod = RENDER_CVREF;
        break;
      }
      #if defined(TARGET_DARWIN_IOS)
      else if (ios_version < 5.0 && m_format == RENDER_FMT_YUV420P)
      {
        CLog::Log(LOGNOTICE, "GL: Using software color conversion/RGBA render method");
        m_renderMethod = RENDER_SW;
        break;
      }
      #endif
      // Try GLSL shaders if supported and user requested auto or GLSL.
      if (glCreateProgram)
      {
        // create regular scan shader
        CLog::Log(LOGNOTICE, "GL: Selecting Single Pass YUV 2 RGB shader");

        m_pYUVProgShader = new YUV2RGBProgressiveShader(false, m_iFlags, m_format);
        m_pYUVBobShader = new YUV2RGBBobShader(false, m_iFlags, m_format);
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
          // drop through and try SW
        }
      }
    case RENDER_METHOD_SOFTWARE:
    default:
      {
        // Use software YUV 2 RGB conversion if user requested it or GLSL failed
        m_renderMethod = RENDER_SW ;
        CLog::Log(LOGNOTICE, "GL: Using software color conversion/RGBA rendering");
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

  // Now that we now the render method, setup texture function handlers
  if (m_format == RENDER_FMT_CVBREF)
  {
    m_textureUpload = &CLinuxRendererGLES::UploadCVRefTexture;
    m_textureCreate = &CLinuxRendererGLES::CreateCVRefTexture;
    m_textureDelete = &CLinuxRendererGLES::DeleteCVRefTexture;
  }
  else if (m_format == RENDER_FMT_BYPASS)
  {
    m_textureUpload = &CLinuxRendererGLES::UploadBYPASSTexture;
    m_textureCreate = &CLinuxRendererGLES::CreateBYPASSTexture;
    m_textureDelete = &CLinuxRendererGLES::DeleteBYPASSTexture;
  }
  else if (m_format == RENDER_FMT_EGLIMG)
  {
    m_textureUpload = &CLinuxRendererGLES::UploadEGLIMGTexture;
    m_textureCreate = &CLinuxRendererGLES::CreateEGLIMGTexture;
    m_textureDelete = &CLinuxRendererGLES::DeleteEGLIMGTexture;
  }
  else if (m_format == RENDER_FMT_MEDIACODEC)
  {
    m_textureUpload = &CLinuxRendererGLES::UploadSurfaceTexture;
    m_textureCreate = &CLinuxRendererGLES::CreateSurfaceTexture;
    m_textureDelete = &CLinuxRendererGLES::DeleteSurfaceTexture;
  }
  else if (m_format == RENDER_FMT_NV12)
  {
    m_textureUpload = &CLinuxRendererGLES::UploadNV12Texture;
    m_textureCreate = &CLinuxRendererGLES::CreateNV12Texture;
    m_textureDelete = &CLinuxRendererGLES::DeleteNV12Texture;
  }
  else if (m_format == RENDER_FMT_IMXMAP)
  {
    m_textureUpload = &CLinuxRendererGLES::UploadIMXMAPTexture;
    m_textureCreate = &CLinuxRendererGLES::CreateIMXMAPTexture;
    m_textureDelete = &CLinuxRendererGLES::DeleteIMXMAPTexture;
  }
  else if (m_format == RENDER_FMT_OMXEGL)
  {
    m_textureUpload = &CLinuxRendererGLES::UploadOpenMaxTexture;
    m_textureCreate = &CLinuxRendererGLES::CreateOpenMaxTexture;
    m_textureDelete = &CLinuxRendererGLES::DeleteOpenMaxTexture;
  }
   else
  {
    // default to YV12 texture handlers
    m_textureUpload = &CLinuxRendererGLES::UploadYV12Texture;
    m_textureCreate = &CLinuxRendererGLES::CreateYV12Texture;
    m_textureDelete = &CLinuxRendererGLES::DeleteYV12Texture;
  }

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

  if (m_rgbBuffer != NULL)
  {
    delete [] m_rgbBuffer;
    m_rgbBuffer = NULL;
  }
  m_rgbBufferSize = 0;

  // YV12 textures
  for (int i = 0; i < NUM_BUFFERS; ++i)
    (this->*m_textureDelete)(i);

  if (m_sw_context)
  {
    sws_freeContext(m_sw_context);
    m_sw_context = NULL;
  }

  // cleanup framebuffer object if it was in use
  m_fbo.Cleanup();
  m_bValidated = false;
  m_bImageReady = false;
  m_bConfigured = false;
  m_RenderUpdateCallBackFn = NULL;
  m_RenderUpdateCallBackCtx = NULL;
  m_RenderFeaturesCallBackFn = NULL;
  m_RenderFeaturesCallBackCtx = NULL;
}

inline void CLinuxRendererGLES::ReorderDrawPoints()
{

  CBaseRenderer::ReorderDrawPoints();//call base impl. for rotating the points

  //corevideo and EGL are flipped in y
  if(m_renderMethod & RENDER_CVREF)
  {
    CPoint tmp;
    tmp = m_rotatedDestCoords[0];
    m_rotatedDestCoords[0] = m_rotatedDestCoords[3];
    m_rotatedDestCoords[3] = tmp;
    tmp = m_rotatedDestCoords[1];
    m_rotatedDestCoords[1] = m_rotatedDestCoords[2];
    m_rotatedDestCoords[2] = tmp;
  }
}

void CLinuxRendererGLES::ReleaseBuffer(int idx)
{
  YUVBUFFER &buf = m_buffers[idx];
#ifdef HAVE_VIDEOTOOLBOXDECODER
  if (m_renderMethod & RENDER_CVREF )
  {
    if (buf.cvBufferRef)
      CVBufferRelease(buf.cvBufferRef);
    buf.cvBufferRef = NULL;
  }
#endif
#if defined(TARGET_ANDROID)
  if ( m_renderMethod & RENDER_MEDIACODEC )
  {
    if (buf.mediacodec)
    {
      // The media buffer has been queued to the SurfaceView but we didn't render it
      // We have to do to the updateTexImage or it will get stuck
      buf.mediacodec->UpdateTexImage();
      SAFE_RELEASE(buf.mediacodec);
    }
  }
#endif
#ifdef HAS_IMXVPU
  if (m_renderMethod & RENDER_IMXMAP)
    SAFE_RELEASE(buf.IMXBuffer);
#endif
}

void CLinuxRendererGLES::Render(DWORD flags, int index)
{
  // If rendered directly by the hardware
  if (m_renderMethod & RENDER_BYPASS)
    return;

  // obtain current field, if interlaced
  if( flags & RENDER_FLAG_TOP)
    m_currentField = FIELD_TOP;

  else if (flags & RENDER_FLAG_BOT)
    m_currentField = FIELD_BOT;

  else
    m_currentField = FIELD_FULL;

  (this->*m_textureUpload)(index);

  if (m_renderMethod & RENDER_GLSL)
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

    case RQ_SOFTWARE:
      RenderSoftware(index, m_currentField);
      VerifyGLState();
      break;
    }
  }
  else if (m_renderMethod & RENDER_OMXEGL)
  {
    RenderOpenMax(index, m_currentField);
    VerifyGLState();
  }
  else if (m_renderMethod & RENDER_EGLIMG)
  {
    RenderEglImage(index, m_currentField);
    VerifyGLState();
  }
  else if (m_renderMethod & RENDER_CVREF)
  {
    RenderCoreVideoRef(index, m_currentField);
    VerifyGLState();
  }
  else if (m_renderMethod & RENDER_MEDIACODEC)
  {
    RenderSurfaceTexture(index, m_currentField);
  }
  else if (m_renderMethod & RENDER_IMXMAP)
  {
    RenderIMXMAPTexture(index, m_currentField);
    VerifyGLState();
  }
  else
  {
    RenderSoftware(index, m_currentField);
    VerifyGLState();
  }
}

void CLinuxRendererGLES::RenderSinglePass(int index, int field)
{
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;
  YUVPLANES &planes = fields[FIELD_FULL];
  YUVPLANES &planesf = fields[field];

  if (m_reloadShaders)
  {
    m_reloadShaders = 0;
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

  pYUVShader->SetBlack(CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness * 0.01f - 0.5f);
  pYUVShader->SetContrast(CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast * 0.02f);
  pYUVShader->SetWidth(im.width);
  pYUVShader->SetHeight(im.height);
  if     (field == FIELD_TOP)
    pYUVShader->SetField(1);
  else if(field == FIELD_BOT)
    pYUVShader->SetField(0);

  pYUVShader->SetMatrices(glMatrixProject.Get(), glMatrixModview.Get());
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
    m_tex[i][0][0] = m_tex[i][3][0] = planesf[i].rect.x1;
    m_tex[i][0][1] = m_tex[i][1][1] = planesf[i].rect.y1;
    m_tex[i][1][0] = m_tex[i][2][0] = planesf[i].rect.x2;
    m_tex[i][2][1] = m_tex[i][3][1] = planesf[i].rect.y2;
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
  // TODO: Multipass rendering does not currently work! FIX!
  CLog::Log(LOGERROR, "GLES: MULTIPASS rendering was called! But it doesnt work!!!");
  return;

  YV12Image &im     = m_buffers[index].image;
  YUVPLANES &planes = m_buffers[index].fields[field];

  if (m_reloadShaders)
  {
    m_reloadShaders = 0;
    LoadShaders(m_currentField);
  }

  glDisable(GL_DEPTH_TEST);

  // Y
  glEnable(m_textureTarget);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(m_textureTarget, planes[0].id);
  VerifyGLState();

  // U
  glActiveTexture(GL_TEXTURE1);
  glEnable(m_textureTarget);
  glBindTexture(m_textureTarget, planes[1].id);
  VerifyGLState();

  // V
  glActiveTexture(GL_TEXTURE2);
  glEnable(m_textureTarget);
  glBindTexture(m_textureTarget, planes[2].id);
  VerifyGLState();

  glActiveTexture(GL_TEXTURE0);
  VerifyGLState();

  // make sure the yuv shader is loaded and ready to go
  if (!m_pYUVProgShader || (!m_pYUVProgShader->OK()))
  {
    CLog::Log(LOGERROR, "GL: YUV shader not active, cannot do multipass render");
    return;
  }

  m_fbo.BeginRender();
  VerifyGLState();

  m_pYUVProgShader->SetBlack(CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness * 0.01f - 0.5f);
  m_pYUVProgShader->SetContrast(CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast * 0.02f);
  m_pYUVProgShader->SetWidth(im.width);
  m_pYUVProgShader->SetHeight(im.height);
  if     (field == FIELD_TOP)
    m_pYUVProgShader->SetField(1);
  else if(field == FIELD_BOT)
    m_pYUVProgShader->SetField(0);

  VerifyGLState();
//TODO
//  glPushAttrib(GL_VIEWPORT_BIT);
//  glPushAttrib(GL_SCISSOR_BIT);
  glMatrixModview.Push();
  glMatrixModview->LoadIdentity();

  glMatrixProject.Push();
  glMatrixProject->LoadIdentity();
  glMatrixProject->Ortho2D(0, m_sourceWidth, 0, m_sourceHeight);

  CRect viewport(0, 0, m_sourceWidth, m_sourceHeight);
  g_Windowing.SetViewPort(viewport);
  VerifyGLState();


  if (!m_pYUVProgShader->Enable())
  {
    CLog::Log(LOGERROR, "GL: Error enabling YUV shader");
  }

// 1st Pass to video frame size
//TODO
//  float imgwidth  = planes[0].rect.x2 - planes[0].rect.x1;
//  float imgheight = planes[0].rect.y2 - planes[0].rect.y1;
//  if (m_textureTarget == GL_TEXTURE_2D)
//  {
//    imgwidth  *= planes[0].pixpertex_x;
//    imgheight *= planes[0].pixpertex_y;
//  }
//
//  glBegin(GL_QUADS);
//
//  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x1, planes[0].rect.y1);
//  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x1, planes[1].rect.y1);
//  glMultiTexCoord2fARB(GL_TEXTURE2, planes[2].rect.x1, planes[2].rect.y1);
//  glVertex2f(0.0f    , 0.0f);
//
//  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x2, planes[0].rect.y1);
//  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x2, planes[1].rect.y1);
//  glMultiTexCoord2fARB(GL_TEXTURE2, planes[2].rect.x2, planes[2].rect.y1);
//  glVertex2f(imgwidth, 0.0f);
//
//  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x2, planes[0].rect.y2);
//  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x2, planes[1].rect.y2);
//  glMultiTexCoord2fARB(GL_TEXTURE2, planes[2].rect.x2, planes[2].rect.y2);
//  glVertex2f(imgwidth, imgheight);
//
//  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x1, planes[0].rect.y2);
//  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x1, planes[1].rect.y2);
//  glMultiTexCoord2fARB(GL_TEXTURE2, planes[2].rect.x1, planes[2].rect.y2);
//  glVertex2f(0.0f    , imgheight);
//
//  glEnd();
//  VerifyGLState();

  m_pYUVProgShader->Disable();

  glMatrixModview.PopLoad();
  glMatrixProject.PopLoad();

//TODO
//  glPopAttrib(); // pop scissor
//  glPopAttrib(); // pop viewport
  VerifyGLState();

  m_fbo.EndRender();

  glActiveTexture(GL_TEXTURE1);
  glDisable(m_textureTarget);
  glActiveTexture(GL_TEXTURE2);
  glDisable(m_textureTarget);
  glActiveTexture(GL_TEXTURE0);
  glDisable(m_textureTarget);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_fbo.Texture());
  VerifyGLState();

  // Use regular normalized texture coordinates

  // 2nd Pass to screen size with optional video filter

  if (m_pVideoFilterShader)
  {
    m_fbo.SetFiltering(GL_TEXTURE_2D, GL_NEAREST);
    m_pVideoFilterShader->SetSourceTexture(0);
    m_pVideoFilterShader->SetWidth(m_sourceWidth);
    m_pVideoFilterShader->SetHeight(m_sourceHeight);
    m_pVideoFilterShader->Enable();
  }
  else
    m_fbo.SetFiltering(GL_TEXTURE_2D, GL_LINEAR);

  VerifyGLState();

//TODO
//  imgwidth  /= m_sourceWidth;
//  imgheight /= m_sourceHeight;
//
//  glBegin(GL_QUADS);
//
//  glMultiTexCoord2fARB(GL_TEXTURE0, 0.0f    , 0.0f);
//  glVertex4f(m_destRect.x1, m_destRect.y1, 0, 1.0f );
//
//  glMultiTexCoord2fARB(GL_TEXTURE0, imgwidth, 0.0f);
//  glVertex4f(m_destRect.x2, m_destRect.y1, 0, 1.0f);
//
//  glMultiTexCoord2fARB(GL_TEXTURE0, imgwidth, imgheight);
//  glVertex4f(m_destRect.x2, m_destRect.y2, 0, 1.0f);
//
//  glMultiTexCoord2fARB(GL_TEXTURE0, 0.0f    , imgheight);
//  glVertex4f(m_destRect.x1, m_destRect.y2, 0, 1.0f);
//
//  glEnd();

  VerifyGLState();

  if (m_pVideoFilterShader)
    m_pVideoFilterShader->Disable();

  VerifyGLState();

  glDisable(m_textureTarget);
  VerifyGLState();
}

void CLinuxRendererGLES::RenderSoftware(int index, int field)
{
  YUVPLANES &planes = m_buffers[index].fields[field];

  glDisable(GL_DEPTH_TEST);

  // Y
  glEnable(m_textureTarget);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(m_textureTarget, planes[0].id);

  g_Windowing.EnableGUIShader(SM_TEXTURE_RGBA);

  GLint   contrastLoc = g_Windowing.GUIShaderGetContrast();
  glUniform1f(contrastLoc, CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast * 0.02f);
  GLint   brightnessLoc = g_Windowing.GUIShaderGetBrightness();
  glUniform1f(brightnessLoc, CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness * 0.01f - 0.5f);

  GLubyte idx[4] = {0, 1, 3, 2};        //determines order of triangle strip
  GLfloat ver[4][4];
  GLfloat tex[4][2];
  GLfloat col[3] = {1.0f, 1.0f, 1.0f};

  GLint   posLoc = g_Windowing.GUIShaderGetPos();
  GLint   texLoc = g_Windowing.GUIShaderGetCoord0();
  GLint   colLoc = g_Windowing.GUIShaderGetCol();

  glVertexAttribPointer(posLoc, 4, GL_FLOAT, 0, 0, ver);
  glVertexAttribPointer(texLoc, 2, GL_FLOAT, 0, 0, tex);
  glVertexAttribPointer(colLoc, 3, GL_FLOAT, 0, 0, col);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(texLoc);
  glEnableVertexAttribArray(colLoc);

  // Set vertex coordinates
  for(int i = 0; i < 4; i++)
  {
    ver[i][0] = m_rotatedDestCoords[i].x;
    ver[i][1] = m_rotatedDestCoords[i].y;
    ver[i][2] = 0.0f;// set z to 0
    ver[i][3] = 1.0f;
  }

  // Set texture coordinates
  tex[0][0] = tex[3][0] = planes[0].rect.x1;
  tex[0][1] = tex[1][1] = planes[0].rect.y1;
  tex[1][0] = tex[2][0] = planes[0].rect.x2;
  tex[2][1] = tex[3][1] = planes[0].rect.y2;

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(texLoc);
  glDisableVertexAttribArray(colLoc);

  g_Windowing.DisableGUIShader();

  VerifyGLState();

  glDisable(m_textureTarget);
  VerifyGLState();
}

void CLinuxRendererGLES::RenderOpenMax(int index, int field)
{
#if defined(HAVE_LIBOPENMAX)
  OpenMaxVideoBufferHolder *bufferHolder = m_buffers[index].openMaxBufferHolder;
  if (!bufferHolder)
    return;
  OpenMaxVideoBuffer *buffer = bufferHolder->m_openMaxVideoBuffer;

  GLuint textureId = buffer->texture_id;

  glDisable(GL_DEPTH_TEST);

  // Y
  glEnable(m_textureTarget);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(m_textureTarget, textureId);

  g_Windowing.EnableGUIShader(SM_TEXTURE_RGBA);

  GLint   contrastLoc = g_Windowing.GUIShaderGetContrast();
  glUniform1f(contrastLoc, CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast * 0.02f);
  GLint   brightnessLoc = g_Windowing.GUIShaderGetBrightness();
  glUniform1f(brightnessLoc, CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness * 0.01f - 0.5f);

  GLubyte idx[4] = {0, 1, 3, 2};        //determines order of triangle strip
  GLfloat ver[4][4];
  GLfloat tex[4][2];
  GLfloat col[3] = {1.0f, 1.0f, 1.0f};

  GLint   posLoc = g_Windowing.GUIShaderGetPos();
  GLint   texLoc = g_Windowing.GUIShaderGetCoord0();
  GLint   colLoc = g_Windowing.GUIShaderGetCol();

  glVertexAttribPointer(posLoc, 4, GL_FLOAT, 0, 0, ver);
  glVertexAttribPointer(texLoc, 2, GL_FLOAT, 0, 0, tex);
  glVertexAttribPointer(colLoc, 3, GL_FLOAT, 0, 0, col);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(texLoc);
  glEnableVertexAttribArray(colLoc);

  // Set vertex coordinates
  for(int i = 0; i < 4; i++)
  {
    ver[i][0] = m_rotatedDestCoords[i].x;
    ver[i][1] = m_rotatedDestCoords[i].y;
    ver[i][2] = 0.0f;// set z to 0
    ver[i][3] = 1.0f;
  }

  // Set texture coordinates
  tex[0][0] = tex[3][0] = 0;
  tex[0][1] = tex[1][1] = 1;
  tex[1][0] = tex[2][0] = 1;
  tex[2][1] = tex[3][1] = 0;

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(texLoc);
  glDisableVertexAttribArray(colLoc);

  g_Windowing.DisableGUIShader();

  VerifyGLState();

  glDisable(m_textureTarget);
  VerifyGLState();

#if defined(EGL_KHR_reusable_sync)
  if (buffer->eglSync) {
    eglDestroySyncKHR(buffer->eglDisplay, buffer->eglSync);
  }
  buffer->eglSync = eglCreateSyncKHR(buffer->eglDisplay, EGL_SYNC_FENCE_KHR, NULL);
  glFlush(); // flush to ensure the sync later will succeed
#endif

#endif
}

void CLinuxRendererGLES::RenderEglImage(int index, int field)
{
#if defined(HAS_LIBSTAGEFRIGHT)
#ifdef DEBUG_VERBOSE
  unsigned int time = XbmcThreads::SystemClockMillis();
#endif

  YUVPLANE &plane = m_buffers[index].fields[0][0];
  YUVPLANE &planef = m_buffers[index].fields[field][0];

  glDisable(GL_DEPTH_TEST);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(m_textureTarget, plane.id);

  if (field != FIELD_FULL)
  {
    g_Windowing.EnableGUIShader(SM_TEXTURE_RGBA_BOB);
    GLint   fieldLoc = g_Windowing.GUIShaderGetField();
    GLint   stepLoc = g_Windowing.GUIShaderGetStep();

    if     (field == FIELD_TOP)
      glUniform1i(fieldLoc, 1);
    else if(field == FIELD_BOT)
      glUniform1i(fieldLoc, 0);
    glUniform1f(stepLoc, 1.0f / (float)plane.texheight);
  }
  else
    g_Windowing.EnableGUIShader(SM_TEXTURE_RGBA);

  GLint   contrastLoc = g_Windowing.GUIShaderGetContrast();
  glUniform1f(contrastLoc, CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast * 0.02f);
  GLint   brightnessLoc = g_Windowing.GUIShaderGetBrightness();
  glUniform1f(brightnessLoc, CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness * 0.01f - 0.5f);

  GLubyte idx[4] = {0, 1, 3, 2};        //determines order of triangle strip
  GLfloat ver[4][4];
  GLfloat tex[4][2];
  GLfloat col[3] = {1.0f, 1.0f, 1.0f};

  GLint   posLoc = g_Windowing.GUIShaderGetPos();
  GLint   texLoc = g_Windowing.GUIShaderGetCoord0();
  GLint   colLoc = g_Windowing.GUIShaderGetCol();

  glVertexAttribPointer(posLoc, 4, GL_FLOAT, 0, 0, ver);
  glVertexAttribPointer(texLoc, 2, GL_FLOAT, 0, 0, tex);
  glVertexAttribPointer(colLoc, 3, GL_FLOAT, 0, 0, col);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(texLoc);
  glEnableVertexAttribArray(colLoc);

  // Set vertex coordinates
  for(int i = 0; i < 4; i++)
  {
    ver[i][0] = m_rotatedDestCoords[i].x;
    ver[i][1] = m_rotatedDestCoords[i].y;
    ver[i][2] = 0.0f;// set z to 0
    ver[i][3] = 1.0f;
  }

  if (field == FIELD_FULL)
  {
    tex[0][0] = tex[3][0] = plane.rect.x1;
    tex[0][1] = tex[1][1] = plane.rect.y1;
    tex[1][0] = tex[2][0] = plane.rect.x2;
    tex[2][1] = tex[3][1] = plane.rect.y2;
  }
  else
  {
    tex[0][0] = tex[3][0] = planef.rect.x1;
    tex[0][1] = tex[1][1] = planef.rect.y1 * 2.0f;
    tex[1][0] = tex[2][0] = planef.rect.x2;
    tex[2][1] = tex[3][1] = planef.rect.y2 * 2.0f;
  }

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(texLoc);
  glDisableVertexAttribArray(colLoc);

  g_Windowing.DisableGUIShader();
  VerifyGLState();

  glBindTexture(m_textureTarget, 0);
  VerifyGLState();

#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, "RenderEglImage %d: tm:%d\n", index, XbmcThreads::SystemClockMillis() - time);
#endif
#endif
}

void CLinuxRendererGLES::RenderSurfaceTexture(int index, int field)
{
#if defined(TARGET_ANDROID)
  #ifdef DEBUG_VERBOSE
    unsigned int time = XbmcThreads::SystemClockMillis();
  #endif

  YUVPLANE &plane = m_buffers[index].fields[0][0];
  YUVPLANE &planef = m_buffers[index].fields[field][0];

  glDisable(GL_DEPTH_TEST);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, plane.id);

  if (field != FIELD_FULL)
  {
    g_Windowing.EnableGUIShader(SM_TEXTURE_RGBA_BOB_OES);
    GLint   fieldLoc = g_Windowing.GUIShaderGetField();
    GLint   stepLoc = g_Windowing.GUIShaderGetStep();

    // Y is inverted, so invert fields
    if     (field == FIELD_TOP)
      glUniform1i(fieldLoc, 0);
    else if(field == FIELD_BOT)
      glUniform1i(fieldLoc, 1);
    glUniform1f(stepLoc, 1.0f / (float)plane.texheight);
  }
  else
    g_Windowing.EnableGUIShader(SM_TEXTURE_RGBA_OES);

  GLint   contrastLoc = g_Windowing.GUIShaderGetContrast();
  glUniform1f(contrastLoc, CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast * 0.02f);
  GLint   brightnessLoc = g_Windowing.GUIShaderGetBrightness();
  glUniform1f(brightnessLoc, CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness * 0.01f - 0.5f);

  glUniformMatrix4fv(g_Windowing.GUIShaderGetCoord0Matrix(), 1, GL_FALSE, m_textureMatrix);

  GLubyte idx[4] = {0, 1, 3, 2};        //determines order of triangle strip
  GLfloat ver[4][4];
  GLfloat tex[4][4];

  GLint   posLoc = g_Windowing.GUIShaderGetPos();
  GLint   texLoc = g_Windowing.GUIShaderGetCoord0();


  glVertexAttribPointer(posLoc, 4, GL_FLOAT, 0, 0, ver);
  glVertexAttribPointer(texLoc, 4, GL_FLOAT, 0, 0, tex);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(texLoc);

  // Set vertex coordinates
  for(int i = 0; i < 4; i++)
  {
    ver[i][0] = m_rotatedDestCoords[i].x;
    ver[i][1] = m_rotatedDestCoords[i].y;
    ver[i][2] = 0.0f;        // set z to 0
    ver[i][3] = 1.0f;
  }

  // Set texture coordinates (MediaCodec is flipped in y)
  if (field == FIELD_FULL)
  {
    tex[0][0] = tex[3][0] = plane.rect.x1;
    tex[0][1] = tex[1][1] = plane.rect.y2;
    tex[1][0] = tex[2][0] = plane.rect.x2;
    tex[2][1] = tex[3][1] = plane.rect.y1;
  }
  else
  {
    tex[0][0] = tex[3][0] = planef.rect.x1;
    tex[0][1] = tex[1][1] = planef.rect.y2 * 2.0f;
    tex[1][0] = tex[2][0] = planef.rect.x2;
    tex[2][1] = tex[3][1] = planef.rect.y1 * 2.0f;
  }

  for(int i = 0; i < 4; i++)
  {
    tex[i][2] = 0.0f;
    tex[i][3] = 1.0f;
  }

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(texLoc);

  const float identity[16] = {
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f
  };
  glUniformMatrix4fv(g_Windowing.GUIShaderGetCoord0Matrix(),  1, GL_FALSE, identity);

  g_Windowing.DisableGUIShader();
  VerifyGLState();

  glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
  VerifyGLState();

  #ifdef DEBUG_VERBOSE
    CLog::Log(LOGDEBUG, "RenderMediaCodecImage %d: tm:%d", index, XbmcThreads::SystemClockMillis() - time);
  #endif
#endif
}

void CLinuxRendererGLES::RenderCoreVideoRef(int index, int field)
{
#ifdef HAVE_VIDEOTOOLBOXDECODER
  YUVPLANE &plane = m_buffers[index].fields[field][0];

  glDisable(GL_DEPTH_TEST);

  glEnable(m_textureTarget);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(m_textureTarget, plane.id);

  g_Windowing.EnableGUIShader(SM_TEXTURE_RGBA);

  GLint   contrastLoc = g_Windowing.GUIShaderGetContrast();
  glUniform1f(contrastLoc, CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast * 0.02f);
  GLint   brightnessLoc = g_Windowing.GUIShaderGetBrightness();
  glUniform1f(brightnessLoc, CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness * 0.01f - 0.5f);

  GLubyte idx[4] = {0, 1, 3, 2};        //determines order of triangle strip
  GLfloat ver[4][4];
  GLfloat tex[4][2];
  GLfloat col[3] = {1.0f, 1.0f, 1.0f};

  GLint   posLoc = g_Windowing.GUIShaderGetPos();
  GLint   texLoc = g_Windowing.GUIShaderGetCoord0();
  GLint   colLoc = g_Windowing.GUIShaderGetCol();

  glVertexAttribPointer(posLoc, 4, GL_FLOAT, 0, 0, ver);
  glVertexAttribPointer(texLoc, 2, GL_FLOAT, 0, 0, tex);
  glVertexAttribPointer(colLoc, 3, GL_FLOAT, 0, 0, col);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(texLoc);
  glEnableVertexAttribArray(colLoc);

  // Set vertex coordinates
  for(int i = 0; i < 4; i++)
  {
    ver[i][0] = m_rotatedDestCoords[i].x;
    ver[i][1] = m_rotatedDestCoords[i].y;
    ver[i][2] = 0.0f;// set z to 0
    ver[i][3] = 1.0f;
  }

  // Set texture coordinates (corevideo is flipped in y)
  tex[0][0] = tex[3][0] = plane.rect.x1;
  tex[0][1] = tex[1][1] = plane.rect.y2;
  tex[1][0] = tex[2][0] = plane.rect.x2;
  tex[2][1] = tex[3][1] = plane.rect.y1;

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(texLoc);
  glDisableVertexAttribArray(colLoc);

  g_Windowing.DisableGUIShader();
  VerifyGLState();

  glDisable(m_textureTarget);
  VerifyGLState();
#endif
}

void CLinuxRendererGLES::RenderIMXMAPTexture(int index, int field)
{
}

bool CLinuxRendererGLES::RenderCapture(CRenderCapture* capture)
{
  if (!m_bValidated)
    return false;

  // If rendered directly by the hardware
  if (m_renderMethod & RENDER_BYPASS)
  {
    capture->BeginRender();
    capture->EndRender();
    return true;
  }

#ifdef HAS_IMXVPU
  if (m_renderMethod & RENDER_IMXMAP)
  {
    CRect rect(0, 0, capture->GetWidth(), capture->GetHeight());
    CDVDVideoCodecIMXBuffer *buffer = m_buffers[m_iYV12RenderBuffer].IMXBuffer;
    capture->BeginRender();
    g_IMXContext.PushCaptureTask(buffer, &rect);
    capture->EndRender();
    return true;
  }
#endif

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
void CLinuxRendererGLES::UploadYV12Texture(int source)
{
  YUVBUFFER& buf    =  m_buffers[source];
  YV12Image* im     = &buf.image;
  YUVFIELDS& fields =  buf.fields;


#if defined(HAVE_LIBOPENMAX)
  if (!(im->flags&IMAGE_FLAG_READY) || m_buffers[source].openMaxBufferHolder)
#else
  if (!(im->flags&IMAGE_FLAG_READY))
#endif
  {
    return;
  }

  // if we don't have a shader, fallback to SW YUV2RGB for now
  if (m_renderMethod & RENDER_SW)
  {
    if(m_rgbBufferSize < m_sourceWidth * m_sourceHeight * 4)
    {
      delete [] m_rgbBuffer;
      m_rgbBufferSize = m_sourceWidth*m_sourceHeight*4;
      m_rgbBuffer = new BYTE[m_rgbBufferSize];
    }

#if defined(__ARM_NEON__)
    if (g_cpuInfo.GetCPUFeatures() & CPU_FEATURE_NEON)
    {
      yuv420_2_rgb8888_neon(m_rgbBuffer, im->plane[0], im->plane[2], im->plane[1],
        m_sourceWidth, m_sourceHeight, im->stride[0], im->stride[1], m_sourceWidth * 4);
    }
    else
#endif
    {
      m_sw_context = sws_getCachedContext(m_sw_context,
        im->width, im->height, PIX_FMT_YUV420P,
        im->width, im->height, PIX_FMT_RGBA,
        SWS_FAST_BILINEAR, NULL, NULL, NULL);

      uint8_t *src[]  = { im->plane[0], im->plane[1], im->plane[2], 0 };
      int srcStride[] = { int(im->stride[0]), int(im->stride[1]), int(im->stride[2]), 0 };
      uint8_t *dst[]  = { m_rgbBuffer, 0, 0, 0 };
      int dstStride[] = { int(m_sourceWidth*4), 0, 0, 0 };
      sws_scale(m_sw_context, src, srcStride, 0, im->height, dst, dstStride);
    }
  }

  bool deinterlacing = false;
  if (m_currentField == FIELD_FULL)
    deinterlacing = false;
  else
    deinterlacing = true;

  glEnable(m_textureTarget);
  VerifyGLState();

  if (m_renderMethod & RENDER_SW)
  {
    // Load RGB image
    if (deinterlacing)
    {
      LoadPlane( fields[FIELD_TOP][0] , GL_RGBA, buf.flipindex
               , im->width, im->height >> 1
               , m_sourceWidth*8, im->bpp, m_rgbBuffer );

      LoadPlane( fields[FIELD_BOT][0], GL_RGBA, buf.flipindex
               , im->width, im->height >> 1
               , m_sourceWidth*8, im->bpp, m_rgbBuffer + m_sourceWidth*4);
    }
    else
    {
      LoadPlane( fields[FIELD_FULL][0], GL_RGBA, buf.flipindex
               , im->width, im->height
               , m_sourceWidth*4, im->bpp, m_rgbBuffer );
    }
  }
  else
  {
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);

    // Load Y plane
    LoadPlane( fields[FIELD_FULL][0], GL_LUMINANCE, buf.flipindex
        , im->width, im->height
        , im->stride[0], im->bpp, im->plane[0] );

    //load U plane
    LoadPlane( fields[FIELD_FULL][1], GL_LUMINANCE, buf.flipindex
        , im->width >> im->cshift_x, im->height >> im->cshift_y
                                                   , im->stride[1], im->bpp, im->plane[1] );

    //load V plane
    LoadPlane( fields[FIELD_FULL][2], GL_ALPHA, buf.flipindex
        , im->width >> im->cshift_x, im->height >> im->cshift_y
                                                   , im->stride[2], im->bpp, im->plane[2] );
  }

  VerifyGLState();

  CalculateTextureSourceRects(source, 3);

  glDisable(m_textureTarget);
}

void CLinuxRendererGLES::DeleteYV12Texture(int index)
{
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;

  if( fields[FIELD_FULL][0].id == 0 ) return;

  /* finish up all textures, and delete them */
  g_graphicsContext.BeginPaint();  //FIXME
  for(int f = 0;f<MAX_FIELDS;f++)
  {
    for(int p = 0;p<MAX_PLANES;p++)
    {
      if( fields[f][p].id )
      {
        if (glIsTexture(fields[f][p].id))
          glDeleteTextures(1, &fields[f][p].id);
        fields[f][p].id = 0;
      }
    }
  }
  g_graphicsContext.EndPaint();

  for(int p = 0;p<MAX_PLANES;p++)
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
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;

  DeleteYV12Texture(index);

  im.height = m_sourceHeight;
  im.width  = m_sourceWidth;
  im.cshift_x = 1;
  im.cshift_y = 1;


  if(m_format == RENDER_FMT_YUV420P16
  || m_format == RENDER_FMT_YUV420P10)
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
    for(int p = 0;p<MAX_PLANES;p++)
    {
      if (!glIsTexture(fields[f][p].id))
      {
        glGenTextures(1, &fields[f][p].id);
        VerifyGLState();
      }
    }
  }

  // YUV
  for (int f = FIELD_FULL; f<=FIELD_BOT ; f++)
  {
    int fieldshift = (f==FIELD_FULL) ? 0 : 1;
    YUVPLANES &planes = fields[f];

    planes[0].texwidth  = im.width;
    planes[0].texheight = im.height >> fieldshift;

    if (m_renderMethod & RENDER_SW)
    {
      planes[1].texwidth  = 0;
      planes[1].texheight = 0;
      planes[2].texwidth  = 0;
      planes[2].texheight = 0;
    }
    else
    {
      planes[1].texwidth  = planes[0].texwidth  >> im.cshift_x;
      planes[1].texheight = planes[0].texheight >> im.cshift_y;
      planes[2].texwidth  = planes[0].texwidth  >> im.cshift_x;
      planes[2].texheight = planes[0].texheight >> im.cshift_y;
    }

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
      if (m_renderMethod & RENDER_SW)
      {
        if(m_renderMethod & RENDER_POT)
          CLog::Log(LOGDEBUG, "GL: Creating RGB POT texture of size %d x %d",  plane.texwidth, plane.texheight);
        else
          CLog::Log(LOGDEBUG,  "GL: Creating RGB NPOT texture of size %d x %d", plane.texwidth, plane.texheight);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexImage2D(m_textureTarget, 0, GL_RGBA, plane.texwidth, plane.texheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
      }
      else
      {
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
      }

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
void CLinuxRendererGLES::UploadNV12Texture(int source)
{
  YUVBUFFER& buf    =  m_buffers[source];
  YV12Image* im     = &buf.image;
  YUVFIELDS& fields =  buf.fields;

  if (!(im->flags & IMAGE_FLAG_READY))
    return;
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
    LoadPlane( fields[FIELD_TOP][0] , GL_LUMINANCE, buf.flipindex
             , im->width, im->height >> 1
             , im->stride[0]*2, im->bpp, im->plane[0] );

    // Load Even Y field
    LoadPlane( fields[FIELD_BOT][0], GL_LUMINANCE, buf.flipindex
             , im->width, im->height >> 1
             , im->stride[0]*2, im->bpp, im->plane[0] + im->stride[0]) ;

    // Load Odd UV Fields
    LoadPlane( fields[FIELD_TOP][1], GL_LUMINANCE_ALPHA, buf.flipindex
             , im->width >> im->cshift_x, im->height >> (im->cshift_y + 1)
             , im->stride[1]*2, im->bpp, im->plane[1] );

    // Load Even UV Fields
    LoadPlane( fields[FIELD_BOT][1], GL_LUMINANCE_ALPHA, buf.flipindex
             , im->width >> im->cshift_x, im->height >> (im->cshift_y + 1)
             , im->stride[1]*2, im->bpp, im->plane[1] + im->stride[1] );

  }
  else
  {
    // Load Y plane
    LoadPlane( fields[FIELD_FULL][0], GL_LUMINANCE, buf.flipindex
             , im->width, im->height
             , im->stride[0], im->bpp, im->plane[0] );

    // Load UV plane
    LoadPlane( fields[FIELD_FULL][1], GL_LUMINANCE_ALPHA, buf.flipindex
             , im->width >> im->cshift_x, im->height >> im->cshift_y
             , im->stride[1], im->bpp, im->plane[1] );
  }

  VerifyGLState();

  CalculateTextureSourceRects(source, 3);

  glDisable(m_textureTarget);
  return;
}

bool CLinuxRendererGLES::CreateNV12Texture(int index)
{
  // since we also want the field textures, pitch must be texture aligned
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;

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
      if (!glIsTexture(fields[f][p].id))
      {
        glGenTextures(1, &fields[f][p].id);
        VerifyGLState();
      }
    }
    fields[f][2].id = fields[f][1].id;
  }

  // YUV
  for (int f = FIELD_FULL; f<=FIELD_BOT ; f++)
  {
    int fieldshift = (f==FIELD_FULL) ? 0 : 1;
    YUVPLANES &planes = fields[f];

    planes[0].texwidth  = im.width;
    planes[0].texheight = im.height >> fieldshift;

    if (m_renderMethod & RENDER_SW)
    {
      planes[1].texwidth  = 0;
      planes[1].texheight = 0;
      planes[2].texwidth  = 0;
      planes[2].texheight = 0;
    }
    else
    {
      planes[1].texwidth  = planes[0].texwidth  >> im.cshift_x;
      planes[1].texheight = planes[0].texheight >> im.cshift_y;
      planes[2].texwidth  = planes[1].texwidth;
      planes[2].texheight = planes[1].texheight;
    }

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
      if (m_renderMethod & RENDER_SW)
      {
        glTexImage2D(m_textureTarget, 0, GL_RGBA, plane.texwidth, plane.texheight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
      }
      else
      {
        if (p == 1)
          glTexImage2D(m_textureTarget, 0, GL_LUMINANCE_ALPHA, plane.texwidth, plane.texheight, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, NULL);
        else
          glTexImage2D(m_textureTarget, 0, GL_LUMINANCE, plane.texwidth, plane.texheight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
      }

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
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;

  if( fields[FIELD_FULL][0].id == 0 ) return;

  // finish up all textures, and delete them
  g_graphicsContext.BeginPaint();  //FIXME
  for(int f = 0;f<MAX_FIELDS;f++)
  {
    for(int p = 0;p<2;p++)
    {
      if( fields[f][p].id )
      {
        if (glIsTexture(fields[f][p].id))
        {
          glDeleteTextures(1, &fields[f][p].id);
        }
        fields[f][p].id = 0;
      }
    }
    fields[f][2].id = 0;
  }
  g_graphicsContext.EndPaint();

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
// CoreVideoRef Texture creation, deletion, copying + clearing
//********************************************************************************************************
void CLinuxRendererGLES::UploadCVRefTexture(int index)
{
#ifdef HAVE_VIDEOTOOLBOXDECODER
  CVBufferRef cvBufferRef = m_buffers[index].cvBufferRef;

  if (cvBufferRef)
  {
    YUVPLANE &plane = m_buffers[index].fields[0][0];

    CVPixelBufferLockBaseAddress(cvBufferRef, kCVPixelBufferLock_ReadOnly);
    #if !TARGET_OS_IPHONE
      int rowbytes = CVPixelBufferGetBytesPerRow(cvBufferRef);
    #endif
    int bufferWidth = CVPixelBufferGetWidth(cvBufferRef);
    int bufferHeight = CVPixelBufferGetHeight(cvBufferRef);
    unsigned char *bufferBase = (unsigned char *)CVPixelBufferGetBaseAddress(cvBufferRef);

    glEnable(m_textureTarget);
    VerifyGLState();

    glBindTexture(m_textureTarget, plane.id);
    #if !TARGET_OS_IPHONE
      #ifdef GL_UNPACK_ROW_LENGTH
        // Set row pixels
        glPixelStorei( GL_UNPACK_ROW_LENGTH, rowbytes);
      #endif
      #ifdef GL_TEXTURE_STORAGE_HINT_APPLE
        // Set storage hint. Can also use GL_STORAGE_SHARED_APPLE see docs.
        glTexParameteri(m_textureTarget, GL_TEXTURE_STORAGE_HINT_APPLE , GL_STORAGE_CACHED_APPLE);
      #endif
    #endif

    // Using BGRA extension to pull in video frame data directly
    glTexSubImage2D(m_textureTarget, 0, 0, 0, bufferWidth, bufferHeight, GL_BGRA_EXT, GL_UNSIGNED_BYTE, bufferBase);

    #if !TARGET_OS_IPHONE
      #ifdef GL_UNPACK_ROW_LENGTH
        // Unset row pixels
        glPixelStorei( GL_UNPACK_ROW_LENGTH, 0);
      #endif
    #endif
    glBindTexture(m_textureTarget, 0);

    glDisable(m_textureTarget);
    VerifyGLState();

    CVPixelBufferUnlockBaseAddress(cvBufferRef, kCVPixelBufferLock_ReadOnly);
    CVBufferRelease(m_buffers[index].cvBufferRef);
    m_buffers[index].cvBufferRef = NULL;

    plane.flipindex = m_buffers[index].flipindex;
  }

  CalculateTextureSourceRects(index, 1);
#endif
}
void CLinuxRendererGLES::DeleteCVRefTexture(int index)
{
#ifdef HAVE_VIDEOTOOLBOXDECODER
  YUVPLANE &plane = m_buffers[index].fields[0][0];

  if (m_buffers[index].cvBufferRef)
    CVBufferRelease(m_buffers[index].cvBufferRef);
  m_buffers[index].cvBufferRef = NULL;

  if(plane.id && glIsTexture(plane.id))
    glDeleteTextures(1, &plane.id);
  plane.id = 0;
#endif
}
bool CLinuxRendererGLES::CreateCVRefTexture(int index)
{
#ifdef HAVE_VIDEOTOOLBOXDECODER
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;
  YUVPLANE  &plane  = fields[0][0];

  DeleteCVRefTexture(index);

  memset(&im    , 0, sizeof(im));
  memset(&fields, 0, sizeof(fields));

  im.height = m_sourceHeight;
  im.width  = m_sourceWidth;

  plane.texwidth  = im.width;
  plane.texheight = im.height;
  plane.pixpertex_x = 1;
  plane.pixpertex_y = 1;

  if(m_renderMethod & RENDER_POT)
  {
    plane.texwidth  = NP2(plane.texwidth);
    plane.texheight = NP2(plane.texheight);
  }
  glEnable(m_textureTarget);
  glGenTextures(1, &plane.id);
  VerifyGLState();

  glBindTexture(m_textureTarget, plane.id);
  #if !TARGET_OS_IPHONE
    #ifdef GL_UNPACK_ROW_LENGTH
      // Set row pixels
      glPixelStorei(GL_UNPACK_ROW_LENGTH, m_sourceWidth);
    #endif
    #ifdef GL_TEXTURE_STORAGE_HINT_APPLE
      // Set storage hint. Can also use GL_STORAGE_SHARED_APPLE see docs.
      glTexParameteri(m_textureTarget, GL_TEXTURE_STORAGE_HINT_APPLE , GL_STORAGE_CACHED_APPLE);
      // Set client storage
    #endif
    glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
  #endif

  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// This is necessary for non-power-of-two textures
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexImage2D(m_textureTarget, 0, GL_RGBA, plane.texwidth, plane.texheight, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);

  #if !TARGET_OS_IPHONE
    // turn off client storage so it doesn't get picked up for the next texture
    glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
  #endif
  glBindTexture(m_textureTarget, 0);
  glDisable(m_textureTarget);
#endif
  return true;
}

//********************************************************************************************************
// BYPASS creation, deletion, copying + clearing
//********************************************************************************************************
void CLinuxRendererGLES::UploadBYPASSTexture(int index)
{
}
void CLinuxRendererGLES::DeleteBYPASSTexture(int index)
{
}
bool CLinuxRendererGLES::CreateBYPASSTexture(int index)
{
  return true;
}

//********************************************************************************************************
// EGLIMG creation, deletion, copying + clearing
//********************************************************************************************************
void CLinuxRendererGLES::UploadEGLIMGTexture(int index)
{
#ifdef HAS_LIBSTAGEFRIGHT
#ifdef DEBUG_VERBOSE
  unsigned int time = XbmcThreads::SystemClockMillis();
#endif

  if(m_buffers[index].eglimg != EGL_NO_IMAGE_KHR)
  {
    YUVPLANE &plane = m_buffers[index].fields[0][0];

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(m_textureTarget, plane.id);
    glEGLImageTargetTexture2DOES(m_textureTarget, (EGLImageKHR)m_buffers[index].eglimg);
    glBindTexture(m_textureTarget, 0);

    plane.flipindex = m_buffers[index].flipindex;
  }

  CalculateTextureSourceRects(index, 1);

#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, "UploadEGLIMGTexture %d: img:%p, tm:%d\n", index, m_buffers[index].eglimg, XbmcThreads::SystemClockMillis() - time);
#endif
#endif
}
void CLinuxRendererGLES::DeleteEGLIMGTexture(int index)
{
#ifdef HAS_LIBSTAGEFRIGHT
  YUVBUFFER &buf = m_buffers[index];
  YUVPLANE &plane = buf.fields[0][0];

  if(plane.id && glIsTexture(plane.id))
    glDeleteTextures(1, &plane.id);
  plane.id = 0;

  buf.stf = NULL;
  buf.eglimg = EGL_NO_IMAGE_KHR;
#endif
}
bool CLinuxRendererGLES::CreateEGLIMGTexture(int index)
{
#ifdef HAS_LIBSTAGEFRIGHT
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;

  DeleteEGLIMGTexture(index);

  memset(&im    , 0, sizeof(im));
  memset(&fields, 0, sizeof(fields));

  im.height = m_sourceHeight;
  im.width  = m_sourceWidth;

  for (int f=0; f<3; ++f)
  {
    YUVPLANE  &plane  = fields[f][0];

    plane.texwidth  = im.width;
    plane.texheight = im.height;
    plane.pixpertex_x = 1;
    plane.pixpertex_y = 1;

    if(m_renderMethod & RENDER_POT)
    {
      plane.texwidth  = NP2(plane.texwidth);
      plane.texheight = NP2(plane.texheight);
    }
  }

  YUVPLANE  &plane  = fields[0][0];
  glEnable(m_textureTarget);
  glGenTextures(1, &plane.id);
  VerifyGLState();

  glBindTexture(m_textureTarget, plane.id);

  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// This is necessary for non-power-of-two textures
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glTexImage2D(m_textureTarget, 0, GL_RGBA, plane.texwidth, plane.texheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  glDisable(m_textureTarget);
#endif
  return true;
}

//********************************************************************************************************
// SurfaceTexture creation, deletion, copying + clearing
//********************************************************************************************************
void CLinuxRendererGLES::UploadSurfaceTexture(int index)
{
#if defined(TARGET_ANDROID)
#ifdef DEBUG_VERBOSE
  unsigned int time = XbmcThreads::SystemClockMillis();
  int mindex = -1;
#endif

  YUVBUFFER &buf = m_buffers[index];

  if (buf.mediacodec)
  {
#ifdef DEBUG_VERBOSE
    mindex = buf.mediacodec->GetIndex();
#endif
    buf.fields[0][0].id = buf.mediacodec->GetTextureID();
    buf.mediacodec->UpdateTexImage();
    buf.mediacodec->GetTransformMatrix(m_textureMatrix);
    SAFE_RELEASE(buf.mediacodec);
  }

  CalculateTextureSourceRects(index, 1);

#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, "UploadSurfaceTexture %d: img: %d tm:%d", index, mindex, XbmcThreads::SystemClockMillis() - time);
#endif
#endif
}
void CLinuxRendererGLES::DeleteSurfaceTexture(int index)
{
#if defined(TARGET_ANDROID)
  SAFE_RELEASE(m_buffers[index].mediacodec);
#endif
}
bool CLinuxRendererGLES::CreateSurfaceTexture(int index)
{
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;

  memset(&im    , 0, sizeof(im));
  memset(&fields, 0, sizeof(fields));

  im.height = m_sourceHeight;
  im.width  = m_sourceWidth;

  for (int f=0; f<3; ++f)
  {
    YUVPLANE  &plane  = fields[f][0];

    plane.texwidth  = im.width;
    plane.texheight = im.height;
    plane.pixpertex_x = 1;
    plane.pixpertex_y = 1;


    if(m_renderMethod & RENDER_POT)
    {
      plane.texwidth  = NP2(plane.texwidth);
      plane.texheight = NP2(plane.texheight);
    }
  }

  return true;
}

//********************************************************************************************************
// SurfaceTexture creation, deletion, copying + clearing
//********************************************************************************************************
void CLinuxRendererGLES::UploadOpenMaxTexture(int index)
{
}

void CLinuxRendererGLES::DeleteOpenMaxTexture(int index)
{
#ifdef HAVE_LIBOPENMAX
  if (m_buffers[index].openMaxBufferHolder) {
    m_buffers[index].openMaxBufferHolder->Release();
    m_buffers[index].openMaxBufferHolder = 0;
  }
#endif
}

bool CLinuxRendererGLES::CreateOpenMaxTexture(int index)
{
#ifdef HAVE_LIBOPENMAX
  m_buffers[index].openMaxBufferHolder = 0;
#endif
  return true;
}


void CLinuxRendererGLES::SetTextureFilter(GLenum method)
{
  for (int i = 0 ; i<m_NumYV12Buffers ; i++)
  {
    YUVFIELDS &fields = m_buffers[i].fields;

    for (int f = FIELD_FULL; f<=FIELD_BOT ; f++)
    {
      glBindTexture(m_textureTarget, fields[f][0].id);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, method);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, method);
      VerifyGLState();

      if (!(m_renderMethod & RENDER_SW))
      {
        glBindTexture(m_textureTarget, fields[f][1].id);
        glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, method);
        glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, method);
        VerifyGLState();

        glBindTexture(m_textureTarget, fields[f][2].id);
        glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, method);
        glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, method);
        VerifyGLState();
      }
    }
  }
}

//********************************************************************************************************
// IMXMAP creation, deletion, copying + clearing
//********************************************************************************************************
void CLinuxRendererGLES::UploadIMXMAPTexture(int index)
{
}

void CLinuxRendererGLES::DeleteIMXMAPTexture(int index)
{
#ifdef HAS_IMXVPU
  YUVBUFFER &buf = m_buffers[index];
  YUVPLANE &plane = buf.fields[0][0];

  if(plane.id && glIsTexture(plane.id))
    glDeleteTextures(1, &plane.id);
  plane.id = 0;

  SAFE_RELEASE(buf.IMXBuffer);
#endif
}

bool CLinuxRendererGLES::CreateIMXMAPTexture(int index)
{
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;
  YUVPLANE  &plane  = fields[0][0];

  DeleteIMXMAPTexture(index);

  memset(&im    , 0, sizeof(im));
  memset(&fields, 0, sizeof(fields));

  im.height = m_sourceHeight;
  im.width  = m_sourceWidth;

  plane.texwidth  = 0; // Must be actual frame width for pseudo-cropping
  plane.texheight = 0; // Must be actual frame height for pseudo-cropping
  plane.pixpertex_x = 1;
  plane.pixpertex_y = 1;

  glEnable(m_textureTarget);
  glGenTextures(1, &plane.id);
  VerifyGLState();

  glBindTexture(m_textureTarget, plane.id);

  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glDisable(m_textureTarget);
  return true;
}


bool CLinuxRendererGLES::Supports(ERENDERFEATURE feature)
{
  // Player controls render, let it dictate available render features
  if((m_renderMethod & RENDER_BYPASS))
  {
    Features::iterator itr = std::find(m_renderFeatures.begin(),m_renderFeatures.end(), feature);
    return itr != m_renderFeatures.end();
  }

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

bool CLinuxRendererGLES::Supports(EDEINTERLACEMODE mode)
{
  // Player controls render, let it dictate available deinterlace modes
  if((m_renderMethod & RENDER_BYPASS))
  {
    Features::iterator itr = std::find(m_deinterlaceModes.begin(),m_deinterlaceModes.end(), mode);
    return itr != m_deinterlaceModes.end();
  }

  if (mode == VS_DEINTERLACEMODE_OFF)
    return true;

  if(m_renderMethod & RENDER_OMXEGL)
    return false;

  if(m_renderMethod & RENDER_CVREF)
    return false;

  if(mode == VS_DEINTERLACEMODE_AUTO
  || mode == VS_DEINTERLACEMODE_FORCE)
    return true;

  return false;
}

bool CLinuxRendererGLES::Supports(EINTERLACEMETHOD method)
{
  // Player controls render, let it dictate available deinterlace methods
  if((m_renderMethod & RENDER_BYPASS))
  {
    Features::iterator itr = std::find(m_deinterlaceMethods.begin(),m_deinterlaceMethods.end(), method);
    return itr != m_deinterlaceMethods.end();
  }

  if(m_renderMethod & RENDER_OMXEGL)
    return false;

  if(m_renderMethod & RENDER_EGLIMG)
  {
    if (method == VS_INTERLACEMETHOD_RENDER_BOB || method == VS_INTERLACEMETHOD_RENDER_BOB_INVERTED)
      return true;
    else
      return false;
  }

  if(m_renderMethod & RENDER_MEDIACODEC)
  {
    if (method == VS_INTERLACEMETHOD_RENDER_BOB || method == VS_INTERLACEMETHOD_RENDER_BOB_INVERTED)
      return true;
    else
      return false;
  }

  if(m_renderMethod & RENDER_CVREF)
    return false;

  if(method == VS_INTERLACEMETHOD_AUTO)
    return true;

  if(m_renderMethod & RENDER_IMXMAP)
  {
    if(method == VS_INTERLACEMETHOD_IMX_FASTMOTION
    || method == VS_INTERLACEMETHOD_IMX_FASTMOTION_DOUBLE)
      return true;
    else
      return false;
  }

#if !defined(TARGET_ANDROID) && (defined(__i386__) || defined(__x86_64__))
  if(method == VS_INTERLACEMETHOD_DEINTERLACE
  || method == VS_INTERLACEMETHOD_DEINTERLACE_HALF
  || method == VS_INTERLACEMETHOD_SW_BLEND)
#else
  if(method == VS_INTERLACEMETHOD_SW_BLEND
  || method == VS_INTERLACEMETHOD_RENDER_BOB
  || method == VS_INTERLACEMETHOD_RENDER_BOB_INVERTED)
#endif
    return true;

  return false;
}

bool CLinuxRendererGLES::Supports(ESCALINGMETHOD method)
{
  // Player controls render, let it dictate available scaling methods
  if((m_renderMethod & RENDER_BYPASS))
  {
    Features::iterator itr = std::find(m_scalingMethods.begin(),m_scalingMethods.end(), method);
    return itr != m_scalingMethods.end();
  }

  if(m_renderMethod & RENDER_IMXMAP)
    return false;

  if(method == VS_SCALINGMETHOD_NEAREST
  || method == VS_SCALINGMETHOD_LINEAR)
    return true;

  return false;
}

EINTERLACEMETHOD CLinuxRendererGLES::AutoInterlaceMethod()
{
  // Player controls render, let it pick the auto-deinterlace method
  if((m_renderMethod & RENDER_BYPASS))
  {
    if (!m_deinterlaceMethods.empty())
      return ((EINTERLACEMETHOD)m_deinterlaceMethods[0]);
    else
      return VS_INTERLACEMETHOD_NONE;
  }

  if(m_renderMethod & RENDER_OMXEGL)
    return VS_INTERLACEMETHOD_NONE;

  if(m_renderMethod & RENDER_EGLIMG)
    return VS_INTERLACEMETHOD_RENDER_BOB_INVERTED;

  if(m_renderMethod & RENDER_MEDIACODEC)
    return VS_INTERLACEMETHOD_RENDER_BOB_INVERTED;

  if(m_renderMethod & RENDER_CVREF)
    return VS_INTERLACEMETHOD_NONE;

  if(m_renderMethod & RENDER_IMXMAP)
    return VS_INTERLACEMETHOD_IMX_FASTMOTION;

#if !defined(TARGET_ANDROID) && (defined(__i386__) || defined(__x86_64__))
  return VS_INTERLACEMETHOD_DEINTERLACE_HALF;
#else
  return VS_INTERLACEMETHOD_RENDER_BOB;
#endif
}

CRenderInfo CLinuxRendererGLES::GetRenderInfo()
{
  CRenderInfo info;
  info.formats = m_formats;
  info.max_buffer_size = NUM_BUFFERS;
  if(m_format == RENDER_FMT_OMXEGL ||
     m_format == RENDER_FMT_CVBREF ||
     m_format == RENDER_FMT_EGLIMG ||
     m_format == RENDER_FMT_MEDIACODEC)
    info.optimal_buffer_size = 2;
  else if(m_format == RENDER_FMT_IMXMAP)
  {
    // Let the codec control the buffer size
    info.optimal_buffer_size = info.max_buffer_size;
  }
  else
    info.optimal_buffer_size = 3;
  return info;
}

#ifdef HAVE_LIBOPENMAX
void CLinuxRendererGLES::AddProcessor(COpenMax* openMax, DVDVideoPicture *picture, int index)
{
  YUVBUFFER &buf = m_buffers[index];
  if (buf.openMaxBufferHolder) {
    buf.openMaxBufferHolder->Release();
  }
  buf.openMaxBufferHolder = picture->openMaxBufferHolder;
  if (buf.openMaxBufferHolder) {
    buf.openMaxBufferHolder->Acquire();
  }
}
#endif
#ifdef HAVE_VIDEOTOOLBOXDECODER
void CLinuxRendererGLES::AddProcessor(struct __CVBuffer *cvBufferRef, int index)
{
  YUVBUFFER &buf = m_buffers[index];
  if (buf.cvBufferRef)
    CVBufferRelease(buf.cvBufferRef);
  buf.cvBufferRef = cvBufferRef;
  // retain another reference, this way dvdplayer and renderer can issue releases.
  CVBufferRetain(buf.cvBufferRef);
}
#endif
#ifdef HAS_LIBSTAGEFRIGHT
void CLinuxRendererGLES::AddProcessor(CDVDVideoCodecStageFright* stf, EGLImageKHR eglimg, int index)
{
#ifdef DEBUG_VERBOSE
  unsigned int time = XbmcThreads::SystemClockMillis();
#endif

  YUVBUFFER &buf = m_buffers[index];
  if (buf.eglimg != EGL_NO_IMAGE_KHR)
    stf->ReleaseBuffer(buf.eglimg);
  stf->LockBuffer(eglimg);

  buf.stf = stf;
  buf.eglimg = eglimg;

#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, "AddProcessor %d: img:%p: tm:%d\n", index, eglimg, XbmcThreads::SystemClockMillis() - time);
#endif
}
#endif

#if defined(TARGET_ANDROID)
void CLinuxRendererGLES::AddProcessor(CDVDMediaCodecInfo *mediacodec, int index)
{
#ifdef DEBUG_VERBOSE
  unsigned int time = XbmcThreads::SystemClockMillis();
  int mindex = -1;
#endif

  YUVBUFFER &buf = m_buffers[index];
  if (mediacodec)
  {
    buf.mediacodec = mediacodec->Retain();
#ifdef DEBUG_VERBOSE
    mindex = buf.mediacodec->GetIndex();
#endif
    // releaseOutputBuffer must be in same thread as
    // dequeueOutputBuffer. We are in DVDPlayerVideo
    // thread here, so we are safe.
    buf.mediacodec->ReleaseOutputBuffer(true);
  }

#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, "AddProcessor %d: img:%d tm:%d", index, mindex, XbmcThreads::SystemClockMillis() - time);
#endif
}
#endif

#ifdef HAS_IMXVPU
void CLinuxRendererGLES::AddProcessor(CDVDVideoCodecIMXBuffer *buffer, int index)
{
  YUVBUFFER &buf = m_buffers[index];

  SAFE_RELEASE(buf.IMXBuffer);
  buf.IMXBuffer = buffer;

  if (buffer)
    buffer->Lock();
}
#endif

bool CLinuxRendererGLES::IsGuiLayer()
{
  if (m_format == RENDER_FMT_BYPASS || m_format == RENDER_FMT_IMXMAP)
    return false;
  else
    return true;
}

#endif

