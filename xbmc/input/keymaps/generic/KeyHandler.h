/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/actions/Action.h"
#include "input/keymaps/KeymapTypes.h"
#include "input/keymaps/interfaces/IKeyHandler.h"

#include <map>
#include <string>
#include <vector>

class CAction;

namespace KODI
{
namespace ACTION
{
class IActionListener;
} // namespace ACTION

namespace KEYMAP
{
class IKeymapHandler;
class IKeymap;

/*!
 * \ingroup keymap
 */
class CKeyHandler : public IKeyHandler
{
public:
  CKeyHandler(const std::string& keyName,
              ACTION::IActionListener* actionHandler,
              const IKeymap* keymap,
              IKeymapHandler* keymapHandler);

  ~CKeyHandler() override = default;

  // implementation of IKeyHandler
  bool IsPressed() const override { return m_bHeld; }
  bool OnDigitalMotion(bool bPressed, unsigned int holdTimeMs) override;
  bool OnAnalogMotion(float magnitude, unsigned int motionTimeMs) override;

private:
  void Reset();

  /*!
   * \brief Process actions to see if an action should be dispatched
   *
   * \param actions All actions from the keymap defined for the current window
   * \param windowId The current window ID
   * \param magnitude The magnitude or distance of the feature being handled
   * \param holdTimeMs The time which the feature has been past the hold threshold
   *
   * \return The action to dispatch, or action with ID ACTION_NONE if no action should be dispatched
   */
  CAction ProcessActions(std::vector<const KeymapAction*> actions,
                         int windowId,
                         float magnitude,
                         unsigned int holdTimeMs);

  /*!
   * \brief Process actions after release event to see if an action should be dispatched
   *
   * \param actions All actions from the keymap defined for the current window
   * \param windowId The current window ID
   *
   * \return The action to dispatch, or action with ID ACTION_NONE if no action should be dispatched
   */
  CAction ProcessRelease(std::vector<const KeymapAction*> actions, int windowId);

  /*!
   * \brief Process an action to see if it should be dispatched
   *
   * \param action The action chosen to be dispatched
   * \param windowId The current window ID
   * \param magnitude The magnitude or distance of the feature being handled
   * \param holdTimeMs The time which the feature has been past the hold threshold
   *
   * \return The action to dispatch, or action with ID ACTION_NONE if no action should be dispatched
   */
  CAction ProcessAction(const KeymapAction& action,
                        int windowId,
                        float magnitude,
                        unsigned int holdTimeMs);

  // Check criteria for sending a repeat action
  bool SendRepeatAction(unsigned int holdTimeMs);

  // Helper function
  static bool IsPressed(float magnitude);

  // Construction parameters
  const std::string m_keyName;
  ACTION::IActionListener* const m_actionHandler;
  const IKeymap* const m_keymap;
  IKeymapHandler* const m_keymapHandler;

  // State variables
  bool m_bHeld;
  float m_magnitude;
  unsigned int m_holdStartTimeMs;
  unsigned int m_lastHoldTimeMs;
  bool m_bActionSent;
  unsigned int m_lastActionMs;
  int m_activeWindowId = -1; // Window that activated the key
  CAction m_lastAction;
};
} // namespace KEYMAP
} // namespace KODI
