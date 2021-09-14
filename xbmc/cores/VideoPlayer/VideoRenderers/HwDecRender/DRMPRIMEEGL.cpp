/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DRMPRIMEEGL.h"

#include "utils/log.h"

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

CDRMPRIMETexture::CDRMPRIMETexture(EGLDisplay eglDisplay)
{
  for (auto& eglImage : m_eglImage)
    eglImage = std::make_unique<CEGLImage>(eglDisplay);
}

CDRMPRIMETexture::~CDRMPRIMETexture()
{
  for (auto texture : m_texture)
    glDeleteTextures(1, &texture);
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
  if (!descriptor || descriptor->nb_layers < 1)
  {
    buffer->ReleaseDescriptor();
    return false;
  }

  if (descriptor->nb_layers > 1)
    m_textureTarget = GL_TEXTURE_2D;

  if (descriptor->nb_layers == 3)
  {
    if (descriptor->layers[0].format == DRM_FORMAT_R16 &&
        descriptor->layers[1].format == DRM_FORMAT_R16 &&
        descriptor->layers[2].format == DRM_FORMAT_R16)
      m_textureBits = 10;
  }

  for (int layer = 0; layer < descriptor->nb_layers; layer++)
  {
    std::array<CEGLImage::EglPlane, CEGLImage::MAX_NUM_PLANES> planes;

    AVDRMLayerDescriptor* layerDesc = &descriptor->layers[layer];
    for (int plane = 0; plane < layerDesc->nb_planes; plane++)
    {
      AVDRMPlaneDescriptor* planeDesc = &layerDesc->planes[plane];
      AVDRMObjectDescriptor* objectDesc = &descriptor->objects[planeDesc->object_index];

      planes[plane].fd = objectDesc->fd;
      planes[plane].modifier = objectDesc->format_modifier;
      planes[plane].offset = planeDesc->offset;
      planes[plane].pitch = planeDesc->pitch;
    }

    CEGLImage::EglAttrs attribs;

    attribs.width = m_texWidth;
    attribs.height = m_texHeight;
    attribs.format = descriptor->layers[layer].format;
    attribs.planes = planes;

    switch (descriptor->layers[layer].format)
    {
      case DRM_FORMAT_R8:
      case DRM_FORMAT_R16:
      case DRM_FORMAT_GR88:
      case DRM_FORMAT_GR1616:
      {
        if (layer > 0)
        {
          attribs.width = m_texWidth >> 1;
          attribs.height = m_texHeight >> 1;
        }
        break;
      }
      default:
      {
        attribs.colorSpace = GetEGLColorSpace(buffer->GetPicture());
        attribs.colorRange = GetEGLColorRange(buffer->GetPicture());
        break;
      }
    }

    if (!m_eglImage[layer]->CreateImage(attribs))
    {
      buffer->ReleaseDescriptor();
      return false;
    }

    if (!glIsTexture(m_texture[layer]))
      glGenTextures(1, &m_texture[layer]);
    glBindTexture(m_textureTarget, m_texture[layer]);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_eglImage[layer]->UploadImage(m_textureTarget);
    m_eglImage[layer]->DestroyImage();
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

  m_primebuffer->ReleaseDescriptor();

  m_primebuffer->Release();
  m_primebuffer = nullptr;
}
