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

extern "C"
{
#include <libavutil/mastering_display_metadata.h>
}

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

enum class CaptureFormat
{
  BGRA8, //!< 8-bit BGRA, requantized from the output depth during the copy
  NATIVE, //!< the output surface's coding at its native depth
};

struct CaptureSpec
{
  CaptureContent content{CaptureContent::COMPOSITE};
  CaptureCadence cadence{CaptureCadence::ONESHOT};
  //! 0x0 requests the native output size; nonzero requests a scaled capture
  unsigned int width{0};
  unsigned int height{0};
  CaptureFormat format{CaptureFormat::BGRA8};
};

struct CaptureResult
{
  std::shared_ptr<uint8_t[]> pixels;
  unsigned int width{0};
  unsigned int height{0};
  unsigned int stride{0};
  int bitDepth{8};
  ImageColorMetadata color;
  //! HDR mastering/light metadata of the captured content, carried verbatim so
  //! an SDR tonemap has the same source peak the thumbnail extractor uses
  bool hasDisplayMetadata{false};
  AVMasteringDisplayMetadata displayMetadata{};
  bool hasLightMetadata{false};
  AVContentLightMetadata lightMetadata{};
};

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
