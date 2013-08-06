/*
 *      Copyright (c) 2007 Frodo/jcmarshall/vulkanr/d4rk
 *      Based on XBoxRenderer by Frodo/jcmarshall
 *      Portions Copyright (c) by the authors of ffmpeg / xvid /mplayer
 *      Copyright (C) 2007-2013 Team XBMC
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
#include "system.h"
#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif

#ifdef HAS_GL
#include <locale.h>

#include "LinuxRendererGL.h"
#include "Application.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "VideoShaders/YUV2RGBShader.h"
#include "VideoShaders/VideoFilterShader.h"
#include "windowing/WindowingFactory.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/Texture.h"
#include "guilib/LocalizeStrings.h"
#include "threads/SingleLock.h"
#include "DllSwScale.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "RenderCapture.h"
#include "RenderFormats.h"
#include "cores/IPlayer.h"

#ifdef HAVE_LIBVDPAU
#include "cores/dvdplayer/DVDCodecs/Video/VDPAU.h"
#endif
#ifdef HAVE_LIBVA
#include <va/va.h>
#include <va/va_x11.h>
#include <va/va_glx.h>
#include "cores/dvdplayer/DVDCodecs/Video/VAAPI.h"

#define USE_VAAPI_GLX_BIND                                \
    (VA_MAJOR_VERSION == 0 &&                             \
     ((VA_MINOR_VERSION == 30 &&                          \
       VA_MICRO_VERSION == 4 && VA_SDS_VERSION >= 5) ||   \
      (VA_MINOR_VERSION == 31 &&                          \
       VA_MICRO_VERSION == 0 && VA_SDS_VERSION < 5)))

#endif

#ifdef TARGET_DARWIN
  #include "osx/CocoaInterface.h"
  #include <CoreVideo/CoreVideo.h>
  #include <OpenGL/CGLIOSurface.h>
#endif

#ifdef HAS_GLX
#include <GL/glx.h>
#endif

//due to a bug on osx nvidia, using gltexsubimage2d with a pbo bound and a null pointer
//screws up the alpha, an offset fixes this, there might still be a problem if stride + PBO_OFFSET
//is a multiple of 128 and deinterlacing is on
#define PBO_OFFSET 16

using namespace Shaders;

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

CLinuxRendererGL::YUVBUFFER::YUVBUFFER()
#ifdef HAVE_LIBVA
 : vaapi(*(new VAAPI::CHolder()))
#endif
{
  memset(&fields, 0, sizeof(fields));
  memset(&image , 0, sizeof(image));
  memset(&pbo   , 0, sizeof(pbo));
  flipindex = 0;
#ifdef HAVE_LIBVDPAU
  vdpau = NULL;
#endif
#ifdef TARGET_DARWIN_OSX
  cvBufferRef = NULL;
#endif
}

CLinuxRendererGL::YUVBUFFER::~YUVBUFFER()
{
#ifdef HAVE_LIBVA
  delete &vaapi;
#endif
#ifdef TARGET_DARWIN_OSX
  if (cvBufferRef)
    CVBufferRelease(cvBufferRef);
#endif
}

CLinuxRendererGL::CLinuxRendererGL()
{
  m_textureTarget = GL_TEXTURE_2D;

  m_renderMethod = RENDER_GLSL;
  m_renderQuality = RQ_SINGLEPASS;
  m_iFlags = 0;
  m_format = RENDER_FMT_NONE;

  m_iYV12RenderBuffer = 0;
  m_flipindex = 0;
  m_currentField = FIELD_FULL;
  m_reloadShaders = 0;
  m_pYUVShader = NULL;
  m_pVideoFilterShader = NULL;
  m_scalingMethod = VS_SCALINGMETHOD_LINEAR;
  m_scalingMethodGui = (ESCALINGMETHOD)-1;

  // default texture handlers to YUV
  m_textureUpload = &CLinuxRendererGL::UploadYV12Texture;
  m_textureCreate = &CLinuxRendererGL::CreateYV12Texture;
  m_textureDelete = &CLinuxRendererGL::DeleteYV12Texture;

  m_rgbBuffer = NULL;
  m_rgbBufferSize = 0;
  m_context = NULL;
  m_rgbPbo = 0;
  m_fbo.width = 0.0;
  m_fbo.height = 0.0;
  m_NumYV12Buffers = 0;
  m_iLastRenderBuffer = 0;
  m_bConfigured = false;
  m_bValidated = false;
  m_bImageReady = false;
  m_clearColour = 0.0f;
  m_pboSupported = false;
  m_pboUsed = false;
  m_nonLinStretch = false;
  m_nonLinStretchGui = false;
  m_pixelRatio = 0.0f;

  m_dllSwScale = new DllSwScale;
}

CLinuxRendererGL::~CLinuxRendererGL()
{
  UnInit();

  if (m_rgbPbo)
  {
    glDeleteBuffersARB(1, &m_rgbPbo);
    m_rgbPbo = 0;
    m_rgbBuffer = NULL;
  }
  else
  {
    delete [] m_rgbBuffer;
    m_rgbBuffer = NULL;
  }

  if (m_context)
  {
    m_dllSwScale->sws_freeContext(m_context);
    m_context = NULL;
  }

  if (m_pYUVShader)
  {
    m_pYUVShader->Free();
    delete m_pYUVShader;
    m_pYUVShader = NULL;
  }

  delete m_dllSwScale;
}

bool CLinuxRendererGL::ValidateRenderer()
{
  if (!m_bConfigured)
    return false;

  // if its first pass, just init textures and return
  if (ValidateRenderTarget())
    return false;

  // this needs to be checked after texture validation
  if (!m_bImageReady)
    return false;

  int index = m_iYV12RenderBuffer;
  YUVBUFFER& buf =  m_buffers[index];

  if (!buf.fields[FIELD_FULL][0].id)
    return false;

  if (buf.image.flags==0)
    return false;

  return true;
}

bool CLinuxRendererGL::ValidateRenderTarget()
{
  if (!m_bValidated)
  {
    if ((m_format == RENDER_FMT_CVBREF) ||
        (!glewIsSupported("GL_ARB_texture_non_power_of_two") && glewIsSupported("GL_ARB_texture_rectangle")))
    {
      CLog::Log(LOGNOTICE,"Using GL_TEXTURE_RECTANGLE_ARB");
      m_textureTarget = GL_TEXTURE_RECTANGLE_ARB;
    }
    else
      CLog::Log(LOGNOTICE,"Using GL_TEXTURE_2D");

    // function pointer for texture might change in
    // call to LoadShaders
    glFinish();
    for (int i = 0 ; i < NUM_BUFFERS ; i++)
      (this->*m_textureDelete)(i);

    // trigger update of video filters
    m_scalingMethodGui = (ESCALINGMETHOD)-1;

     // create the yuv textures
    LoadShaders();

    for (int i = 0 ; i < m_NumYV12Buffers ; i++)
      (this->*m_textureCreate)(i);

    m_bValidated = true;
    return true;
  }
  return false;
}

bool CLinuxRendererGL::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, ERenderFormat format, unsigned extended_format, unsigned int orientation)
{
  m_sourceWidth = width;
  m_sourceHeight = height;
  m_renderOrientation = orientation;
  m_fps = fps;

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

  m_nonLinStretch    = false;
  m_nonLinStretchGui = false;
  m_pixelRatio       = 1.0;

  m_pboSupported = glewIsSupported("GL_ARB_pixel_buffer_object") && CSettings::Get().GetBool("videoplayer.usepbo");

  return true;
}

int CLinuxRendererGL::NextYV12Texture()
{
  return (m_iYV12RenderBuffer + 1) % m_NumYV12Buffers;
}

int CLinuxRendererGL::GetImage(YV12Image *image, int source, bool readonly)
{
  if (!image) return -1;
  if (!m_bValidated) return -1;

  /* take next available buffer */
  if( source == AUTOSOURCE )
    source = NextYV12Texture();

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
  image->bpp      = im.bpp;

  return source;
}

void CLinuxRendererGL::ReleaseImage(int source, bool preserve)
{
  YV12Image &im = m_buffers[source].image;

  im.flags &= ~IMAGE_FLAG_INUSE;
  im.flags |= IMAGE_FLAG_READY;
  /* if image should be preserved reserve it so it's not auto seleceted */

  if( preserve )
    im.flags |= IMAGE_FLAG_RESERVED;

  m_bImageReady = true;
}

void CLinuxRendererGL::GetPlaneTextureSize(YUVPLANE& plane)
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
    CLog::Log(LOGDEBUG, "CLinuxRendererGL::GetPlaneTextureSize - invalid size %dx%d - %d", width, height, border);
    /* to something that avoid division by zero */
    plane.texwidth  = 1;
    plane.texheight = 1;
  }

}

void CLinuxRendererGL::CalculateTextureSourceRects(int source, int num_planes)
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

void CLinuxRendererGL::LoadPlane( YUVPLANE& plane, int type, unsigned flipindex
                                , unsigned width, unsigned height
                                , int stride, int bpp, void* data, GLuint* pbo/*= NULL*/)
{
  if(plane.flipindex == flipindex)
    return;

  //if no pbo given, use the plane pbo
  GLuint currPbo;
  if (pbo)
    currPbo = *pbo;
  else
    currPbo = plane.pbo;

  if(currPbo)
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, currPbo);

  int bps = bpp * glFormatElementByteCount(type);

  unsigned datatype;
  if(bpp == 2)
    datatype = GL_UNSIGNED_SHORT;
  else
    datatype = GL_UNSIGNED_BYTE;

  glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / bps);
  glBindTexture(m_textureTarget, plane.id);
  glTexSubImage2D(m_textureTarget, 0, 0, 0, width, height, type, datatype, data);

  /* check if we need to load any border pixels */
  if(height < plane.texheight)
    glTexSubImage2D( m_textureTarget, 0
                   , 0, height, width, 1
                   , type, datatype
                   , (unsigned char*)data + stride * (height-1));

  if(width  < plane.texwidth)
    glTexSubImage2D( m_textureTarget, 0
                   , width, 0, 1, height
                   , type, datatype
                   , (unsigned char*)data + bps * (width-1));

  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glBindTexture(m_textureTarget, 0);
  if(currPbo)
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

  plane.flipindex = flipindex;
}

