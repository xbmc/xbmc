/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererVTBGLES.h"

#include "../RenderFactory.h"
#include "ServiceBroker.h"
#include "cores/IPlayer.h"
#include "cores/VideoPlayer/DVDCodecs/Video/VTB.h"
#include "settings/MediaSettings.h"
#include "utils/GLUtils.h"
#include "utils/log.h"
#if defined(TARGET_DARWIN_IOS)
#include "windowing/ios/WinSystemIOS.h"
#define WIN_SYSTEM_CLASS CWinSystemIOS
#elif defined(TARGET_DARWIN_TVOS)
#include "windowing/tvos/WinSystemTVOS.h"
#define WIN_SYSTEM_CLASS CWinSystemTVOS
#endif

#include <CoreVideo/CVBuffer.h>
#include <CoreVideo/CVPixelBuffer.h>
#include <OpenGLES/ES2/glext.h>

CBaseRenderer* CRendererVTB::Create(CVideoBuffer *buffer)
{
  VTB::CVideoBufferVTB *vb = dynamic_cast<VTB::CVideoBufferVTB*>(buffer);
  if (vb)
    return new CRendererVTB();

  return nullptr;
}

bool CRendererVTB::Register()
{
  VIDEOPLAYER::CRendererFactory::RegisterRenderer("vtbgles", CRendererVTB::Create);
  return true;
}

CRendererVTB::CRendererVTB()
{
  auto winSystem = dynamic_cast<WIN_SYSTEM_CLASS*>(CServiceBroker::GetWinSystem());
  m_glContext = winSystem->GetEAGLContextObj();
  CVReturn ret = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault,
                                              NULL,
                                              m_glContext,
                                              NULL,
                                              &m_textureCache);
  if (ret != kCVReturnSuccess)
  {
    CLog::Log(LOGERROR, "CRendererVTB::CRendererVTB - Error creating texture cache (err: {})", ret);
  }

  for (auto &buf : m_vtbBuffers)
  {
    buf.m_textureY = nullptr;
    buf.m_textureUV = nullptr;
    buf.m_videoBuffer = nullptr;
    buf.m_fence = nullptr;
  }
}

CRendererVTB::~CRendererVTB()
{
  if (m_textureCache)
    CFRelease(m_textureCache);

  for (int i = 0; i < NUM_BUFFERS; ++i)
  {
    DeleteTexture(i);
  }
}

void CRendererVTB::ReleaseBuffer(int idx)
{
  CPictureBuffer &buf = m_buffers[idx];
  CRenderBuffer &renderBuf = m_vtbBuffers[idx];
  if (buf.videoBuffer)
  {
    if (renderBuf.m_fence && glIsSyncAPPLE(renderBuf.m_fence))
    {
      glDeleteSyncAPPLE(renderBuf.m_fence);
      renderBuf.m_fence = 0;
    }
    buf.videoBuffer->Release();
    buf.videoBuffer = nullptr;
  }
}

EShaderFormat CRendererVTB::GetShaderFormat()
{
  return SHADER_YV12;
}

bool CRendererVTB::LoadShadersHook()
{
  CLog::Log(LOGINFO, "GL: Using CVBREF render method");
  m_textureTarget = GL_TEXTURE_2D;
  m_renderMethod = RENDER_CUSTOM;

  if (!m_textureCache)
  {
    CLog::Log(LOGINFO, "CRendererVTB::LoadShadersHook: no texture cache");
    return false;
  }

  CVReturn ret = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault,
                                              NULL,
                                              m_glContext,
                                              NULL,
                                              &m_textureCache);
  if (ret != kCVReturnSuccess)
    return false;

  return false;
}

