/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Geometry.h"

#include <cstdint>
#include <vector>

namespace KODI
{
namespace RENDERING
{
namespace CAPTURE
{

struct CaptureResult;

/*!
 \brief Scaled copy of already-presented pixels into a private FBO, plus readback.

 Render thread only, with the frame's GL context current. Blit() copies the
 given region of the currently bound draw framebuffer; Read() hands the copied
 pixels back as top-down 8-bit BGRA. GLES2 cannot blit and reads the region at
 its native size instead; builds without GL fail both calls. The GPU-to-CPU
 seam stays inside this class so an async PBO readback can replace Read()
 without touching callers.
 */
class CCaptureBlit
{
public:
  CCaptureBlit() = default;
  ~CCaptureBlit();
  CCaptureBlit(const CCaptureBlit&) = delete;
  CCaptureBlit& operator=(const CCaptureBlit&) = delete;

  //! \brief Copy srcRect (display coordinates, top-left origin) scaled to width x height.
  bool Blit(const CRect& srcRect, unsigned int width, unsigned int height);

  //! \brief Read the copied pixels back; valid after a successful Blit().
  bool Read(CaptureResult& result);

private:
  bool EnsureFramebuffer(unsigned int width, unsigned int height);
  void Release();

  //! GL object names as plain integers so this header stays GL-free
  uint32_t m_fbo{0};
  uint32_t m_texture{0};
  unsigned int m_width{0};
  unsigned int m_height{0};
  //! GLES2 no-blit path: pixels staged by Blit() for Read()
  std::vector<uint8_t> m_staged;
};

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
