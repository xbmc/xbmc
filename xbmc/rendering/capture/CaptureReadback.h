/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <memory>

extern "C"
{
#include <libavutil/pixfmt.h>
}

namespace KODI
{
namespace RENDERING
{
namespace CAPTURE
{

//! Pixels read back from the GPU, tagged with their coding. Floating-point channels
//! are normalized to 16-bit; channel order, row order and alpha are unchanged.
struct ReadbackBuffer
{
  std::unique_ptr<uint8_t[]> pixels;
  unsigned int width{0};
  unsigned int height{0};
  //! signed linesize; negative when the rows are bottom-up
  int stride{0};
  AVPixelFormat format{AV_PIX_FMT_NONE};
};

//! Read a width x height region of the bound read framebuffer at (x, y) in GL
//! (bottom-left origin) coordinates. The bytes land in out.pixels;
//! out.format names their coding and out.stride is signed (negative when flipY,
//! i.e. bottom-up window rows). bitDepth selects an 8-bit or deep readback; on
//! GLES normalized buffers negotiate a deep type and fall back to 8-bit when none
//! is offered; floating-point buffers are converted to RGBA64 at either depth.
//! Returns false on GL error, leaving out untouched. The single GPU readback
//! shared by the screenshot surfaces and the capture blit.
bool ReadFramebufferRegion(int x,
                           int y,
                           unsigned int width,
                           unsigned int height,
                           int bitDepth,
                           bool flipY,
                           ReadbackBuffer& out);

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
