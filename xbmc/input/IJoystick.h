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

#pragma once

#include "settings/AdvancedSettings.h"

#include <string>
#include <string.h>
#include <vector>
#include <boost/shared_ptr.hpp>

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

struct SJoystick;

/**
 * Interface IJoystick
 *
 * Joysticks are abstracted as devices that repeatedly refresh and report their
 * ineternal state. Update() is called by CJoystickManager once per FrameMove()
 * to poll for input and should sync the SJoystick struct returned by GetState()
 * to the joystick's current state.
 */
class IJoystick
{
public:
  /**
   * Implementers should provide the following factories to create IJoystick objects
   * (See CJoystickManager::Initialize()):
   *
   * static void Initialize(JoystickArray &joysticks);
   * static void DeInitialize(JoystickArray &joysticks);
   */

  virtual ~IJoystick() { }

  virtual void Update() = 0;

  virtual const SJoystick &GetState() const = 0;
};

typedef std::vector<boost::shared_ptr<IJoystick> > JoystickArray;

/**
 * Utility function to normalize joystick axes.
 */
inline float NormalizeAxis(long value, long maxAxisAmount)
{
  if (value > maxAxisAmount)
    value = maxAxisAmount;
  else if (value < -maxAxisAmount)
    value = -maxAxisAmount;
  long deadzoneRange = (long)(g_advancedSettings.m_controllerDeadzone * maxAxisAmount);
  if (value > deadzoneRange)
    return (float)(value - deadzoneRange) / (float)(maxAxisAmount - deadzoneRange);
  else if (value < -deadzoneRange)
    return (float)(value + deadzoneRange) / (float)(maxAxisAmount - deadzoneRange);
  return 0.0f;
}

/**
  * An arrow-based device on a gamepad. Legally, no more than two buttons can
  * be pressed, and only if they are adjacent. If no buttons are pressed, the
  * hat is centered.
  */
struct SHat
{
  SHat() { Center(); }
  void Center() { up = right = down = left = 0; }

  bool operator==(const SHat &lhs) const { return up == lhs.up && right == lhs.right && down == lhs.down && left == lhs.left; }
  bool operator!=(const SHat &lhs) const { return *this != lhs; }

  // Iterate through cardinal directions in an ordinal fashion
  const unsigned char &operator[](unsigned int i) const { return const_cast<SHat&>(*this)[i]; }
  unsigned char &operator[](unsigned int i)
  {
    switch (i)
    {
    case 0:  return up;
    case 1:  return right;
    case 2:  return down;
    case 3:
    default: return left;
    }
  }

  // Translate this hat into a cardinal direction ("N", "NE", "E", ...) or "CENTERED"
  #define HAT_MAKE_DIRECTION(n, e, s, w) ((n) << 3 | (e) << 2 | (s) << 1 | (w))
  const char *GetDirection() const
  {
    switch (HAT_MAKE_DIRECTION(up, right, down, left))
    {
    case HAT_MAKE_DIRECTION(1, 0, 0, 0): return "N";
    case HAT_MAKE_DIRECTION(1, 1, 0, 0): return "NE";
    case HAT_MAKE_DIRECTION(0, 1, 0, 0): return "E";
    case HAT_MAKE_DIRECTION(0, 1, 1, 0): return "SE";
    case HAT_MAKE_DIRECTION(0, 0, 1, 0): return "S";
    case HAT_MAKE_DIRECTION(0, 0, 1, 1): return "SW";
    case HAT_MAKE_DIRECTION(0, 0, 0, 1): return "W";
    case HAT_MAKE_DIRECTION(1, 0, 0, 1): return "NW";
    default:                             return "centered";
    }
  }

  unsigned char up;
  unsigned char right;
  unsigned char down;
  unsigned char left;
};

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
  SJoystick() : id(0) { Reset(); }
  void Reset()
  {
    buttonCount = sizeof(buttons) / sizeof(buttons[0]);
    hatCount    = sizeof(hats) / sizeof(hats[0]);
    axisCount   = sizeof(axes) / sizeof(axes[0]);
    memset(buttons, 0, sizeof(buttons));
    memset(axes, 0, sizeof(axes));
  }

  std::string   name;
  unsigned int  id;
  unsigned char buttons[GAMEPAD_BUTTON_COUNT];
  unsigned int  buttonCount;
  SHat          hats[GAMEPAD_HAT_COUNT];
  unsigned int  hatCount;
  float         axes[GAMEPAD_AXIS_COUNT];
  unsigned int  axisCount;
};