bool CLinuxRendererGL::UploadYV12Texture(int source)
{
  YUVBUFFER& buf    =  m_buffers[source];
  YV12Image* im     = &buf.image;
  YUVFIELDS& fields =  buf.fields;

  if (!(im->flags&IMAGE_FLAG_READY))
    return false;
  bool deinterlacing;
  if (m_currentField == FIELD_FULL)
    deinterlacing = false;
  else
    deinterlacing = true;

  glEnable(m_textureTarget);
  VerifyGLState();

  glPixelStorei(GL_UNPACK_ALIGNMENT,1);

  if (deinterlacing)
  {
    // Load Even Y Field
    LoadPlane( fields[FIELD_TOP][0] , GL_LUMINANCE, buf.flipindex
             , im->width, im->height >> 1
             , im->stride[0]*2, im->bpp, im->plane[0] );

    //load Odd Y Field
    LoadPlane( fields[FIELD_BOT][0], GL_LUMINANCE, buf.flipindex
             , im->width, im->height >> 1
             , im->stride[0]*2, im->bpp, im->plane[0] + im->stride[0]) ;

    // Load Even U & V Fields
    LoadPlane( fields[FIELD_TOP][1], GL_LUMINANCE, buf.flipindex
             , im->width >> im->cshift_x, im->height >> (im->cshift_y + 1)
             , im->stride[1]*2, im->bpp, im->plane[1] );

    LoadPlane( fields[FIELD_TOP][2], GL_ALPHA, buf.flipindex
             , im->width >> im->cshift_x, im->height >> (im->cshift_y + 1)
             , im->stride[2]*2, im->bpp, im->plane[2] );

    // Load Odd U & V Fields
    LoadPlane( fields[FIELD_BOT][1], GL_LUMINANCE, buf.flipindex
             , im->width >> im->cshift_x, im->height >> (im->cshift_y + 1)
             , im->stride[1]*2, im->bpp, im->plane[1] + im->stride[1] );

    LoadPlane( fields[FIELD_BOT][2], GL_ALPHA, buf.flipindex
             , im->width >> im->cshift_x, im->height >> (im->cshift_y + 1)
             , im->stride[2]*2, im->bpp, im->plane[2] + im->stride[2] );
  }
  else
  {
    //Load Y plane
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
  return true;
}

void CLinuxRendererGL::Reset()
{
  for(int i=0; i<m_NumYV12Buffers; i++)
  {
    /* reset all image flags, this will cleanup textures later */
    m_buffers[i].image.flags = 0;
  }
}

void CLinuxRendererGL::Flush()
{
  if (!m_bValidated)
    return;

  glFinish();

  for (int i = 0 ; i < m_NumYV12Buffers ; i++)
    (this->*m_textureDelete)(i);

  glFinish();
  m_bValidated = false;
  m_fbo.fbo.Cleanup();
  m_iYV12RenderBuffer = 0;
}

void CLinuxRendererGL::ReleaseBuffer(int idx)
{
#if defined(HAVE_LIBVDPAU) || defined(HAVE_LIBVA) || defined(TARGET_DARWIN)
  YUVBUFFER &buf = m_buffers[idx];
#endif
#ifdef HAVE_LIBVDPAU
  SAFE_RELEASE(buf.vdpau);
#endif
#ifdef HAVE_LIBVA
  buf.vaapi.surface.reset();
#endif
#ifdef TARGET_DARWIN
  if (buf.cvBufferRef)
    CVBufferRelease(buf.cvBufferRef);
  buf.cvBufferRef = NULL;
#endif
}

void CLinuxRendererGL::Update()
{
  if (!m_bConfigured) return;
  ManageDisplay();
  m_scalingMethodGui = (ESCALINGMETHOD)-1;
}

void CLinuxRendererGL::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  int index = m_iYV12RenderBuffer;

  if (!ValidateRenderer())
  {
    if (clear) //if clear is set, we're expected to overwrite all backbuffer pixels, even if we have nothing to render
      ClearBackBuffer();

    return;
  }

  ManageDisplay();

  g_graphicsContext.BeginPaint();

  if (clear)
  {
    //draw black bars when video is not transparent, clear the entire backbuffer when it is
    if (alpha == 255)
      DrawBlackBars();
    else
      ClearBackBuffer();
  }

  if (alpha<255)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 1.0f, 1.0f, alpha / 255.0f);
  }
  else
  {
    glDisable(GL_BLEND);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  }

  if(flags & RENDER_FLAG_WEAVE)
  {
    int top_index = index;
    int bot_index = index;

    if((flags & RENDER_FLAG_FIELD0) && m_iLastRenderBuffer > -1)
    {
      if(flags & RENDER_FLAG_TOP)
        bot_index = m_iLastRenderBuffer;
      else
        top_index = m_iLastRenderBuffer;
    }

    glEnable(GL_POLYGON_STIPPLE);
    glPolygonStipple(stipple_weave);
    Render((flags & ~RENDER_FLAG_FIELDMASK) | RENDER_FLAG_TOP, top_index);
    glPolygonStipple(stipple_weave+4);
    Render((flags & ~RENDER_FLAG_FIELDMASK) | RENDER_FLAG_BOT, bot_index);
    glDisable(GL_POLYGON_STIPPLE);

  }
  else
    Render(flags, index);

  VerifyGLState();
  glEnable(GL_BLEND);
  glFlush();

  g_graphicsContext.EndPaint();
}

void CLinuxRendererGL::ClearBackBuffer()
{
  //set the entire backbuffer to black
  glClearColor(m_clearColour, m_clearColour, m_clearColour, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(0,0,0,0);
}

//draw black bars around the video quad, this is more efficient than glClear()
//since it only sets pixels to black that aren't going to be overwritten by the video
void CLinuxRendererGL::DrawBlackBars()
{
  glColor4f(m_clearColour, m_clearColour, m_clearColour, 1.0f);
  glDisable(GL_BLEND);
  glBegin(GL_QUADS);

  //top quad
  if (m_rotatedDestCoords[0].y > 0.0)
  {
    glVertex4f(0.0,                          0.0,                      0.0, 1.0);
    glVertex4f(g_graphicsContext.GetWidth(), 0.0,                      0.0, 1.0);
    glVertex4f(g_graphicsContext.GetWidth(), m_rotatedDestCoords[0].y, 0.0, 1.0);
    glVertex4f(0.0,                          m_rotatedDestCoords[0].y, 0.0, 1.0);
  }

  //bottom quad
  if (m_rotatedDestCoords[2].y < g_graphicsContext.GetHeight())
  {
    glVertex4f(0.0,                          m_rotatedDestCoords[2].y,      0.0, 1.0);
    glVertex4f(g_graphicsContext.GetWidth(), m_rotatedDestCoords[2].y,      0.0, 1.0);
    glVertex4f(g_graphicsContext.GetWidth(), g_graphicsContext.GetHeight(), 0.0, 1.0);
    glVertex4f(0.0,                          g_graphicsContext.GetHeight(), 0.0, 1.0);
  }

  //left quad
  if (m_rotatedDestCoords[0].x > 0.0)
  {
    glVertex4f(0.0,                      m_rotatedDestCoords[0].y, 0.0, 1.0);
    glVertex4f(m_rotatedDestCoords[0].x, m_rotatedDestCoords[0].y, 0.0, 1.0);
    glVertex4f(m_rotatedDestCoords[0].x, m_rotatedDestCoords[2].y, 0.0, 1.0);
    glVertex4f(0.0,                      m_rotatedDestCoords[2].y, 0.0, 1.0);
  }

  //right quad
  if (m_rotatedDestCoords[2].x < g_graphicsContext.GetWidth())
  {
    glVertex4f(m_rotatedDestCoords[2].x,     m_rotatedDestCoords[0].y, 0.0, 1.0);
    glVertex4f(g_graphicsContext.GetWidth(), m_rotatedDestCoords[0].y, 0.0, 1.0);
    glVertex4f(g_graphicsContext.GetWidth(), m_rotatedDestCoords[2].y, 0.0, 1.0);
    glVertex4f(m_rotatedDestCoords[2].x,     m_rotatedDestCoords[2].y, 0.0, 1.0);
  }

  glEnd();
}

void CLinuxRendererGL::FlipPage(int source)
{
  UnBindPbo(m_buffers[m_iYV12RenderBuffer]);

  m_iLastRenderBuffer = m_iYV12RenderBuffer;

  if( source >= 0 && source < m_NumYV12Buffers )
    m_iYV12RenderBuffer = source;
  else
    m_iYV12RenderBuffer = NextYV12Texture();

  BindPbo(m_buffers[m_iYV12RenderBuffer]);

  m_buffers[m_iYV12RenderBuffer].flipindex = ++m_flipindex;

  return;
}

unsigned int CLinuxRendererGL::PreInit()
{
  CSingleLock lock(g_graphicsContext);
  m_bConfigured = false;
  m_bValidated = false;
  UnInit();
  m_resolution = CDisplaySettings::Get().GetCurrentResolution();
  if ( m_resolution == RES_WINDOW )
    m_resolution = RES_DESKTOP;

  m_iYV12RenderBuffer = 0;

  m_formats.push_back(RENDER_FMT_YUV420P);
  GLint size;
  glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_LUMINANCE16, NP2(1920), NP2(1080), 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, NULL);
  glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_LUMINANCE_SIZE, &size);
  if(size >= 16)
  {
    m_formats.push_back(RENDER_FMT_YUV420P10);
    m_formats.push_back(RENDER_FMT_YUV420P16);
  }
  CLog::Log(LOGDEBUG, "CLinuxRendererGL::PreInit - precision of luminance 16 is %d", size);
  m_formats.push_back(RENDER_FMT_NV12);
  m_formats.push_back(RENDER_FMT_YUYV422);
  m_formats.push_back(RENDER_FMT_UYVY422);
#ifdef TARGET_DARWIN
  m_formats.push_back(RENDER_FMT_CVBREF);
#endif

  // setup the background colour
  m_clearColour = (float)(g_advancedSettings.m_videoBlackBarColour & 0xff) / 0xff;

  if (!m_dllSwScale->Load())
    CLog::Log(LOGERROR,"CLinuxRendererGL::PreInit - failed to load rescale libraries!");

  return true;
}

void CLinuxRendererGL::UpdateVideoFilter()
{
  bool pixelRatioChanged    = (CDisplaySettings::Get().GetPixelRatio() > 1.001f || CDisplaySettings::Get().GetPixelRatio() < 0.999f) !=
                              (m_pixelRatio > 1.001f || m_pixelRatio < 0.999f);
  bool nonLinStretchChanged = false;
  if (m_nonLinStretchGui != CDisplaySettings::Get().IsNonLinearStretched() || pixelRatioChanged)
  {
    m_nonLinStretchGui   = CDisplaySettings::Get().IsNonLinearStretched();
    m_pixelRatio         = CDisplaySettings::Get().GetPixelRatio();
    m_reloadShaders      = 1;
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

  if (m_scalingMethodGui == CMediaSettings::Get().GetCurrentVideoSettings().m_ScalingMethod && !nonLinStretchChanged)
    return;

  //recompile YUV shader when non-linear stretch is turned on/off
  //or when it's on and the scaling method changed
  if (m_nonLinStretch || nonLinStretchChanged)
    m_reloadShaders = 1;

  m_scalingMethodGui = CMediaSettings::Get().GetCurrentVideoSettings().m_ScalingMethod;
  m_scalingMethod    = m_scalingMethodGui;

  if(!Supports(m_scalingMethod))
  {
    CLog::Log(LOGWARNING, "CLinuxRendererGL::UpdateVideoFilter - choosen scaling method %d, is not supported by renderer", (int)m_scalingMethod);
    m_scalingMethod = VS_SCALINGMETHOD_LINEAR;
  }

  if (m_pVideoFilterShader)
  {
    m_pVideoFilterShader->Free();
    delete m_pVideoFilterShader;
    m_pVideoFilterShader = NULL;
  }
  m_fbo.fbo.Cleanup();

  VerifyGLState();

  if (m_scalingMethod == VS_SCALINGMETHOD_AUTO)
  {
    bool scaleSD = m_sourceHeight < 720 && m_sourceWidth < 1280;
    bool scaleUp = (int)m_sourceHeight < g_graphicsContext.GetHeight() && (int)m_sourceWidth < g_graphicsContext.GetWidth();
    bool scaleFps = m_fps < g_advancedSettings.m_videoAutoScaleMaxFps + 0.01f;

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
    if (((m_renderMethod & RENDER_VDPAU) || (m_renderMethod & RENDER_VAAPI)) && m_nonLinStretch)
    {
      m_pVideoFilterShader = new StretchFilterShader();
      if (!m_pVideoFilterShader->CompileAndLink())
      {
        CLog::Log(LOGERROR, "GL: Error compiling and linking video filter shader");
        break;
      }
    }
    return;

  case VS_SCALINGMETHOD_LANCZOS2:
  case VS_SCALINGMETHOD_SPLINE36_FAST:
  case VS_SCALINGMETHOD_LANCZOS3_FAST:
  case VS_SCALINGMETHOD_SPLINE36:
  case VS_SCALINGMETHOD_LANCZOS3:
  case VS_SCALINGMETHOD_CUBIC:
    if (m_renderMethod & RENDER_GLSL)
    {
      if (!m_fbo.fbo.Initialize())
      {
        CLog::Log(LOGERROR, "GL: Error initializing FBO");
        break;
      }

      if (!m_fbo.fbo.CreateAndBindToTexture(GL_TEXTURE_2D, m_sourceWidth, m_sourceHeight, GL_RGBA))
      {
        CLog::Log(LOGERROR, "GL: Error creating texture and binding to FBO");
        break;
      }
    }

    m_pVideoFilterShader = new ConvolutionFilterShader(m_scalingMethod, m_nonLinStretch);
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
  case VS_SCALINGMETHOD_NEDI:
    CLog::Log(LOGERROR, "GL: TODO: This scaler has not yet been implemented");
    break;

  default:
    break;
  }

  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(34400), g_localizeStrings.Get(34401));
  CLog::Log(LOGERROR, "GL: Falling back to bilinear due to failure to init scaler");
  if (m_pVideoFilterShader)
  {
    m_pVideoFilterShader->Free();
    delete m_pVideoFilterShader;
    m_pVideoFilterShader = NULL;
  }
  m_fbo.fbo.Cleanup();

  SetTextureFilter(GL_LINEAR);
  m_renderQuality = RQ_SINGLEPASS;
}

