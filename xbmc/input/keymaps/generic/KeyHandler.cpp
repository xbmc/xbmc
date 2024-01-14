/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "KeyHandler.h"

#include "input/actions/ActionIDs.h"
#include "input/actions/ActionTranslator.h"
#include "input/actions/interfaces/IActionListener.h"
#include "input/joysticks/JoystickUtils.h"
#include "input/keymaps/interfaces/IKeymap.h"
#include "input/keymaps/interfaces/IKeymapHandler.h"

#include <algorithm>
#include <assert.h>

using namespace KODI;
using namespace KEYMAP;

#define DIGITAL_ANALOG_THRESHOLD 0.5f

#define HOLD_TIMEOUT_MS 500
#define REPEAT_TIMEOUT_MS 50

CKeyHandler::CKeyHandler(const std::string& keyName,
                         ACTION::IActionListener* actionHandler,
                         const IKeymap* keymap,
                         IKeymapHandler* keymapHandler)
  : m_keyName(keyName),
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
  m_activeWindowId = -1;
  m_lastAction = CAction();
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

  // Get actions for the key
  const auto& actionGroup = m_keymap->GetActions(m_keyName);
  const int windowId = actionGroup.windowId;
  const auto& actions = actionGroup.actions;

  // Calculate press state
  const bool bPressed = IsPressed(magnitude);
  const bool bJustPressed = bPressed && !m_bHeld;

  if (bJustPressed)
  {
    // Reset key if just pressed
    Reset();

    // Record hold start time if just pressed
    m_holdStartTimeMs = motionTimeMs;

    // Record window ID
    if (windowId >= 0)
      m_activeWindowId = windowId;
  }

  // Calculate holdtime relative to when magnitude crossed the threshold
  unsigned int holdTimeMs = 0;
  if (bPressed)
    holdTimeMs = motionTimeMs - m_holdStartTimeMs;

  // Give priority to actions with hotkeys
  std::vector<const KeymapAction*> actionsWithHotkeys;

  for (const auto& action : actions)
  {
    if (!action.hotkeys.empty())
      actionsWithHotkeys.emplace_back(&action);
  }

  CAction dispatchAction =
      ProcessActions(std::move(actionsWithHotkeys), windowId, magnitude, holdTimeMs);

  // If that failed, try again with all actions
  if (dispatchAction.GetID() == ACTION_NONE)
  {
    std::vector<const KeymapAction*> allActions;

    allActions.reserve(actions.size());
    for (const auto& action : actions)
      allActions.emplace_back(&action);

    dispatchAction = ProcessActions(std::move(allActions), windowId, magnitude, holdTimeMs);
  }

  // If specific action was sent last frame but not this one, send a release event
  if (dispatchAction.GetID() != m_lastAction.GetID())
  {
    if (ACTION::CActionTranslator::IsAnalog(m_lastAction.GetID()) &&
        m_lastAction.GetAmount() > 0.0f)
    {
      m_lastAction.ClearAmount();
      m_actionHandler->OnAction(m_lastAction);
    }
  }

  // Dispatch action
  bool bHandled = false;
  if (dispatchAction.GetID() != ACTION_NONE)
  {
    m_actionHandler->OnAction(dispatchAction);
    bHandled = true;
  }

  m_bHeld = bPressed;
  m_magnitude = magnitude;
  m_lastHoldTimeMs = holdTimeMs;
  m_lastAction = dispatchAction;

  return bHandled;
}

CAction CKeyHandler::ProcessActions(std::vector<const KeymapAction*> actions,
                                    int windowId,
                                    float magnitude,
                                    unsigned int holdTimeMs)
{
  CAction dispatchAction;

  // Filter out actions without pressed hotkeys
  actions.erase(std::remove_if(actions.begin(), actions.end(),
                               [this](const KeymapAction* action)
                               { return !m_keymapHandler->HotkeysPressed(action->hotkeys); }),
                actions.end());

  if (actions.empty())
    return false;

  // Actions are sorted by holdtime, so the final action is the one with the
  // greatest holdtime
  const KeymapAction& finalAction = **actions.rbegin();
  const unsigned int maxHoldTimeMs = finalAction.holdTimeMs;

  const bool bHasDelay = (maxHoldTimeMs > 0);
  if (!bHasDelay)
  {
    dispatchAction = ProcessAction(finalAction, windowId, magnitude, holdTimeMs);
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

      dispatchAction = ProcessAction(finalAction, windowId, magnitude, holdTimeMs);
    }
    else
    {
      // Calculate press state
      const bool bPressed = IsPressed(magnitude);
      const bool bJustReleased = m_bHeld && !bPressed;

      // If button was just released, send a release action
      if (bJustReleased)
        dispatchAction = ProcessRelease(actions, windowId);
    }
  }

  return dispatchAction;
}

CAction CKeyHandler::ProcessRelease(std::vector<const KeymapAction*> actions, int windowId)
{
  CAction dispatchAction;

  // Use previous holdtime from before button release
  const unsigned int holdTimeMs = m_lastHoldTimeMs;

  // Send an action on release if one occurs before the holdtime
  for (auto it = actions.begin(); it != actions.end();)
  {
    const KeymapAction& action = **it;

    unsigned int thisHoldTime = (*it)->holdTimeMs;

    ++it;
    if (it == actions.end())
      break;

    unsigned int nextHoldTime = (*it)->holdTimeMs;

    if (thisHoldTime <= holdTimeMs && holdTimeMs < nextHoldTime)
    {
      dispatchAction = ProcessAction(action, windowId, 1.0f, 0);
      break;
    }
  }

  return dispatchAction;
}

CAction CKeyHandler::ProcessAction(const KeymapAction& action,
                                   int windowId,
                                   float magnitude,
                                   unsigned int holdTimeMs)
{
  CAction dispatchAction;

  bool bSendAction = false;

  if (windowId != m_activeWindowId)
  {
    // Don't send actions if the window has changed since being pressed
  }
  else if (ACTION::CActionTranslator::IsAnalog(action.actionId))
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
    m_keymapHandler->OnPress(m_keyName);
    m_bActionSent = true;
    m_lastActionMs = holdTimeMs;
    dispatchAction = guiAction;
  }

  return dispatchAction;
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
