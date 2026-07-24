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

namespace KODI
{
namespace RENDERING
{
namespace CAPTURE
{

//! Pixels read back from the GPU; rows are top-down, BGRA8 or RGBA16.
struct ReadbackBuffer
{
  std::unique_ptr<uint8_t[]> pixels;
  unsigned int width{0};
  unsigned int height{0};
  unsigned int stride{0};
  int bitDepth{8};
};

//! Read a width x height region of the currently bound read framebuffer,
//! starting at (x, y) in GL (bottom-left origin) coordinates.
//!
//! bitDepth 8 delivers BGRA8; a deeper request delivers RGBA16 with opaque
//! alpha. On GLES the deep readback type is negotiated via
//! GL_IMPLEMENTATION_COLOR_READ_*; when the driver offers nothing beyond
//! 8-bit the call still succeeds with out.bitDepth == 8 (the caller tonemaps
//! by tags). flipY reverses bottom-up window rows to top-down; pass false for
//! a target that is already top-down (the capture blit FBO). Returns false on
//! GL error, leaving out untouched. This is the single readback path shared by
//! the screenshot surfaces and the capture blit.
bool ReadFramebufferRegion(int x,
                           int y,
                           unsigned int width,
                           unsigned int height,
                           int bitDepth,
                           bool flipY,
                           ReadbackBuffer& out);

//! Expand one row of packed 10:10:10:2 RGBA (as delivered by
//! GL_UNSIGNED_INT_2_10_10_10_REV or DXGI R10G10B10A2) to RGBA16, projecting
//! each 10-bit sample onto the full 16-bit scale so 1023 maps to 65535, with
//! opaque alpha. The single home of the 10-bit expansion convention; the GL
//! path and the Direct3D staging path both call it. dst holds width*4 uint16_t.
void Unpack1010102ToRGBA16(const uint32_t* src, uint16_t* dst, unsigned int width);

//! Decimate one row of packed 10:10:10:2 RGBA to 8-bit BGRA (v * 255 / 1023,
//! matching the GL driver's UNSIGNED_BYTE readback), opaque alpha. The Direct3D
//! staging copy has no free conversion, so a BGRA8 request from a 10-bit
//! swapchain unpacks through here. dst holds width*4 bytes.
void Unpack1010102ToBGRA8(const uint32_t* src, uint8_t* dst, unsigned int width);

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
