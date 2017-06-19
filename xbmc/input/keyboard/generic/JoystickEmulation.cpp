/*
 *      Copyright (C) 2015-2017 Team Kodi
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

#include "JoystickEmulation.h"
#include "input/joysticks/IDriverHandler.h"
#include "input/Key.h"

#include <algorithm>
#include <assert.h>

#define BUTTON_INDEX_MASK  0x01ff

using namespace KODI;
using namespace KEYBOARD;

CJoystickEmulation::CJoystickEmulation(JOYSTICK::IDriverHandler* handler) :
  m_handler(handler)
{
  assert(m_handler != nullptr);
}

bool CJoystickEmulation::OnKeyPress(const CKey& key)
{
  bool bHandled = false;

  unsigned int buttonIndex = GetButtonIndex(key);
  if (buttonIndex != 0)
    bHandled = OnPress(buttonIndex);

  return bHandled;
}

void CJoystickEmulation::OnKeyRelease(const CKey& key)
{
  unsigned int buttonIndex = GetButtonIndex(key);
  if (buttonIndex != 0)
    OnRelease(buttonIndex);
}

bool CJoystickEmulation::OnPress(unsigned int buttonIndex)
{
  bool bHandled = false;

  KeyEvent event;
  if (GetEvent(buttonIndex, event))
  {
    bHandled = event.bHandled;
  }
  else
  {
    bHandled = m_handler->OnButtonMotion(buttonIndex, true);
    m_pressedKeys.push_back({buttonIndex, bHandled});
  }

  return bHandled;
}

void CJoystickEmulation::OnRelease(unsigned int buttonIndex)
{
  KeyEvent event;
  if (GetEvent(buttonIndex, event))
  {
    m_handler->OnButtonMotion(buttonIndex, false);
    m_pressedKeys.erase(std::remove_if(m_pressedKeys.begin(), m_pressedKeys.end(),
      [buttonIndex](const KeyEvent& event)
      {
        return buttonIndex == event.buttonIndex;
      }), m_pressedKeys.end());
  }
}

bool CJoystickEmulation::GetEvent(unsigned int buttonIndex, KeyEvent& event) const
{
  std::vector<KeyEvent>::const_iterator it = std::find_if(m_pressedKeys.begin(), m_pressedKeys.end(),
    [buttonIndex](const KeyEvent& event)
    {
      return buttonIndex == event.buttonIndex;
    });

  if (it != m_pressedKeys.end())
  {
    event = *it;
    return true;
  }

  return false;
}

unsigned int CJoystickEmulation::GetButtonIndex(const CKey& key)
{
  return key.GetButtonCode() & BUTTON_INDEX_MASK;
}
