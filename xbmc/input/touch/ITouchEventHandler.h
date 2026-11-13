/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/touch/ITouchInputHandler.h"

#include <stdint.h>

/*!
 * \ingroup touch
 * \brief Interface for handling raw touch events before Kodi recognises
 *        gestures in them
 *
 * A handler that forwards touch to another process needs the touch points
 * themselves. It is called on the thread that delivers touch input.
 */
class ITouchEventHandler
{
public:
  virtual ~ITouchEventHandler() = default;

  /*!
   * \brief A touch point went down, moved or went up, or the touch was aborted
   *
   * \param event    The touch event (abort, down, up, move)
   * \param x        The x coordinate in Kodi's GUI coordinates
   * \param y        The y coordinate in Kodi's GUI coordinates
   * \param time     The time (in nanoseconds) when this touch occurred
   * \param pointer  The number of the touch point
   *
   * \return True if the event was handled and Kodi must not act on it
   */
  virtual bool OnTouchEvent(TouchInput event, float x, float y, int64_t time, int32_t pointer) = 0;
};
