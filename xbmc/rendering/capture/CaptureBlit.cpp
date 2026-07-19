/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CaptureBlit.h"

#include "ServiceBroker.h"
#include "rendering/capture/CaptureTypes.h"
#include "utils/MathUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <algorithm>
#include <cstring>
#include <utility>

#include "system_gl.h"

namespace KODI
{
namespace RENDERING
{
namespace CAPTURE
{

#if defined(HAS_GL) || HAS_GLES >= 2
namespace
{
// Integer source region clamped to the output surface; zoom pushes destRect beyond it.
bool ClampToOutput(const CRect& srcRect, int& x0, int& y0, int& x1, int& y1, int& fbHeight)
{
  const CGraphicContext& gfx = CServiceBroker::GetWinSystem()->GetGfxContext();
  fbHeight = gfx.GetHeight();
  x0 = std::max(0, MathUtils::round_int(srcRect.x1));
  y0 = std::max(0, MathUtils::round_int(srcRect.y1));
  x1 = std::min(gfx.GetWidth(), MathUtils::round_int(srcRect.x2));
  y1 = std::min(fbHeight, MathUtils::round_int(srcRect.y2));
  return x1 > x0 && y1 > y0;
}
} // namespace
#endif

#if defined(HAS_GL) || HAS_GLES == 3

CCaptureBlit::~CCaptureBlit()
{
  Release();
}

void CCaptureBlit::Release()
{
  if (m_fbo)
    glDeleteFramebuffers(1, &m_fbo);
  if (m_texture)
    glDeleteTextures(1, &m_texture);
  m_fbo = 0;
  m_texture = 0;
  m_width = 0;
  m_height = 0;
}

bool CCaptureBlit::EnsureFramebuffer(unsigned int width, unsigned int height, bool highDepth)
{
  if (m_fbo && width == m_width && height == m_height && highDepth == m_isHighDepth)
    return true;

  Release();

  // high-depth target: GL_RGBA16 is required color-renderable in GL 3.2;
  // GLES3 has no renderable RGBA16 so the 10-bit GL_RGB10_A2 core format is used
#ifdef HAS_GL
  const GLint internalFormat = highDepth ? GL_RGBA16 : GL_RGBA8;
  const GLenum type = highDepth ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE;
#else
  const GLint internalFormat = highDepth ? GL_RGB10_A2 : GL_RGBA8;
  const GLenum type = highDepth ? GL_UNSIGNED_INT_2_10_10_10_REV : GL_UNSIGNED_BYTE;
#endif

  GLint prevTexture{0};
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTexture);

  glGenTextures(1, &m_texture);
  glBindTexture(GL_TEXTURE_2D, m_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, static_cast<GLsizei>(width),
               static_cast<GLsizei>(height), 0, GL_RGBA, type, nullptr);
  glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(prevTexture));

  glGenFramebuffers(1, &m_fbo);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);
  const GLenum status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE)
  {
    CLog::LogF(LOGERROR, "capture framebuffer incomplete: {:#x}", status);
    Release();
    return false;
  }

  m_width = width;
  m_height = height;
  m_isHighDepth = highDepth;
  return true;
}

bool CCaptureBlit::Blit(const CRect& srcRect,
                        unsigned int width,
                        unsigned int height,
                        CaptureFormat format)
{
  if (width == 0 || height == 0)
    return false;

  int x0{0};
  int y0{0};
  int x1{0};
  int y1{0};
  int fbHeight{0};
  if (!ClampToOutput(srcRect, x0, y0, x1, y1, fbHeight))
    return false;

  // native depth is only distinct from BGRA8 when the output runs above 8 bits
  const int outputBitDepth = CServiceBroker::GetWinSystem()->GetOutputBitDepth();
  const bool highDepth = format == CaptureFormat::NATIVE && outputBitDepth > 8;
  m_outputBitDepth = outputBitDepth;

  GLint prevReadFBO{0};
  GLint prevDrawFBO{0};
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevReadFBO);
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDrawFBO);
  const GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
  if (scissor)
    glDisable(GL_SCISSOR_TEST);

  bool ok = EnsureFramebuffer(width, height, highDepth);
  if (ok)
  {
    while (glGetError() != GL_NO_ERROR)
      ;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(prevDrawFBO));
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);

    // source is bottom-up GL coordinates; the inverted destination yields top-down rows
    glBlitFramebuffer(x0, fbHeight - y1, x1, fbHeight - y0, 0, static_cast<GLint>(height),
                      static_cast<GLint>(width), 0, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    ok = glGetError() == GL_NO_ERROR;
    if (!ok)
      CLog::LogF(LOGERROR, "glBlitFramebuffer failed");
  }

  glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(prevReadFBO));
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(prevDrawFBO));
  if (scissor)
    glEnable(GL_SCISSOR_TEST);

  return ok;
}

