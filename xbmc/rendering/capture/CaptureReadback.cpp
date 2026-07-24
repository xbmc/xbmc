/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CaptureReadback.h"

#include "utils/log.h"

#include <cmath>
#include <cstring>
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

namespace
{
// Project a 10-bit sample onto the 16-bit scale so full scale maps to full scale.
uint16_t Project10To16(uint32_t v)
{
  return static_cast<uint16_t>(std::lround(v * 65535.0 / 1023.0));
}

// Decimate a 10-bit sample to 8-bit, matching the GL driver's normalized readback.
uint8_t Project10To8(uint32_t v)
{
  return static_cast<uint8_t>(std::lround(v * 255.0 / 1023.0));
}
} // namespace

void Unpack1010102ToRGBA16(const uint32_t* src, uint16_t* dst, unsigned int width)
{
  for (unsigned int x = 0; x < width; x++, dst += 4)
  {
    const uint32_t p = src[x];
    dst[0] = Project10To16((p >> 0) & 0x3FF); // R (least significant field)
    dst[1] = Project10To16((p >> 10) & 0x3FF); // G
    dst[2] = Project10To16((p >> 20) & 0x3FF); // B
    dst[3] = 0xFFFF; // A
  }
}

void Unpack1010102ToBGRA8(const uint32_t* src, uint8_t* dst, unsigned int width)
{
  for (unsigned int x = 0; x < width; x++, dst += 4)
  {
    const uint32_t p = src[x];
    dst[0] = Project10To8((p >> 20) & 0x3FF); // B (into byte 0, the capture contract)
    dst[1] = Project10To8((p >> 10) & 0x3FF); // G
    dst[2] = Project10To8((p >> 0) & 0x3FF); // R (least significant field)
    dst[3] = 0xFF; // A
  }
}

#if defined(HAS_GL) || HAS_GLES == 3

namespace
{
// Copy h rows of rowBytes from src into a fresh buffer, top-down when flipY.
std::unique_ptr<uint8_t[]> EmitRows(const uint8_t* src,
                                    unsigned int height,
                                    unsigned int rowBytes,
                                    bool flipY)
{
  std::unique_ptr<uint8_t[]> out(new uint8_t[rowBytes * height]);
  for (unsigned int y = 0; y < height; y++)
  {
    const unsigned int srcRow = flipY ? (height - 1 - y) : y;
    std::memcpy(out.get() + y * rowBytes, src + srcRow * rowBytes, rowBytes);
  }
  return out;
}

bool ReadBGRA8Region(
    int x, int y, unsigned int width, unsigned int height, bool flipY, ReadbackBuffer& out)
{
  const unsigned int stride = width * 4;
  std::vector<uint8_t> raw(static_cast<size_t>(stride) * height);
#ifdef HAS_GL
  glReadPixels(x, y, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_BGRA,
               GL_UNSIGNED_BYTE, raw.data());
#else
  glReadPixels(x, y, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGBA,
               GL_UNSIGNED_BYTE, raw.data());
#endif
  if (glGetError() != GL_NO_ERROR)
  {
    CLog::LogF(LOGERROR, "capture readback: glReadPixels 8-bit failed");
    return false;
  }
#ifndef HAS_GL
  // GLES reads RGBA; the capture contract is BGRA
  for (size_t i = 0; i < static_cast<size_t>(width) * height; i++)
    std::swap(raw[i * 4 + 0], raw[i * 4 + 2]);
#endif
  out.pixels = EmitRows(raw.data(), height, stride, flipY);
  out.width = width;
  out.height = height;
  out.stride = stride;
  out.bitDepth = 8;
  return true;
}
} // namespace

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

  if (bitDepth <= 8)
    return ReadBGRA8Region(x, y, width, height, flipY, out);

  const unsigned int stride = width * 8; // RGBA16

#ifdef HAS_GL
  // desktop GL normalizes any framebuffer depth to 16-bit per channel on readback
  std::vector<uint16_t> raw(static_cast<size_t>(width) * height * 4);
  glReadPixels(x, y, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGBA,
               GL_UNSIGNED_SHORT, raw.data());
  if (glGetError() != GL_NO_ERROR)
  {
    CLog::LogF(LOGERROR, "capture readback: glReadPixels 16-bit failed");
    return false;
  }
  for (size_t i = 0; i < static_cast<size_t>(width) * height; i++)
    raw[i * 4 + 3] = 0xFFFF; // composited alpha is not display truth
  out.pixels = EmitRows(reinterpret_cast<const uint8_t*>(raw.data()), height, stride, flipY);
  out.width = width;
  out.height = height;
  out.stride = stride;
  out.bitDepth = bitDepth;
  return true;
