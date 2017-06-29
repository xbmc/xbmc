/*
 *      Copyright (C) 2007-2017 Team XBMC
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

#include "VaapiEGL.h"

#include "cores/VideoPlayer/DVDCodecs/Video/VAAPI.h"
#include <va/va_drmcommon.h>
#include <drm_fourcc.h>
#include "utils/log.h"

using namespace VAAPI;

void CVaapiTexture::Init(InteropInfo &interop)
{
  m_interop = interop;
}

bool CVaapiTexture::Map(CVaapiRenderPicture *pic)
{
  VAStatus status;

  if (m_vaapiPic)
    return true;

  vaSyncSurface(pic->vadsp, pic->procPic.videoSurface);

  status = vaDeriveImage(pic->vadsp, pic->procPic.videoSurface, &m_glSurface.vaImage);
  if (status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGERROR, "CVaapiTexture::%s - Error: %s(%d)", __FUNCTION__, vaErrorStr(status), status);
    return false;
  }
  memset(&m_glSurface.vBufInfo, 0, sizeof(m_glSurface.vBufInfo));
  m_glSurface.vBufInfo.mem_type = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;
  status = vaAcquireBufferHandle(pic->vadsp, m_glSurface.vaImage.buf, &m_glSurface.vBufInfo);
  if (status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGERROR, "CVaapiTexture::%s - Error: %s(%d)", __FUNCTION__, vaErrorStr(status), status);
    return false;
  }

  m_texWidth = m_glSurface.vaImage.width;
  m_texHeight = m_glSurface.vaImage.height;

  GLint attribs[23], *attrib;

  switch (m_glSurface.vaImage.format.fourcc)
  {
    case VA_FOURCC('N','V','1','2'):
    {
      attrib = attribs;
      *attrib++ = EGL_LINUX_DRM_FOURCC_EXT;
      *attrib++ = fourcc_code('R', '8', ' ', ' ');
      *attrib++ = EGL_WIDTH;
      *attrib++ = m_glSurface.vaImage.width;
      *attrib++ = EGL_HEIGHT;
      *attrib++ = m_glSurface.vaImage.height;
      *attrib++ = EGL_DMA_BUF_PLANE0_FD_EXT;
      *attrib++ = (intptr_t)m_glSurface.vBufInfo.handle;
      *attrib++ = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
      *attrib++ = m_glSurface.vaImage.offsets[0];
      *attrib++ = EGL_DMA_BUF_PLANE0_PITCH_EXT;
      *attrib++ = m_glSurface.vaImage.pitches[0];
      *attrib++ = EGL_NONE;
      m_glSurface.eglImageY = m_interop.eglCreateImageKHR(m_interop.eglDisplay,
                                          EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, (EGLClientBuffer)NULL,
                                          attribs);
      if (!m_glSurface.eglImageY)
      {
        EGLint err = eglGetError();
        CLog::Log(LOGERROR, "failed to import VA buffer NV12 into EGL image: %d", err);
        return false;
      }

      attrib = attribs;
      *attrib++ = EGL_LINUX_DRM_FOURCC_EXT;
      *attrib++ = fourcc_code('G', 'R', '8', '8');
      *attrib++ = EGL_WIDTH;
      *attrib++ = (m_glSurface.vaImage.width + 1) >> 1;
      *attrib++ = EGL_HEIGHT;
      *attrib++ = (m_glSurface.vaImage.height + 1) >> 1;
      *attrib++ = EGL_DMA_BUF_PLANE0_FD_EXT;
      *attrib++ = (intptr_t)m_glSurface.vBufInfo.handle;
      *attrib++ = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
      *attrib++ = m_glSurface.vaImage.offsets[1];
      *attrib++ = EGL_DMA_BUF_PLANE0_PITCH_EXT;
      *attrib++ = m_glSurface.vaImage.pitches[1];
      *attrib++ = EGL_NONE;
      m_glSurface.eglImageVU = m_interop.eglCreateImageKHR(m_interop.eglDisplay,
                                          EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, (EGLClientBuffer)NULL,
                                          attribs);
      if (!m_glSurface.eglImageVU)
      {
        EGLint err = eglGetError();
        CLog::Log(LOGERROR, "failed to import VA buffer NV12 into EGL image: %d", err);
        return false;
      }

      GLint format;

      glGenTextures(1, &m_textureY);
      glEnable(m_interop.textureTarget);
      glBindTexture(m_interop.textureTarget, m_textureY);
      glTexParameteri(m_interop.textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(m_interop.textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(m_interop.textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(m_interop.textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      m_interop.glEGLImageTargetTexture2DOES(m_interop.textureTarget, m_glSurface.eglImageY);
      glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &format);

      glGenTextures(1, &m_textureVU);
      glEnable(m_interop.textureTarget);
      glBindTexture(m_interop.textureTarget, m_textureVU);
      glTexParameteri(m_interop.textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(m_interop.textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(m_interop.textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(m_interop.textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      m_interop.glEGLImageTargetTexture2DOES(m_interop.textureTarget, m_glSurface.eglImageVU);
      glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &format);

      glBindTexture(m_interop.textureTarget, 0);
      glDisable(m_interop.textureTarget);

      break;
    }
    case VA_FOURCC('P','0','1','0'):
    {
      attrib = attribs;
      *attrib++ = EGL_LINUX_DRM_FOURCC_EXT;
      *attrib++ = fourcc_code('R', '1', '6', ' ');
      *attrib++ = EGL_WIDTH;
      *attrib++ = m_glSurface.vaImage.width;
      *attrib++ = EGL_HEIGHT;
      *attrib++ = m_glSurface.vaImage.height;
      *attrib++ = EGL_DMA_BUF_PLANE0_FD_EXT;
      *attrib++ = (intptr_t)m_glSurface.vBufInfo.handle;
      *attrib++ = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
      *attrib++ = m_glSurface.vaImage.offsets[0];
      *attrib++ = EGL_DMA_BUF_PLANE0_PITCH_EXT;
      *attrib++ = m_glSurface.vaImage.pitches[0];
      *attrib++ = EGL_NONE;
      m_glSurface.eglImageY = m_interop.eglCreateImageKHR(m_interop.eglDisplay,
                                          EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, (EGLClientBuffer)NULL,
                                          attribs);
      if (!m_glSurface.eglImageY)
      {
        EGLint err = eglGetError();
        CLog::Log(LOGERROR, "failed to import VA buffer P010 into EGL image: %d", err);
        return false;
      }

      attrib = attribs;
      *attrib++ = EGL_LINUX_DRM_FOURCC_EXT;
      *attrib++ = fourcc_code('G', 'R', '3', '2');
      *attrib++ = EGL_WIDTH;
      *attrib++ = (m_glSurface.vaImage.width + 1) >> 1;
      *attrib++ = EGL_HEIGHT;
      *attrib++ = (m_glSurface.vaImage.height + 1) >> 1;
      *attrib++ = EGL_DMA_BUF_PLANE0_FD_EXT;
      *attrib++ = (intptr_t)m_glSurface.vBufInfo.handle;
      *attrib++ = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
      *attrib++ = m_glSurface.vaImage.offsets[1];
      *attrib++ = EGL_DMA_BUF_PLANE0_PITCH_EXT;
      *attrib++ = m_glSurface.vaImage.pitches[1];
      *attrib++ = EGL_NONE;
      m_glSurface.eglImageVU = m_interop.eglCreateImageKHR(m_interop.eglDisplay,
                                          EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, (EGLClientBuffer)NULL,
                                          attribs);
      if (!m_glSurface.eglImageVU)
      {
        EGLint err = eglGetError();
        CLog::Log(LOGERROR, "failed to import VA buffer P010 into EGL image: %d", err);
        return false;
      }

      GLint format;

      glGenTextures(1, &m_textureY);
      glEnable(m_interop.textureTarget);
      glBindTexture(m_interop.textureTarget, m_textureY);
      glTexParameteri(m_interop.textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(m_interop.textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(m_interop.textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(m_interop.textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      m_interop.glEGLImageTargetTexture2DOES(m_interop.textureTarget, m_glSurface.eglImageY);
      glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &format);

      glGenTextures(1, &m_textureVU);
      glEnable(m_interop.textureTarget);
      glBindTexture(m_interop.textureTarget, m_textureVU);
      glTexParameteri(m_interop.textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(m_interop.textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(m_interop.textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(m_interop.textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      m_interop.glEGLImageTargetTexture2DOES(m_interop.textureTarget, m_glSurface.eglImageVU);
      glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &format);

      glBindTexture(m_interop.textureTarget, 0);
      glDisable(m_interop.textureTarget);

      break;
    }
    case VA_FOURCC('B','G','R','A'):
    {
      attrib = attribs;
      *attrib++ = EGL_DRM_BUFFER_FORMAT_MESA;
      *attrib++ = EGL_DRM_BUFFER_FORMAT_ARGB32_MESA;
      *attrib++ = EGL_WIDTH;
      *attrib++ = m_glSurface.vaImage.width;
      *attrib++ = EGL_HEIGHT;
      *attrib++ = m_glSurface.vaImage.height;
      *attrib++ = EGL_DRM_BUFFER_STRIDE_MESA;
      *attrib++ = m_glSurface.vaImage.pitches[0] / 4;
      *attrib++ = EGL_NONE;
      m_glSurface.eglImage = m_interop.eglCreateImageKHR(m_interop.eglDisplay, EGL_NO_CONTEXT,
                                         EGL_DRM_BUFFER_MESA,
                                         (EGLClientBuffer)m_glSurface.vBufInfo.handle,
                                         attribs);
      if (!m_glSurface.eglImage)
      {
        EGLint err = eglGetError();
        CLog::Log(LOGERROR, "failed to import VA buffer BGRA into EGL image: %d", err);
        return false;
      }

      glGenTextures(1, &m_texture);
      glEnable(m_interop.textureTarget);
      glBindTexture(m_interop.textureTarget, m_texture);
      glTexParameteri(m_interop.textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(m_interop.textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(m_interop.textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(m_interop.textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      m_interop.glEGLImageTargetTexture2DOES(m_interop.textureTarget, m_glSurface.eglImage);

      glBindTexture(m_interop.textureTarget, 0);
      glDisable(m_interop.textureTarget);

      break;
    }
    default:
      return false;
  }

  m_vaapiPic = pic;
  m_vaapiPic->Acquire();
  return true;
}

void CVaapiTexture::Unmap()
{
  if (!m_vaapiPic)
    return;

  if (m_glSurface.vaImage.image_id == VA_INVALID_ID)
    return;

  m_interop.eglDestroyImageKHR(m_interop.eglDisplay, m_glSurface.eglImageY);
  m_interop.eglDestroyImageKHR(m_interop.eglDisplay, m_glSurface.eglImageVU);

  VAStatus status;
  status = vaReleaseBufferHandle(m_vaapiPic->vadsp, m_glSurface.vaImage.buf);
  if (status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGERROR, "VAAPI::%s - Error: %s(%d)", __FUNCTION__, vaErrorStr(status), status);
  }

  status = vaDestroyImage(m_vaapiPic->vadsp, m_glSurface.vaImage.image_id);
  if (status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGERROR, "VAAPI::%s - Error: %s(%d)", __FUNCTION__, vaErrorStr(status), status);
  }

  m_glSurface.vaImage.image_id = VA_INVALID_ID;

  glDeleteTextures(1, &m_textureY);
  glDeleteTextures(1, &m_textureVU);

  m_vaapiPic->Release();
  m_vaapiPic = nullptr;
}

bool CVaapiTexture::TestInterop(VADisplay vaDpy, EGLDisplay eglDisplay)
{
  bool ret = false;

  int major_version, minor_version;
  vaInitialize(vaDpy, &major_version, &minor_version);

  int width = 1920;
  int height = 1080;

  // create surfaces
  VASurfaceID surface;
  VAStatus status;
  VAImage image;
  VABufferInfo bufferInfo;

  if (vaCreateSurfaces(vaDpy,  VA_RT_FORMAT_YUV420,
                       width, height,
                       &surface, 1, NULL, 0) != VA_STATUS_SUCCESS)
  {
    vaTerminate(vaDpy);
    return false;
  }

  // check interop
  PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
  PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
  if (!eglCreateImageKHR || !eglDestroyImageKHR)
  {
    vaTerminate(vaDpy);
    return false;
  }

  status = vaDeriveImage(vaDpy, surface, &image);
  if (status == VA_STATUS_SUCCESS)
  {
    memset(&bufferInfo, 0, sizeof(bufferInfo));
    bufferInfo.mem_type = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;
    status = vaAcquireBufferHandle(vaDpy, image.buf, &bufferInfo);
    if (status == VA_STATUS_SUCCESS)
    {
      EGLImageKHR eglImage;
      GLint attribs[23], *attrib;

      attrib = attribs;
      *attrib++ = EGL_LINUX_DRM_FOURCC_EXT;
      *attrib++ = fourcc_code('R', '8', ' ', ' ');
      *attrib++ = EGL_WIDTH;
      *attrib++ = image.width;
      *attrib++ = EGL_HEIGHT;
      *attrib++ = image.height;
      *attrib++ = EGL_DMA_BUF_PLANE0_FD_EXT;
      *attrib++ = (intptr_t)bufferInfo.handle;
      *attrib++ = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
      *attrib++ = image.offsets[0];
      *attrib++ = EGL_DMA_BUF_PLANE0_PITCH_EXT;
      *attrib++ = image.pitches[0];
      *attrib++ = EGL_NONE;
      eglImage = eglCreateImageKHR(eglDisplay,
                                   EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, (EGLClientBuffer)NULL,
                                   attribs);
      if (eglImage)
      {
        eglDestroyImageKHR(eglDisplay, eglImage);
        ret = true;
      }

    }
    vaDestroyImage(vaDpy, image.image_id);
  }
  vaDestroySurfaces(vaDpy, &surface, 1);
  vaTerminate(vaDpy);

  return ret;
}

bool CVaapiTexture::TestInteropHevc(VADisplay vaDpy, EGLDisplay eglDisplay)
{
  bool ret = false;

  int major_version, minor_version;
  vaInitialize(vaDpy, &major_version, &minor_version);

  int width = 1920;
  int height = 1080;

  // create surfaces
  VASurfaceID surface;
  VAStatus status;
  VAImage image;
  VABufferInfo bufferInfo;

  if (vaCreateSurfaces(vaDpy,  VA_RT_FORMAT_YUV420_10BPP,
                       width, height,
                       &surface, 1, NULL, 0) != VA_STATUS_SUCCESS)
  {
    vaTerminate(vaDpy);
    return ret;
  }

  PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
  PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
  if (!eglCreateImageKHR || !eglDestroyImageKHR)
  {
    vaTerminate(vaDpy);
    return false;
  }

  // check interop
  status = vaDeriveImage(vaDpy, surface, &image);
  if (status == VA_STATUS_SUCCESS)
  {
    memset(&bufferInfo, 0, sizeof(bufferInfo));
    bufferInfo.mem_type = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;
    status = vaAcquireBufferHandle(vaDpy, image.buf, &bufferInfo);
    if (status == VA_STATUS_SUCCESS)
    {
      EGLImageKHR eglImage;
      GLint attribs[23], *attrib;

      attrib = attribs;
      *attrib++ = EGL_LINUX_DRM_FOURCC_EXT;
      *attrib++ = fourcc_code('G', 'R', '3', '2');
      *attrib++ = EGL_WIDTH;
      *attrib++ = (image.width +1) >> 1;
      *attrib++ = EGL_HEIGHT;
      *attrib++ = (image.height +1) >> 1;
      *attrib++ = EGL_DMA_BUF_PLANE0_FD_EXT;
      *attrib++ = (intptr_t)bufferInfo.handle;
      *attrib++ = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
      *attrib++ = image.offsets[1];
      *attrib++ = EGL_DMA_BUF_PLANE0_PITCH_EXT;
      *attrib++ = image.pitches[1];
      *attrib++ = EGL_NONE;
      eglImage = eglCreateImageKHR(eglDisplay,
                                   EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, (EGLClientBuffer)NULL,
                                   attribs);
      if (eglImage)
      {
        eglDestroyImageKHR(eglDisplay, eglImage);
        ret = true;
      }

    }
    vaDestroyImage(vaDpy, image.image_id);
  }
  vaDestroySurfaces(vaDpy, &surface, 1);
  vaTerminate(vaDpy);

  return ret;
}