void CLinuxRendererGL::LoadShaders(int field)
{
  if (m_format == RENDER_FMT_VDPAU)
  {
    CLog::Log(LOGNOTICE, "GL: Using VDPAU render method");
    m_renderMethod = RENDER_VDPAU;
  }
  else if (m_format == RENDER_FMT_VAAPI)
  {
    CLog::Log(LOGNOTICE, "GL: Using VAAPI render method");
    m_renderMethod = RENDER_VAAPI;
  }
  else if (m_format == RENDER_FMT_CVBREF)
  {
    CLog::Log(LOGNOTICE, "GL: Using CVBREF render method");
    m_renderMethod = RENDER_CVREF;
  }
  else
  {
    int requestedMethod = CSettings::Get().GetInt("videoplayer.rendermethod");
    CLog::Log(LOGDEBUG, "GL: Requested render method: %d", requestedMethod);

    if (m_pYUVShader)
    {
      m_pYUVShader->Free();
      delete m_pYUVShader;
      m_pYUVShader = NULL;
    }

    bool tryGlsl = true;
    switch(requestedMethod)
    {
      case RENDER_METHOD_AUTO:
#if defined(TARGET_POSIX) && !defined(TARGET_DARWIN)
       //with render method set to auto, don't try glsl on ati if we're on linux
       //it seems to be broken in a random way with every new driver release
       tryGlsl = g_Windowing.GetRenderVendor().Left(3).CompareNoCase("ati") != 0;
#endif
      
      case RENDER_METHOD_GLSL:
      // Try GLSL shaders if supported and user requested auto or GLSL.
      if (glCreateProgram && tryGlsl)
      {
        // create regular progressive scan shader
        m_pYUVShader = new YUV2RGBProgressiveShader(m_textureTarget==GL_TEXTURE_RECTANGLE_ARB, m_iFlags, m_format,
                                                    m_nonLinStretch && m_renderQuality == RQ_SINGLEPASS);

        CLog::Log(LOGNOTICE, "GL: Selecting Single Pass YUV 2 RGB shader");

        if (m_pYUVShader && m_pYUVShader->CompileAndLink())
        {
          m_renderMethod = RENDER_GLSL;
          UpdateVideoFilter();
          break;
        }
        else if (m_pYUVShader)
        {
          m_pYUVShader->Free();
          delete m_pYUVShader;
          m_pYUVShader = NULL;
        }
        CLog::Log(LOGERROR, "GL: Error enabling YUV2RGB GLSL shader");
        // drop through and try ARB
      }
      case RENDER_METHOD_ARB:
      // Try ARB shaders if supported and user requested it or GLSL shaders failed.
      if (glewIsSupported("GL_ARB_fragment_program"))
      {
        CLog::Log(LOGNOTICE, "GL: ARB shaders support detected");
        m_renderMethod = RENDER_ARB ;

        // create regular progressive scan shader
        m_pYUVShader = new YUV2RGBProgressiveShaderARB(m_textureTarget==GL_TEXTURE_RECTANGLE_ARB, m_iFlags, m_format);
        CLog::Log(LOGNOTICE, "GL: Selecting Single Pass ARB YUV2RGB shader");

        if (m_pYUVShader && m_pYUVShader->CompileAndLink())
        {
          m_renderMethod = RENDER_ARB;
          UpdateVideoFilter();
          break;
        }
        else if (m_pYUVShader)
        {
          m_pYUVShader->Free();
          delete m_pYUVShader;
          m_pYUVShader = NULL;
        }
        CLog::Log(LOGERROR, "GL: Error enabling YUV2RGB ARB shader");
        // drop through and use SW
      }
      case RENDER_METHOD_SOFTWARE:
      default:
      // Use software YUV 2 RGB conversion if user requested it or GLSL and/or ARB shaders failed
      {
        m_renderMethod = RENDER_SW ;
        CLog::Log(LOGNOTICE, "GL: Shaders support not present, falling back to SW mode");
        break;
      }
    }
  }

  // determine whether GPU supports NPOT textures
  if (!glewIsSupported("GL_ARB_texture_non_power_of_two"))
  {
    if (!glewIsSupported("GL_ARB_texture_rectangle"))
    {
      CLog::Log(LOGNOTICE, "GL: GL_ARB_texture_rectangle not supported and OpenGL version is not 2.x");
      CLog::Log(LOGNOTICE, "GL: Reverting to POT textures");
      m_renderMethod |= RENDER_POT;
    }
    else
      CLog::Log(LOGNOTICE, "GL: NPOT textures are supported through GL_ARB_texture_rectangle extension");
  }
  else
    CLog::Log(LOGNOTICE, "GL: NPOT texture support detected");

  
  if (m_pboSupported &&
    !(m_renderMethod & RENDER_SW) && 
    !(m_renderMethod & RENDER_CVREF))
  {
    CLog::Log(LOGNOTICE, "GL: Using GL_ARB_pixel_buffer_object");
    m_pboUsed = true;
  }
  else
    m_pboUsed = false;

  // Now that we now the render method, setup texture function handlers
  if (m_format == RENDER_FMT_NV12)
  {
    m_textureUpload = &CLinuxRendererGL::UploadNV12Texture;
    m_textureCreate = &CLinuxRendererGL::CreateNV12Texture;
    m_textureDelete = &CLinuxRendererGL::DeleteNV12Texture;
  }
  else if (m_format == RENDER_FMT_YUYV422 ||
           m_format == RENDER_FMT_UYVY422)
  {
    m_textureUpload = &CLinuxRendererGL::UploadYUV422PackedTexture;
    m_textureCreate = &CLinuxRendererGL::CreateYUV422PackedTexture;
    m_textureDelete = &CLinuxRendererGL::DeleteYUV422PackedTexture;
  }
  else if (m_format == RENDER_FMT_VDPAU)
  {
    m_textureUpload = &CLinuxRendererGL::UploadVDPAUTexture;
    m_textureCreate = &CLinuxRendererGL::CreateVDPAUTexture;
    m_textureDelete = &CLinuxRendererGL::DeleteVDPAUTexture;
  }
  else if (m_format == RENDER_FMT_VDPAU_420)
  {
    m_textureUpload = &CLinuxRendererGL::UploadVDPAUTexture420;
    m_textureCreate = &CLinuxRendererGL::CreateVDPAUTexture420;
    m_textureDelete = &CLinuxRendererGL::DeleteVDPAUTexture420;
  }
  else if (m_format == RENDER_FMT_VAAPI)
  {
    m_textureUpload = &CLinuxRendererGL::UploadVAAPITexture;
    m_textureCreate = &CLinuxRendererGL::CreateVAAPITexture;
    m_textureDelete = &CLinuxRendererGL::DeleteVAAPITexture;
  }
  else if (m_format == RENDER_FMT_CVBREF)
  {
    m_textureUpload = &CLinuxRendererGL::UploadCVRefTexture;
    m_textureCreate = &CLinuxRendererGL::CreateCVRefTexture;
    m_textureDelete = &CLinuxRendererGL::DeleteCVRefTexture;
  }
  else
  {
    // setup default YV12 texture handlers
    m_textureUpload = &CLinuxRendererGL::UploadYV12Texture;
    m_textureCreate = &CLinuxRendererGL::CreateYV12Texture;
    m_textureDelete = &CLinuxRendererGL::DeleteYV12Texture;
  }

  //in case of software colorspace conversion, all formats are handled by the same method
  if (m_renderMethod & RENDER_SW)
    m_textureUpload = &CLinuxRendererGL::UploadRGBTexture;
}

void CLinuxRendererGL::UnInit()
{
  CLog::Log(LOGDEBUG, "LinuxRendererGL: Cleaning up GL resources");
  CSingleLock lock(g_graphicsContext);

  glFinish();

  if (m_rgbPbo)
  {
    glDeleteBuffersARB(1, &m_rgbPbo);
    m_rgbPbo = 0;
    m_rgbBuffer = NULL;
  }
  else
  {
    delete [] m_rgbBuffer;
    m_rgbBuffer = NULL;
  }
  m_rgbBufferSize = 0;

  if (m_context)
  {
    m_dllSwScale->sws_freeContext(m_context);
    m_context = NULL;
  }

  // YV12 textures
  for (int i = 0; i < NUM_BUFFERS; ++i)
    (this->*m_textureDelete)(i);

  // cleanup framebuffer object if it was in use
  m_fbo.fbo.Cleanup();
  m_bValidated = false;
  m_bImageReady = false;
  m_bConfigured = false;
}

void CLinuxRendererGL::Render(DWORD flags, int renderBuffer)
{
  // obtain current field, if interlaced
  if( flags & RENDER_FLAG_TOP)
    m_currentField = FIELD_TOP;

  else if (flags & RENDER_FLAG_BOT)
    m_currentField = FIELD_BOT;

  else
    m_currentField = FIELD_FULL;

  // call texture load function
  if (!(this->*m_textureUpload)(renderBuffer))
    return;

  if (m_renderMethod & RENDER_GLSL)
  {
    UpdateVideoFilter();
    switch(m_renderQuality)
    {
    case RQ_LOW:
    case RQ_SINGLEPASS:
      if (m_format == RENDER_FMT_VDPAU_420 && m_currentField == FIELD_FULL)
        RenderProgressiveWeave(renderBuffer, m_currentField);
      else
        RenderSinglePass(renderBuffer, m_currentField);
      VerifyGLState();
      break;

    case RQ_MULTIPASS:
      if (m_format == RENDER_FMT_VDPAU_420 && m_currentField == FIELD_FULL)
        RenderProgressiveWeave(renderBuffer, m_currentField);
      else
      {
        RenderToFBO(renderBuffer, m_currentField);
        RenderFromFBO();
      }
      VerifyGLState();
      break;
    }
  }
  else if (m_renderMethod & RENDER_ARB)
  {
    RenderSinglePass(renderBuffer, m_currentField);
  }
#ifdef HAVE_LIBVDPAU
  else if (m_renderMethod & RENDER_VDPAU)
  {
    UpdateVideoFilter();
    RenderVDPAU(renderBuffer, m_currentField);
  }
#endif
#ifdef HAVE_LIBVA
  else if (m_renderMethod & RENDER_VAAPI)
  {
    UpdateVideoFilter();
    RenderVAAPI(renderBuffer, m_currentField);
  }
#endif
  else
  {
    // RENDER_CVREF uses the same render as the default case
    RenderSoftware(renderBuffer, m_currentField);
    VerifyGLState();
  }
}

void CLinuxRendererGL::RenderSinglePass(int index, int field)
{
  YUVFIELDS &fields = m_buffers[index].fields;
  YUVPLANES &planes = fields[field];

  if (m_reloadShaders)
  {
    m_reloadShaders = 0;
    LoadShaders(field);
  }

  glDisable(GL_DEPTH_TEST);

  // Y
  glActiveTextureARB(GL_TEXTURE0);
  glEnable(m_textureTarget);
  glBindTexture(m_textureTarget, planes[0].id);

  // U
  glActiveTextureARB(GL_TEXTURE1);
  glEnable(m_textureTarget);
  glBindTexture(m_textureTarget, planes[1].id);

  // V
  glActiveTextureARB(GL_TEXTURE2);
  glEnable(m_textureTarget);
  glBindTexture(m_textureTarget, planes[2].id);

  glActiveTextureARB(GL_TEXTURE0);
  VerifyGLState();

  m_pYUVShader->SetBlack(CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness * 0.01f - 0.5f);
  m_pYUVShader->SetContrast(CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast * 0.02f);
  m_pYUVShader->SetWidth(planes[0].texwidth);
  m_pYUVShader->SetHeight(planes[0].texheight);

  //disable non-linear stretch when a dvd menu is shown, parts of the menu are rendered through the overlay renderer
  //having non-linear stretch on breaks the alignment
  if (g_application.m_pPlayer->IsInMenu())
    m_pYUVShader->SetNonLinStretch(1.0);
  else
    m_pYUVShader->SetNonLinStretch(pow(CDisplaySettings::Get().GetPixelRatio(), g_advancedSettings.m_videoNonLinStretchRatio));

  if     (field == FIELD_TOP)
    m_pYUVShader->SetField(1);
  else if(field == FIELD_BOT)
    m_pYUVShader->SetField(0);

  m_pYUVShader->Enable();

  glBegin(GL_QUADS);

  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x1, planes[0].rect.y1);
  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x1, planes[1].rect.y1);
  glMultiTexCoord2fARB(GL_TEXTURE2, planes[2].rect.x1, planes[2].rect.y1);
  glVertex4f(m_rotatedDestCoords[0].x, m_rotatedDestCoords[0].y, 0, 1.0f );//top left

  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x2, planes[0].rect.y1);
  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x2, planes[1].rect.y1);
  glMultiTexCoord2fARB(GL_TEXTURE2, planes[2].rect.x2, planes[2].rect.y1);
  glVertex4f(m_rotatedDestCoords[1].x, m_rotatedDestCoords[1].y, 0, 1.0f );//top right

  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x2, planes[0].rect.y2);
  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x2, planes[1].rect.y2);
  glMultiTexCoord2fARB(GL_TEXTURE2, planes[2].rect.x2, planes[2].rect.y2);
  glVertex4f(m_rotatedDestCoords[2].x, m_rotatedDestCoords[2].y, 0, 1.0f );//bottom right

  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x1, planes[0].rect.y2);
  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x1, planes[1].rect.y2);
  glMultiTexCoord2fARB(GL_TEXTURE2, planes[2].rect.x1, planes[2].rect.y2);
  glVertex4f(m_rotatedDestCoords[3].x, m_rotatedDestCoords[3].y, 0, 1.0f );//bottom left

  glEnd();
  VerifyGLState();

  m_pYUVShader->Disable();
  VerifyGLState();

  glActiveTextureARB(GL_TEXTURE1);
  glDisable(m_textureTarget);

  glActiveTextureARB(GL_TEXTURE2);
  glDisable(m_textureTarget);

  glActiveTextureARB(GL_TEXTURE0);
  glDisable(m_textureTarget);

  glMatrixMode(GL_MODELVIEW);

  VerifyGLState();
}

