/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ScreenshotSurfaceDRM.h"

#include "ServiceBroker.h"
#include "cores/VideoPlayer/Buffers/VideoBufferDRMPRIME.h"
#include "cores/VideoPlayer/VideoRenderers/FrameBufferObject.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "threads/SingleLock.h"
#include "utils/EGLImage.h"
#include "utils/Screenshot.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/gbm/WinSystemGbm.h"
#include "windowing/gbm/drm/DRMAtomic.h"
#include "windowing/linux/WinSystemEGL.h"

#include <array>

#include <xf86drmMode.h>

#include "system_gl.h"

namespace
{

int GetColorSpace(int colorSpace)
{
  switch (colorSpace)
  {
    case DRMPRIME::DRM_COLOR_YCBCR_BT2020:
      return EGL_ITU_REC2020_EXT;
    case DRMPRIME::DRM_COLOR_YCBCR_BT601:
      return EGL_ITU_REC601_EXT;
    case DRMPRIME::DRM_COLOR_YCBCR_BT709:
    default:
      return EGL_ITU_REC709_EXT;
  }
}

int GetColorRange(int colorRange)
{
  switch (colorRange)
  {
    case DRMPRIME::DRM_COLOR_YCBCR_FULL_RANGE:
      return EGL_YUV_FULL_RANGE_EXT;
    case DRMPRIME::DRM_COLOR_YCBCR_LIMITED_RANGE:
    default:
      return EGL_YUV_NARROW_RANGE_EXT;
  }
}

} // namespace

void CScreenshotSurfaceDRM::Register()
{
  CScreenShot::Register(CScreenshotSurfaceDRM::CreateSurface);
}

std::unique_ptr<IScreenshotSurface> CScreenshotSurfaceDRM::CreateSurface()
{
  return std::make_unique<CScreenshotSurfaceDRM>();
}

