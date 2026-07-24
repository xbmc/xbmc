/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>

namespace KODI
{
namespace RENDERING
{
namespace CAPTURE
{

struct CaptureResult;

//! \brief Fit an already-8-bit-BGRA capture to width x height, copying or
//! scaling only, with NO tonemap or color management: the output coding is
//! carried through verbatim. This is the Python RenderCapture contract (raw
//! output-coded bytes, HDR tags ignored) and the cheap path other consumers
//! reach when no conversion is needed. The buffer must hold width*height*4.
bool CaptureCopyBGRA8(const CaptureResult& result,
                      unsigned int width,
                      unsigned int height,
                      uint8_t* buffer);

//! \brief Convert a delivered capture to width x height 8-bit BGRA, tonemapping
//! HDR-tagged or high-depth captures to SDR (bookmark-thumbnail policy).
//! Runs on the consumer's thread, never the render thread; the buffer must
//! hold width * height * 4 bytes.
bool CaptureToBGRA(const CaptureResult& result,
                   unsigned int width,
                   unsigned int height,
                   uint8_t* buffer);

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
