/*
 *  Copyright (C) 2016-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace JOYSTICK
{
/*!
 * \ingroup joystick
 *
 * \brief Interface for sending input events to joystick drivers
 */
class IDriverReceiver
{
public:
  virtual ~IDriverReceiver() = default;

  /*!
   * \brief Set the value of a rumble motor
   *
   * \param motorIndex   The driver index of the motor to rumble
   * \param magnitude    The motor's new magnitude of vibration in the closed interval [0, 1]
   *
   * \return True if the event was handled otherwise false
   */
  virtual bool SetMotorState(unsigned int motorIndex, float magnitude) = 0;
};
} // namespace JOYSTICK
} // namespace KODI
