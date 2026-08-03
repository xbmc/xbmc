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

#include <functional>

namespace KODI
{
namespace RENDERING
{
namespace CAPTURE
{

enum class CaptureState
{
  PENDING, //!< submitted, not yet visible to taps
  LATCHED, //!< picked up at a frame boundary, serviced by taps
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
  CEvent event;
};

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