void CLinuxRendererGL::RenderToFBO(int index, int field, bool weave /*= false*/)
{
  YUVPLANES &planes = m_buffers[index].fields[field];

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

    if (!m_fbo.fbo.CreateAndBindToTexture(GL_TEXTURE_2D, m_sourceWidth, m_sourceHeight, GL_RGBA))
    {
      CLog::Log(LOGERROR, "GL: Error creating texture and binding to FBO");
      return;
    }
  }

  glDisable(GL_DEPTH_TEST);

  // Y
  glEnable(m_textureTarget);
  glActiveTextureARB(GL_TEXTURE0);
  glBindTexture(m_textureTarget, planes[0].id);
  VerifyGLState();

  // U
  glActiveTextureARB(GL_TEXTURE1);
  glEnable(m_textureTarget);
  glBindTexture(m_textureTarget, planes[1].id);
  VerifyGLState();

  // V
  glActiveTextureARB(GL_TEXTURE2);
  glEnable(m_textureTarget);
  glBindTexture(m_textureTarget, planes[2].id);
  VerifyGLState();

  glActiveTextureARB(GL_TEXTURE0);
  VerifyGLState();

  // make sure the yuv shader is loaded and ready to go
  if (!m_pYUVShader || (!m_pYUVShader->OK()))
  {
    CLog::Log(LOGERROR, "GL: YUV shader not active, cannot do multipass render");
    return;
  }

  m_fbo.fbo.BeginRender();
  VerifyGLState();

  m_pYUVShader->SetBlack(CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness * 0.01f - 0.5f);
  m_pYUVShader->SetContrast(CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast * 0.02f);
  m_pYUVShader->SetWidth(planes[0].texwidth);
  m_pYUVShader->SetHeight(planes[0].texheight);
  m_pYUVShader->SetNonLinStretch(1.0);
  if     (field == FIELD_TOP)
    m_pYUVShader->SetField(1);
  else if(field == FIELD_BOT)
    m_pYUVShader->SetField(0);

  VerifyGLState();

  glPushAttrib(GL_VIEWPORT_BIT);
  glPushAttrib(GL_SCISSOR_BIT);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  VerifyGLState();

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  VerifyGLState();
  gluOrtho2D(0, m_sourceWidth, 0, m_sourceHeight);
  glViewport(0, 0, m_sourceWidth, m_sourceHeight);
  glScissor (0, 0, m_sourceWidth, m_sourceHeight);
  glMatrixMode(GL_MODELVIEW);
  VerifyGLState();


  if (!m_pYUVShader->Enable())
  {
    CLog::Log(LOGERROR, "GL: Error enabling YUV shader");
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
  if (weave)
    m_fbo.height *= 2;

  // 1st Pass to video frame size
  glBegin(GL_QUADS);

  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x1, planes[0].rect.y1);
  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x1, planes[1].rect.y1);
  glMultiTexCoord2fARB(GL_TEXTURE2, planes[2].rect.x1, planes[2].rect.y1);
  glVertex2f(0.0f    , 0.0f);

  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x2, planes[0].rect.y1);
  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x2, planes[1].rect.y1);
  glMultiTexCoord2fARB(GL_TEXTURE2, planes[2].rect.x2, planes[2].rect.y1);
  glVertex2f(m_fbo.width, 0.0f);

  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x2, planes[0].rect.y2);
  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x2, planes[1].rect.y2);
  glMultiTexCoord2fARB(GL_TEXTURE2, planes[2].rect.x2, planes[2].rect.y2);
  glVertex2f(m_fbo.width, m_fbo.height);

  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x1, planes[0].rect.y2);
  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x1, planes[1].rect.y2);
  glMultiTexCoord2fARB(GL_TEXTURE2, planes[2].rect.x1, planes[2].rect.y2);
  glVertex2f(0.0f    , m_fbo.height);

  glEnd();
  VerifyGLState();

  m_pYUVShader->Disable();

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix(); // pop modelview
  glMatrixMode(GL_PROJECTION);
  glPopMatrix(); // pop projection
  glPopAttrib(); // pop scissor
  glPopAttrib(); // pop viewport
  glMatrixMode(GL_MODELVIEW);
  VerifyGLState();

  m_fbo.fbo.EndRender();

  glActiveTextureARB(GL_TEXTURE1);
  glDisable(m_textureTarget);
  glActiveTextureARB(GL_TEXTURE2);
  glDisable(m_textureTarget);
  glActiveTextureARB(GL_TEXTURE0);
  glDisable(m_textureTarget);
}

void CLinuxRendererGL::RenderFromFBO()
{
  glEnable(m_textureTarget);
  glActiveTextureARB(GL_TEXTURE0);
  VerifyGLState();

  // Use regular normalized texture coordinates

  // 2nd Pass to screen size with optional video filter

  if (m_pVideoFilterShader)
  {
    GLint filter;
    if (!m_pVideoFilterShader->GetTextureFilter(filter))
      filter = m_scalingMethod == VS_SCALINGMETHOD_NEAREST ? GL_NEAREST : GL_LINEAR;

    m_fbo.fbo.SetFiltering(m_textureTarget, filter);
    m_pVideoFilterShader->SetSourceTexture(0);
    m_pVideoFilterShader->SetWidth(m_sourceWidth);
    m_pVideoFilterShader->SetHeight(m_sourceHeight);

    //disable non-linear stretch when a dvd menu is shown, parts of the menu are rendered through the overlay renderer
    //having non-linear stretch on breaks the alignment
    if (g_application.m_pPlayer->IsInMenu())
      m_pVideoFilterShader->SetNonLinStretch(1.0);
    else
      m_pVideoFilterShader->SetNonLinStretch(pow(CDisplaySettings::Get().GetPixelRatio(), g_advancedSettings.m_videoNonLinStretchRatio));

    m_pVideoFilterShader->Enable();
  }
  else
  {
    GLint filter = m_scalingMethod == VS_SCALINGMETHOD_NEAREST ? GL_NEAREST : GL_LINEAR;
    m_fbo.fbo.SetFiltering(m_textureTarget, filter);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  }

  VerifyGLState();

  float imgwidth = m_fbo.width / m_sourceWidth;
  float imgheight = m_fbo.height / m_sourceHeight;

  glBegin(GL_QUADS);

  glMultiTexCoord2fARB(GL_TEXTURE0, 0.0f, 0.0f);
  glVertex4f(m_rotatedDestCoords[0].x, m_rotatedDestCoords[0].y, 0, 1.0f );

  glMultiTexCoord2fARB(GL_TEXTURE0, imgwidth, 0.0f);
  glVertex4f(m_rotatedDestCoords[1].x, m_rotatedDestCoords[1].y, 0, 1.0f );

  glMultiTexCoord2fARB(GL_TEXTURE0, imgwidth, imgheight);
  glVertex4f(m_rotatedDestCoords[2].x, m_rotatedDestCoords[2].y, 0, 1.0f );
  
  glMultiTexCoord2fARB(GL_TEXTURE0, 0.0f, imgheight);
  glVertex4f(m_rotatedDestCoords[3].x, m_rotatedDestCoords[3].y, 0, 1.0f );

  glEnd();

  VerifyGLState();

  if (m_pVideoFilterShader)
    m_pVideoFilterShader->Disable();

  VerifyGLState();

  glBindTexture(m_textureTarget, 0);
  glDisable(m_textureTarget);
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

void CLinuxRendererGL::RenderVDPAU(int index, int field)
{
#ifdef HAVE_LIBVDPAU
  YUVPLANE &plane = m_buffers[index].fields[FIELD_FULL][0];

  glEnable(m_textureTarget);
  glActiveTextureARB(GL_TEXTURE0);

  glBindTexture(m_textureTarget, plane.id);

  // make sure we know the correct texture size
  GetPlaneTextureSize(plane);
  CalculateTextureSourceRects(index, 1);

  // Try some clamping or wrapping
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  if (m_pVideoFilterShader)
  {
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
    if (g_application.m_pPlayer->IsInMenu())
      m_pVideoFilterShader->SetNonLinStretch(1.0);
    else
      m_pVideoFilterShader->SetNonLinStretch(pow(CDisplaySettings::Get().GetPixelRatio(), g_advancedSettings.m_videoNonLinStretchRatio));

    m_pVideoFilterShader->Enable();
  }
  else
  {
    GLint filter = m_scalingMethod == VS_SCALINGMETHOD_NEAREST ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, filter);
  }

  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  VerifyGLState();

  glBegin(GL_QUADS);
  if (m_textureTarget==GL_TEXTURE_2D)
  {
    glTexCoord2f(plane.rect.x1, plane.rect.y1);  glVertex2f(m_rotatedDestCoords[0].x, m_rotatedDestCoords[0].y);
    glTexCoord2f(plane.rect.x2, plane.rect.y1);  glVertex2f(m_rotatedDestCoords[1].x, m_rotatedDestCoords[1].y);
    glTexCoord2f(plane.rect.x2, plane.rect.y2);  glVertex2f(m_rotatedDestCoords[2].x, m_rotatedDestCoords[2].y);
    glTexCoord2f(plane.rect.x1, plane.rect.y2);  glVertex2f(m_rotatedDestCoords[3].x, m_rotatedDestCoords[3].y);
  }
  else
  {
    glTexCoord2f(m_rotatedDestCoords[0].x, m_rotatedDestCoords[0].y); glVertex4f(m_rotatedDestCoords[0].x, m_rotatedDestCoords[0].y, 0.0f, 0.0f);
    glTexCoord2f(m_rotatedDestCoords[1].x, m_rotatedDestCoords[1].y); glVertex4f(m_rotatedDestCoords[1].x, m_rotatedDestCoords[1].y, 1.0f, 0.0f);
    glTexCoord2f(m_rotatedDestCoords[2].x, m_rotatedDestCoords[2].y); glVertex4f(m_rotatedDestCoords[2].x, m_rotatedDestCoords[2].y, 1.0f, 1.0f);
    glTexCoord2f(m_rotatedDestCoords[3].x, m_rotatedDestCoords[3].y); glVertex4f(m_rotatedDestCoords[3].x, m_rotatedDestCoords[3].y, 0.0f, 1.0f);
  }
  glEnd();
  VerifyGLState();

  if (m_pVideoFilterShader)
    m_pVideoFilterShader->Disable();

  glBindTexture (m_textureTarget, 0);
  glDisable(m_textureTarget);
#endif
}

