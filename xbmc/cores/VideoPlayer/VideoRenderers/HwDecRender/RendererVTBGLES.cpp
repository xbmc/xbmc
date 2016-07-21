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

#include "RendererVTBGLES.h"

#if defined(TARGET_DARWIN_IOS)
#include "cores/IPlayer.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "settings/MediaSettings.h"
#include "windowing/WindowingFactory.h"
#include "platform/darwin/DarwinUtils.h"
#include <CoreVideo/CVBuffer.h>
#include <CoreVideo/CVPixelBuffer.h>

CRendererVTB::CRendererVTB()
{
  m_textureCache = nullptr;
  CVReturn ret = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault,
                                              NULL,
                                              g_Windowing.GetEAGLContextObj(),
                                              NULL,
                                              &m_textureCache);
  if (ret != kCVReturnSuccess)
  {
    CLog::Log(LOGERROR, "CRendererVTB::CRendererVTB - Error creating texture cache (err: %d)", ret);
  }

  for (auto &buf : m_vtbBuffers)
  {
    buf.m_textureY = nullptr;
    buf.m_textureUV = nullptr;
    buf.m_videoBuffer = nullptr;
  }
}

CRendererVTB::~CRendererVTB()
{
  if (m_textureCache)
    CFRelease(m_textureCache);
}

void CRendererVTB::AddVideoPictureHW(DVDVideoPicture &picture, int index)
{
  CRenderBuffer &buf = m_vtbBuffers[index];
  if (buf.m_videoBuffer)
    CVBufferRelease(buf.m_videoBuffer);
  buf.m_videoBuffer = picture.cvBufferRef;
  // retain another reference, this way VideoPlayer and renderer can issue releases.
  CVBufferRetain(picture.cvBufferRef);
}

void CRendererVTB::ReleaseBuffer(int idx)
{
  CRenderBuffer &buf = m_vtbBuffers[idx];
  if (buf.m_videoBuffer)
    CVBufferRelease(buf.m_videoBuffer);
  buf.m_videoBuffer = nullptr;
}

int CRendererVTB::GetImageHook(YV12Image *image, int source, bool readonly)
{
  return source;
}

bool CRendererVTB::Supports(EINTERLACEMETHOD method)
{
  return false;
}

EINTERLACEMETHOD CRendererVTB::AutoInterlaceMethod()
{
  return VS_INTERLACEMETHOD_NONE;
}

CRenderInfo CRendererVTB::GetRenderInfo()
{
  CRenderInfo info;
  info.formats = m_formats;
  info.max_buffer_size = NUM_BUFFERS;
  info.optimal_buffer_size = 4;
  return info;
}

bool CRendererVTB::LoadShadersHook()
{
  float ios_version = CDarwinUtils::GetIOSVersion();
  CLog::Log(LOGNOTICE, "GL: Using CVBREF render method");
  m_textureTarget = GL_TEXTURE_2D;
  m_renderMethod = RENDER_CVREF;

  if (ios_version < 5.0)
  {
    CLog::Log(LOGNOTICE, "GL: ios version < 5 is not supported");
    return false;
  }

  if (!m_textureCache)
  {
    CLog::Log(LOGNOTICE, "CRendererVTB::LoadShadersHook: no texture cache");
    return false;
  }

  CVReturn ret = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault,
                                              NULL,
                                              g_Windowing.GetEAGLContextObj(),
                                              NULL,
                                              &m_textureCache);
  if (ret != kCVReturnSuccess)
    return false;

  return false;
}

bool CRendererVTB::CreateTexture(int index)
{
  YV12Image &im = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;
  YUVPLANES &planes = fields[0];
  
  DeleteTexture(index);
  
  memset(&im    , 0, sizeof(im));
  memset(&fields, 0, sizeof(fields));
  
  im.height = m_sourceHeight;
  im.width = m_sourceWidth;
  
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

  planes[0].id = 1;
  return true;
}

void CRendererVTB::DeleteTexture(int index)
{
  CRenderBuffer &buf = m_vtbBuffers[index];
  
  if (buf.m_videoBuffer)
    CVBufferRelease(buf.m_videoBuffer);
  buf.m_videoBuffer = nullptr;

  if (buf.m_textureY)
    CFRelease(buf.m_textureY);
  buf.m_textureY = nullptr;

  if (buf.m_textureUV)
    CFRelease(buf.m_textureUV);
  buf.m_textureUV = nullptr;

  YUVFIELDS &fields = m_buffers[index].fields;
  fields[FIELD_FULL][0].id = 0;
  fields[FIELD_FULL][1].id = 0;
  fields[FIELD_FULL][2].id = 0;
}

bool CRendererVTB::UploadTexture(int index)
{
  CRenderBuffer &buf = m_vtbBuffers[index];

  if (!buf.m_videoBuffer)
    return false;

  CVOpenGLESTextureCacheFlush(m_textureCache, 0);

  if (buf.m_textureY)
    CFRelease(buf.m_textureY);
  buf.m_textureY = nullptr;

  if (buf.m_textureUV)
    CFRelease(buf.m_textureUV);
  buf.m_textureUV = nullptr;

  YV12Image &im = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;
  YUVPLANES &planes = fields[0];

  CVReturn ret;
  ret = CVOpenGLESTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                     m_textureCache,
                                                     buf.m_videoBuffer, NULL, GL_TEXTURE_2D, GL_LUMINANCE,
                                                     im.width, im.height, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                                                     0,
                                                     &buf.m_textureY);

  if (ret != kCVReturnSuccess)
  {
    CLog::Log(LOGERROR, "CRendererVTB::UploadTexture - Error uploading texture Y (err: %d)", ret);
    return false;
  }

  ret = CVOpenGLESTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                     m_textureCache,
                                                     buf.m_videoBuffer, NULL, GL_TEXTURE_2D, GL_LUMINANCE_ALPHA,
                                                     im.width/2, im.height/2, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
                                                     1,
                                                     &buf.m_textureUV);

  if (ret != kCVReturnSuccess)
  {
    CLog::Log(LOGERROR, "CRendererVTB::UploadTexture - Error uploading texture UV (err: %d)", ret);
    return false;
  }

  // set textures
  planes[0].id = CVOpenGLESTextureGetName(buf.m_textureY);
  planes[1].id = CVOpenGLESTextureGetName(buf.m_textureUV);
  planes[2].id = CVOpenGLESTextureGetName(buf.m_textureUV);

  glEnable(m_textureTarget);

  for (int p=0; p<2; p++)
  {
    glBindTexture(m_textureTarget, planes[p].id);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(m_textureTarget, 0);
    VerifyGLState();
  }

  CalculateTextureSourceRects(index, 3);
  return true;
}

#endif