/*
 *  Copyright (C) 2017-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderBufferDMAOpenGL.h"

#include "utils/log.h"

#include <drm_fourcc.h>

using namespace KODI;
using namespace RETRO;

CRenderBufferDMAOpenGL::CRenderBufferDMAOpenGL(int fourcc) : CRenderBufferDMA(fourcc)
{
}

void CRenderBufferDMAOpenGL::ConfigureTexture()
{
  // Force alpha to 1, because game clients can leave it undefined
  if (m_fourcc == DRM_FORMAT_ARGB8888 || m_fourcc == DRM_FORMAT_ARGB1555)
    glTexParameteri(m_textureTarget, GL_TEXTURE_SWIZZLE_A, GL_ONE);
}

bool CRenderBufferDMAOpenGL::UploadFromMemory()
{
  GLint internalFormat = 0;
  GLenum pixelFormat = 0;
  GLenum pixelType = 0;
  unsigned int bytesPerPixel = 0;

  switch (m_fourcc)
  {
    case DRM_FORMAT_ARGB8888:
    case DRM_FORMAT_XRGB8888:
      internalFormat = GL_RGBA8;
      pixelFormat = GL_BGRA;
      pixelType = GL_UNSIGNED_BYTE;
      bytesPerPixel = 4;
      break;
    case DRM_FORMAT_ARGB1555:
      internalFormat = GL_RGB;
      pixelFormat = GL_RGB;
      pixelType = GL_UNSIGNED_SHORT_5_5_5_1;
      bytesPerPixel = 2;
      break;
    case DRM_FORMAT_RGB565:
      internalFormat = GL_RGB565;
      pixelFormat = GL_RGB;
      pixelType = GL_UNSIGNED_SHORT_5_6_5;
      bytesPerPixel = 2;
      break;
    default:
      // Better to drop the renderer than to draw the frame in the wrong colors
      CLog::Log(LOGERROR, "CRenderBufferDMAOpenGL: no upload path for fourcc {:#x}", m_fourcc);
      return false;
  }

  const uint8_t* const memory = GetMemory();
  if (memory == nullptr)
    return false;

  GLint previousAlignment = 0;
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
  glPixelStorei(GL_UNPACK_ALIGNMENT, static_cast<GLint>(bytesPerPixel));

  GLint previousRowLength = 0;
  glGetIntegerv(GL_UNPACK_ROW_LENGTH, &previousRowLength);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, static_cast<GLint>(GetStride() / bytesPerPixel));
  glTexImage2D(m_textureTarget, 0, internalFormat, static_cast<GLsizei>(m_width),
               static_cast<GLsizei>(m_height), 0, pixelFormat, pixelType, memory);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, previousRowLength);

  glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);

  ReleaseMemory();

  return true;
}
