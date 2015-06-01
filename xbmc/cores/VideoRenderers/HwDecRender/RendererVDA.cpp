/*
 *      Copyright (C) 2007-2015 Team XBMC
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

#include "RendererVDA.h"

#if defined(TARGET_DARWIN_OSX)

#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "cores/dvdplayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "utils/log.h"
#include "osx/CocoaInterface.h"
#include <CoreVideo/CoreVideo.h>
#include <OpenGL/CGLIOSurface.h>
#include "windowing/WindowingFactory.h"

CRendererVDA::CRendererVDA()
{

}

CRendererVDA::~CRendererVDA()
{

}

void CRendererVDA::AddVideoPictureHW(DVDVideoPicture &picture, int index)
{
  YUVBUFFER &buf = m_buffers[index];
  if (buf.hwDec)
    CVBufferRelease((struct __CVBuffer *)buf.hwDec);
  buf.hwDec = picture.cvBufferRef;
  // retain another reference, this way dvdplayer and renderer can issue releases.
  CVBufferRetain(picture.cvBufferRef);
}

void CRendererVDA::ReleaseBuffer(int idx)
{
  YUVBUFFER &buf = m_buffers[idx];
  if (buf.hwDec)
    CVBufferRelease((struct __CVBuffer *)buf.hwDec);
  buf.hwDec = NULL;
}


bool CRendererVDA::Supports(EINTERLACEMETHOD method)
{
  return false;
}

bool CRendererVDA::Supports(EDEINTERLACEMODE mode)
{
  return false;
}

EINTERLACEMETHOD CRendererVDA::AutoInterlaceMethod()
{
  return VS_INTERLACEMETHOD_NONE;
}

bool CRendererVDA::LoadShadersHook()
{
  CLog::Log(LOGNOTICE, "GL: Using CVBREF render method");
  // m_renderMethod = RENDER_CVREF;
  m_textureTarget = GL_TEXTURE_RECTANGLE_ARB;
  return false;
}

bool CRendererVDA::CreateTexture(int index)
{
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;
  YUVPLANE  &plane  = fields[0][0];

  DeleteTexture(index);

  memset(&im    , 0, sizeof(im));
  memset(&fields, 0, sizeof(fields));

  im.bpp    = 1;
  im.width  = m_sourceWidth;
  im.height = m_sourceHeight;
  im.cshift_x = 0;
  im.cshift_y = 0;

  plane.pixpertex_x = 2;
  plane.pixpertex_y = 1;
  plane.texwidth    = im.width  / plane.pixpertex_x;
  plane.texheight   = im.height / plane.pixpertex_y;

  if(m_renderMethod & RENDER_POT)
  {
    plane.texwidth  = NP2(plane.texwidth);
    plane.texheight = NP2(plane.texheight);
  }

  glEnable(m_textureTarget);
  glGenTextures(1, &plane.id);
  glDisable(m_textureTarget);

  return true;
}

void CRendererVDA::DeleteTexture(int index)
{
  YUVPLANE  &plane = m_buffers[index].fields[0][0];

  if (m_buffers[index].hwDec)
    CVBufferRelease((struct __CVBuffer *)m_buffers[index].hwDec);
  m_buffers[index].hwDec = NULL;

  if (plane.id && glIsTexture(plane.id))
    glDeleteTextures(1, &plane.id), plane.id = 0;
}

bool CRendererVDA::UploadTexture(int index)
{
  YUVBUFFER &buf    = m_buffers[index];
  YUVFIELDS &fields = buf.fields;

  CVBufferRef cvBufferRef = (struct __CVBuffer *)m_buffers[index].hwDec;

  glEnable(m_textureTarget);

  if (cvBufferRef && fields[m_currentField][0].flipindex != buf.flipindex)
  {

    // It is the fastest way to render a CVPixelBuffer backed
    // with an IOSurface as there is no CPU -> GPU upload.
    CGLContextObj cgl_ctx  = (CGLContextObj)g_Windowing.GetCGLContextObj();
    IOSurfaceRef	surface  = CVPixelBufferGetIOSurface(cvBufferRef);
    GLsizei       texWidth = IOSurfaceGetWidth(surface);
    GLsizei       texHeight= IOSurfaceGetHeight(surface);
    OSType        format_type = IOSurfaceGetPixelFormat(surface);

    glBindTexture(m_textureTarget, fields[FIELD_FULL][0].id);

    if (format_type == kCVPixelFormatType_422YpCbCr8)
      CGLTexImageIOSurface2D(cgl_ctx, m_textureTarget, GL_RGBA8,
                             texWidth / 2, texHeight, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, surface, 0);
    else if (format_type == kCVPixelFormatType_32BGRA)
      CGLTexImageIOSurface2D(cgl_ctx, m_textureTarget, GL_RGBA8,
                             texWidth, texHeight, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, surface, 0);

    glBindTexture(m_textureTarget, 0);
    fields[FIELD_FULL][0].flipindex = buf.flipindex;

  }


  CalculateTextureSourceRects(index, 3);
  glDisable(m_textureTarget);

  return true;
}

#endif