#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <stdint.h>

#include "input/touch/ITouchInputHandling.h"

/*!
 * \ingroup touch
 * \brief Touch input event
 */
typedef enum {
  TouchInputUnchanged = 0,
  TouchInputAbort,
  TouchInputDown,
  TouchInputUp,
  TouchInputMove
} TouchInput;

/*!
 * \ingroup touch
 * \brief Interface (implements ITouchInputHandling) defining methods to handle
 *        raw touch input events (down, up, move).
 *
 * This interface should be implemented on platforms only supporting low level
 * (raw) touch input events like touch down/up/move and with no gesture
 * recognition logic.
 */
class ITouchInputHandler : public ITouchInputHandling
{
public:
  ITouchInputHandler()
    : m_dpi(160.0f)
  { }
  virtual ~ITouchInputHandler() { }

  /*!
   * \brief Handle a touch event
   *
   * Handles the given touch event at the given location.
   * This takes into account all the currently active pointers
   * which need to be updated before calling this method to
   * actually interprete and handle the changes in touch.
   *
   * \param event    The actual touch event (abort, down, up, move)
   * \param x        The x coordinate (with sub-pixel) of the touch
   * \param y        The y coordinate (with sub-pixel) of the touch
   * \param time     The time (in nanoseconds) when this touch occured
   * \param pointer  The number of the touch pointer which caused this event (default 0)
   * \param size     The size of the touch pointer (with sub-pixel) (default 0.0)
   * 
   * \return True if the event was handled otherwise false.
   * 
   * \sa Update
   */
  virtual bool HandleTouchInput(TouchInput event, float x, float y, int64_t time, int32_t pointer = 0, float size = 0.0f) = 0;

  /*!
   * \brief Update the coordinates of a pointer
   *
   * Depending on how a platform handles touch input and provides the necessary events
   * this method needs to be called at different times. If there's an event for every
   * touch action this method does not need to be called at all. If there's only a
   * touch event for the primary pointer (and no special events for any secondary
   * pointers in a multi touch gesture) this method should be called for every active
   * secondary pointer before calling Handle.
   *
   * \param pointer  The number of the touch pointer which caused this event (default 0)
   * \param x        The x coordinate (with sub-pixel) of the touch
   * \param y        The y coordinate (with sub-pixel) of the touch
   * \param time     The time (in nanoseconds) when this touch occured
   * \param size     The size of the touch pointer (with sub-pixel) (default 0.0)
   * 
   * \return True if the pointer was updated otherwise false.
   * 
   * \sa Handle
   */
  virtual bool UpdateTouchPointer(int32_t pointer, float x, float y, int64_t time, float size = 0.0f) { return false; }

  void SetScreenDPI(float dpi) { if (dpi > 0.0f) m_dpi = dpi; }

protected:
  /*!
   * \brief DPI value of the touch screen
   */
  float m_dpi;
};
