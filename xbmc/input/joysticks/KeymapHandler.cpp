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

#include "KeymapHandler.h"
#include "input/Action.h"
#include "input/ButtonTranslator.h"
#include "input/InputManager.h"
#include "input/Key.h"
#include "interfaces/IActionListener.h"
#include "utils/log.h"

#include <algorithm>

using namespace KODI;
using namespace JOYSTICK;

#define DIGITAL_ANALOG_THRESHOLD  0.5f

#define HOLD_TIMEOUT_MS     500
#define REPEAT_TIMEOUT_MS   50

CKeymapHandler::CKeymapHandler(IActionListener *actionHandler) :
    m_actionHandler(actionHandler),
    m_lastButtonPress(0),
    m_lastDigitalActionMs(0)
{
}

CKeymapHandler::~CKeymapHandler(void)
{
}

unsigned int CKeymapHandler::GetActionID(unsigned int keyId, int windowId, bool bFallthrough) const
{
  CAction action(ACTION_NONE);

  if (keyId != 0)
    action = CButtonTranslator::GetInstance().GetAction(windowId, CKey(keyId), bFallthrough);

  //! @todo make CAction::GetID() return unsigned
  if (action.GetID() >= 0)
    return action.GetID();
  else
    return ACTION_NONE;
}

void CKeymapHandler::OnDigitalKey(unsigned int keyId, int windowId, bool bFallthrough, bool bPressed, unsigned int holdTimeMs /* = 0 */)
{
  CAction action(ACTION_NONE);

  if (keyId != 0)
    action = CButtonTranslator::GetInstance().GetAction(windowId, CKey(keyId, holdTimeMs), bFallthrough);

  if (action.GetID() != ACTION_NONE)
  {
    if (action.IsAnalog())
    {
      OnAnalogKey(keyId, windowId, bFallthrough, bPressed ? 1.0f : 0.0f, 0);
    }
    else
    {
      if (bPressed)
        SendDigitalAction(action);
      else
        ProcessButtonRelease(keyId);
    }
  }
}

void CKeymapHandler::OnAnalogKey(unsigned int keyId, int windowId, bool bFallthrough, float magnitude, unsigned int motionTimeMs)
{
  CAction action(ACTION_NONE);

  if (keyId != 0)
    action = CButtonTranslator::GetInstance().GetAction(windowId, CKey(keyId), bFallthrough);

  if (action.GetID() != ACTION_NONE)
  {
    if (action.IsAnalog())
    {
      CAction actionWithAmount(action.GetID(), magnitude, 0.0f, action.GetName());
      m_actionHandler->OnAction(actionWithAmount);
    }
    else
    {
      unsigned int holdTimeMs = 0;
      
      const bool bIsPressed = (magnitude >= DIGITAL_ANALOG_THRESHOLD);
      if (bIsPressed)
      {
        const bool bIsHeld = (m_holdStartTimes.find(keyId) != m_holdStartTimes.end());
        if (bIsHeld)
          holdTimeMs = motionTimeMs - m_holdStartTimes[keyId];
        else
          m_holdStartTimes[keyId] = motionTimeMs;
      }
      else
      {
        m_holdStartTimes.erase(keyId);
      }

      OnDigitalKey(keyId, windowId, bFallthrough, bIsPressed, holdTimeMs);
    }
  }
}

void CKeymapHandler::SendDigitalAction(const CAction& action)
{
  const unsigned int keyId = action.GetButtonCode();
  const unsigned int holdTimeMs = action.GetHoldTime();

  if (!IsPressed(keyId))
  {
    m_pressedButtons.push_back(keyId);

    // Only dispatch action if button was pressed this frame
    if (holdTimeMs == 0)
    {
      if (m_actionHandler->OnAction(action))
      {
        m_lastButtonPress = keyId;
        m_lastDigitalActionMs = holdTimeMs;
      }
    }
  }
  else if (keyId == m_lastButtonPress && holdTimeMs > HOLD_TIMEOUT_MS)
  {
    if (holdTimeMs > m_lastDigitalActionMs + REPEAT_TIMEOUT_MS)
    {
      m_actionHandler->OnAction(action);
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
