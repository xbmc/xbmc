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

#include "JoystickTranslator.h"
#include "Key.h"
#include "input/joysticks/JoystickIDs.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

#include <sstream>

uint32_t CJoystickTranslator::TranslateButton(const TiXmlNode* pDevice, const TiXmlElement *pButton, unsigned int& holdtimeMs)
{
  std::string controllerId = DEFAULT_CONTROLLER_ID;

  const TiXmlElement* deviceElem = pDevice->ToElement();
  if (deviceElem != nullptr)
    deviceElem->QueryValueAttribute("profile", &controllerId);

  holdtimeMs = 0;

  const char *szButton = pButton->Value();
  if (szButton == nullptr)
    return 0;

  std::string strButton = szButton;
  StringUtils::ToLower(strButton);

  uint32_t buttonCode = 0;
  if (controllerId == DEFAULT_CONTROLLER_ID)
  {
    if (strButton == "a") buttonCode = KEY_JOYSTICK_BUTTON_A;
    else if (strButton == "b") buttonCode = KEY_JOYSTICK_BUTTON_B;
    else if (strButton == "x") buttonCode = KEY_JOYSTICK_BUTTON_X;
    else if (strButton == "y") buttonCode = KEY_JOYSTICK_BUTTON_Y;
    else if (strButton == "start") buttonCode = KEY_JOYSTICK_BUTTON_START;
    else if (strButton == "back") buttonCode = KEY_JOYSTICK_BUTTON_BACK;
    else if (strButton == "left") buttonCode = KEY_JOYSTICK_BUTTON_DPAD_LEFT;
    else if (strButton == "right") buttonCode = KEY_JOYSTICK_BUTTON_DPAD_RIGHT;
    else if (strButton == "up") buttonCode = KEY_JOYSTICK_BUTTON_DPAD_UP;
    else if (strButton == "down") buttonCode = KEY_JOYSTICK_BUTTON_DPAD_DOWN;
    else if (strButton == "leftthumb") buttonCode = KEY_JOYSTICK_BUTTON_LEFT_STICK_BUTTON;
    else if (strButton == "rightthumb") buttonCode = KEY_JOYSTICK_BUTTON_RIGHT_STICK_BUTTON;
    else if (strButton == "leftstickup") buttonCode = KEY_JOYSTICK_BUTTON_LEFT_THUMB_STICK_UP;
    else if (strButton == "leftstickdown") buttonCode = KEY_JOYSTICK_BUTTON_LEFT_THUMB_STICK_DOWN;
    else if (strButton == "leftstickleft") buttonCode = KEY_JOYSTICK_BUTTON_LEFT_THUMB_STICK_LEFT;
    else if (strButton == "leftstickright") buttonCode = KEY_JOYSTICK_BUTTON_LEFT_THUMB_STICK_RIGHT;
    else if (strButton == "rightstickup") buttonCode = KEY_JOYSTICK_BUTTON_RIGHT_THUMB_STICK_UP;
    else if (strButton == "rightstickdown") buttonCode = KEY_JOYSTICK_BUTTON_RIGHT_THUMB_STICK_DOWN;
    else if (strButton == "rightstickleft") buttonCode = KEY_JOYSTICK_BUTTON_RIGHT_THUMB_STICK_LEFT;
    else if (strButton == "rightstickright") buttonCode = KEY_JOYSTICK_BUTTON_RIGHT_THUMB_STICK_RIGHT;
    else if (strButton == "lefttrigger") buttonCode = KEY_JOYSTICK_BUTTON_LEFT_TRIGGER;
    else if (strButton == "righttrigger") buttonCode = KEY_JOYSTICK_BUTTON_RIGHT_TRIGGER;
    else if (strButton == "leftbumper") buttonCode = KEY_JOYSTICK_BUTTON_LEFT_SHOULDER;
    else if (strButton == "rightbumper") buttonCode = KEY_JOYSTICK_BUTTON_RIGHT_SHOULDER;
    else if (strButton == "guide") buttonCode = KEY_JOYSTICK_BUTTON_GUIDE;
  }
  else if (controllerId == DEFAULT_REMOTE_ID)
  {
    if (strButton == "ok") buttonCode = KEY_REMOTE_BUTTON_OK;
    else if (strButton == "back") buttonCode = KEY_REMOTE_BUTTON_BACK;
    else if (strButton == "up") buttonCode = KEY_REMOTE_BUTTON_UP;
    else if (strButton == "down") buttonCode = KEY_REMOTE_BUTTON_DOWN;
    else if (strButton == "left") buttonCode = KEY_REMOTE_BUTTON_LEFT;
    else if (strButton == "right") buttonCode = KEY_REMOTE_BUTTON_RIGHT;
    else if (strButton == "home") buttonCode = KEY_REMOTE_BUTTON_HOME;
  }

  if (buttonCode == 0)
  {
    CLog::Log(LOGERROR, "Joystick Translator: Can't find button %s for controller %s", szButton, controllerId.c_str());
  }
  else
  {
    // Process holdtime parameter
    std::string strHoldTime;
    if (pButton->QueryValueAttribute("holdtime", &strHoldTime) == TIXML_SUCCESS)
    {
      std::stringstream ss(std::move(strHoldTime));
      ss >> holdtimeMs;
    }
  }

  return buttonCode;
}
