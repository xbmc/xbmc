#pragma once
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
 */

#include <string>
#include <vector>

#define JACTIVE_NONE      0x00000000
#define JACTIVE_BUTTON    0x00000001
#define JACTIVE_HAT       0x00000002
#define JACTIVE_AXIS      0x00000004
#define JACTIVE_HAT_UP    0x01
#define JACTIVE_HAT_RIGHT 0x02
#define JACTIVE_HAT_DOWN  0x04
#define JACTIVE_HAT_LEFT  0x08

#define GAMEPAD_BUTTON_COUNT    32
#define GAMEPAD_HAT_COUNT       4
#define GAMEPAD_AXIS_COUNT      6

#define GAMEPAD_MAX_CONTROLLERS 4

namespace JOYSTICK
{

/**
 * An arrow-based device on a gamepad. Legally, no more than two buttons can be
 * pressed, and only if they are adjacent. If no buttons are pressed (or the
 * hat is in an invalid state), the hat is considered centered.
 */
struct Hat
{
  Hat() { Center(); }
  void Center();

  bool operator==(const Hat &rhs) const;
  bool operator!=(const Hat &rhs) const { return !(*this == rhs); }

  /**
   * Iterate through cardinal directions in an ordinal fashion.
   *   Hat[0] == up
   *   Hat[1] == right
   *   Hat[2] == down
   *   Hat[3] == left
   */
  bool       &operator[](unsigned int i);
  const bool &operator[](unsigned int i) const { return const_cast<Hat&>(*this)[i]; }
  
  /**
   * Helper function to translate this hat into a cardinal direction
   * ("N", "NE", "E", ...) or "CENTERED".
   */
  const char *GetDirection() const;

  bool up;
  bool right;
  bool down;
  bool left;
};

/**
 * Abstract representation of a joystick. Joysticks can have buttons, hats and
 * analog axes in the range [-1, 1]. Some joystick APIs (the Linux Joystick API,
 * for example) report hats as axes with an integer value of -1, 0 or 1. No
 * effort should be made to decode these axes back to hats, as this processing
 * is done in CJoystickManager.
 */
struct Joystick
{
public:
  Joystick() : id(0) { ResetState(); }
  void ResetState(unsigned int buttonCount = GAMEPAD_BUTTON_COUNT,
                  unsigned int hatCount = GAMEPAD_HAT_COUNT,
                  unsigned int axisCount = GAMEPAD_AXIS_COUNT);

  /**
   * Helper function to normalize a value to maxAxisAmount.
   */
  void SetAxis(unsigned int axis, long value, long maxAxisAmount);

  std::string        name;
  unsigned int       id;
  std::vector<bool>  buttons;
  std::vector<Hat>   hats;
  std::vector<float> axes;
};

} // namespace INPUT