void CLinuxRendererGL::RenderVAAPI(int index, int field)
{
#ifdef HAVE_LIBVA
  YUVPLANE       &plane = m_buffers[index].fields[0][0];
  VAAPI::CHolder &va    = m_buffers[index].vaapi;

  if(!va.surface)
  {
    CLog::Log(LOGINFO, "CLinuxRendererGL::RenderVAAPI - no vaapi object");
    return;
  }
  VAAPI::CDisplayPtr& display(va.surface->m_display);
  CSingleLock lock(*display);

  glEnable(m_textureTarget);
  glActiveTextureARB(GL_TEXTURE0);
  glBindTexture(m_textureTarget, plane.id);

#if USE_VAAPI_GLX_BIND
  VAStatus status;
  status = vaBeginRenderSurfaceGLX(display->get(), va.surfglx->m_id);
  if(status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGERROR, "CLinuxRendererGL::RenderVAAPI - vaBeginRenderSurfaceGLX failed (%d)", status);
    return;
  }
#endif

  // make sure we know the correct texture size
  GetPlaneTextureSize(plane);
  CalculateTextureSourceRects(index, 1);

  // Try some clamping or wrapping
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  if (m_pVideoFilterShader)
  {
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
    if (g_application.m_pPlayer->IsInMenu())
      m_pVideoFilterShader->SetNonLinStretch(1.0);
    else
      m_pVideoFilterShader->SetNonLinStretch(pow(CDisplaySettings::Get().GetPixelRatio(), g_advancedSettings.m_videoNonLinStretchRatio));

    m_pVideoFilterShader->Enable();
  }
  else
  {
    GLint filter = m_scalingMethod == VS_SCALINGMETHOD_NEAREST ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, filter);
  }

  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  VerifyGLState();

  glBegin(GL_QUADS);
  glTexCoord2f(plane.rect.x1, plane.rect.y1);  glVertex2f(m_rotatedDestCoords[0].x, m_rotatedDestCoords[0].y);
  glTexCoord2f(plane.rect.x2, plane.rect.y1);  glVertex2f(m_rotatedDestCoords[1].x, m_rotatedDestCoords[1].y);
  glTexCoord2f(plane.rect.x2, plane.rect.y2);  glVertex2f(m_rotatedDestCoords[2].x, m_rotatedDestCoords[2].y);
  glTexCoord2f(plane.rect.x1, plane.rect.y2);  glVertex2f(m_rotatedDestCoords[3].x, m_rotatedDestCoords[3].y);
  glEnd();

  VerifyGLState();

  if (m_pVideoFilterShader)
    m_pVideoFilterShader->Disable();

#if USE_VAAPI_GLX_BIND
  status = vaEndRenderSurfaceGLX(display->get(), va.surfglx->m_id);
  if(status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGERROR, "CLinuxRendererGL::RenderVAAPI - vaEndRenderSurfaceGLX failed (%d)", status);
    return;
  }
#endif

  glBindTexture (m_textureTarget, 0);
  glDisable(m_textureTarget);
#endif
}

void CLinuxRendererGL::RenderSoftware(int index, int field)
{
  // used for textues uploaded from rgba or CVPixelBuffers.
  YUVPLANES &planes = m_buffers[index].fields[field];

  glDisable(GL_DEPTH_TEST);

  glEnable(m_textureTarget);
  glActiveTextureARB(GL_TEXTURE0);
  glBindTexture(m_textureTarget, planes[0].id);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  glBegin(GL_QUADS);
  glTexCoord2f(planes[0].rect.x1, planes[0].rect.y1);
  glVertex4f(m_rotatedDestCoords[0].x, m_rotatedDestCoords[0].y, 0, 1.0f );

  glTexCoord2f(planes[0].rect.x2, planes[0].rect.y1);
  glVertex4f(m_rotatedDestCoords[1].x, m_rotatedDestCoords[1].y, 0, 1.0f);

  glTexCoord2f(planes[0].rect.x2, planes[0].rect.y2);
  glVertex4f(m_rotatedDestCoords[2].x, m_rotatedDestCoords[2].y, 0, 1.0f);

  glTexCoord2f(planes[0].rect.x1, planes[0].rect.y2);
  glVertex4f(m_rotatedDestCoords[3].x, m_rotatedDestCoords[3].y, 0, 1.0f);

  glEnd();

  VerifyGLState();

  glDisable(m_textureTarget);
  VerifyGLState();
}

bool CLinuxRendererGL::RenderCapture(CRenderCapture* capture)
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
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslatef(0, capture->GetHeight(), 0);
  glScalef(1.0, -1.0f, 1.0f);

  capture->BeginRender();

  Render(RENDER_FLAG_NOOSD, m_iYV12RenderBuffer);
  // read pixels
  glReadPixels(0, g_graphicsContext.GetHeight() - capture->GetHeight(), capture->GetWidth(), capture->GetHeight(),
               GL_BGRA, GL_UNSIGNED_BYTE, capture->GetRenderBuffer());

  capture->EndRender();

  // revert model view matrix
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  // restore original video rect
  m_destRect = saveSize;
  restoreRotatedCoords();//restores the previous state of the rotated dest coords

  return true;
}

