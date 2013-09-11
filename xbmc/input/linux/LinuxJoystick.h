/*
 *      Copyright (C) 2007-2013 Team XBMC
 *      http://www.xbmc.org
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
 *  Parts of this file are Copyright (C) 2009 Stephen Kitt <steve@sk2.org>
 */

#pragma once

#include "input/IJoystick.h"

#include <stdint.h>
#include <string>

namespace JOYSTICK
{

class CLinuxJoystick : public IJoystick
{
public:
  static void Initialize(JoystickArray &joysticks);
  static void DeInitialize(JoystickArray &joysticks);

  virtual ~CLinuxJoystick();
  virtual void Update();
  virtual const JOYSTICK::Joystick &GetState() const { return m_state; }

private:
  CLinuxJoystick(int fd, unsigned int id, const char *name, const std::string &filename, unsigned char buttons, unsigned char axes);

  static int GetButtonMap(int fd, uint16_t *buttonMap);
  static int GetAxisMap(int fd, uint8_t *axisMap);
  static int DetermineIoctl(int fd, int *ioctls, uint16_t *buttonMap, int &ioctl_used);

  JOYSTICK::Joystick m_state;
  int                m_fd;
  std::string        m_filename; // for debugging purposes
};

}
