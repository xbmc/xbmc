/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CaptureReadback.h"

#include "utils/log.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <utility>
#include <vector>

#if defined(HAS_GL) || defined(HAS_GLES)
#include "system_gl.h"
#endif

namespace KODI
{
namespace RENDERING
{
namespace CAPTURE
{

#if defined(HAS_GL) || HAS_GLES == 3

bool ReadFramebufferRegion(int x,
                           int y,
                           unsigned int width,
                           unsigned int height,
                           int bitDepth,
                           bool flipY,
                           ReadbackBuffer& out)
{
  if (width == 0 || height == 0)
    return false;

  while (glGetError() != GL_NO_ERROR)
    ;

  GLenum glFormat = GL_RGBA;
  GLenum glType = GL_UNSIGNED_BYTE;
  AVPixelFormat avFormat = AV_PIX_FMT_RGBA;
  unsigned int rowBytes = width * 4;

#if HAS_GLES == 3
  GLint componentType = GL_UNSIGNED_NORMALIZED;
  unsigned int majorVersion = 0;
  const auto* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
  if (version)
    std::sscanf(version, "OpenGL ES %u", &majorVersion);
  if (majorVersion >= 3)
  {
    GLint readBuffer = GL_BACK;
    glGetIntegerv(GL_READ_BUFFER, &readBuffer);
    glGetFramebufferAttachmentParameteriv(GL_READ_FRAMEBUFFER, readBuffer,
                                          GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, &componentType);
    if (glGetError() != GL_NO_ERROR)
    {
      CLog::LogF(LOGWARNING, "capture readback: framebuffer attachment query failed");
      return false;
    }
  }
  if (componentType == GL_FLOAT)
  {
    glType = GL_FLOAT;
    avFormat = AV_PIX_FMT_RGBA64LE;
    rowBytes = width * 8;
  }
  else
#endif
  if (bitDepth <= 8)
  {
#ifdef HAS_GL
    glFormat = GL_BGRA; // desktop GL reads BGRA directly
    avFormat = AV_PIX_FMT_BGRA;
#endif
  }
  else
  {
#ifdef HAS_GL
    // desktop GL normalizes any framebuffer depth to 16-bit per channel on read
    glType = GL_UNSIGNED_SHORT;
    avFormat = AV_PIX_FMT_RGBA64LE;
    rowBytes = width * 8;
#else
    // For normalized buffers, negotiate a deep readback before falling back to RGBA8.
    GLint readFormat = GL_RGBA;
    GLint readType = GL_UNSIGNED_BYTE;
    glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &readFormat);
    glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE, &readType);
    if (readFormat == GL_RGBA && readType == GL_UNSIGNED_INT_2_10_10_10_REV)
    {
      glType = GL_UNSIGNED_INT_2_10_10_10_REV;
      avFormat = AV_PIX_FMT_X2BGR10LE; // R in the low field, as GL_RGBA/2_10_10_10_REV
    }
    else if (readFormat == GL_RGBA && readType == GL_UNSIGNED_SHORT)
    {
      glType = GL_UNSIGNED_SHORT;
      avFormat = AV_PIX_FMT_RGBA64LE;
      rowBytes = width * 8;
    }
    // else: no deep readback offered; fall through to the 8-bit RGBA defaults
#endif
  }

  auto buffer = std::make_unique<uint8_t[]>(static_cast<size_t>(rowBytes) * height);
  std::vector<float> floatPixels;
  void* pixels = buffer.get();
  if (glType == GL_FLOAT)
  {
    floatPixels.resize(static_cast<size_t>(width) * height * 4);
    pixels = floatPixels.data();
  }
  glReadPixels(x, y, static_cast<GLsizei>(width), static_cast<GLsizei>(height), glFormat, glType,
               pixels);
  if (glGetError() != GL_NO_ERROR)
  {
    CLog::LogF(LOGWARNING, "capture readback: glReadPixels failed");
    return false;
  }

  // FFmpeg's packed RGBA input supports normalized 16-bit channels, not 32-bit floats.
  for (size_t i = 0; i < floatPixels.size(); ++i)
  {
    const float sample = std::isnan(floatPixels[i]) ? 0.0f : std::clamp(floatPixels[i], 0.0f, 1.0f);
    const uint16_t value = static_cast<uint16_t>(static_cast<double>(sample) * 65535.0 + 0.5);
    buffer[2 * i] = static_cast<uint8_t>(value & 0xff);
    buffer[2 * i + 1] = static_cast<uint8_t>(value >> 8);
  }

  out.pixels = std::move(buffer);
  out.width = width;
  out.height = height;
  out.stride = flipY ? -static_cast<int>(rowBytes) : static_cast<int>(rowBytes);
  out.format = avFormat;
  return true;
}

#elif HAS_GLES == 2

bool ReadFramebufferRegion(int x,
                           int y,
                           unsigned int width,
                           unsigned int height,
                           int /*bitDepth*/,
                           bool flipY,
                           ReadbackBuffer& out)
{
  // GLES2 guarantees no deep readback: always 8-bit RGBA, tonemap by tags
  if (width == 0 || height == 0)
    return false;

  const unsigned int rowBytes = width * 4;
  auto buffer = std::make_unique<uint8_t[]>(static_cast<size_t>(rowBytes) * height);

  while (glGetError() != GL_NO_ERROR)
    ;
  glReadPixels(x, y, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGBA,
               GL_UNSIGNED_BYTE, buffer.get());
  if (glGetError() != GL_NO_ERROR)
  {
    CLog::LogF(LOGWARNING, "capture readback: glReadPixels failed");
    return false;
  }

  out.pixels = std::move(buffer);
  out.width = width;
  out.height = height;
  out.stride = flipY ? -static_cast<int>(rowBytes) : static_cast<int>(rowBytes);
  out.format = AV_PIX_FMT_RGBA;
  return true;
}

#else

bool ReadFramebufferRegion(int, int, unsigned int, unsigned int, int, bool, ReadbackBuffer&)
{
  // no GL context (e.g. Direct3D): callers read through their own API
  return false;
}

#endif

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
