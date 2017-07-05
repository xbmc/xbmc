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

#include "KeyHandler.h"
#include "input/joysticks/interfaces/IKeymapHandler.h"
#include "input/joysticks/JoystickUtils.h"
#include "input/Action.h"
#include "input/ActionTranslator.h"
#include "input/IKeymap.h"
#include "input/IKeymapEnvironment.h"
#include "interfaces/IActionListener.h"

#include <algorithm>
#include <assert.h>

using namespace KODI;
using namespace JOYSTICK;

#define DIGITAL_ANALOG_THRESHOLD  0.5f

#define HOLD_TIMEOUT_MS     500
#define REPEAT_TIMEOUT_MS   50

CKeyHandler::CKeyHandler(const std::string &keyName, IActionListener *actionHandler, const IKeymap *keymap, IKeymapHandler *keymapHandler) :
  m_keyName(keyName),
  m_actionHandler(actionHandler),
  m_keymap(keymap),
  m_keymapHandler(keymapHandler)
{
  assert(m_actionHandler != nullptr);
  assert(m_keymap != nullptr);
  assert(m_keymapHandler != nullptr);

  Reset();
}

void CKeyHandler::Reset()
{
  m_bHeld = false;
  m_magnitude = 0.0f;
  m_holdStartTimeMs = 0;
  m_lastHoldTimeMs = 0;
  m_bActionSent = false;
  m_lastActionMs = 0;
}

bool CKeyHandler::OnDigitalMotion(bool bPressed, unsigned int holdTimeMs)
{
  return OnAnalogMotion(bPressed ? 1.0f : 0.0f, holdTimeMs);
}

bool CKeyHandler::OnAnalogMotion(float magnitude, unsigned int motionTimeMs)
{
  // Don't send deactivation event more than once
  if (m_magnitude == 0.0f && magnitude == 0.0f)
    return false;

  // Calculate press state
  const bool bPressed = IsPressed(magnitude);
  const bool bJustPressed = bPressed && !m_bHeld;

  if (bJustPressed)
  {
    // Reset key if just pressed
    Reset();

    // Record hold start time if just pressed
    m_holdStartTimeMs = motionTimeMs;
  }

  // Calculate holdtime relative to when magnitude crossed the threshold
  unsigned int holdTimeMs = 0;
  if (bPressed)
    holdTimeMs = motionTimeMs - m_holdStartTimeMs;

  // Get actions for the key
  const auto &actions = m_keymap->GetActions(m_keyName);

  // Give priority to actions with hotkeys
  std::vector<const KeymapAction*> actionsWithHotkeys;

  for (const auto &action : actions)
  {
    if (!action.hotkeys.empty())
      actionsWithHotkeys.emplace_back(&action);
  }

  bool bHandled = HandleActions(std::move(actionsWithHotkeys), magnitude, holdTimeMs);

  // If that failed, try again with all actions
  if (!bHandled)
  {
    std::vector<const KeymapAction*> allActions;

    for (const auto &action : actions)
      allActions.emplace_back(&action);

    bHandled = HandleActions(std::move(allActions), magnitude, holdTimeMs);
  }

  m_bHeld = bPressed;
  m_magnitude = magnitude;
  m_lastHoldTimeMs = holdTimeMs;

  return bHandled;
}

bool CKeyHandler::HandleActions(std::vector<const KeymapAction*> actions, float magnitude, unsigned int holdTimeMs)
{
  // Filter out actions without pressed hotkeys
  actions.erase(std::remove_if(actions.begin(), actions.end(),
    [this](const KeymapAction *action)
    {
      return !m_keymapHandler->HotkeysPressed(action->hotkeys);
    }), actions.end());

  if (actions.empty())
    return false;

  bool bHandled = false;

  // Actions are sorted by holdtime, so the final action is the one with the
  // greatest holdtime
  const KeymapAction& finalAction = **actions.rbegin();
  const unsigned int maxHoldTimeMs = finalAction.holdTimeMs;

  const bool bHasDelay = (maxHoldTimeMs > 0);
  if (!bHasDelay)
  {
    bHandled = HandleAction(finalAction, magnitude, holdTimeMs);
  }
  else
  {
    // If holdtime has exceeded the last action, execute it now
    if (holdTimeMs >= finalAction.holdTimeMs)
    {
      // Force holdtime to zero for the initial press
      if (!m_bActionSent)
        holdTimeMs = 0;
      else
        holdTimeMs -= finalAction.holdTimeMs;

      bHandled = HandleAction(finalAction, magnitude, holdTimeMs);
    }
    else
    {
      // Calculate press state
      const bool bPressed = IsPressed(magnitude);
      const bool bJustReleased = m_bHeld && !bPressed;

      // If button was just released, send a release action
      if (bJustReleased)
        bHandled = HandleRelease(actions);
    }
  }

  return bHandled;
}

bool CKeyHandler::HandleRelease(std::vector<const KeymapAction*> actions)
{
  bool bHandled = false;

  // Use previous holdtime from before button release
  const unsigned int holdTimeMs = m_lastHoldTimeMs;

  // Send an action on release if one occurs before the holdtime
  for (auto it = actions.begin(); it != actions.end(); )
  {
    const KeymapAction &action = **it;

    unsigned int thisHoldTime = (*it)->holdTimeMs;

    ++it;
    if (it == actions.end())
      break;

    unsigned int nextHoldTime = (*it)->holdTimeMs;

    if (thisHoldTime <= holdTimeMs && holdTimeMs < nextHoldTime)
    {
      bHandled = HandleAction(action, 1.0f, 0);
      break;
    }
  }

  return bHandled;
}

bool CKeyHandler::HandleAction(const KeymapAction& action, float magnitude, unsigned int holdTimeMs)
{
  bool bSendAction = false;

  if (CActionTranslator::IsAnalog(action.actionId))
  {
    bSendAction = true;
  }
  else if (IsPressed(magnitude))
  {
    // Dispatch action if button was pressed this frame
    if (holdTimeMs == 0)
      bSendAction = true;
    else
      bSendAction = SendRepeatAction(holdTimeMs);
  }

  if (bSendAction)
  {
    const CAction guiAction(action.actionId, magnitude, 0.0f, action.actionString, holdTimeMs);
    m_actionHandler->OnAction(guiAction);
    m_keymapHandler->OnPress(m_keyName);
    m_bActionSent = true;
    m_lastActionMs = holdTimeMs;
  }

  return m_bActionSent;
}

bool CKeyHandler::SendRepeatAction(unsigned int holdTimeMs)
{
  bool bSendRepeat = true;

  // Don't send a repeat action if the last key has changed
  if (m_keymapHandler->GetLastPressed() != m_keyName)
    bSendRepeat = false;

  // Ensure initial timeout has elapsed
  else if (holdTimeMs < HOLD_TIMEOUT_MS)
    bSendRepeat = false;

  // Ensure repeat timeout has elapsed
  else if (holdTimeMs < m_lastActionMs + REPEAT_TIMEOUT_MS)
    bSendRepeat = false;

  return bSendRepeat;
}

bool CKeyHandler::IsPressed(float magnitude)
{
  return magnitude >= DIGITAL_ANALOG_THRESHOLD;
}
