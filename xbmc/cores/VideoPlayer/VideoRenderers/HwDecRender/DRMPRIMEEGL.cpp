/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DRMPRIMEEGL.h"

#include "utils/log.h"

void CDRMPRIMETexture::Init(EGLDisplay eglDisplay)
{
  m_eglImage.reset(new CEGLImage(eglDisplay));
}

bool CDRMPRIMETexture::Map(IVideoBufferDRMPRIME* buffer)
{
  if (m_primebuffer)
    return true;

  if (!buffer->Map())
    return false;

  m_texWidth = buffer->GetWidth();
  m_texHeight = buffer->GetHeight();

  AVDRMFrameDescriptor* descriptor = buffer->GetDescriptor();
  if (descriptor && descriptor->nb_layers)
  {
    // get drm format of the frame
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(56, 27, 100)
    uint32_t format = descriptor->format;
#else
    uint32_t format = 0;
#endif
    if (!format && descriptor->nb_layers == 1)
      format = descriptor->layers[0].format;
    if (!format)
      return false;

    std::array<CEGLImage::EglPlane, CEGLImage::MAX_NUM_PLANES> planes;

    int index = 0;
    for (int i = 0; i < descriptor->nb_layers; i++)
    {
      AVDRMLayerDescriptor* layer = &descriptor->layers[i];
      for (int j = 0; j < layer->nb_planes; j++)
      {
        AVDRMPlaneDescriptor* plane = &layer->planes[j];
        AVDRMObjectDescriptor* object = &descriptor->objects[plane->object_index];

        planes[index].fd = object->fd;
        planes[index].modifier = object->format_modifier;
        planes[index].offset = plane->offset;
        planes[index].pitch = plane->pitch;

        index++;
      }
    }

    CEGLImage::EglAttrs attribs;

    attribs.width = m_texWidth;
    attribs.height = m_texHeight;
    attribs.format = format;
    attribs.colorSpace = GetColorSpace(buffer->GetColorEncoding());
    attribs.colorRange = GetColorRange(buffer->GetColorRange());
    attribs.planes = planes;

    if (!m_eglImage->CreateImage(attribs))
      return false;

    glGenTextures(1, &m_texture);
    glBindTexture(m_textureTarget, m_texture);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_eglImage->UploadImage(m_textureTarget);
    glBindTexture(m_textureTarget, 0);
  }

  m_primebuffer = buffer;
  m_primebuffer->Acquire();

  return true;
}

void CDRMPRIMETexture::Unmap()
{
  if (!m_primebuffer)
    return;

  m_eglImage->DestroyImage();

  glDeleteTextures(1, &m_texture);

  m_primebuffer->Unmap();

  m_primebuffer->Release();
  m_primebuffer = nullptr;
}

int CDRMPRIMETexture::GetColorSpace(int colorSpace)
{
  switch(colorSpace)
  {
    case DRM_COLOR_YCBCR_BT2020:
      return EGL_ITU_REC2020_EXT;
    case DRM_COLOR_YCBCR_BT601:
      return EGL_ITU_REC601_EXT;
    case DRM_COLOR_YCBCR_BT709:
      return EGL_ITU_REC709_EXT;
    default:
      CLog::Log(LOGERROR, "CEGLImage::%s - failed to get colorspace for: %d", __FUNCTION__, colorSpace);
      break;
  }

  return -1;
}

int CDRMPRIMETexture::GetColorRange(int colorRange)
{
  switch(colorRange)
  {
    case DRM_COLOR_YCBCR_FULL_RANGE:
      return EGL_YUV_FULL_RANGE_EXT;
    case DRM_COLOR_YCBCR_LIMITED_RANGE:
      return EGL_YUV_NARROW_RANGE_EXT;
    default:
      CLog::Log(LOGERROR, "CEGLImage::%s - failed to get colorrange for: %d", __FUNCTION__, colorRange);
      break;
  }

  return -1;
}
