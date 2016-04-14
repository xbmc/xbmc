/*
*      Copyright (C) 2016 Team Kodi
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

namespace JOYSTICK
{
  /*!
  * \brief Interface for sending input events to joystick drivers
  */
  class IDriverReceiver
  {
  public:
    virtual ~IDriverReceiver(void) { }

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
}
