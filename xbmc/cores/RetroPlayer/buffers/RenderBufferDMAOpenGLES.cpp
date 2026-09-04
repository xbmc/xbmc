/*
 *  Copyright (C) 2017-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderBufferDMAOpenGLES.h"

#include "rendering/GLExtensions.h"
#include "utils/log.h"

#include <cstring>
#include <vector>

#include <drm_fourcc.h>

namespace
{
enum class UploadConversion
{
  NONE,
  BGRA_TO_RGBA,
  ARGB1555_TO_RGB565,
};

void ConvertBgraToRgba(const uint8_t* source, uint8_t* destination, unsigned int width)
{
  for (unsigned int x = 0; x < width; ++x)
  {
    destination[x * 4] = source[x * 4 + 2];
    destination[x * 4 + 1] = source[x * 4 + 1];
    destination[x * 4 + 2] = source[x * 4];
    destination[x * 4 + 3] = source[x * 4 + 3];
  }
}

void ConvertArgb1555ToRgb565(const uint8_t* source, uint8_t* destination, unsigned int width)
{
  for (unsigned int x = 0; x < width; ++x)
  {
    uint16_t argb1555 = 0;
    std::memcpy(&argb1555, source + x * sizeof(argb1555), sizeof(argb1555));

    const uint16_t red = (argb1555 >> 10) & 0x1f;
    const uint16_t green = (argb1555 >> 5) & 0x1f;
    const uint16_t blue = argb1555 & 0x1f;
    const uint16_t green6 = (green << 1) | (green >> 4);
    const uint16_t rgb565 = (red << 11) | (green6 << 5) | blue;

    std::memcpy(destination + x * sizeof(rgb565), &rgb565, sizeof(rgb565));
  }
}
} // namespace

using namespace KODI;
using namespace RETRO;

CRenderBufferDMAOpenGLES::CRenderBufferDMAOpenGLES(int fourcc) : CRenderBufferDMA(fourcc)
{
}

void CRenderBufferDMAOpenGLES::ConfigureTexture()
{
  // Force alpha to 1, because game clients can leave it undefined
#if defined(GL_ES_VERSION_3_0)
  if (m_fourcc == DRM_FORMAT_ARGB8888 || m_fourcc == DRM_FORMAT_ARGB1555)
    glTexParameteri(m_textureTarget, GL_TEXTURE_SWIZZLE_A, GL_ONE);
#endif
}

bool CRenderBufferDMAOpenGLES::UploadFromMemory()
{
  GLint internalFormat = 0;
  GLenum pixelFormat = 0;
  GLenum pixelType = 0;
  unsigned int bytesPerPixel = 0;
  UploadConversion conversion = UploadConversion::NONE;

  switch (m_fourcc)
  {
    case DRM_FORMAT_ARGB8888:
    case DRM_FORMAT_XRGB8888:
      pixelType = GL_UNSIGNED_BYTE;
      bytesPerPixel = 4;
      if (CGLExtensions::IsExtensionSupported(CGLExtensions::EXT_texture_format_BGRA8888) ||
          CGLExtensions::IsExtensionSupported(CGLExtensions::IMG_texture_format_BGRA8888))
      {
        internalFormat = GL_BGRA_EXT;
        pixelFormat = GL_BGRA_EXT;
      }
      else if (CGLExtensions::IsExtensionSupported(CGLExtensions::APPLE_texture_format_BGRA8888))
      {
        // Apple's extension requires RGBA as the internal format.
        internalFormat = GL_RGBA;
        pixelFormat = GL_BGRA_EXT;
      }
      else
      {
        internalFormat = GL_RGBA;
        pixelFormat = GL_RGBA;
        conversion = UploadConversion::BGRA_TO_RGBA;
      }
      break;
    case DRM_FORMAT_ARGB1555:
      internalFormat = GL_RGB;
      pixelFormat = GL_RGB;
      pixelType = GL_UNSIGNED_SHORT_5_6_5;
      bytesPerPixel = 2;
      conversion = UploadConversion::ARGB1555_TO_RGB565;
      break;
    case DRM_FORMAT_RGB565:
      internalFormat = GL_RGB;
      pixelFormat = GL_RGB;
      pixelType = GL_UNSIGNED_SHORT_5_6_5;
      bytesPerPixel = 2;
      break;
    default:
      // Better to drop the renderer than to draw the frame in the wrong colors
      CLog::Log(LOGERROR, "CRenderBufferDMAOpenGLES: no upload path for fourcc {:#x}", m_fourcc);
      return false;
  }

  const uint8_t* const memory = GetMemory();
  if (memory == nullptr)
    return false;

  const uint32_t stride = GetStride();

  GLint previousAlignment = 0;
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
  glPixelStorei(GL_UNPACK_ALIGNMENT, static_cast<GLint>(bytesPerPixel));

  if (conversion != UploadConversion::NONE)
  {
    glTexImage2D(m_textureTarget, 0, internalFormat, static_cast<GLsizei>(m_width),
                 static_cast<GLsizei>(m_height), 0, pixelFormat, pixelType, nullptr);

    std::vector<uint8_t> convertedRow(static_cast<size_t>(m_width) * bytesPerPixel);

    for (unsigned int y = 0; y < m_height; ++y)
    {
      const uint8_t* const sourceRow = memory + y * stride;
      if (conversion == UploadConversion::BGRA_TO_RGBA)
        ConvertBgraToRgba(sourceRow, convertedRow.data(), m_width);
      else
        ConvertArgb1555ToRgb565(sourceRow, convertedRow.data(), m_width);

      glTexSubImage2D(m_textureTarget, 0, 0, static_cast<GLint>(y), static_cast<GLsizei>(m_width),
                      1, pixelFormat, pixelType, convertedRow.data());
    }
  }
  else
  {
#if defined(GL_UNPACK_ROW_LENGTH_EXT)
    if (CGLExtensions::IsExtensionSupported(CGLExtensions::EXT_unpack_subimage))
    {
      GLint previousRowLength = 0;
      glGetIntegerv(GL_UNPACK_ROW_LENGTH_EXT, &previousRowLength);
      glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, static_cast<GLint>(stride / bytesPerPixel));
      glTexImage2D(m_textureTarget, 0, internalFormat, static_cast<GLsizei>(m_width),
                   static_cast<GLsizei>(m_height), 0, pixelFormat, pixelType, memory);
      glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, previousRowLength);
    }
    else
#endif
    {
      glTexImage2D(m_textureTarget, 0, internalFormat, static_cast<GLsizei>(m_width),
                   static_cast<GLsizei>(m_height), 0, pixelFormat, pixelType, nullptr);

      for (unsigned int y = 0; y < m_height; ++y)
      {
        const uint8_t* const sourceRow = memory + y * stride;
        glTexSubImage2D(m_textureTarget, 0, 0, static_cast<GLint>(y), static_cast<GLsizei>(m_width),
                        1, pixelFormat, pixelType, sourceRow);
      }
    }
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);

  ReleaseMemory();

  return true;
}
