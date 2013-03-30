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

#include "Joystick.h"
#include "settings/AdvancedSettings.h"

#include <string.h>

unsigned char &SJoystick::Hat::operator[](unsigned int i)
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

#define HAT_MAKE_DIRECTION(n, e, s, w) ((n) << 3 | (e) << 2 | (s) << 1 | (w))
const char *SJoystick::Hat::GetDirection() const
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

void SJoystick::Reset()
{
  buttonCount = sizeof(buttons) / sizeof(buttons[0]);
  hatCount    = sizeof(hats) / sizeof(hats[0]);
  axisCount   = sizeof(axes) / sizeof(axes[0]);
  memset(buttons, 0, sizeof(buttons));
  memset(axes, 0, sizeof(axes));
}

void SJoystick::NormalizeAxis(unsigned int axis, long value, long maxAxisAmount)
{
  if (axis >= axisCount)
    return;
  if (value > maxAxisAmount)
    value = maxAxisAmount;
  else if (value < -maxAxisAmount)
    value = -maxAxisAmount;

  long deadzoneRange = (long)(g_advancedSettings.m_controllerDeadzone * maxAxisAmount);

  if (value > deadzoneRange)
    axes[axis] = (float)(value - deadzoneRange) / (float)(maxAxisAmount - deadzoneRange);
  else if (value < -deadzoneRange)
    axes[axis] = (float)(value + deadzoneRange) / (float)(maxAxisAmount - deadzoneRange);
  else
    axes[axis] = 0.0f;
}
