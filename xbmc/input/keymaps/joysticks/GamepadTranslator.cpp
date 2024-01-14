/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GamepadTranslator.h"

#include "input/keyboard/KeyIDs.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <string>

using namespace KODI;
using namespace KEYMAP;

uint32_t CGamepadTranslator::TranslateString(std::string strButton)
{
  if (strButton.empty())
    return 0;

  StringUtils::ToLower(strButton);

  uint32_t buttonCode = 0;
  if (strButton == "a")
    buttonCode = KEY_BUTTON_A;
  else if (strButton == "b")
    buttonCode = KEY_BUTTON_B;
  else if (strButton == "x")
    buttonCode = KEY_BUTTON_X;
  else if (strButton == "y")
    buttonCode = KEY_BUTTON_Y;
  else if (strButton == "white")
    buttonCode = KEY_BUTTON_WHITE;
  else if (strButton == "black")
    buttonCode = KEY_BUTTON_BLACK;
  else if (strButton == "start")
    buttonCode = KEY_BUTTON_START;
  else if (strButton == "back")
    buttonCode = KEY_BUTTON_BACK;
  else if (strButton == "leftthumbbutton")
    buttonCode = KEY_BUTTON_LEFT_THUMB_BUTTON;
  else if (strButton == "rightthumbbutton")
    buttonCode = KEY_BUTTON_RIGHT_THUMB_BUTTON;
  else if (strButton == "leftthumbstick")
    buttonCode = KEY_BUTTON_LEFT_THUMB_STICK;
  else if (strButton == "leftthumbstickup")
    buttonCode = KEY_BUTTON_LEFT_THUMB_STICK_UP;
  else if (strButton == "leftthumbstickdown")
    buttonCode = KEY_BUTTON_LEFT_THUMB_STICK_DOWN;
  else if (strButton == "leftthumbstickleft")
    buttonCode = KEY_BUTTON_LEFT_THUMB_STICK_LEFT;
  else if (strButton == "leftthumbstickright")
    buttonCode = KEY_BUTTON_LEFT_THUMB_STICK_RIGHT;
  else if (strButton == "rightthumbstick")
    buttonCode = KEY_BUTTON_RIGHT_THUMB_STICK;
  else if (strButton == "rightthumbstickup")
    buttonCode = KEY_BUTTON_RIGHT_THUMB_STICK_UP;
  else if (strButton == "rightthumbstickdown")
    buttonCode = KEY_BUTTON_RIGHT_THUMB_STICK_DOWN;
  else if (strButton == "rightthumbstickleft")
    buttonCode = KEY_BUTTON_RIGHT_THUMB_STICK_LEFT;
  else if (strButton == "rightthumbstickright")
    buttonCode = KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT;
  else if (strButton == "lefttrigger")
    buttonCode = KEY_BUTTON_LEFT_TRIGGER;
  else if (strButton == "righttrigger")
    buttonCode = KEY_BUTTON_RIGHT_TRIGGER;
  else if (strButton == "leftanalogtrigger")
    buttonCode = KEY_BUTTON_LEFT_ANALOG_TRIGGER;
  else if (strButton == "rightanalogtrigger")
    buttonCode = KEY_BUTTON_RIGHT_ANALOG_TRIGGER;
  else if (strButton == "dpadleft")
    buttonCode = KEY_BUTTON_DPAD_LEFT;
  else if (strButton == "dpadright")
    buttonCode = KEY_BUTTON_DPAD_RIGHT;
  else if (strButton == "dpadup")
    buttonCode = KEY_BUTTON_DPAD_UP;
  else if (strButton == "dpaddown")
    buttonCode = KEY_BUTTON_DPAD_DOWN;
  else
    CLog::Log(LOGERROR, "Gamepad Translator: Can't find button {}", strButton);

  return buttonCode;
}
