/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "rendering/capture/CaptureReadback.h"
#include "rendering/capture/CaptureTypes.h"
#include "utils/Geometry.h"

#include <cstdint>

namespace KODI
{
namespace RENDERING
{
namespace CAPTURE
{

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
  //! NATIVE keeps the output depth in the copy; BGRA8 requantizes to 8-bit during it.
  bool Blit(const CRect& srcRect,
            unsigned int width,
            unsigned int height,
            CaptureFormat format = CaptureFormat::BGRA8);

  //! \brief Read the copied pixels back; valid after a successful Blit().
  bool Read(CaptureResult& result);

private:
  bool EnsureFramebuffer(unsigned int width, unsigned int height, bool highDepth);
  void Release();

  //! GL object names as plain integers so this header stays GL-free
  uint32_t m_fbo{0};
  uint32_t m_texture{0};
  unsigned int m_width{0};
  unsigned int m_height{0};
  //! target holds the output's native depth instead of 8-bit BGRA
  bool m_isHighDepth{false};
  //! output surface depth captured at Blit() time, valid when m_isHighDepth
  int m_outputBitDepth{8};
  //! true when Blit() read the region directly (no scaling) into m_staged
  bool m_useStaged{false};
  //! pixels read straight from the framebuffer when no scaling is needed
  //! (always on GLES2, and on GL/GLES3 for a native-size request)
  ReadbackBuffer m_staged;
};

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