bool CCaptureBlit::Read(CaptureResult& result)
{
  if (!m_fbo)
    return false;

  GLint prevReadFBO{0};
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevReadFBO);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);

  bool ok = false;
  if (m_isHighDepth)
    ok = ReadHighDepth(result);
  else
    ok = ReadBGRA8(result);

  glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(prevReadFBO));
  return ok;
}

bool CCaptureBlit::ReadBGRA8(CaptureResult& result)
{
  const unsigned int stride = m_width * 4;
  std::shared_ptr<uint8_t[]> pixels(new uint8_t[stride * m_height]);
#ifdef HAS_GL
  glReadPixels(0, 0, static_cast<GLsizei>(m_width), static_cast<GLsizei>(m_height), GL_BGRA,
               GL_UNSIGNED_BYTE, pixels.get());
#else
  glReadPixels(0, 0, static_cast<GLsizei>(m_width), static_cast<GLsizei>(m_height), GL_RGBA,
               GL_UNSIGNED_BYTE, pixels.get());
#endif
  if (glGetError() != GL_NO_ERROR)
  {
    CLog::LogF(LOGERROR, "glReadPixels failed");
    return false;
  }

#ifndef HAS_GL
  // GLES reads RGBA; the capture contract is BGRA
  uint8_t* swap = pixels.get();
  for (unsigned int i = 0; i < m_width * m_height; i++, swap += 4)
    std::swap(swap[0], swap[2]);
#endif

  result.pixels = std::move(pixels);
  result.width = m_width;
  result.height = m_height;
  result.stride = stride;
  result.bitDepth = 8;
  return true;
}

bool CCaptureBlit::ReadHighDepth(CaptureResult& result)
{
  const unsigned int stride = m_width * 8;

#ifdef HAS_GL
  // desktop GL converts any framebuffer depth to 16-bit per channel on readback
  std::shared_ptr<uint8_t[]> pixels(new uint8_t[stride * m_height]);
  glReadPixels(0, 0, static_cast<GLsizei>(m_width), static_cast<GLsizei>(m_height), GL_RGBA,
               GL_UNSIGNED_SHORT, pixels.get());
  if (glGetError() != GL_NO_ERROR)
  {
    CLog::LogF(LOGERROR, "glReadPixels 16-bit failed");
    return false;
  }

  uint16_t* alpha = reinterpret_cast<uint16_t*>(pixels.get()) + 3;
  for (unsigned int i = 0; i < m_width * m_height; i++, alpha += 4)
    *alpha = 0xFFFF;

  result.pixels = std::move(pixels);
  result.width = m_width;
  result.height = m_height;
  result.stride = stride;
  result.bitDepth = m_outputBitDepth;
  return true;
#else
  // GLES negotiates the readback type; only GL_RGBA is guaranteed beyond 8-bit
  GLint readFormat = GL_RGBA;
  GLint readType = GL_UNSIGNED_BYTE;
  glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &readFormat);
  glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE, &readType);

  if (readFormat == GL_RGBA && readType == GL_UNSIGNED_INT_2_10_10_10_REV)
  {
    std::vector<uint32_t> packed(m_width * m_height);
    glReadPixels(0, 0, static_cast<GLsizei>(m_width), static_cast<GLsizei>(m_height), GL_RGBA,
                 GL_UNSIGNED_INT_2_10_10_10_REV, packed.data());
    if (glGetError() != GL_NO_ERROR)
    {
      CLog::LogF(LOGERROR, "glReadPixels 10-bit failed");
      return false;
    }

    std::shared_ptr<uint8_t[]> pixels(new uint8_t[stride * m_height]);
    uint16_t* dst = reinterpret_cast<uint16_t*>(pixels.get());
    for (unsigned int i = 0; i < m_width * m_height; i++, dst += 4)
    {
      const uint32_t p = packed[i];
      // renormalize 10-bit to the full 16-bit scale
      const uint16_t r = static_cast<uint16_t>((p >> 0) & 0x3FF);
      const uint16_t g = static_cast<uint16_t>((p >> 10) & 0x3FF);
      const uint16_t b = static_cast<uint16_t>((p >> 20) & 0x3FF);
      dst[0] = static_cast<uint16_t>((r << 6) | (r >> 4));
      dst[1] = static_cast<uint16_t>((g << 6) | (g >> 4));
      dst[2] = static_cast<uint16_t>((b << 6) | (b >> 4));
      dst[3] = 0xFFFF;
    }

    result.pixels = std::move(pixels);
    result.width = m_width;
    result.height = m_height;
    result.stride = stride;
    result.bitDepth = m_outputBitDepth;
    return true;
  }

  if (readFormat == GL_RGBA && readType == GL_UNSIGNED_SHORT)
  {
    std::shared_ptr<uint8_t[]> pixels(new uint8_t[stride * m_height]);
    glReadPixels(0, 0, static_cast<GLsizei>(m_width), static_cast<GLsizei>(m_height), GL_RGBA,
                 GL_UNSIGNED_SHORT, pixels.get());
    if (glGetError() != GL_NO_ERROR)
    {
      CLog::LogF(LOGERROR, "glReadPixels 16-bit failed");
      return false;
    }

    uint16_t* alpha = reinterpret_cast<uint16_t*>(pixels.get()) + 3;
    for (unsigned int i = 0; i < m_width * m_height; i++, alpha += 4)
      *alpha = 0xFFFF;

    result.pixels = std::move(pixels);
    result.width = m_width;
    result.height = m_height;
    result.stride = stride;
    result.bitDepth = m_outputBitDepth;
    return true;
  }

  // driver offers no high-depth readback: 8-bit output coding, tonemap goes by tags
  return ReadBGRA8(result);