//********************************************************************************************************
// YV12 Texture creation, deletion, copying + clearing
//********************************************************************************************************
void CLinuxRendererGL::DeleteYV12Texture(int index)
{
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;
  GLuint    *pbo    = m_buffers[index].pbo;

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
    if (pbo[p])
    {
      if (im.plane[p])
      {
        glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo[p]);
        glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
        im.plane[p] = NULL;
        glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
      }
      glDeleteBuffersARB(1, pbo + p);
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

bool CLinuxRendererGL::CreateYV12Texture(int index)
{
  /* since we also want the field textures, pitch must be texture aligned */
  unsigned p;

  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;
  GLuint    *pbo    = m_buffers[index].pbo;

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

  bool pboSetup = false;
  if (m_pboUsed)
  {
    pboSetup = true;
    glGenBuffersARB(3, pbo);

    for (int i = 0; i < 3; i++)
    {
      glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo[i]);
      glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, im.planesize[i] + PBO_OFFSET, 0, GL_STREAM_DRAW_ARB);
      void* pboPtr = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
      if (pboPtr)
      {
        im.plane[i] = (BYTE*) pboPtr + PBO_OFFSET;
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
        glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo[i]);
        glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
      }
      glDeleteBuffersARB(3, pbo);
      memset(m_buffers[index].pbo, 0, sizeof(m_buffers[index].pbo));
    }

    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
  }

  if (!pboSetup)
  {
    for (int i = 0; i < 3; i++)
      im.plane[i] = new BYTE[im.planesize[i]];
  }

  glEnable(m_textureTarget);
  for(int f = 0;f<MAX_FIELDS;f++)
  {
    for(p = 0;p<MAX_PLANES;p++)
    {
      if (!glIsTexture(fields[f][p].id))
      {
        glGenTextures(1, &fields[f][p].id);
        VerifyGLState();
      }
      fields[f][p].pbo = pbo[p];
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
        glTexImage2D(m_textureTarget, 0, GL_RGBA, plane.texwidth, plane.texheight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
      }
      else
      {
        GLenum format;
        GLint internalformat;
        if (p == 2) //V plane needs an alpha texture
        {
          format = GL_ALPHA;
          if(im.bpp == 2)
            internalformat = GL_ALPHA16;
          else
            internalformat = GL_ALPHA;
        }
        else
        {
          format = GL_LUMINANCE;
          if(im.bpp == 2)
            internalformat = GL_LUMINANCE16;
          else
            internalformat = GL_LUMINANCE;
        }

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
bool CLinuxRendererGL::UploadNV12Texture(int source)
{
  YUVBUFFER& buf    =  m_buffers[source];
  YV12Image* im     = &buf.image;
  YUVFIELDS& fields =  buf.fields;

  if (!(im->flags & IMAGE_FLAG_READY))
    return false;
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
  return true;
}

bool CLinuxRendererGL::CreateNV12Texture(int index)
{
  // since we also want the field textures, pitch must be texture aligned
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;
  GLuint    *pbo    = m_buffers[index].pbo;

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
    glGenBuffersARB(2, pbo);

    for (int i = 0; i < 2; i++)
    {
      glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo[i]);
      glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, im.planesize[i] + PBO_OFFSET, 0, GL_STREAM_DRAW_ARB);
      void* pboPtr = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
      if (pboPtr)
      {
        im.plane[i] = (BYTE*)pboPtr + PBO_OFFSET;
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
        glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo[i]);
        glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
      }
      glDeleteBuffersARB(2, pbo);
      memset(m_buffers[index].pbo, 0, sizeof(m_buffers[index].pbo));
    }

    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
  }

  if (!pboSetup)
  {
    for (int i = 0; i < 2; i++)
      im.plane[i] = new BYTE[im.planesize[i]];
  }

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
      fields[f][p].pbo = pbo[p];
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
void CLinuxRendererGL::DeleteNV12Texture(int index)
{
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;
  GLuint    *pbo    = m_buffers[index].pbo;

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
    if (pbo[p])
    {
      if (im.plane[p])
      {
        glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo[p]);
        glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
        im.plane[p] = NULL;
        glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
      }
      glDeleteBuffersARB(1, pbo + p);
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

void CLinuxRendererGL::DeleteVDPAUTexture(int index)
{
#ifdef HAVE_LIBVDPAU
  YUVPLANE &plane = m_buffers[index].fields[FIELD_FULL][0];

  SAFE_RELEASE(m_buffers[index].vdpau);

  plane.id = 0;
#endif
}

bool CLinuxRendererGL::CreateVDPAUTexture(int index)
{
#ifdef HAVE_LIBVDPAU
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;
  YUVPLANE  &plane  = fields[FIELD_FULL][0];

  DeleteVDPAUTexture(index);

  memset(&im    , 0, sizeof(im));
  memset(&fields, 0, sizeof(fields));
  im.height = m_sourceHeight;
  im.width  = m_sourceWidth;

  plane.texwidth  = im.width;
  plane.texheight = im.height;

  plane.pixpertex_x = 1;
  plane.pixpertex_y = 1;

  plane.id = 1;

#endif
  return true;
}

bool CLinuxRendererGL::UploadVDPAUTexture(int index)
{
#ifdef HAVE_LIBVDPAU
  VDPAU::CVdpauRenderPicture *vdpau = m_buffers[index].vdpau;

  unsigned int flipindex = m_buffers[index].flipindex;
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;
  YUVPLANE &plane = fields[FIELD_FULL][0];

  if (!vdpau || !vdpau->valid)
  {
    return false;
  }

  plane.id = vdpau->texture[0];

  plane.rect = m_sourceRect;
  plane.width  = im.width;
  plane.height = im.height;

  plane.height  /= plane.pixpertex_y;
  plane.rect.y1 /= plane.pixpertex_y;
  plane.rect.y2 /= plane.pixpertex_y;
  plane.width   /= plane.pixpertex_x;
  plane.rect.x1 /= plane.pixpertex_x;
  plane.rect.x2 /= plane.pixpertex_x;

  if (m_textureTarget == GL_TEXTURE_2D)
  {
    plane.height  /= plane.texheight;
    plane.rect.y1 /= plane.texheight;
    plane.rect.y2 /= plane.texheight;
    plane.width   /= plane.texwidth;
    plane.rect.x1 /= plane.texwidth;
    plane.rect.x2 /= plane.texwidth;
  }

#endif
  return true;
}

void CLinuxRendererGL::DeleteVDPAUTexture420(int index)
{
#ifdef HAVE_LIBVDPAU
  YUVFIELDS &fields = m_buffers[index].fields;

  SAFE_RELEASE(m_buffers[index].vdpau);

  fields[0][0].id = 0;
  fields[1][0].id = 0;
  fields[1][1].id = 0;
  fields[2][0].id = 0;
  fields[2][1].id = 0;

#endif
}

bool CLinuxRendererGL::CreateVDPAUTexture420(int index)
{
#ifdef HAVE_LIBVDPAU
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;
  YUVPLANE &plane = fields[0][0];
  GLuint    *pbo    = m_buffers[index].pbo;

  DeleteVDPAUTexture420(index);

  memset(&im    , 0, sizeof(im));
  memset(&fields, 0, sizeof(fields));

  im.cshift_x = 1;
  im.cshift_y = 1;

  im.plane[0] = NULL;
  im.plane[1] = NULL;
  im.plane[2] = NULL;

  for(int p=0; p<3; p++)
  {
    pbo[p] = None;
  }

  plane.id = 1;

#endif
  return true;
}

bool CLinuxRendererGL::UploadVDPAUTexture420(int index)
{
#ifdef HAVE_LIBVDPAU
  VDPAU::CVdpauRenderPicture *vdpau = m_buffers[index].vdpau;
  YV12Image &im = m_buffers[index].image;

  unsigned int flipindex = m_buffers[index].flipindex;
  YUVFIELDS &fields = m_buffers[index].fields;

  if (!vdpau || !vdpau->valid)
  {
    return false;
  }

  im.height = vdpau->texHeight;
  im.width  = vdpau->texWidth;

  // YUV
  for (int f = FIELD_TOP; f<=FIELD_BOT ; f++)
  {
    YUVPLANES &planes = fields[f];

    planes[0].texwidth  = im.width;
    planes[0].texheight = im.height >> 1;

    planes[1].texwidth  = planes[0].texwidth  >> im.cshift_x;
    planes[1].texheight = planes[0].texheight >> im.cshift_y;
    planes[2].texwidth  = planes[1].texwidth;
    planes[2].texheight = planes[1].texheight;

    for (int p = 0; p < 3; p++)
    {
      planes[p].pixpertex_x = 1;
      planes[p].pixpertex_y = 1;
    }
  }
  // crop
//  m_sourceRect.x1 += vdpau->crop.x1;
//  m_sourceRect.x2 -= vdpau->crop.x2;
//  m_sourceRect.y1 += vdpau->crop.y1;
//  m_sourceRect.y2 -= vdpau->crop.y2;

  // set textures
  fields[1][0].id = vdpau->texture[0];
  fields[1][1].id = vdpau->texture[2];
  fields[1][2].id = vdpau->texture[2];
  fields[2][0].id = vdpau->texture[1];
  fields[2][1].id = vdpau->texture[3];
  fields[2][2].id = vdpau->texture[3];

  glEnable(m_textureTarget);
  for (int f = FIELD_TOP; f <= FIELD_BOT; f++)
  {
    for (int p=0; p<2; p++)
    {
      glBindTexture(m_textureTarget,fields[f][p].id);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      glBindTexture(m_textureTarget,0);
      VerifyGLState();
    }
  }
  CalculateTextureSourceRects(index, 3);
  glDisable(m_textureTarget);

#endif
  return true;
}

void CLinuxRendererGL::DeleteVAAPITexture(int index)
{
#ifdef HAVE_LIBVA
  YUVPLANE       &plane = m_buffers[index].fields[0][0];
  VAAPI::CHolder &va    = m_buffers[index].vaapi;

  va.display.reset();
  va.surface.reset();
  va.surfglx.reset();

  if(plane.id && glIsTexture(plane.id))
    glDeleteTextures(1, &plane.id);
  plane.id = 0;

#endif
}

bool CLinuxRendererGL::CreateVAAPITexture(int index)
{
#ifdef HAVE_LIBVA
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;
  YUVPLANE  &plane  = fields[0][0];

  DeleteVAAPITexture(index);

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
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexImage2D(m_textureTarget, 0, GL_RGBA, plane.texwidth, plane.texheight, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
  glBindTexture(m_textureTarget, 0);
  glDisable(m_textureTarget);
#endif
  return true;
}

bool CLinuxRendererGL::UploadVAAPITexture(int index)
{
#ifdef HAVE_LIBVA
  YUVPLANE       &plane = m_buffers[index].fields[0][0];
  VAAPI::CHolder &va    = m_buffers[index].vaapi;
  VAStatus status;

  if(!va.surface)
    return false;

  if(va.display && va.surface->m_display != va.display)
  {
    CLog::Log(LOGDEBUG, "CLinuxRendererGL::UploadVAAPITexture - context changed %d", index);
    va.surfglx.reset();
  }
  va.display = va.surface->m_display;

  CSingleLock lock(*va.display);

  if(va.display->lost())
    return false;

  if(!va.surfglx)
  {
    CLog::Log(LOGDEBUG, "CLinuxRendererGL::UploadVAAPITexture - creating vaapi surface for texture %d", index);
    void* surface;
    status = vaCreateSurfaceGLX(va.display->get()
                              , m_textureTarget
                              , plane.id
                              , &surface);
    if(status != VA_STATUS_SUCCESS)
    {
      CLog::Log(LOGERROR, "CLinuxRendererGL::UploadVAAPITexture - failed to create vaapi glx surface (%d)", status);
      return false;
    }
    va.surfglx = VAAPI::CSurfaceGLPtr(new VAAPI::CSurfaceGL(surface, va.display));
  }
  int colorspace;
  if(CONF_FLAGS_YUVCOEF_MASK(m_iFlags) == CONF_FLAGS_YUVCOEF_BT709)
    colorspace = VA_SRC_BT709;
  else
    colorspace = VA_SRC_BT601;

  int field;
  if      (m_currentField == FIELD_TOP)
    field = VA_TOP_FIELD;
  else if (m_currentField == FIELD_BOT)
    field = VA_BOTTOM_FIELD;
  else
    field = VA_FRAME_PICTURE;

#if USE_VAAPI_GLX_BIND
  status = vaAssociateSurfaceGLX(va.display->get()
                               , va.surfglx->m_id
                               , va.surface->m_id
                               , field | colorspace);
#else
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  status = vaCopySurfaceGLX(va.display->get()
                          , va.surfglx->m_id
                          , va.surface->m_id
                          , field | colorspace);
#endif

  // when a vaapi backend is lost (vdpau), we start getting these errors
  if(status == VA_STATUS_ERROR_INVALID_SURFACE
  || status == VA_STATUS_ERROR_INVALID_DISPLAY)
  {
    va.display->lost(true);
    for(int i = 0; i < m_NumYV12Buffers; i++)
    {
      m_buffers[i].vaapi.display.reset();
      m_buffers[i].vaapi.surface.reset();
      m_buffers[i].vaapi.surfglx.reset();
    }
  }

  if(status != VA_STATUS_SUCCESS)
    CLog::Log(LOGERROR, "CLinuxRendererGL::UploadVAAPITexture - failed to copy surface to glx %d - %s", status, vaErrorStr(status));

#endif
  return true;
}

//********************************************************************************************************
// CoreVideoRef Texture creation, deletion, copying + clearing
//********************************************************************************************************
bool CLinuxRendererGL::UploadCVRefTexture(int index)
{
#ifdef TARGET_DARWIN
  CVBufferRef cvBufferRef = m_buffers[index].cvBufferRef;

  glEnable(m_textureTarget);

  if (cvBufferRef)
  {
    YUVFIELDS &fields = m_buffers[index].fields;
    YUVPLANE  &plane  = fields[0][0];

    if (Cocoa_GetOSVersion() >= 0x1074)
    {
      // 10.7.4 for Retina Macbooks on Lion breaks CGLTexImageIOSurface2D/GL_YCBCR_422_APPLE,
      // 10.8 Mountain Lion breaks CGLTexImageIOSurface2D/GL_YCBCR_422_APPLE,
      // upload the old way.
      CVPixelBufferLockBaseAddress(cvBufferRef, kCVPixelBufferLock_ReadOnly);

      GLsizei       texHeight   = CVPixelBufferGetHeight(cvBufferRef);
      size_t        rowbytes    = CVPixelBufferGetBytesPerRow(cvBufferRef);
      unsigned char *bufferBase = (unsigned char*)CVPixelBufferGetBaseAddress(cvBufferRef);

      glBindTexture(m_textureTarget, plane.id);
      glTexSubImage2D(m_textureTarget, 0, 0, 0, rowbytes/2, texHeight, GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_APPLE, bufferBase);
      glBindTexture(m_textureTarget, 0);

      CVPixelBufferUnlockBaseAddress(cvBufferRef, kCVPixelBufferLock_ReadOnly);
    }
    else
    {
      // It is the fastest way to render a CVPixelBuffer backed
      // with an IOSurface as there is no CPU -> GPU upload.
      CGLContextObj cgl_ctx  = (CGLContextObj)g_Windowing.GetCGLContextObj();
      IOSurfaceRef	surface  = CVPixelBufferGetIOSurface(cvBufferRef);
      GLsizei       texWidth = IOSurfaceGetWidth(surface);
      GLsizei       texHeight= IOSurfaceGetHeight(surface);
      OSType        format_type = CVPixelBufferGetPixelFormatType(cvBufferRef);

      glBindTexture(m_textureTarget, plane.id);

      if (format_type == kCVPixelFormatType_422YpCbCr8)
        CGLTexImageIOSurface2D(cgl_ctx, m_textureTarget, GL_RGB8,
          texWidth, texHeight, GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_APPLE, surface, 0);
      else if (format_type == kCVPixelFormatType_32BGRA)
        CGLTexImageIOSurface2D(cgl_ctx, m_textureTarget, GL_RGBA8,
          texWidth, texHeight, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, surface, 0);

      glBindTexture(m_textureTarget, 0);
    }

    CVBufferRelease(cvBufferRef);
    m_buffers[index].cvBufferRef = NULL;

    plane.flipindex = m_buffers[index].flipindex;
  }


  CalculateTextureSourceRects(index, 3);
  glDisable(m_textureTarget);

#endif
  return true;
}

void CLinuxRendererGL::DeleteCVRefTexture(int index)
{
#ifdef TARGET_DARWIN
  YUVPLANE  &plane = m_buffers[index].fields[0][0];

  if (m_buffers[index].cvBufferRef)
    CVBufferRelease(m_buffers[index].cvBufferRef);
  m_buffers[index].cvBufferRef = NULL;

  if (plane.id && glIsTexture(plane.id))
    glDeleteTextures(1, &plane.id), plane.id = 0;
#endif
}

bool CLinuxRendererGL::CreateCVRefTexture(int index)
{
#ifdef TARGET_DARWIN
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;
  YUVPLANE  &plane  = fields[0][0];

  DeleteCVRefTexture(index);

  memset(&im    , 0, sizeof(im));
  memset(&fields, 0, sizeof(fields));

  im.bpp    = 1;
  im.width  = m_sourceWidth;
  im.height = m_sourceHeight;
  im.cshift_x = 0;
  im.cshift_y = 0;

  plane.texwidth  = NP2(im.width);
  plane.texheight = NP2(im.height);
  plane.pixpertex_x = 1;
  plane.pixpertex_y = 1;

  glEnable(m_textureTarget);
  glGenTextures(1, &plane.id);
  if (Cocoa_GetOSVersion() >= 0x1074)
  {
    // 10.7.4 for Retina Macbooks on Lion breaks CGLTexImageIOSurface2D/GL_YCBCR_422_APPLE,
    // 10.8 Mountain Lion breaks CGLTexImageIOSurface2D/GL_YCBCR_422_APPLE,
    // upload the old way.
    glBindTexture(m_textureTarget, plane.id);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // This is necessary for non-power-of-two textures
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(m_textureTarget, 0, GL_RGBA, plane.texwidth, plane.texheight, 0, GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_APPLE, NULL);
    glBindTexture(m_textureTarget, 0);
  }
  glDisable(m_textureTarget);

#endif
  return true;
}

bool CLinuxRendererGL::UploadYUV422PackedTexture(int source)
{
  YUVBUFFER& buf    =  m_buffers[source];
  YV12Image* im     = &buf.image;
  YUVFIELDS& fields =  buf.fields;

  if (!(im->flags & IMAGE_FLAG_READY))
    return false;

  bool deinterlacing;
  if (m_currentField == FIELD_FULL)
    deinterlacing = false;
  else
    deinterlacing = true;

  glEnable(m_textureTarget);
  VerifyGLState();

  glPixelStorei(GL_UNPACK_ALIGNMENT,1);

  if (deinterlacing)
  {
    // Load YUYV fields
    LoadPlane( fields[FIELD_TOP][0], GL_BGRA, buf.flipindex
             , im->width / 2, im->height >> 1
             , im->stride[0] * 2, im->bpp, im->plane[0] );

    LoadPlane( fields[FIELD_BOT][0], GL_BGRA, buf.flipindex
             , im->width / 2, im->height >> 1
             , im->stride[0] * 2, im->bpp, im->plane[0] + im->stride[0]) ;
  }
  else
  {
    // Load YUYV plane
    LoadPlane( fields[FIELD_FULL][0], GL_BGRA, buf.flipindex
             , im->width / 2, im->height
             , im->stride[0], im->bpp, im->plane[0] );
  }

  VerifyGLState();

  CalculateTextureSourceRects(source, 3);

  glDisable(m_textureTarget);
  return true;
}

void CLinuxRendererGL::DeleteYUV422PackedTexture(int index)
{
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;
  GLuint    *pbo    = m_buffers[index].pbo;

  if( fields[FIELD_FULL][0].id == 0 ) return;

  // finish up all textures, and delete them
  g_graphicsContext.BeginPaint();  //FIXME
  for(int f = 0;f<MAX_FIELDS;f++)
  {
    if( fields[f][0].id )
    {
      if (glIsTexture(fields[f][0].id))
      {
        glDeleteTextures(1, &fields[f][0].id);
      }
      fields[f][0].id = 0;
    }
    fields[f][1].id = 0;
    fields[f][2].id = 0;
  }
  g_graphicsContext.EndPaint();

  if (pbo[0])
  {
    if (im.plane[0])
    {
      glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo[0]);
      glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
      im.plane[0] = NULL;
      glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    }
    glDeleteBuffersARB(1, pbo);
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
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;
  GLuint    *pbo    = m_buffers[index].pbo;

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
    glGenBuffersARB(1, pbo);

    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo[0]);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, im.planesize[0] + PBO_OFFSET, 0, GL_STREAM_DRAW_ARB);
    void* pboPtr = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    if (pboPtr)
    {
      im.plane[0] = (BYTE*)pboPtr + PBO_OFFSET;
      memset(im.plane[0], 0, im.planesize[0]);
    }
    else
    {
      CLog::Log(LOGWARNING,"GL: failed to set up pixel buffer object");
      pboSetup = false;
    }

    if (!pboSetup)
    {
      glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, *pbo);
      glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
      glDeleteBuffersARB(1, pbo);
      memset(m_buffers[index].pbo, 0, sizeof(m_buffers[index].pbo));
    }

    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
  }

  if (!pboSetup)
  {
    im.plane[0] = new BYTE[im.planesize[0]];
  }

  glEnable(m_textureTarget);
  for(int f = 0;f<MAX_FIELDS;f++)
  {
    if (!glIsTexture(fields[f][0].id))
    {
      glGenTextures(1, &fields[f][0].id);
      VerifyGLState();
    }
    fields[f][0].pbo = pbo[0];
    fields[f][1].id = fields[f][0].id;
    fields[f][2].id = fields[f][1].id;
  }

  // YUV
  for (int f = FIELD_FULL; f<=FIELD_BOT ; f++)
  {
    int fieldshift = (f==FIELD_FULL) ? 0 : 1;
    YUVPLANES &planes = fields[f];

    if (m_renderMethod & RENDER_SW)
    {
      planes[0].texwidth  = im.width;
      planes[0].texheight = im.height >> fieldshift;
      planes[1].texwidth  = 0;
      planes[1].texheight = 0;
      planes[2].texwidth  = 0;
      planes[2].texheight = 0;

      for (int p = 0; p < 3; p++)
      {
        planes[p].pixpertex_x = 1;
        planes[p].pixpertex_y = 1;
      }
    }
    else
    {
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
    }

    if(m_renderMethod & RENDER_POT)
    {
      for(int p = 0; p < 3; p++)
      {
        planes[p].texwidth  = NP2(planes[p].texwidth);
        planes[p].texheight = NP2(planes[p].texheight);
      }
    }

    YUVPLANE &plane = planes[0];
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
  glDisable(m_textureTarget);

  return true;
}

