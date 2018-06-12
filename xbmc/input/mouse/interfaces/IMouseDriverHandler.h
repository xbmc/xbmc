/*
 *      Copyright (C) 2016-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "input/mouse/MouseTypes.h"

namespace KODI
{
namespace MOUSE
{
  /*!
   * \ingroup mouse
   * \brief Interface for handling mouse driver events
   */
  class IMouseDriverHandler
  {
  public:
    virtual ~IMouseDriverHandler(void) = default;

    /*!
     * \brief Handle mouse position updates
     *
     * \param x  The new x coordinate of the pointer
     * \param y  The new y coordinate of the pointer
     *
     * The mouse uses a left-handed (graphics) cartesian coordinate system.
     * Positive X is right, positive Y is down.
     *
     * \return True if the event was handled, false otherwise
     */
    virtual bool OnPosition(int x, int y) = 0;

    /*!
     * \brief A mouse button has been pressed
     *
     * \param button   The index of the pressed button
     *
     * \return True if the event was handled, otherwise false
     */
    virtual bool OnButtonPress(BUTTON_ID button) = 0;

    /*!
     * \brief A mouse button has been released
     *
     * \param button   The index of the released button
     */
    virtual void OnButtonRelease(BUTTON_ID button) = 0;
  };
}
}
