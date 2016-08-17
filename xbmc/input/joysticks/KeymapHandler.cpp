/*
 *      Copyright (C) 2015-2016 Team Kodi
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

#include "KeymapHandler.h"
#include "guilib/GUIWindowManager.h"
#include "input/ButtonTranslator.h"
#include "input/InputManager.h"
#include "input/Key.h"
#include "utils/log.h"

#include <algorithm>

using namespace KODI;
using namespace MESSAGING;

#define HOLD_TIMEOUT_MS     500
#define REPEAT_TIMEOUT_MS   50

using namespace JOYSTICK;

CKeymapHandler::CKeymapHandler(void) :
    m_state(STATE_UNPRESSED),
    m_lastButtonPress(0),
    m_lastDigitalActionMs(0)
{
}

CKeymapHandler::~CKeymapHandler(void)
{
}

INPUT_TYPE CKeymapHandler::GetInputType(unsigned int keyId) const
{
  if (keyId != 0)
  {
    CAction action(CButtonTranslator::GetInstance().GetAction(g_windowManager.GetActiveWindowID(), CKey(keyId)));
    if (action.GetID() > 0)
    {
      if (action.IsAnalog())
        return INPUT_TYPE::ANALOG;
      else
        return INPUT_TYPE::DIGITAL;
    }
  }

  return INPUT_TYPE::UNKNOWN;
}

void CKeymapHandler::OnDigitalKey(unsigned int keyId, bool bPressed, unsigned int holdTimeMs /* = 0 */)
{
  if (keyId != 0)
  {
    if (bPressed)
      ProcessButtonPress(keyId, holdTimeMs);
    else
      ProcessButtonRelease(keyId);
  }
}

void CKeymapHandler::OnAnalogKey(unsigned int keyId, float magnitude)
{
  if (keyId != 0)
    SendAnalogAction(keyId, magnitude);
}

void CKeymapHandler::ProcessButtonPress(unsigned int keyId, unsigned int holdTimeMs)
{
  if (!IsPressed(keyId))
  {
    m_pressedButtons.push_back(keyId);

    if (SendDigitalAction(keyId))
    {
      m_lastButtonPress = keyId;
      m_lastDigitalActionMs = holdTimeMs;
    }
  }
  else if (keyId == m_lastButtonPress && holdTimeMs > HOLD_TIMEOUT_MS)
  {
    if (holdTimeMs > m_lastDigitalActionMs + REPEAT_TIMEOUT_MS)
    {
      SendDigitalAction(keyId, holdTimeMs);
      m_lastDigitalActionMs = holdTimeMs;
    }
  }
}

void CKeymapHandler::ProcessButtonRelease(unsigned int keyId)
{
  m_pressedButtons.erase(std::remove(m_pressedButtons.begin(), m_pressedButtons.end(), keyId), m_pressedButtons.end());

  // Update last button press if the button was released
  if (keyId == m_lastButtonPress)
    m_lastButtonPress = 0;

  // If all buttons are depressed, m_lastButtonPress must be 0
  if (m_pressedButtons.empty() && m_lastButtonPress != 0)
  {
    CLog::Log(LOGDEBUG, "ERROR: invalid state in CKeymapHandler!");
    m_lastButtonPress = 0;
  }
}

bool CKeymapHandler::IsPressed(unsigned int keyId) const
{
  return std::find(m_pressedButtons.begin(), m_pressedButtons.end(), keyId) != m_pressedButtons.end();
}

bool CKeymapHandler::SendDigitalAction(unsigned int keyId, unsigned int holdTimeMs /* = 0 */)
{
  CAction action(CButtonTranslator::GetInstance().GetAction(g_windowManager.GetActiveWindowID(), CKey(keyId, holdTimeMs)));
  if (action.GetID() > 0)
  {
    CInputManager::GetInstance().QueueAction(action);
    return true;
  }

  return false;
}

bool CKeymapHandler::SendAnalogAction(unsigned int keyId, float magnitude)
{
  CAction action(CButtonTranslator::GetInstance().GetAction(g_windowManager.GetActiveWindowID(), CKey(keyId)));
  if (action.GetID() > 0)
  {
    CAction actionWithAmount(action.GetID(), magnitude, 0.0f, action.GetName());
    CInputManager::GetInstance().QueueAction(actionWithAmount);
    return true;
  }

  return false;
}
