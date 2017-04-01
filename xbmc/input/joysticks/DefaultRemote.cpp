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

#include "DefaultRemote.h"
#include "JoystickEasterEgg.h"
#include "JoystickIDs.h"
#include "input/Key.h"

using namespace KODI;
using namespace JOYSTICK;

CDefaultRemote::CDefaultRemote(void)
{
  // initialize CDefaultJoystick
  m_easterEgg.reset(new CJoystickEasterEgg(GetControllerID()));
}

std::string CDefaultRemote::GetControllerID(void) const
{
  return DEFAULT_REMOTE_ID;
}

unsigned int CDefaultRemote::GetKeyID(const FeatureName& feature, ANALOG_STICK_DIRECTION dir /* = ANALOG_STICK_DIRECTION::UNKNOWN */) const
{
  if      (feature == "ok")            return KEY_REMOTE_BUTTON_OK;
  else if (feature == "back")          return KEY_REMOTE_BUTTON_BACK;
  else if (feature == "up")            return KEY_REMOTE_BUTTON_UP;
  else if (feature == "down")          return KEY_REMOTE_BUTTON_DOWN;
  else if (feature == "right")         return KEY_REMOTE_BUTTON_RIGHT;
  else if (feature == "left")          return KEY_REMOTE_BUTTON_LEFT;
  else if (feature == "home")          return KEY_REMOTE_BUTTON_HOME;

  return 0;
}
