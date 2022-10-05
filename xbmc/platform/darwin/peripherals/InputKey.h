/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

enum class GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON
{
  UP = 0,
  DOWN = 1,
  LEFT = 2,
  RIGHT = 3,
  A = 4,
  B = 5,
  X = 6,
  Y = 7,
  LEFTSHOULDER = 8,
  LEFTTRIGGER = 9,
  RIGHTSHOULDER = 10,
  RIGHTTRIGGER = 11,
  MENU = 12,
  OPTION = 13,
  LEFTTHUMBSTICKBUTTON = 14,
  RIGHTTHUMBSTICKBUTTON = 15,
  HOME = 16,
  UNUSED = 99
};

enum class GCCONTROLLER_EXTENDED_GAMEPAD_AXIS
{
  // Thumbstick Axis
  LEFTTHUMB_X = 0,
  LEFTTHUMB_Y = 1,
  RIGHTTHUMB_X = 2,
  RIGHTTHUMB_Y = 3,
  // Thumbstick
  LEFT = 90,
  RIGHT = 91,
  UNUSED = 99
};

enum class GCCONTROLLER_MICRO_GAMEPAD_BUTTON
{
  UP = 0,
  DOWN = 1,
  LEFT = 2,
  RIGHT = 3,
  MENU = 4,
  A = 5,
  X = 6,
  UNUSED = 99
};

enum class GCCONTROLLER_TYPE
{
  UNKNOWN = 0,
  EXTENDED = 1,
  MICRO = 2,
  NOTFOUND = 98,
  UNUSED = 99
};

struct InputValueInfo
{
  GCCONTROLLER_TYPE controllerType;
  GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON extendedButton;
  GCCONTROLLER_EXTENDED_GAMEPAD_AXIS extendedAxis;
  GCCONTROLLER_MICRO_GAMEPAD_BUTTON microButton;

  InputValueInfo(GCCONTROLLER_TYPE controllerType)
    : controllerType(controllerType),
      extendedButton(GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON::UNUSED),
      extendedAxis(GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::UNUSED),
      microButton(GCCONTROLLER_MICRO_GAMEPAD_BUTTON::UNUSED)
  {
  }

  InputValueInfo(GCCONTROLLER_TYPE controllerType,
                 GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON extendedButton)
    : controllerType(controllerType),
      extendedButton(extendedButton),
      extendedAxis(GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::UNUSED),
      microButton(GCCONTROLLER_MICRO_GAMEPAD_BUTTON::UNUSED)
  {
  }

  InputValueInfo(GCCONTROLLER_TYPE controllerType, GCCONTROLLER_MICRO_GAMEPAD_BUTTON microButton)
    : controllerType(controllerType),
      extendedButton(GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON::UNUSED),
      extendedAxis(GCCONTROLLER_EXTENDED_GAMEPAD_AXIS::UNUSED),
      microButton(microButton)
  {
  }

  InputValueInfo(GCCONTROLLER_TYPE controllerType, GCCONTROLLER_EXTENDED_GAMEPAD_AXIS extendedAxis)
    : controllerType(controllerType),
      extendedButton(GCCONTROLLER_EXTENDED_GAMEPAD_BUTTON::UNUSED),
      extendedAxis(extendedAxis),
      microButton(GCCONTROLLER_MICRO_GAMEPAD_BUTTON::UNUSED)
  {
  }
};
