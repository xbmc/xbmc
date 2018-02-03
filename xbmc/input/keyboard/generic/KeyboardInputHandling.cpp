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

#include "KeyboardInputHandling.h"
#include "input/joysticks/interfaces/IButtonMap.h"
#include "input/joysticks/DriverPrimitive.h"
#include "input/keyboard/interfaces/IKeyboardInputHandler.h"
#include "input/XBMC_keysym.h"

using namespace KODI;
using namespace KEYBOARD;

CKeyboardInputHandling::CKeyboardInputHandling(IKeyboardInputHandler* handler, JOYSTICK::IButtonMap* buttonMap) :
  m_handler(handler),
  m_buttonMap(buttonMap)
{
}

bool CKeyboardInputHandling::OnKeyPress(const CKey& key)
{
  bool bHandled = false;

  JOYSTICK::CDriverPrimitive source(static_cast<XBMCKey>(key.GetKeycode()));

  KeyName keyName;
  if (m_buttonMap->GetFeature(source, keyName))
  {
    const Modifier mod = static_cast<Modifier>(key.GetModifiers() | key.GetLockingModifiers());
    bHandled = m_handler->OnKeyPress(keyName, mod, key.GetUnicode());
  }

  return bHandled;
}

void CKeyboardInputHandling::OnKeyRelease(const CKey& key)
{
  JOYSTICK::CDriverPrimitive source(static_cast<XBMCKey>(key.GetKeycode()));

  KeyName keyName;
  if (m_buttonMap->GetFeature(source, keyName))
  {
    const Modifier mod = static_cast<Modifier>(key.GetModifiers() | key.GetLockingModifiers());
    m_handler->OnKeyRelease(keyName, mod, key.GetUnicode());
  }
}
