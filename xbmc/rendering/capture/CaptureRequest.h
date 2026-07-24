/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "rendering/capture/CaptureTypes.h"
#include "threads/Event.h"

#include <cstdint>
#include <functional>

namespace KODI
{
namespace RENDERING
{
namespace CAPTURE
{

enum class CaptureState
{
  PENDING, //!< submitted and not yet latched, or standing by below the stack top
  LATCHED, //!< the stack top, serviced by this frame's taps
  DONE,
  FAILED,
  CANCELLED,
};

using CaptureCallback = std::function<void(const CaptureResult&)>;

//! Passive data holder: every state transition happens inside
//! CCaptureService under its registry lock.
struct CaptureRequest
{
  CaptureSpec spec;
  CaptureCallback callback;
  CaptureState state{CaptureState::PENDING};
  CaptureResult result;
  //! Completed deliveries; a CONTINUOUS request accumulates one per served frame
  uint64_t deliveries{0};
  //! Cleared when the request is latched, set when a tap delivers. If it is
  //! still false at FrameComplete, a frame drew and no tap served the request.
  bool served{false};
  CEvent event;
};

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
