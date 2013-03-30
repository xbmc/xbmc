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

/**
 * Abstract representation of a joystick. Joysticks can have buttons, hats and
 * analog axes in the range [-1, 1]. Some joystick APIs (the Linux Joystick API,
 * (for example) report hats as axes with an integer value of -1, 0 or 1. No
 * effort should be made to decode these axes back to hats, as this processing
 * is done in CJoystickManager.
 */
struct SJoystick
{
private:
  SJoystick(const SJoystick &other);
  SJoystick &operator=(const SJoystick &rhs);

public:
  /**
    * An arrow-based device on a gamepad. Legally, no more than two buttons can
    * be pressed, and only if they are adjacent. If no buttons are pressed, the
    * hat is centered.
    */
  struct Hat
  {
    Hat() { Center(); }
    void Center() { up = right = down = left = 0; }

    bool operator==(const Hat &lhs) const { return up == lhs.up && right == lhs.right && down == lhs.down && left == lhs.left; }
    bool operator!=(const Hat &lhs) const { return *this != lhs; }

    // Iterate through cardinal directions in an ordinal fashion
    const unsigned char &operator[](unsigned int i) const { return const_cast<Hat&>(*this)[i]; }
    unsigned char &operator[](unsigned int i);

    // Translate this hat into a cardinal direction ("N", "NE", "E", ...) or "CENTERED"
    const char *GetDirection() const;

    unsigned char up;
    unsigned char right;
    unsigned char down;
    unsigned char left;
  };

  SJoystick() : id(0) { Reset(); }
  void Reset();

  void NormalizeAxis(unsigned int axis, long value, long maxAxisAmount);

  std::string   name;
  unsigned int  id;
  unsigned char buttons[GAMEPAD_BUTTON_COUNT];
  unsigned int  buttonCount;
  Hat           hats[GAMEPAD_HAT_COUNT];
  unsigned int  hatCount;
  float         axes[GAMEPAD_AXIS_COUNT];
  unsigned int  axisCount;
};
