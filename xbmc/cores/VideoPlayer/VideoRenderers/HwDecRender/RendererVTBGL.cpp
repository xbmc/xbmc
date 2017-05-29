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
#include "cores/VideoPlayer/DVDCodecs/Video/VTB.h"
#include "utils/log.h"
#include "platform/darwin/osx/CocoaInterface.h"
#include <CoreVideo/CoreVideo.h>
#include <OpenGL/CGLIOSurface.h>
#include "windowing/WindowingFactory.h"

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

bool CRendererVTB::HandlesVideoBuffer(CVideoBuffer *buffer)
{
  VTB::CVideoBufferVTB *vb = dynamic_cast<VTB::CVideoBufferVTB*>(buffer);
  if (vb)
    return true;

  return false;
}

void CRendererVTB::ReleaseBuffer(int idx)
{
  YUVBUFFER &buf = m_buffers[idx];
  if (buf.videoBuffer)
  {
    VTB::CVideoBufferVTB *vb = dynamic_cast<VTB::CVideoBufferVTB*>(buf.videoBuffer);
    if (vb)
    {
      if (vb->m_fence && glIsFenceAPPLE(vb->m_fence))
      {
        glDeleteFencesAPPLE(1, &vb->m_fence);
        vb->m_fence = 0;
      }
    }
    vb->Release();
    buf.videoBuffer = nullptr;
  }
}

EShaderFormat CRendererVTB::GetShaderFormat()
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
  YUVBUFFER &buf = m_buffers[index];
  YuvImage &im = buf.image;
  YUVPLANE (&planes)[YuvImage::MAX_PLANES] = buf.fields[0];

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

  return true;
}

void CRendererVTB::DeleteTexture(int index)
{
  YUVPLANE (&planes)[YuvImage::MAX_PLANES] = m_buffers[index].fields[0];

  ReleaseBuffer(index);

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

  VTB::CVideoBufferVTB *vb = dynamic_cast<VTB::CVideoBufferVTB*>(buf.videoBuffer);
  if (!vb)
  {
    return false;
  }

  CVImageBufferRef cvBufferRef = vb->GetPB();

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
  YUVBUFFER &buf = m_buffers[idx];
  VTB::CVideoBufferVTB *vb = dynamic_cast<VTB::CVideoBufferVTB*>(buf.videoBuffer);
  if (!vb)
  {
    return;
  }

  if (vb->m_fence && glIsFenceAPPLE(vb->m_fence))
  {
    glDeleteFencesAPPLE(1, &vb->m_fence);
  }
  glGenFencesAPPLE(1, &vb->m_fence);
  glSetFenceAPPLE(vb->m_fence);
}

bool CRendererVTB::NeedBuffer(int idx)
{
  YUVBUFFER &buf = m_buffers[idx];
  VTB::CVideoBufferVTB *vb = dynamic_cast<VTB::CVideoBufferVTB*>(buf.videoBuffer);
  if (!vb)
  {
    return false;
  }

  if (vb->m_fence && glIsFenceAPPLE(vb->m_fence))
  {
    if (!glTestFenceAPPLE(vb->m_fence))
      return true;
  }

  return false;
}
