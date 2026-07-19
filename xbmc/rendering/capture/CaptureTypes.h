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

//! \brief What a capture request wants copied out.
//!
//! Captures are served by taps: points in the render loop where a finished
//! frame is copied out before it is overwritten. There are two, and they see
//! different pixels because they run at different moments:
//! - the video-only tap (CRenderManager::ServiceVideoCaptures) runs right after
//!   the video is presented, before any GUI, OSD or subtitle is drawn;
//! - the composite tap (ServiceCaptureTaps in Application.cpp) runs after
//!   compositing, before the swap.
//! VIDEO is served by the first, COMPOSITE by the second, and BOTH by each in
//! turn in one frame, yielding a video-only and a composite capture told apart
//! by CaptureResult::content.
enum class CaptureContent
{
  COMPOSITE,
  VIDEO,
  BOTH,
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
  //! which tap produced this result; stamped at delivery so a BOTH consumer
  //! knows whether it holds the video-only or the composite capture
  CaptureContent content{CaptureContent::COMPOSITE};
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