#else
  // GLES guarantees only RGBA/UNSIGNED_BYTE; negotiate the deep readback type
  GLint readFormat = GL_RGBA;
  GLint readType = GL_UNSIGNED_BYTE;
  glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &readFormat);
  glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE, &readType);

  if (readFormat == GL_RGBA && readType == GL_UNSIGNED_INT_2_10_10_10_REV)
  {
    std::vector<uint32_t> packed(static_cast<size_t>(width) * height);
    glReadPixels(x, y, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGBA,
                 GL_UNSIGNED_INT_2_10_10_10_REV, packed.data());
    if (glGetError() != GL_NO_ERROR)
    {
      CLog::LogF(LOGERROR, "capture readback: glReadPixels 10-bit failed");
      return false;
    }
    out.pixels.reset(new uint8_t[static_cast<size_t>(stride) * height]);
    for (unsigned int row = 0; row < height; row++)
    {
      const unsigned int srcRow = flipY ? (height - 1 - row) : row;
      Unpack1010102ToRGBA16(packed.data() + static_cast<size_t>(srcRow) * width,
                            reinterpret_cast<uint16_t*>(out.pixels.get() + row * stride), width);
    }
    out.width = width;
    out.height = height;
    out.stride = stride;
    out.bitDepth = bitDepth;
    return true;
  }

  if (readFormat == GL_RGBA && readType == GL_UNSIGNED_SHORT)
  {
    std::vector<uint16_t> raw(static_cast<size_t>(width) * height * 4);
    glReadPixels(x, y, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGBA,
                 GL_UNSIGNED_SHORT, raw.data());
    if (glGetError() != GL_NO_ERROR)
    {
      CLog::LogF(LOGERROR, "capture readback: glReadPixels 16-bit failed");
      return false;
    }
    for (size_t i = 0; i < static_cast<size_t>(width) * height; i++)
      raw[i * 4 + 3] = 0xFFFF;
    out.pixels = EmitRows(reinterpret_cast<const uint8_t*>(raw.data()), height, stride, flipY);
    out.width = width;
    out.height = height;
    out.stride = stride;
    out.bitDepth = bitDepth;
    return true;
  }

  // no deep readback offered: deliver 8-bit output coding, tonemap goes by tags
  return ReadBGRA8Region(x, y, width, height, flipY, out);
#endif
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
  // GLES2 guarantees no deep readback: always 8-bit BGRA, tonemap by tags
  if (width == 0 || height == 0)
    return false;

  const unsigned int stride = width * 4;
  std::vector<uint8_t> raw(static_cast<size_t>(stride) * height);

  while (glGetError() != GL_NO_ERROR)
    ;
  glReadPixels(x, y, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGBA,
               GL_UNSIGNED_BYTE, raw.data());
  if (glGetError() != GL_NO_ERROR)
  {
    CLog::LogF(LOGERROR, "capture readback: glReadPixels 8-bit failed");
    return false;
  }

  out.pixels.reset(new uint8_t[static_cast<size_t>(stride) * height]);
  for (unsigned int row = 0; row < height; row++)
  {
    const unsigned int srcRow = flipY ? (height - 1 - row) : row;
    const uint8_t* src = raw.data() + static_cast<size_t>(srcRow) * stride;
    uint8_t* dst = out.pixels.get() + static_cast<size_t>(row) * stride;
    for (unsigned int px = 0; px < width; px++, src += 4, dst += 4)
    {
      dst[0] = src[2];
      dst[1] = src[1];
      dst[2] = src[0];
      dst[3] = src[3];
    }
  }
  out.width = width;
  out.height = height;
  out.stride = stride;
  out.bitDepth = 8;
  return true;
}

#else

bool ReadFramebufferRegion(
    int, int, unsigned int, unsigned int, int, bool, ReadbackBuffer&)
{
  // no GL context (e.g. Direct3D): callers read through their own API and use
  // Unpack1010102ToRGBA16 for the CPU-side expansion
  return false;
}

#endif

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