bool CScreenshotSurfaceDRM::CaptureVideo()
{
  auto renderSystem = static_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem());
  if (!renderSystem)
  {
    CLog::Log(LOGDEBUG, "CScreenshotSurfaceDRM::{} - failed to get render system", __FUNCTION__);
    return false;
  }

  auto winSystem =
      static_cast<KODI::WINDOWING::GBM::CWinSystemGbm*>(CServiceBroker::GetWinSystem());
  if (!winSystem)
  {
    CLog::Log(LOGDEBUG, "CScreenshotSurfaceDRM::{} - failed to get window system", __FUNCTION__);
    return false;
  }

  CSingleLock lock(winSystem->GetGfxContext());

  auto winSystemEGL =
      dynamic_cast<KODI::WINDOWING::LINUX::CWinSystemEGL*>(CServiceBroker::GetWinSystem());
  if (!winSystemEGL)
  {
    CLog::Log(LOGDEBUG, "CScreenshotSurfaceDRM::{} - failed to get egl window system",
              __FUNCTION__);
    return false;
  }

  auto drm = std::static_pointer_cast<KODI::WINDOWING::GBM::CDRMAtomic>(winSystem->GetDrm());
  if (!drm)
  {
    CLog::Log(LOGDEBUG, "CScreenshotSurfaceDRM::{} - failed to get drm system", __FUNCTION__);
    return false;
  }

  auto videoPlane = drm->GetVideoPlane();
  if (!videoPlane)
  {
    CLog::Log(LOGDEBUG, "CScreenshotSurfaceDRM::{} - video plane unavailable", __FUNCTION__);
    return true; // video plane may not be present so this isn't a failure
  }

  uint32_t fb_id = videoPlane->GetPlaneFbId();
  if (fb_id == 0)
  {
    CLog::Log(LOGDEBUG, "CScreenshotSurfaceDRM::{} - video plane doesn't have an attached fb_id",
              __FUNCTION__);
    return true; // video plane may exists but doesn't currently have a bound fb
  }

  auto fb = drmModeGetFB2(drm->GetFileDescriptor(), fb_id);
  if (!fb)
  {
    CLog::Log(LOGDEBUG, "CScreenshotSurfaceDRM::{} - failed to get framebuffer for id: {}",
              __FUNCTION__, fb_id);
    return false;
  }

  std::array<CEGLImage::EglPlane, CEGLImage::MAX_NUM_PLANES> planes;

  for (int i = 0; i < CEGLImage::MAX_NUM_PLANES; i++)
  {
    drmPrimeHandleToFD(drm->GetFileDescriptor(), fb->handles[i], 0, &planes[i].fd);
    planes[i].offset = fb->offsets[i];
    planes[i].pitch = fb->pitches[i];
    planes[i].modifier = fb->modifier;
  }

  CEGLImage::EglAttrs attribs;

  attribs.width = m_width;
  attribs.height = m_height;
  attribs.format = fb->pixel_format;
  attribs.colorSpace = GetColorSpace(videoPlane->GetProperty("COLOR_ENCODING"));
  attribs.colorRange = GetColorRange(videoPlane->GetProperty("COLOR_RANGE"));
  attribs.planes = planes;

  CEGLImage image(winSystemEGL->GetEGLDisplay());

  if (!image.CreateImage(attribs))
    return false;

  CFrameBufferObject fbo;
  if (!fbo.Initialize())
  {
    CLog::Log(LOGDEBUG, "CScreenshotSurfaceDRM::{} - failed to initialize framebuffer object",
              __FUNCTION__);
    return false;
  }

  if (!fbo.CreateAndBindToTexture(GL_TEXTURE_2D, m_width, m_height, GL_RGBA))
  {
    CLog::Log(LOGDEBUG, "CScreenshotSurfaceDRM::{} - failed to create framebuffer object texture",
              __FUNCTION__);
    return false;
  }

  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  image.UploadImage(GL_TEXTURE_EXTERNAL_OES);
  image.DestroyImage();

  if (!fbo.BeginRender())
  {
    CLog::Log(LOGDEBUG, "CScreenshotSurfaceDRM::{} - failed to bind framebuffer object",
              __FUNCTION__);
    return false;
  }

  renderSystem->EnableGUIShader(SM_TEXTURE_RGBA_OES);

  GLubyte idx[4] = {0, 1, 3, 2}; // Determines order of triangle strip
  GLuint vertexVBO;
  GLuint indexVBO;
  struct PackedVertex
  {
    float x, y, z;
    float u1, v1;
  };

  std::array<PackedVertex, 4> vertex;

  GLint vertLoc = renderSystem->GUIShaderGetPos();
  GLint loc = renderSystem->GUIShaderGetCoord0();

  // top left
  vertex[0].x = 0.0f;
  vertex[0].y = m_height;
  vertex[0].z = 0.0f;
  vertex[0].u1 = 0.0f;
  vertex[0].v1 = 0.0f;

  // top right
  vertex[1].x = m_width;
  vertex[1].y = m_height;
  vertex[1].z = 0.0f;
  vertex[1].u1 = 1.0f;
  vertex[1].v1 = 0.0f;

  // bottom right
  vertex[2].x = m_width;
  vertex[2].y = 0.0f;
  vertex[2].z = 0.0f;
  vertex[2].u1 = 1.0f;
  vertex[2].v1 = 1.0f;

  // bottom left
  vertex[3].x = 0.0f;
  vertex[3].y = 0.0f;
  vertex[3].z = 0.0f;
  vertex[3].u1 = 0.0f;
  vertex[3].v1 = 1.0f;

  glGenBuffers(1, &vertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(PackedVertex) * vertex.size(), vertex.data(),
               GL_STATIC_DRAW);

  glVertexAttribPointer(vertLoc, 3, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, x)));
  glVertexAttribPointer(loc, 2, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, u1)));

  glEnableVertexAttribArray(vertLoc);
  glEnableVertexAttribArray(loc);

  glGenBuffers(1, &indexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * 4, idx, GL_STATIC_DRAW);

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, 0);

  glDisableVertexAttribArray(vertLoc);
  glDisableVertexAttribArray(loc);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &vertexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &indexVBO);

  renderSystem->DisableGUIShader();

  std::vector<uint8_t> surface(m_stride * m_height);
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE,
               static_cast<GLvoid*>(surface.data()));

  fbo.Cleanup();

  unsigned char* buffer = m_buffer;

  for (int y = 0; y < m_height; ++y)
  {
    unsigned char* videoPtr = surface.data() + y * m_stride;

    for (int x = 0; x < m_width; ++x, buffer += 4, videoPtr += 4)
    {
      float alpha = buffer[3] / 255.0f;

      //B
      buffer[0] =
          alpha * static_cast<float>(buffer[0]) + (1 - alpha) * static_cast<float>(videoPtr[2]);
      //G
      buffer[1] =
          alpha * static_cast<float>(buffer[1]) + (1 - alpha) * static_cast<float>(videoPtr[1]);
      //R
      buffer[2] =
          alpha * static_cast<float>(buffer[2]) + (1 - alpha) * static_cast<float>(videoPtr[0]);
      //A
      buffer[3] = 0xFF;
    }
  }

  CLog::Log(LOGDEBUG, "CScreenshotSurfaceDRM::{} - success", __FUNCTION__);

  return true;
}
