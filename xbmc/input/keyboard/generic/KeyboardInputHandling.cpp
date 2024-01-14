/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "KeyboardInputHandling.h"

#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/interfaces/IButtonMap.h"
#include "input/keyboard/XBMC_keysym.h"
#include "input/keyboard/interfaces/IKeyboardInputHandler.h"

using namespace KODI;
using namespace KEYBOARD;

CKeyboardInputHandling::CKeyboardInputHandling(IKeyboardInputHandler* handler,
                                               JOYSTICK::IButtonMap* buttonMap)
  : m_handler(handler), m_buttonMap(buttonMap)
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
