/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/joysticks/JoystickTypes.h"

namespace KODI
{
namespace JOYSTICK
{
/*!
 * \ingroup joystick
 *
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
  virtual bool OnAxisMotion(unsigned int axisIndex,
                            float position,
                            int center,
                            unsigned int range) = 0;

  /*!
   * \brief Handle buffered input motion for features that require multiple axes
   *
   * OnInputFrame() is called at the end of the frame when all axis motions
   * have been reported. This has several uses, including:
   *
   *  - Combining multiple axes into a single analog stick or accelerometer event
   *  - Imitating an analog feature with a digital button so that events can be
   *    dispatched every frame.
   */
  virtual void OnInputFrame(void) = 0;
};
} // namespace JOYSTICK
} // namespace KODI