void CLinuxRendererGL::ToRGBFrame(YV12Image* im, unsigned flipIndexPlane, unsigned flipIndexBuf)
{
  if(m_rgbBufferSize != m_sourceWidth * m_sourceHeight * 4)
    SetupRGBBuffer();
  else if(flipIndexPlane == flipIndexBuf)
    return; //conversion already done on the previous iteration

  uint8_t *src[4]       = {};
  int      srcStride[4] = {};
  int      srcFormat    = -1;

  if (m_format == RENDER_FMT_YUV420P)
  {
    srcFormat = PIX_FMT_YUV420P;
    for (int i = 0; i < 3; i++)
    {
      src[i]       = im->plane[i];
      srcStride[i] = im->stride[i];
    }
  }
  else if (m_format == RENDER_FMT_NV12)
  {
    srcFormat = PIX_FMT_NV12;
    for (int i = 0; i < 2; i++)
    {
      src[i]       = im->plane[i];
      srcStride[i] = im->stride[i];
    }
  }
  else if (m_format == RENDER_FMT_YUYV422)
  {
    srcFormat    = PIX_FMT_YUYV422;
    src[0]       = im->plane[0];
    srcStride[0] = im->stride[0];
  }
  else if (m_format == RENDER_FMT_UYVY422)
  {
    srcFormat    = PIX_FMT_UYVY422;
    src[0]       = im->plane[0];
    srcStride[0] = im->stride[0];
  }
  else //should never happen
  {
    CLog::Log(LOGERROR, "CLinuxRendererGL::ToRGBFrame: called with unsupported format %i", m_format);
    return;
  }

  if (m_rgbPbo)
  {
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, m_rgbPbo);
    m_rgbBuffer = (BYTE*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB) + PBO_OFFSET;
  }

  m_context = m_dllSwScale->sws_getCachedContext(m_context,
                                                 im->width, im->height, srcFormat,
                                                 im->width, im->height, PIX_FMT_BGRA,
                                                 SWS_FAST_BILINEAR | SwScaleCPUFlags(), NULL, NULL, NULL);

  uint8_t *dst[]       = { m_rgbBuffer, 0, 0, 0 };
  int      dstStride[] = { (int)m_sourceWidth * 4, 0, 0, 0 };
  m_dllSwScale->sws_scale(m_context, src, srcStride, 0, im->height, dst, dstStride);

  if (m_rgbPbo)
  {
    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    m_rgbBuffer = (BYTE*)PBO_OFFSET;
  }
}

void CLinuxRendererGL::ToRGBFields(YV12Image* im, unsigned flipIndexPlaneTop, unsigned flipIndexPlaneBot, unsigned flipIndexBuf)
{
  if(m_rgbBufferSize != m_sourceWidth * m_sourceHeight * 4)
    SetupRGBBuffer();
  else if(flipIndexPlaneTop == flipIndexBuf && flipIndexPlaneBot == flipIndexBuf)
    return; //conversion already done on the previous iteration

  uint8_t *srcTop[4]       = {};
  int      srcStrideTop[4] = {};
  uint8_t *srcBot[4]       = {};
  int      srcStrideBot[4] = {};
  int      srcFormat       = -1;

  if (m_format == RENDER_FMT_YUV420P)
  {
    srcFormat = PIX_FMT_YUV420P;
    for (int i = 0; i < 3; i++)
    {
      srcTop[i]       = im->plane[i];
      srcStrideTop[i] = im->stride[i] * 2;
      srcBot[i]       = im->plane[i] + im->stride[i];
      srcStrideBot[i] = im->stride[i] * 2;
    }
  }
  else if (m_format == RENDER_FMT_NV12)
  {
    srcFormat = PIX_FMT_NV12;
    for (int i = 0; i < 2; i++)
    {
      srcTop[i]       = im->plane[i];
      srcStrideTop[i] = im->stride[i] * 2;
      srcBot[i]       = im->plane[i] + im->stride[i];
      srcStrideBot[i] = im->stride[i] * 2;
    }
  }
  else if (m_format == RENDER_FMT_YUYV422)
  {
    srcFormat       = PIX_FMT_YUYV422;
    srcTop[0]       = im->plane[0];
    srcStrideTop[0] = im->stride[0] * 2;
    srcBot[0]       = im->plane[0] + im->stride[0];
    srcStrideBot[0] = im->stride[0] * 2;
  }
  else if (m_format == RENDER_FMT_UYVY422)
  {
    srcFormat       = PIX_FMT_UYVY422;
    srcTop[0]       = im->plane[0];
    srcStrideTop[0] = im->stride[0] * 2;
    srcBot[0]       = im->plane[0] + im->stride[0];
    srcStrideBot[0] = im->stride[0] * 2;
  }
  else //should never happen
  {
    CLog::Log(LOGERROR, "CLinuxRendererGL::ToRGBFields: called with unsupported format %i", m_format);
    return;
  }

  if (m_rgbPbo)
  {
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, m_rgbPbo);
    m_rgbBuffer = (BYTE*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB) + PBO_OFFSET;
  }

  m_context = m_dllSwScale->sws_getCachedContext(m_context,
                                                 im->width, im->height >> 1, srcFormat,
                                                 im->width, im->height >> 1, PIX_FMT_BGRA,
                                                 SWS_FAST_BILINEAR | SwScaleCPUFlags(), NULL, NULL, NULL);
  uint8_t *dstTop[]    = { m_rgbBuffer, 0, 0, 0 };
  uint8_t *dstBot[]    = { m_rgbBuffer + m_sourceWidth * m_sourceHeight * 2, 0, 0, 0 };
  int      dstStride[] = { (int)m_sourceWidth * 4, 0, 0, 0 };

  //convert each YUV field to an RGB field, the top field is placed at the top of the rgb buffer
  //the bottom field is placed at the bottom of the rgb buffer
  m_dllSwScale->sws_scale(m_context, srcTop, srcStrideTop, 0, im->height >> 1, dstTop, dstStride);
  m_dllSwScale->sws_scale(m_context, srcBot, srcStrideBot, 0, im->height >> 1, dstBot, dstStride);

  if (m_rgbPbo)
  {
    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    m_rgbBuffer = (BYTE*)PBO_OFFSET;
  }
}

void CLinuxRendererGL::SetupRGBBuffer()
{
  m_rgbBufferSize = m_sourceWidth * m_sourceHeight * 4;

  if (!m_rgbPbo)
    delete [] m_rgbBuffer;

  if (m_pboSupported)
  {
    CLog::Log(LOGNOTICE, "GL: Using GL_ARB_pixel_buffer_object");

    if (!m_rgbPbo)
      glGenBuffersARB(1, &m_rgbPbo);

    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, m_rgbPbo);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, m_rgbBufferSize + PBO_OFFSET, 0, GL_STREAM_DRAW_ARB);
    m_rgbBuffer = (BYTE*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB) + PBO_OFFSET;

    if (!m_rgbBuffer)
    {
      CLog::Log(LOGWARNING,"GL: failed to set up pixel buffer object");
      glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
      glDeleteBuffersARB(1, &m_rgbPbo);
      m_rgbPbo = 0;
    }
    else
    {
      glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
      glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
      m_rgbBuffer = (BYTE*)PBO_OFFSET;
    }
  }

  if (!m_rgbPbo)
    m_rgbBuffer = new BYTE[m_rgbBufferSize];
}

