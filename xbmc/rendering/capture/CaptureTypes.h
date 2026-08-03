/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/iimage.h"

#include <cstdint>
#include <memory>

namespace KODI
{
namespace RENDERING
{
namespace CAPTURE
{

enum class CaptureContent
{
  COMPOSITE,
  VIDEO,
};

enum class CaptureCadence
{
  ONESHOT,
  CONTINUOUS,
};

struct CaptureSpec
{
  CaptureContent content{CaptureContent::COMPOSITE};
  CaptureCadence cadence{CaptureCadence::ONESHOT};
  //! 0x0 requests the native output size; nonzero requests a scaled capture
  unsigned int width{0};
  unsigned int height{0};
};

struct CaptureResult
{
  std::shared_ptr<uint8_t[]> pixels;
  unsigned int width{0};
  unsigned int height{0};
  unsigned int stride{0};
  int bitDepth{8};
  ImageColorMetadata color;
};

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
