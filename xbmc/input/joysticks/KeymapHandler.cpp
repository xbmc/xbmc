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

#include <algorithm>

using namespace KODI;
using namespace MESSAGING;

#define HOLD_TIMEOUT_MS     500
#define REPEAT_TIMEOUT_MS   50

using namespace JOYSTICK;

CKeymapHandler::CKeymapHandler(void)
  : CThread("KeymapHandler"),
    m_state(STATE_UNPRESSED),
    m_lastButtonPress(0)
{
  Create(false);
}

CKeymapHandler::~CKeymapHandler(void)
{
  StopThread(true);
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

void CKeymapHandler::OnDigitalKey(unsigned int keyId, bool bPressed)
{
  if (keyId != 0)
  {
    CSingleLock lock(m_digitalMutex);

    if (bPressed && !IsPressed(keyId))
      ProcessButtonPress(keyId);
    else if (!bPressed && IsPressed(keyId))
      ProcessButtonRelease(keyId);
  }
}

void CKeymapHandler::OnAnalogKey(unsigned int keyId, float magnitude)
{
  if (keyId != 0)
    SendAnalogAction(keyId, magnitude);
}

void CKeymapHandler::Process()
{
  unsigned int holdStartTime = 0;
  unsigned int pressedButton = 0;

  while (!m_bStop)
  {
    switch (m_state)
    {
      case STATE_UNPRESSED:
      {
        // Wait for button press
        WaitResponse waitResponse = AbortableWait(m_pressEvent);

        CSingleLock lock(m_digitalMutex);

        if (waitResponse == WAIT_SIGNALED && m_lastButtonPress != 0)
        {
          pressedButton = m_lastButtonPress;
          m_state = STATE_BUTTON_PRESSED;
        }
        break;
      }

      case STATE_BUTTON_PRESSED:
      {
        holdStartTime = XbmcThreads::SystemClockMillis();

        // Wait for hold time to elapse
        WaitResponse waitResponse = AbortableWait(m_pressEvent, HOLD_TIMEOUT_MS);

        CSingleLock lock(m_digitalMutex);

        if (m_lastButtonPress == 0)
        {
          m_state = STATE_UNPRESSED;
        }
        else if (waitResponse == WAIT_SIGNALED || m_lastButtonPress != pressedButton)
        {
          pressedButton = m_lastButtonPress;
          // m_state is unchanged
        }
        else if (waitResponse == WAIT_TIMEDOUT)
        {
          m_state = STATE_BUTTON_HELD;
        }
        break;
      }

      case STATE_BUTTON_HELD:
      {
        const unsigned int holdTimeMs = XbmcThreads::SystemClockMillis() - holdStartTime;
        SendDigitalAction(pressedButton, holdTimeMs);

        // Wait for repeat time to elapse
        WaitResponse waitResponse = AbortableWait(m_pressEvent, REPEAT_TIMEOUT_MS);

        CSingleLock lock(m_digitalMutex);

        if (m_lastButtonPress == 0)
        {
          m_state = STATE_UNPRESSED;
        }
        else if (waitResponse == WAIT_SIGNALED || m_lastButtonPress != pressedButton)
        {
          pressedButton = m_lastButtonPress;
          m_state = STATE_BUTTON_PRESSED;
        }
        break;
      }

      default:
        break;
    }
  }
}

bool CKeymapHandler::ProcessButtonPress(unsigned int keyId)
{
  m_pressedButtons.push_back(keyId);

  if (SendDigitalAction(keyId))
  {
    m_lastButtonPress = keyId;
    m_pressEvent.Set();
    return true;
  }

  return false;
}

void CKeymapHandler::ProcessButtonRelease(unsigned int keyId)
{
  m_pressedButtons.erase(std::remove(m_pressedButtons.begin(), m_pressedButtons.end(), keyId), m_pressedButtons.end());

  if (keyId == m_lastButtonPress || m_pressedButtons.empty())
  {
    m_lastButtonPress = 0;
    m_pressEvent.Set();
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
