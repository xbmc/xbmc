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

//! \brief Convert a delivered capture to width x height 8-bit BGRA.
//! Runs on the consumer's thread, never the render thread; the buffer must
//! hold width * height * 4 bytes.
bool CaptureToBGRA(const CaptureResult& result,
                   unsigned int width,
                   unsigned int height,
                   uint8_t* buffer);

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