#endif
}

#elif HAS_GLES == 2

CCaptureBlit::~CCaptureBlit() = default;

void CCaptureBlit::Release()
{
}

bool CCaptureBlit::EnsureFramebuffer(unsigned int width, unsigned int height, bool highDepth)
{
  return false;
}

bool CCaptureBlit::Blit(const CRect& srcRect,
                        unsigned int width,
                        unsigned int height,
                        CaptureFormat format)
{
  // GLES2 has no framebuffer blit and no high-depth readback: read the region
  // at native size in 8-bit; the consumer-side conversion scales and goes by tags
  int x0{0};
  int y0{0};
  int x1{0};
  int y1{0};
  int fbHeight{0};
  if (!ClampToOutput(srcRect, x0, y0, x1, y1, fbHeight))
    return false;

  m_width = static_cast<unsigned int>(x1 - x0);
  m_height = static_cast<unsigned int>(y1 - y0);
  m_staged.resize(m_width * m_height * 4);

  while (glGetError() != GL_NO_ERROR)
    ;
  glReadPixels(x0, fbHeight - y1, static_cast<GLsizei>(m_width), static_cast<GLsizei>(m_height),
               GL_RGBA, GL_UNSIGNED_BYTE, m_staged.data());
  if (glGetError() != GL_NO_ERROR)
  {
    CLog::LogF(LOGERROR, "glReadPixels failed");
    m_staged.clear();
    return false;
  }
  return true;
}

bool CCaptureBlit::Read(CaptureResult& result)
{
  if (m_staged.empty())
    return false;

  const unsigned int stride = m_width * 4;
  std::shared_ptr<uint8_t[]> pixels(new uint8_t[stride * m_height]);
  for (unsigned int y = 0; y < m_height; y++)
  {
    // flip to top-down rows and swap RGBA to BGRA
    const uint8_t* src = m_staged.data() + (m_height - 1 - y) * stride;
    uint8_t* dst = pixels.get() + y * stride;
    for (unsigned int x = 0; x < m_width; x++, src += 4, dst += 4)
    {
      dst[0] = src[2];
      dst[1] = src[1];
      dst[2] = src[0];
      dst[3] = src[3];
    }
  }

  result.pixels = std::move(pixels);
  result.width = m_width;
  result.height = m_height;
  result.stride = stride;
  result.bitDepth = 8;
  return true;
}

#else

CCaptureBlit::~CCaptureBlit() = default;

void CCaptureBlit::Release()
{
}

bool CCaptureBlit::EnsureFramebuffer(unsigned int width, unsigned int height, bool highDepth)
{
  return false;
}

bool CCaptureBlit::Blit(const CRect& srcRect,
                        unsigned int width,
                        unsigned int height,
                        CaptureFormat format)
{
  //! @todo D3D11 sub-region staging copy so the video tap serves Windows
  return false;
}

bool CCaptureBlit::Read(CaptureResult& result)
{
  return false;
}

#endif

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