bool CLinuxRendererGL::UploadRGBTexture(int source)
{
  YUVBUFFER& buf    =  m_buffers[source];
  YV12Image* im     = &buf.image;
  YUVFIELDS& fields =  buf.fields;

  if (!(im->flags&IMAGE_FLAG_READY))
    return false;

  bool deinterlacing;
  if (m_currentField == FIELD_FULL)
    deinterlacing = false;
  else
    deinterlacing = true;

  glEnable(m_textureTarget);
  VerifyGLState();

  if (deinterlacing)
    ToRGBFields(im, fields[FIELD_TOP][0].flipindex, fields[FIELD_BOT][0].flipindex, buf.flipindex);
  else
    ToRGBFrame(im, fields[FIELD_FULL][0].flipindex, buf.flipindex);

  static int imaging = -1;
  if (imaging==-1)
  {
    imaging = 0;
    if (glewIsSupported("GL_ARB_imaging"))
    {
      CLog::Log(LOGINFO, "GL: ARB Imaging extension supported");
      imaging = 1;
    }
    else
    {
      unsigned int maj=0, min=0;
      g_Windowing.GetRenderVersion(maj, min);
      if (maj>=2)
      {
        imaging = 1;
      }
      else if (min>=2)
      {
        imaging = 1;
      }
    }
  }

  if (imaging==1 &&
      ((CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness!=50) ||
       (CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast!=50)))
  {
    GLfloat brightness = ((GLfloat)CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness - 50.0f)/100.0f;;
    GLfloat contrast   = ((GLfloat)CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast)/50.0f;

    glPixelTransferf(GL_RED_SCALE  , contrast);
    glPixelTransferf(GL_GREEN_SCALE, contrast);
    glPixelTransferf(GL_BLUE_SCALE , contrast);
    glPixelTransferf(GL_RED_BIAS   , brightness);
    glPixelTransferf(GL_GREEN_BIAS , brightness);
    glPixelTransferf(GL_BLUE_BIAS  , brightness);
    VerifyGLState();
    imaging++;
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT,1);

  // Load RGB image
  if (deinterlacing)
  {
    LoadPlane( fields[FIELD_TOP][0] , GL_BGRA, buf.flipindex
             , im->width, im->height >> 1
             , m_sourceWidth*4, 1, m_rgbBuffer, &m_rgbPbo );

    LoadPlane( fields[FIELD_BOT][0], GL_BGRA, buf.flipindex
             , im->width, im->height >> 1
             , m_sourceWidth*4, 1, m_rgbBuffer + m_sourceWidth*m_sourceHeight*2, &m_rgbPbo );
  }
  else
  {
    LoadPlane( fields[FIELD_FULL][0], GL_BGRA, buf.flipindex
             , im->width, im->height
             , m_sourceWidth*4, 1, m_rgbBuffer, &m_rgbPbo );
  }

  //after using the pbo to upload, allocate a new buffer so we don't have to wait
  //for the upload to finish when mapping the buffer 
  if (m_rgbPbo)
  {
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, m_rgbPbo);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, m_rgbBufferSize + PBO_OFFSET, 0, GL_STREAM_DRAW_ARB);
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
  }

  if (imaging==2)
  {
    imaging--;
    glPixelTransferf(GL_RED_SCALE, 1.0);
    glPixelTransferf(GL_GREEN_SCALE, 1.0);
    glPixelTransferf(GL_BLUE_SCALE, 1.0);
    glPixelTransferf(GL_RED_BIAS, 0.0);
    glPixelTransferf(GL_GREEN_BIAS, 0.0);
    glPixelTransferf(GL_BLUE_BIAS, 0.0);
    VerifyGLState();
  }

  VerifyGLState();

  CalculateTextureSourceRects(source, 3);

  glDisable(m_textureTarget);
  return true;
}

void CLinuxRendererGL::SetTextureFilter(GLenum method)
{
  for (int i = 0 ; i<m_NumYV12Buffers ; i++)
  {
    YUVFIELDS &fields = m_buffers[i].fields;

    for (int f = FIELD_FULL; f<=FIELD_BOT ; f++)
    {
      for (int p = 0; p < 3; p++)
      {
        if(glIsTexture(fields[f][p].id))
        {
          glBindTexture(m_textureTarget, fields[f][p].id);
          glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, method);
          glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, method);
          VerifyGLState();
        }
      }
    }
  }
}

bool CLinuxRendererGL::Supports(ERENDERFEATURE feature)
{
  if(feature == RENDERFEATURE_BRIGHTNESS)
  {
    if ((m_renderMethod & RENDER_VDPAU) && !CSettings::Get().GetBool("videoscreen.limitedrange"))
      return true;

    if (m_renderMethod & RENDER_VAAPI)
      return false;

    return (m_renderMethod & RENDER_GLSL)
        || (m_renderMethod & RENDER_ARB)
        || ((m_renderMethod & RENDER_SW) && glewIsSupported("GL_ARB_imaging") == GL_TRUE);
  }
  
  if(feature == RENDERFEATURE_CONTRAST)
  {
    if ((m_renderMethod & RENDER_VDPAU) && !CSettings::Get().GetBool("videoscreen.limitedrange"))
      return true;

    if (m_renderMethod & RENDER_VAAPI)
      return false;

    return (m_renderMethod & RENDER_GLSL)
        || (m_renderMethod & RENDER_ARB)
        || ((m_renderMethod & RENDER_SW) && glewIsSupported("GL_ARB_imaging") == GL_TRUE);
  }

  if(feature == RENDERFEATURE_GAMMA)
    return false;
  
  if(feature == RENDERFEATURE_NOISE)
  {
    if(m_renderMethod & RENDER_VDPAU)
      return true;
  }

  if(feature == RENDERFEATURE_SHARPNESS)
  {
    if(m_renderMethod & RENDER_VDPAU)
      return true;
  }

  if (feature == RENDERFEATURE_NONLINSTRETCH)
  {
    if (((m_renderMethod & RENDER_GLSL) && !(m_renderMethod & RENDER_POT)) ||
        (m_renderMethod & RENDER_VDPAU) || (m_renderMethod & RENDER_VAAPI))
      return true;
  }

  if (feature == RENDERFEATURE_STRETCH         ||
      feature == RENDERFEATURE_CROP            ||
      feature == RENDERFEATURE_ZOOM            ||
      feature == RENDERFEATURE_VERTICAL_SHIFT  ||
      feature == RENDERFEATURE_PIXEL_RATIO     ||
      feature == RENDERFEATURE_POSTPROCESS     ||
      feature == RENDERFEATURE_ROTATION)
    return true;

  return false;
}

bool CLinuxRendererGL::SupportsMultiPassRendering()
{
  return glewIsSupported("GL_EXT_framebuffer_object") && glCreateProgram;
}

bool CLinuxRendererGL::Supports(EDEINTERLACEMODE mode)
{
  if(m_renderMethod & RENDER_CVREF)
    return false;

  if(mode == VS_DEINTERLACEMODE_OFF
  || mode == VS_DEINTERLACEMODE_AUTO
  || mode == VS_DEINTERLACEMODE_FORCE)
    return true;

  return false;
}

bool CLinuxRendererGL::Supports(EINTERLACEMETHOD method)
{
  if(m_renderMethod & RENDER_CVREF)
    return false;

  if(method == VS_INTERLACEMETHOD_AUTO)
    return true;

  if(m_renderMethod & RENDER_VDPAU ||
      m_format == RENDER_FMT_VDPAU_420)
  {
#ifdef HAVE_LIBVDPAU
    VDPAU::CVdpauRenderPicture *vdpauPic = m_buffers[m_iYV12RenderBuffer].vdpau;
    if(vdpauPic && vdpauPic->vdpau)
      return vdpauPic->vdpau->Supports(method);
#endif
    return false;
  }

  if(m_renderMethod & RENDER_VAAPI)
  {
#ifdef HAVE_LIBVA
    VAAPI::CDisplayPtr disp = m_buffers[m_iYV12RenderBuffer].vaapi.display;
    if(disp)
    {
      CSingleLock lock(*disp);

      if(disp->support_deinterlace())
      {
        if( method == VS_INTERLACEMETHOD_RENDER_BOB_INVERTED
        ||  method == VS_INTERLACEMETHOD_RENDER_BOB )
          return true;
      }
    }
#endif
    return false;
  }

#ifdef TARGET_DARWIN
  // YADIF too slow for HD but we have no methods to fall back
  // to something that works so just turn it off.
  if(method == VS_INTERLACEMETHOD_DEINTERLACE)
    return false;
#endif
  
  if(method == VS_INTERLACEMETHOD_DEINTERLACE
  || method == VS_INTERLACEMETHOD_DEINTERLACE_HALF
  || method == VS_INTERLACEMETHOD_SW_BLEND)
    return true;

  if((method == VS_INTERLACEMETHOD_RENDER_BLEND
  ||  method == VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED
  ||  method == VS_INTERLACEMETHOD_RENDER_WEAVE
  ||  method == VS_INTERLACEMETHOD_RENDER_BOB_INVERTED
  ||  method == VS_INTERLACEMETHOD_RENDER_BOB))
    return true;

  return false;
}

bool CLinuxRendererGL::Supports(ESCALINGMETHOD method)
{
  //nearest neighbor doesn't work on YUY2 and UYVY
  if (method == VS_SCALINGMETHOD_NEAREST &&
      m_format != RENDER_FMT_YUYV422 &&
      m_format != RENDER_FMT_UYVY422)
    return true;

  if(method == VS_SCALINGMETHOD_LINEAR
  || method == VS_SCALINGMETHOD_AUTO)
    return true;

  if(method == VS_SCALINGMETHOD_CUBIC
  || method == VS_SCALINGMETHOD_LANCZOS2
  || method == VS_SCALINGMETHOD_SPLINE36_FAST
  || method == VS_SCALINGMETHOD_LANCZOS3_FAST
  || method == VS_SCALINGMETHOD_SPLINE36
  || method == VS_SCALINGMETHOD_LANCZOS3)
  {
    // if scaling is below level, avoid hq scaling
    float scaleX = fabs(((float)m_sourceWidth - m_destRect.Width())/m_sourceWidth)*100;
    float scaleY = fabs(((float)m_sourceHeight - m_destRect.Height())/m_sourceHeight)*100;
    int minScale = CSettings::Get().GetInt("videoplayer.hqscalers");
    if (scaleX < minScale && scaleY < minScale)
      return false;

    if ((glewIsSupported("GL_EXT_framebuffer_object") && (m_renderMethod & RENDER_GLSL)) ||
        (m_renderMethod & RENDER_VDPAU) || (m_renderMethod & RENDER_VAAPI))
    {
      // spline36 and lanczos3 are only allowed through advancedsettings.xml
      if(method != VS_SCALINGMETHOD_SPLINE36
      && method != VS_SCALINGMETHOD_LANCZOS3)
        return true;
      else
        return g_advancedSettings.m_videoEnableHighQualityHwScalers;
    }
  }
 
  return false;
}

EINTERLACEMETHOD CLinuxRendererGL::AutoInterlaceMethod()
{
  if(m_renderMethod & RENDER_CVREF)
    return VS_INTERLACEMETHOD_NONE;

  if(m_renderMethod & RENDER_VDPAU)
    return VS_INTERLACEMETHOD_NONE;

  if(Supports(VS_INTERLACEMETHOD_RENDER_BOB))
    return VS_INTERLACEMETHOD_RENDER_BOB;

  return VS_INTERLACEMETHOD_NONE;
}

void CLinuxRendererGL::BindPbo(YUVBUFFER& buff)
{
  bool pbo = false;
  for(int plane = 0; plane < MAX_PLANES; plane++)
  {
    if(!buff.pbo[plane] || buff.image.plane[plane] == (BYTE*)PBO_OFFSET)
      continue;
    pbo = true;

    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, buff.pbo[plane]);
    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
    buff.image.plane[plane] = (BYTE*)PBO_OFFSET;
  }
  if(pbo)
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
}

void CLinuxRendererGL::UnBindPbo(YUVBUFFER& buff)
{
  bool pbo = false;
  for(int plane = 0; plane < MAX_PLANES; plane++)
  {
    if(!buff.pbo[plane] || buff.image.plane[plane] != (BYTE*)PBO_OFFSET)
      continue;
    pbo = true;

    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, buff.pbo[plane]);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, buff.image.planesize[plane] + PBO_OFFSET, NULL, GL_STREAM_DRAW_ARB);
    buff.image.plane[plane] = (BYTE*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB) + PBO_OFFSET;
  }
  if(pbo)
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
}

unsigned int CLinuxRendererGL::GetProcessorSize()
{
  if(m_format == RENDER_FMT_VDPAU
  || m_format == RENDER_FMT_VDPAU_420
  || m_format == RENDER_FMT_VAAPI
  || m_format == RENDER_FMT_CVBREF)
    return 1;
  else
    return 0;
}

#ifdef HAVE_LIBVDPAU
void CLinuxRendererGL::AddProcessor(VDPAU::CVdpauRenderPicture *vdpau, int index)
{
  YUVBUFFER &buf = m_buffers[index];
  VDPAU::CVdpauRenderPicture *pic = vdpau->Acquire();
  SAFE_RELEASE(buf.vdpau);
  buf.vdpau = pic;
}
#endif

#ifdef HAVE_LIBVA
void CLinuxRendererGL::AddProcessor(VAAPI::CHolder& holder, int index)
{
  YUVBUFFER &buf = m_buffers[index];
  buf.vaapi.surface = holder.surface;
}
#endif

#ifdef TARGET_DARWIN
void CLinuxRendererGL::AddProcessor(struct __CVBuffer *cvBufferRef, int index)
{
  YUVBUFFER &buf = m_buffers[index];
  if (buf.cvBufferRef)
    CVBufferRelease(buf.cvBufferRef);
  buf.cvBufferRef = cvBufferRef;
  // retain another reference, this way dvdplayer and renderer can issue releases.
  CVBufferRetain(buf.cvBufferRef);
}
#endif

#endif
