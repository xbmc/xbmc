/*
 *      Copyright (C) 2017 Team Kodi
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

#include "GamepadTranslator.h"
#include "Key.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include <string>

uint32_t CGamepadTranslator::TranslateString(std::string strButton)
{
  if (strButton.empty())
    return 0;

  StringUtils::ToLower(strButton);

  uint32_t buttonCode = 0;
  if (strButton == "a") buttonCode = KEY_BUTTON_A;
  else if (strButton == "b") buttonCode = KEY_BUTTON_B;
  else if (strButton == "x") buttonCode = KEY_BUTTON_X;
  else if (strButton == "y") buttonCode = KEY_BUTTON_Y;
  else if (strButton == "white") buttonCode = KEY_BUTTON_WHITE;
  else if (strButton == "black") buttonCode = KEY_BUTTON_BLACK;
  else if (strButton == "start") buttonCode = KEY_BUTTON_START;
  else if (strButton == "back") buttonCode = KEY_BUTTON_BACK;
  else if (strButton == "leftthumbbutton") buttonCode = KEY_BUTTON_LEFT_THUMB_BUTTON;
  else if (strButton == "rightthumbbutton") buttonCode = KEY_BUTTON_RIGHT_THUMB_BUTTON;
  else if (strButton == "leftthumbstick") buttonCode = KEY_BUTTON_LEFT_THUMB_STICK;
  else if (strButton == "leftthumbstickup") buttonCode = KEY_BUTTON_LEFT_THUMB_STICK_UP;
  else if (strButton == "leftthumbstickdown") buttonCode = KEY_BUTTON_LEFT_THUMB_STICK_DOWN;
  else if (strButton == "leftthumbstickleft") buttonCode = KEY_BUTTON_LEFT_THUMB_STICK_LEFT;
  else if (strButton == "leftthumbstickright") buttonCode = KEY_BUTTON_LEFT_THUMB_STICK_RIGHT;
  else if (strButton == "rightthumbstick") buttonCode = KEY_BUTTON_RIGHT_THUMB_STICK;
  else if (strButton == "rightthumbstickup") buttonCode = KEY_BUTTON_RIGHT_THUMB_STICK_UP;
  else if (strButton == "rightthumbstickdown") buttonCode = KEY_BUTTON_RIGHT_THUMB_STICK_DOWN;
  else if (strButton == "rightthumbstickleft") buttonCode = KEY_BUTTON_RIGHT_THUMB_STICK_LEFT;
  else if (strButton == "rightthumbstickright") buttonCode = KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT;
  else if (strButton == "lefttrigger") buttonCode = KEY_BUTTON_LEFT_TRIGGER;
  else if (strButton == "righttrigger") buttonCode = KEY_BUTTON_RIGHT_TRIGGER;
  else if (strButton == "leftanalogtrigger") buttonCode = KEY_BUTTON_LEFT_ANALOG_TRIGGER;
  else if (strButton == "rightanalogtrigger") buttonCode = KEY_BUTTON_RIGHT_ANALOG_TRIGGER;
  else if (strButton == "dpadleft") buttonCode = KEY_BUTTON_DPAD_LEFT;
  else if (strButton == "dpadright") buttonCode = KEY_BUTTON_DPAD_RIGHT;
  else if (strButton == "dpadup") buttonCode = KEY_BUTTON_DPAD_UP;
  else if (strButton == "dpaddown") buttonCode = KEY_BUTTON_DPAD_DOWN;
  else CLog::Log(LOGERROR, "Gamepad Translator: Can't find button %s", strButton.c_str());

  return buttonCode;
}
