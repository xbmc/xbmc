/*
* XBMC Media Center
* Linux OpenGL Renderer for ATI cards
* Copyright (c) 2007 Frodo/jcmarshall/vulkanr/d4rk
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "stdafx.h"
#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#ifndef HAS_SDL_2D
#include "LinuxRendererATI.h"
#include "../../Application.h"
#include "../../Util.h"
#include "../../Settings.h"
#include "../../XBVideoConfig.h"
#include "../../../guilib/Surface.h"

#ifdef HAS_SDL_OPENGL

using namespace Surface;

CLinuxRendererATI::CLinuxRendererATI(bool enableshaders):CLinuxRendererGL()
{
  if (enableshaders)
    m_renderMethod = RENDER_GLSL;
  else
    m_renderMethod = RENDER_SW;
}

CLinuxRendererATI::~CLinuxRendererATI()
{
}

bool CLinuxRendererATI::ValidateRenderTarget()
{
  if (!m_pBuffer)
  {
    CLog::Log(LOGNOTICE, "GL: Selected ATI Mode");
    m_pBuffer = new CSurface(g_graphicsContext.getScreenSurface());
  }
  return true;
}


void CLinuxRendererATI::ReleaseImage(int source, bool preserve)
{
  // Eventual FIXME
  if (source!=0)
    source=0;

  m_image[source].flags = 0;

  YV12Image &im = m_image[source];

  m_image[source].flags &= ~IMAGE_FLAG_INUSE;
  m_image[source].flags = 0;

  // if we don't have a shader, fallback to SW YUV2RGB for now
  if (m_renderMethod & RENDER_SW)
  {
#ifdef HAS_DVD_SWSCALE
    struct SwsContext *context = m_dllSwScale.sws_getContext(im.width, im.height, PIX_FMT_YUV420P, im.width, im.height, PIX_FMT_BGRA, SWS_BILINEAR, NULL, NULL, NULL);
    uint8_t *src[] = { im.plane[0], im.plane[1], im.plane[2] };
    int     srcStride[] = { im.stride[0], im.stride[1], im.stride[2] };
    uint8_t *dst[] = { m_rgbBuffer, 0, 0 };
    int     dstStride[] = { m_iSourceWidth*4, 0, 0 };
    m_dllSwScale.sws_scale(context, src, srcStride, 0, im.height, dst, dstStride);

    m_dllSwScale.sws_freeContext(context);
#endif
  }
}


void CLinuxRendererATI::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  static bool firsttime = true;
  const int source = 0;
  static int imaging = -1;
  static GLfloat brightness = 0.0f;
  static GLfloat contrast   = 0.0f;

  brightness =  ((GLfloat)g_settings.m_currentVideoSettings.m_Brightness - 50.0f)/100.0f;
  contrast =  ((GLfloat)g_settings.m_currentVideoSettings.m_Contrast)/50.0f;


  ManageDisplay();
  ManageTextures();

  g_graphicsContext.BeginPaint();

  m_image[source].flags = 0;

  YV12Image &im = m_image[source];
  YUVFIELDS &fields = m_YUVTexture[source];

  m_image[source].flags &= ~IMAGE_FLAG_INUSE;
  m_image[source].flags = 0;

  glEnable(m_textureTarget);

  if (!glIsTexture(fields[0][0])) //(firsttime)
  {
    firsttime = false;
    if (m_renderMethod & RENDER_GLSL)
      LoadShaders();
    CreateYV12Texture(0, false);
  }

  VerifyGLState();

  if (m_renderMethod & RENDER_SW) {
    if (imaging==-1)
    {
      imaging = 0;
      if (glewIsSupported("GL_ARB_imaging"))
      {
	CLog::Log(LOGINFO, "GL: ARB Imaging extension supported");
	imaging = 1;
      }
      else {
	int maj=0, min=0;
	g_graphicsContext.getScreenSurface()->GetGLVersion(maj, min);
	if (maj>=2)
	{
	  imaging = 1;
	} else if (min>=2) {
	  imaging = 1;
	}
      }
    }
    if (imaging)
    {
      glPixelTransferf(GL_RED_SCALE, contrast);
      glPixelTransferf(GL_GREEN_SCALE, contrast);
      glPixelTransferf(GL_BLUE_SCALE, contrast);
      glPixelTransferf(GL_RED_BIAS, brightness);
      glPixelTransferf(GL_GREEN_BIAS, brightness);
      glPixelTransferf(GL_BLUE_BIAS, brightness);
      VerifyGLState();
    }
  }

  glBindTexture(m_textureTarget, fields[0][0]);
  VerifyGLState();
#ifdef HAS_DVD_SWSCALE
  if (m_renderMethod & RENDER_SW)
    glTexSubImage2D(m_textureTarget, 0, 0, 0, im.width, im.height, GL_BGRA, GL_UNSIGNED_BYTE, m_rgbBuffer);
  else
#endif
    glTexSubImage2D(m_textureTarget, 0, 0, 0, im.width, im.height, GL_LUMINANCE, GL_UNSIGNED_BYTE, im.plane[0]);
  VerifyGLState();
  if (m_renderMethod & RENDER_GLSL)
  {
    glBindTexture(m_textureTarget, fields[0][1]);
    VerifyGLState();
    glTexSubImage2D(m_textureTarget, 0, 0, 0, im.width/2, im.height/2, GL_LUMINANCE, GL_UNSIGNED_BYTE, im.plane[1]);
    VerifyGLState();
    glBindTexture(m_textureTarget, fields[0][2]);
    VerifyGLState();
    glTexSubImage2D(m_textureTarget, 0, 0, 0, im.width/2, im.height/2, GL_LUMINANCE, GL_UNSIGNED_BYTE, im.plane[2]);
    VerifyGLState();
  }

  if (imaging)
  {
    glPixelTransferf(GL_RED_SCALE, 1.0);
    glPixelTransferf(GL_GREEN_SCALE, 1.0);
    glPixelTransferf(GL_BLUE_SCALE, 1.0);
    glPixelTransferf(GL_RED_BIAS, 0.0);
    glPixelTransferf(GL_GREEN_BIAS, 0.0);
    glPixelTransferf(GL_BLUE_BIAS, 0.0);
    VerifyGLState();
  }

  if (clear)
  {
    glClearColor((int)m_clearColour&0xff000000,
		 (int)m_clearColour&0x00ff0000,
		 (int)m_clearColour&0x0000ff00,
		 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0,0,0,0);
    if (alpha<255)
    {
#ifdef  __GNUC__
// FIXME: Alpha blending currently disabled
#endif
      //glDisable(GL_BLEND);
    } else {
      //glDisable(GL_BLEND);
    }
  }
  glDisable(GL_BLEND);
  Render(flags,source);
  VerifyGLState();
  glEnable(GL_BLEND);
  g_graphicsContext.EndPaint();
}

void CLinuxRendererATI::FlipPage(int source)
{
  CLog::Log(LOGNOTICE, "Calling ATI FlipPage");
  m_iYV12RenderBuffer = source;

  if( !m_OSDRendered )
    m_OSDWidth = m_OSDHeight = 0;

  m_OSDRendered = false;

  return;
}



unsigned int CLinuxRendererATI::PreInit()
{
  CSingleLock lock(g_graphicsContext);
  m_bConfigured = false;
  UnInit();
  m_iResolution = PAL_4x3;

  m_iOSDRenderBuffer = 0;
  m_iYV12RenderBuffer = 0;
  m_NumOSDBuffers = 0;
  m_NumYV12Buffers = 1;
  m_OSDHeight = m_OSDWidth = 0;
  m_OSDRendered = false;

  m_iOSDTextureWidth = 0;
  m_iOSDTextureHeight[0] = 0;
  m_iOSDTextureHeight[1] = 0;

  // setup the background colour
  m_clearColour = 0 ; //(g_advancedSettings.m_videoBlackBarColour & 0xff) * 0x010101;

  // make sure we have a valid context that supports rendering
  if (!ValidateRenderTarget())
    return false;

#ifdef HAS_DVD_SWSCALE
  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load() || !m_dllSwScale.Load())
#else
  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load())
#endif
    CLog::Log(LOGERROR,"CLinuxRendererATI::PreInit - failed to load rescale libraries!");

  #if (! defined USE_EXTERNAL_FFMPEG) && (defined HAS_DVD_SWSCALE)
    m_dllSwScale.sws_rgb2rgb_init(SWS_CPU_CAPS_MMX2);
  #elif ((defined HAVE_LIBSWSCALE_RGB2RGB_H) || (defined HAVE_FFMPEG_RGB2RGB_H)) && (defined HAS_DVD_SWSCALE)
    m_dllSwScale.sws_rgb2rgb_init(SWS_CPU_CAPS_MMX2);
  #endif
  return true;
}

void CLinuxRendererATI::UnInit()
{
  CSingleLock lock(g_graphicsContext);

  // YV12 textures, subtitle and osd stuff
  for (int i = 0; i < NUM_BUFFERS; ++i)
  {
    DeleteYV12Texture(i);
    DeleteOSDTextures(i);
  }

  if (m_pBuffer)
  {
    delete m_pBuffer;
    m_pBuffer = 0;
  }

  if (m_rgbBuffer != NULL)
  {
    delete [] m_rgbBuffer;
    m_rgbBuffer = NULL;
  }

}

bool CLinuxRendererATI::CreateYV12Texture(int index, bool clear)
{
  /* since we also want the field textures, pitch must be texture aligned */
  unsigned p;

  YV12Image &im = m_image[index];
  YUVFIELDS &fields = m_YUVTexture[index];

  if (clear)
  {
    DeleteYV12Texture(index);

    im.height = m_iSourceHeight;
    im.width = m_iSourceWidth;

    im.stride[0] = m_iSourceWidth;
    im.stride[1] = m_iSourceWidth/2;
    im.stride[2] = m_iSourceWidth/2;
    im.plane[0] = new BYTE[m_iSourceWidth * m_iSourceHeight];
    im.plane[1] = new BYTE[(m_iSourceWidth/2) * (m_iSourceHeight/2)];
    im.plane[2] = new BYTE[(m_iSourceWidth/2) * (m_iSourceHeight/2)];

    im.cshift_x = 1;
    im.cshift_y = 1;
    im.texcoord_x = 1.0;
    im.texcoord_y = 1.0;
    return true;
  }

  g_graphicsContext.BeginPaint(m_pBuffer);

  glEnable(m_textureTarget);
  for(int f = 0;f<MAX_FIELDS;f++)
  {
    for(p = 0;p<MAX_PLANES;p++)
    {
      if (!glIsTexture(fields[f][p]))
      {
	glGenTextures(1, &fields[f][p]);
	VerifyGLState();
      }
    }
  }

  // YUV
  p = 0;
  glBindTexture(m_textureTarget, fields[0][0]);
  if (m_renderMethod & RENDER_SW)
  {
    // require Power Of Two textures?
    if (m_renderMethod & RENDER_POT)
    {
      static unsigned long np2x = 0, np2y = 0;
      np2x = NP2(im.width);
      np2y = NP2(im.height);
      CLog::Log(LOGNOTICE, "GL: Creating power of two texture of size %ld x %ld", np2x, np2y);
      glTexImage2D(m_textureTarget, 0, GL_RGBA, np2x, np2y, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
      im.texcoord_x = ((float)im.width / (float)np2x);
      im.texcoord_y = ((float)im.height / (float)np2y);
    }
    else
    {
      glTexImage2D(m_textureTarget, 0, GL_RGBA, im.width, im.height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
    }
  }
  else
    glTexImage2D(m_textureTarget, 0, GL_LUMINANCE, im.width, im.height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);
  VerifyGLState();

  if (m_renderMethod & RENDER_GLSL)
  {
    glBindTexture(m_textureTarget, fields[0][1]);
    glTexImage2D(m_textureTarget, 0, GL_LUMINANCE, im.width/2, im.height/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);
    VerifyGLState();

    glBindTexture(m_textureTarget, fields[0][2]);
    glTexImage2D(m_textureTarget, 0, GL_LUMINANCE, im.width/2, im.height/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);
    VerifyGLState();
  }

  g_graphicsContext.EndPaint(m_pBuffer);
  return true;
}

#endif

#endif

