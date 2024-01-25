/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/keyboard/KeyboardTypes.h"
#include "input/mouse/MouseTypes.h"

#include <set>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 *
 * \brief Class to hold state about the current activity of a controller
 *
 * The state is held as a single float value, which is updated by the various
 * On*() methods. The activity is the maximum value on a single input frame.
 * The value is saved to m_lastActivity on each call to OnInputFrame().
 */
class CControllerActivity
{
public:
  CControllerActivity() = default;
  ~CControllerActivity() = default;

  float GetActivation() const { return m_lastActivation; }
  void ClearButtonState();

  void OnButtonPress(bool pressed);
  void OnButtonMotion(float magnitude);
  void OnAnalogStickMotion(float x, float y);
  void OnWheelMotion(float position);
  void OnThrottleMotion(float position);
  void OnKeyPress(const KEYBOARD::KeyName& key);
  void OnKeyRelease(const KEYBOARD::KeyName& key);
  void OnMouseMotion(const MOUSE::PointerName& relpointer, int differenceX, int differenceY);
  void OnMouseButtonPress(const MOUSE::ButtonName& button);
  void OnMouseButtonRelease(const MOUSE::ButtonName& button);
  void OnInputFrame();

private:
  // Input helpers
  static INPUT::INTERCARDINAL_DIRECTION GetPointerDirection(int differenceX, int differenceY);

  // State parameters
  float m_lastActivation{0.0f};
  float m_currentActivation{0.0f};
  KEYBOARD::KeyName m_activeKey;
  std::set<MOUSE::PointerName> m_activePointers;
  std::set<MOUSE::ButtonName> m_activeButtons;
  bool m_bKeyPressed{false};
};
} // namespace GAME
} // namespace KODI
