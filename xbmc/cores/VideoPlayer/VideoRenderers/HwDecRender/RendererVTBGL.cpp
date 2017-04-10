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

#include "RendererVTBGL.h"

#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "utils/log.h"
#include "platform/darwin/osx/CocoaInterface.h"
#include <CoreVideo/CoreVideo.h>
#include <OpenGL/CGLIOSurface.h>
#include "windowing/WindowingFactory.h"

struct CVTBData
{
  struct __CVBuffer* m_vtbbuf;
  GLuint m_fence;
};

CRendererVTB::CRendererVTB()
{
}

CRendererVTB::~CRendererVTB()
{
  for (int i = 0; i < NUM_BUFFERS; ++i)
  {
    DeleteTexture(i);
  }
}

CRenderInfo CRendererVTB::GetRenderInfo()
{
  CRenderInfo info;
  info.formats = m_formats;
  info.max_buffer_size = NUM_BUFFERS;
  info.optimal_buffer_size = 5;
  return info;
}

void CRendererVTB::AddVideoPicture(VideoPicture &picture, int index)
{
  YUVBUFFER &buf = m_buffers[index];
  CVTBData *vtbdata = (CVTBData*)buf.hwDec;
  if (vtbdata->m_vtbbuf)
  {
    CVBufferRelease(vtbdata->m_vtbbuf);
  }
  CVPixelBufferRef cvref = static_cast<CVPixelBufferRef>(picture.hwPic);
  vtbdata->m_vtbbuf = cvref;

  // retain another reference, this way VideoPlayer and renderer can issue releases.
  CVBufferRetain(cvref);
}

void CRendererVTB::ReleaseBuffer(int idx)
{
  YUVBUFFER &buf = m_buffers[idx];
  if (buf.hwDec)
  {
    CVTBData *vtbdata = (CVTBData*)buf.hwDec;
    if (vtbdata->m_vtbbuf)
    {
      CVBufferRelease(vtbdata->m_vtbbuf);
      vtbdata->m_vtbbuf = nullptr;
    }

    if (vtbdata->m_fence && glIsFenceAPPLE(vtbdata->m_fence))
    {
      glDeleteFencesAPPLE(1, &vtbdata->m_fence);
      vtbdata->m_fence = 0;
    }
  }
}

EShaderFormat CRendererVTB::GetShaderFormat(ERenderFormat renderFormat)
{
  return SHADER_YV12;
}

bool CRendererVTB::LoadShadersHook()
{
  CLog::Log(LOGNOTICE, "GL: Using CVBREF render method");
  m_textureTarget = GL_TEXTURE_RECTANGLE_ARB;
  return false;
}

bool CRendererVTB::CreateTexture(int index)
{
  YuvImage &im = m_buffers[index].image;
  YUVPLANE (&planes)[YuvImage::MAX_PLANES] = m_buffers[index].fields[0];

  DeleteTexture(index);

  memset(&im    , 0, sizeof(im));
  memset(&planes, 0, sizeof(YUVPLANE[YuvImage::MAX_PLANES]));

  im.bpp    = 1;
  im.width  = m_sourceWidth;
  im.height = m_sourceHeight;
  im.cshift_x = 1;
  im.cshift_y = 1;

  planes[0].texwidth  = im.width;
  planes[0].texheight = im.height;

  planes[1].texwidth  = planes[0].texwidth >> im.cshift_x;
  planes[1].texheight = planes[0].texheight >> im.cshift_y;
  planes[2].texwidth  = planes[1].texwidth;
  planes[2].texheight = planes[1].texheight;

  for (int p = 0; p < 3; p++)
  {
    planes[p].pixpertex_x = 1;
    planes[p].pixpertex_y = 1;
  }

  glEnable(m_textureTarget);
  glGenTextures(1, &planes[0].id);
  glGenTextures(1, &planes[1].id);
  planes[2].id = planes[1].id;
  glDisable(m_textureTarget);

  CVTBData *data = new CVTBData();
  data->m_fence = 0;
  data->m_vtbbuf = nullptr;
  m_buffers[index].hwDec = data;

  return true;
}

void CRendererVTB::DeleteTexture(int index)
{
  YUVPLANE (&planes)[YuvImage::MAX_PLANES] = m_buffers[index].fields[0];

  ReleaseBuffer(index);
  delete (CVTBData*)m_buffers[index].hwDec;
  m_buffers[index].hwDec = nullptr;

  if (planes[0].id && glIsTexture(planes[0].id))
  {
    glDeleteTextures(1, &planes[0].id);
  }
  if (planes[1].id && glIsTexture(planes[1].id))
  {
    glDeleteTextures(1, &planes[1].id);
  }
  planes[0].id = 0;
  planes[1].id = 0;
  planes[2].id = 0;
}

bool CRendererVTB::UploadTexture(int index)
{
  YUVBUFFER &buf = m_buffers[index];
  YUVPLANE (&planes)[YuvImage::MAX_PLANES] = m_buffers[index].fields[0];
  CVTBData *vtbdata = (CVTBData*)m_buffers[index].hwDec;

  CVImageBufferRef cvBufferRef = vtbdata->m_vtbbuf;

  glEnable(m_textureTarget);

  // It is the fastest way to render a CVPixelBuffer backed
  // with an IOSurface as there is no CPU -> GPU upload.
  CGLContextObj cgl_ctx  = (CGLContextObj)g_Windowing.GetCGLContextObj();
  IOSurfaceRef surface  = CVPixelBufferGetIOSurface(cvBufferRef);
  OSType format_type = IOSurfaceGetPixelFormat(surface);

  if (format_type != kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange)
  {
    return false;
  }

  GLsizei surfplanes = IOSurfaceGetPlaneCount(surface);

  if (surfplanes != 2)
  {
    return false;
  }

  GLsizei widthY = IOSurfaceGetWidthOfPlane(surface, 0);
  GLsizei widthUV = IOSurfaceGetWidthOfPlane(surface, 1);
  GLsizei heightY = IOSurfaceGetHeightOfPlane(surface, 0);
  GLsizei heightUV = IOSurfaceGetHeightOfPlane(surface, 1);

  glBindTexture(m_textureTarget, planes[0].id);

  CGLTexImageIOSurface2D(cgl_ctx, m_textureTarget, GL_LUMINANCE,
                         widthY, heightY, GL_LUMINANCE, GL_UNSIGNED_BYTE, surface, 0);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBindTexture(m_textureTarget, planes[1].id);

  CGLTexImageIOSurface2D(cgl_ctx, m_textureTarget, GL_LUMINANCE_ALPHA,
                         widthUV, heightUV, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, surface, 1);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBindTexture(m_textureTarget, 0);

  glDisable(m_textureTarget);

  CalculateTextureSourceRects(index, 3);

  return true;
}

void CRendererVTB::AfterRenderHook(int idx)
{
  CVTBData *vtbdata = (CVTBData*)m_buffers[idx].hwDec;
  if (vtbdata->m_fence && glIsFenceAPPLE(vtbdata->m_fence))
  {
    glDeleteFencesAPPLE(1, &vtbdata->m_fence);
  }
  glGenFencesAPPLE(1, &vtbdata->m_fence);
  glSetFenceAPPLE(vtbdata->m_fence);
}

bool CRendererVTB::NeedBuffer(int idx)
{
  CVTBData *vtbdata = (CVTBData*)m_buffers[idx].hwDec;
  if (!vtbdata)
    return false;

  if (vtbdata->m_fence && glIsFenceAPPLE(vtbdata->m_fence))
  {
    if (!glTestFenceAPPLE(vtbdata->m_fence))
      return true;
  }

  return false;
}