bool CRendererVTB::CreateTexture(int index)
{
  CPictureBuffer &buf = m_buffers[index];
  YuvImage &im = buf.image;
  CYuvPlane (&planes)[YuvImage::MAX_PLANES] = buf.fields[0];

  DeleteTexture(index);

  memset(&im    , 0, sizeof(im));
  memset(&planes, 0, sizeof(CYuvPlane[YuvImage::MAX_PLANES]));

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
  CRenderBuffer &renderBuf = m_vtbBuffers[index];
  CYuvPlane (&planes)[YuvImage::MAX_PLANES] = m_buffers[index].fields[0];

  if (renderBuf.m_textureY)
    CFRelease(renderBuf.m_textureY);
  renderBuf.m_textureY = nullptr;

  if (renderBuf.m_textureUV)
    CFRelease(renderBuf.m_textureUV);
  renderBuf.m_textureUV = nullptr;

  ReleaseBuffer(index);

  planes[0].id = 0;
  planes[1].id = 0;
  planes[2].id = 0;
}

bool CRendererVTB::UploadTexture(int index)
{
  CRenderBuffer &renderBuf = m_vtbBuffers[index];
  CPictureBuffer &buf = m_buffers[index];
  CYuvPlane (&planes)[YuvImage::MAX_PLANES] = m_buffers[index].fields[0];
  YuvImage &im = m_buffers[index].image;

  VTB::CVideoBufferVTB *vb = dynamic_cast<VTB::CVideoBufferVTB*>(buf.videoBuffer);
  if (!vb)
  {
    return false;
  }

  CVOpenGLESTextureCacheFlush(m_textureCache, 0);

  if (renderBuf.m_textureY)
    CFRelease(renderBuf.m_textureY);
  renderBuf.m_textureY = nullptr;

  if (renderBuf.m_textureUV)
    CFRelease(renderBuf.m_textureUV);
  renderBuf.m_textureUV = nullptr;

  CVReturn ret;
  ret = CVOpenGLESTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                     m_textureCache,
                                                     vb->GetPB(), nullptr, GL_TEXTURE_2D, GL_LUMINANCE,
                                                     im.width, im.height, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                                                     0,
                                                     &renderBuf.m_textureY);

  if (ret != kCVReturnSuccess)
  {
    CLog::Log(LOGERROR, "CRendererVTB::UploadTexture - Error uploading texture Y (err: {})", ret);
    return false;
  }

  ret = CVOpenGLESTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                     m_textureCache,
                                                     vb->GetPB(), nullptr, GL_TEXTURE_2D, GL_LUMINANCE_ALPHA,
                                                     im.width/2, im.height/2, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
                                                     1,
                                                     &renderBuf.m_textureUV);

  if (ret != kCVReturnSuccess)
  {
    CLog::Log(LOGERROR, "CRendererVTB::UploadTexture - Error uploading texture UV (err: {})", ret);
    return false;
  }

  // set textures
  planes[0].id = CVOpenGLESTextureGetName(renderBuf.m_textureY);
  planes[1].id = CVOpenGLESTextureGetName(renderBuf.m_textureUV);
  planes[2].id = CVOpenGLESTextureGetName(renderBuf.m_textureUV);

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

void CRendererVTB::AfterRenderHook(int idx)
{
  CRenderBuffer &renderBuf = m_vtbBuffers[idx];
  if (renderBuf.m_fence && glIsSyncAPPLE(renderBuf.m_fence))
  {
    glDeleteSyncAPPLE(renderBuf.m_fence);
  }
  renderBuf.m_fence = glFenceSyncAPPLE(GL_SYNC_GPU_COMMANDS_COMPLETE_APPLE, 0);
}

bool CRendererVTB::NeedBuffer(int idx)
{
  CRenderBuffer &renderBuf = m_vtbBuffers[idx];
  if (renderBuf.m_fence && glIsSyncAPPLE(renderBuf.m_fence))
  {
    int syncState = GL_UNSIGNALED_APPLE;
    glGetSyncivAPPLE(renderBuf.m_fence, GL_SYNC_STATUS_APPLE, 1, nullptr, &syncState);
    if (syncState != GL_SIGNALED_APPLE)
      return true;
  }

  return false;
}

