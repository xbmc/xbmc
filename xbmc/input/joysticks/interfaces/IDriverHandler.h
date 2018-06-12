/*
 *      Copyright (C) 2014-2017 Team Kodi
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

#include "input/joysticks/JoystickTypes.h"

namespace KODI
{
namespace JOYSTICK
{
  /*!
   * \brief Interface defining methods to handle joystick events for raw driver
   *        elements (buttons, hats, axes)
   */
  class IDriverHandler
  {
  public:
    virtual ~IDriverHandler() = default;

    /*!
     * \brief Handle button motion
     *
     * \param buttonIndex The index of the button as reported by the driver
     * \param bPressed    true for press motion, false for release motion
     *
     * \return True if a press was handled, false otherwise
     */
    virtual bool OnButtonMotion(unsigned int buttonIndex, bool bPressed) = 0;

    /*!
     * \brief Handle hat motion
     *
     * \param hatIndex     The index of the hat as reported by the driver
     * \param state        The direction the hat is now being pressed
     *
     * \return True if the new direction was handled, false otherwise
     */
    virtual bool OnHatMotion(unsigned int hatIndex, HAT_STATE state) = 0;

    /*!
     * \brief Handle axis motion
     *
     * If a joystick feature requires multiple axes (analog sticks, accelerometers),
     * they can be buffered for later processing.
     *
     * \param axisIndex   The index of the axis as reported by the driver
     * \param position    The position of the axis in the closed interval [-1.0, 1.0]
     * \param center      The center point of the axis (either -1, 0 or 1)
     * \param range       The maximum distance the axis can move (either 1 or 2)
     *
     * \return True if the motion was handled, false otherwise
     */
    virtual bool OnAxisMotion(unsigned int axisIndex, float position, int center, unsigned int range) = 0;

    /*!
     * \brief Handle buffered axis positions for features that require multiple axes
     *
     * ProcessAxisMotions() is called at the end of the frame when all axis motions
     * have been reported. This has several uses, including:
     *
     *  - Combining multiple axes into a single analog stick or accelerometer event
     *  - Imitating an analog feature with a digital button so that events can be
     *    dispatched every frame.
     */
    virtual void ProcessAxisMotions(void) = 0;
  };
}
}
