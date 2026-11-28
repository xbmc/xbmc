/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/joysticks/interfaces/IInputHandler.h"

#include <chrono>
#include <mutex>
#include <string>

namespace KODI
{
namespace GAME
{
class CGameClient;

/*!
 * \ingroup games
 *
 * \brief Moves a game's mouse with a controller's right stick
 *
 * The right trigger is the left mouse button and the left trigger the right
 * one. This only happens while the game has a mouse and the setting is on;
 * otherwise the stick and triggers reach the game as usual.
 */
class CGameClientStickMouse : public JOYSTICK::IInputHandler
{
public:
  explicit CGameClientStickMouse(CGameClient& gameClient);
  ~CGameClientStickMouse() override;

  // Implementation of IInputHandler
  std::string ControllerID() const override;
  bool HasFeature(const JOYSTICK::FeatureName& feature) const override;
  bool AcceptsInput(const JOYSTICK::FeatureName& feature) const override;
  bool OnButtonPress(const JOYSTICK::FeatureName& feature, bool bPressed) override;
  void OnButtonHold(const JOYSTICK::FeatureName& feature, unsigned int holdTimeMs) override {}
  bool OnButtonMotion(const JOYSTICK::FeatureName& feature,
                      float magnitude,
                      unsigned int motionTimeMs) override;
  bool OnAnalogStickMotion(const JOYSTICK::FeatureName& feature,
                           float x,
                           float y,
                           unsigned int motionTimeMs) override;
  bool OnWheelMotion(const JOYSTICK::FeatureName& feature,
                     float position,
                     unsigned int motionTimeMs) override
  {
    return false;
  }
  bool OnThrottleMotion(const JOYSTICK::FeatureName& feature,
                        float position,
                        unsigned int motionTimeMs) override
  {
    return false;
  }
  void OnInputFrame() override {}

  /*!
   * \brief Let go of the mouse buttons the triggers still hold
   */
  void ReleaseButtons();

private:
  void SetButton(const std::string& button, float magnitude, bool& pressed);

  CGameClient& m_gameClient;

  std::chrono::steady_clock::time_point m_lastMotion{};
  float m_remainderX{0.0f};
  float m_remainderY{0.0f};

  // Set from input callbacks, and cleared by ReleaseButtons() from any thread
  std::mutex m_buttonMutex;
  bool m_leftPressed{false};
  bool m_rightPressed{false};
};
} // namespace GAME
} // namespace KODI
