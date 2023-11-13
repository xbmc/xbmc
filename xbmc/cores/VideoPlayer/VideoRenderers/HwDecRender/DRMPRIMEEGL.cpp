/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DRMPRIMEEGL.h"

#include "utils/log.h"

#include <memory>

using namespace DRMPRIME;

namespace
{

int GetEGLColorSpace(const VideoPicture& picture)
{
  switch (picture.color_space)
  {
    case AVCOL_SPC_BT2020_CL:
    case AVCOL_SPC_BT2020_NCL:
      return EGL_ITU_REC2020_EXT;
    case AVCOL_SPC_SMPTE170M:
    case AVCOL_SPC_BT470BG:
    case AVCOL_SPC_FCC:
      return EGL_ITU_REC601_EXT;
    case AVCOL_SPC_BT709:
      return EGL_ITU_REC709_EXT;
    case AVCOL_SPC_RESERVED:
    case AVCOL_SPC_UNSPECIFIED:
    default:
      if (picture.iWidth > 1024 || picture.iHeight >= 600)
        return EGL_ITU_REC709_EXT;
      else
        return EGL_ITU_REC601_EXT;
  }
}

int GetEGLColorRange(const VideoPicture& picture)
{
  if (picture.color_range)
    return EGL_YUV_FULL_RANGE_EXT;

  return EGL_YUV_NARROW_RANGE_EXT;
}

} // namespace

CDRMPRIMETexture::~CDRMPRIMETexture()
{
  glDeleteTextures(1, &m_texture);
}

void CDRMPRIMETexture::Init(EGLDisplay eglDisplay)
{
  m_eglImage = std::make_unique<CEGLImage>(eglDisplay);
}

bool CDRMPRIMETexture::Map(CVideoBufferDRMPRIME* buffer)
{
  if (m_primebuffer)
    return true;

  if (!buffer->AcquireDescriptor())
  {
    CLog::Log(LOGERROR, "CDRMPRIMETexture::{} - failed to acquire descriptor", __FUNCTION__);
    return false;
  }

  m_texWidth = buffer->GetWidth();
  m_texHeight = buffer->GetHeight();

  AVDRMFrameDescriptor* descriptor = buffer->GetDescriptor();
  if (descriptor && descriptor->nb_layers)
  {
    AVDRMLayerDescriptor* layer = &descriptor->layers[0];

    std::array<CEGLImage::EglPlane, CEGLImage::MAX_NUM_PLANES> planes;

    for (int i = 0; i < layer->nb_planes; i++)
    {
      planes[i].fd = descriptor->objects[layer->planes[i].object_index].fd;
      planes[i].modifier = descriptor->objects[layer->planes[i].object_index].format_modifier;
      planes[i].offset = layer->planes[i].offset;
      planes[i].pitch = layer->planes[i].pitch;
    }

    CEGLImage::EglAttrs attribs;

    attribs.width = m_texWidth;
    attribs.height = m_texHeight;
    attribs.format = layer->format;
    attribs.colorSpace = GetEGLColorSpace(buffer->GetPicture());
    attribs.colorRange = GetEGLColorRange(buffer->GetPicture());
    attribs.planes = planes;

    if (!m_eglImage->CreateImage(attribs))
    {
      buffer->ReleaseDescriptor();
      return false;
    }

    if (!glIsTexture(m_texture))
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

  m_primebuffer->ReleaseDescriptor();

  m_primebuffer->Release();
  m_primebuffer = nullptr;
}
