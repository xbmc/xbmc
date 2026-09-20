/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CaptureBlit.h"

#include "ServiceBroker.h"
#include "rendering/RenderSystem.h"
#include "rendering/capture/CapturePixels.h"
#include "rendering/capture/CaptureReadback.h"
#include "rendering/capture/CaptureTypes.h"
#include "utils/MathUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <utility>

#if defined(HAS_GL) || HAS_GLES >= 2
#include "system_gl.h"
#endif

#if !defined(HAS_GL) && HAS_GLES != 2 && HAS_GLES != 3
#include "guilib/D3DResource.h"
#include "rendering/dx/DeviceResources.h"

#include <wrl/client.h>
#endif

namespace KODI
{
namespace RENDERING
{
namespace CAPTURE
{

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

// Move a readback buffer's pixels and shape into a delivered CaptureResult.
void FillResult(CaptureResult& result, ReadbackBuffer& buffer)
{
  result.pixels = std::make_shared<CHeapCapturePixels>(std::move(buffer.pixels));
  result.width = buffer.width;
  result.height = buffer.height;
  result.stride = buffer.stride;
  result.format = buffer.format;
}
} // namespace

#if defined(HAS_GL) || HAS_GLES == 3

namespace
{
// glBlitFramebuffer is core in GL 3.0 and GLES 3.0, but the context version is
// a runtime fact: some winsystems request an ES2 context even where the ES3
// headers were present at build time, so gate on what the driver reports
bool HasFramebufferBlit()
{
  unsigned int major{0};
  unsigned int minor{0};
  CServiceBroker::GetRenderSystem()->GetRenderVersion(major, minor);
  return major >= 3;
}
} // namespace

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
    CLog::LogF(LOGWARNING, "capture framebuffer incomplete: {:#x}", status);
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

  // an ES2 context has no framebuffer blit: read the region at its native size
  // and let the consumer's swscale resize, as the GLES2 and Direct3D paths do
  if (!HasFramebufferBlit())
  {
    m_width = static_cast<unsigned int>(x1 - x0);
    m_height = static_cast<unsigned int>(y1 - y0);
    m_useStaged = ReadFramebufferRegion(x0, fbHeight - y1, m_width, m_height, 8, true, m_staged);
    return m_useStaged;
  }

  // native depth is only distinct from BGRA8 when the output runs above 8 bits
  const int outputBitDepth = CServiceBroker::GetWinSystem()->GetOutputBitDepth();
  const bool highDepth = format == CaptureFormat::NATIVE && outputBitDepth > 8;
  const int readDepth = highDepth ? outputBitDepth : 8;
  m_outputBitDepth = outputBitDepth;

  const unsigned int srcWidth = static_cast<unsigned int>(x1 - x0);
  const unsigned int srcHeight = static_cast<unsigned int>(y1 - y0);

  GLint prevReadFBO{0};
  GLint prevDrawFBO{0};
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevReadFBO);
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDrawFBO);

  // no scaling: read the region straight from the just-drawn framebuffer, no
  // FBO round trip (the driver requantizes a deep surface to 8 bits on read)
  if (width == srcWidth && height == srcHeight)
  {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(prevDrawFBO));
    const bool ok =
        ReadFramebufferRegion(x0, fbHeight - y1, srcWidth, srcHeight, readDepth, true, m_staged);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(prevReadFBO));
    m_useStaged = ok;
    return ok;
  }

  m_useStaged = false;
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
      CLog::LogF(LOGWARNING, "glBlitFramebuffer failed");
  }

  glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(prevReadFBO));
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(prevDrawFBO));
  if (scissor)
    glEnable(GL_SCISSOR_TEST);

  return ok;
}

