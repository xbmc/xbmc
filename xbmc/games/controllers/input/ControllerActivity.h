/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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

  void OnButtonPress(bool pressed);
  void OnButtonMotion(float magnitude);
  void OnAnalogStickMotion(float x, float y);
  void OnWheelMotion(float position);
  void OnThrottleMotion(float position);
  void OnInputFrame();

private:
  float m_lastActivation{0.0f};
  float m_currentActivation{0.0f};
};
} // namespace GAME
} // namespace KODI