bool CCaptureBlit::Read(CaptureResult& result)
{
  // native-size request: the pixels were read straight into m_staged by Blit()
  if (m_useStaged)
  {
    if (!m_staged.pixels)
      return false;
    FillResult(result, m_staged);
    return true;
  }

  if (!m_fbo)
    return false;

  GLint prevReadFBO{0};
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevReadFBO);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);

  // the FBO already holds top-down rows (the blit inverted them), so no flip
  ReadbackBuffer buffer;
  const bool ok = ReadFramebufferRegion(0, 0, m_width, m_height,
                                        m_isHighDepth ? m_outputBitDepth : 8, false, buffer);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(prevReadFBO));

  if (!ok)
    return false;

  FillResult(result, buffer);
  return true;
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

  // window rows are bottom-up: flip to top-down, 8-bit only on GLES2
  return ReadFramebufferRegion(x0, fbHeight - y1, m_width, m_height, 8, true, m_staged);
}

bool CCaptureBlit::Read(CaptureResult& result)
{
  if (!m_staged.pixels)
    return false;

  FillResult(result, m_staged);
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
  // Direct3D cannot scale in a copy, so the tap always reads the source region
  // at its native size into m_staged; the consumer's swscale conversion scales
  // and requantizes. width/height are advisory here.
  int x0{0};
  int y0{0};
  int x1{0};
  int y1{0};
  int fbHeight{0};
  if (!ClampToOutput(srcRect, x0, y0, x1, y1, fbHeight))
    return false;

  const unsigned int rw = static_cast<unsigned int>(x1 - x0);
  const unsigned int rh = static_cast<unsigned int>(y1 - y0);

  auto deviceResources = DX::DeviceResources::Get();
  deviceResources->FinishCommandList();
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> context = deviceResources->GetImmediateContext();
  Microsoft::WRL::ComPtr<ID3D11Device> device = deviceResources->GetD3DDevice();
  CD3DTexture& backbuffer = deviceResources->GetBackBuffer();
  if (!backbuffer.Get())
    return false;

  D3D11_TEXTURE2D_DESC desc = {};
  backbuffer.GetDesc(&desc);
  const DXGI_FORMAT sourceFormat = desc.Format;
  desc.Width = rw;
  desc.Height = rh;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.Usage = D3D11_USAGE_STAGING;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  desc.BindFlags = 0;
  desc.MiscFlags = 0;

  Microsoft::WRL::ComPtr<ID3D11Texture2D> staging;
  if (FAILED(device->CreateTexture2D(&desc, nullptr, &staging)))
  {
    CLog::LogF(LOGWARNING, "capture staging texture creation failed");
    return false;
  }

  // D3D textures are top-left origin: the box maps directly, no Y flip
  const D3D11_BOX box{static_cast<UINT>(x0), static_cast<UINT>(y0), 0,
                      static_cast<UINT>(x1), static_cast<UINT>(y1), 1};
  context->CopySubresourceRegion(staging.Get(), 0, 0, 0, 0, backbuffer.Get(), 0, &box);

  D3D11_MAPPED_SUBRESOURCE mapped;
  if (FAILED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)))
  {
    CLog::LogF(LOGWARNING, "capture staging map failed");
    return false;
  }

  // no CPU unpack: swscale on the consumer expands or decimates the packed
  // 10-bit as its target needs. width/height are advisory here.
  const bool packed10 = sourceFormat == DXGI_FORMAT_R10G10B10A2_UNORM;
  m_staged.stride = static_cast<int>(rw * 4);
  m_staged.format = packed10 ? AV_PIX_FMT_X2BGR10LE : AV_PIX_FMT_BGRA;
  m_staged.pixels.reset(new uint8_t[static_cast<size_t>(m_staged.stride) * rh]);
  for (unsigned int y = 0; y < rh; y++)
    std::memcpy(m_staged.pixels.get() + static_cast<size_t>(y) * m_staged.stride,
                static_cast<const uint8_t*>(mapped.pData) +
                    static_cast<size_t>(y) * mapped.RowPitch,
                m_staged.stride);
  m_staged.width = rw;
  m_staged.height = rh;
  context->Unmap(staging.Get(), 0);
  m_useStaged = true;
  return true;
}

bool CCaptureBlit::Read(CaptureResult& result)
{
  if (!m_useStaged || !m_staged.pixels)
    return false;

  FillResult(result, m_staged);
  return true;
}

#endif

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
