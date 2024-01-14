/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/joysticks/interfaces/IInputHandler.h"
#include "input/keymaps/interfaces/IKeymapEnvironment.h"

#include <memory>

namespace KODI
{
namespace JOYSTICK
{
class IInputProvider;
} // namespace JOYSTICK

namespace KEYMAP
{
class CKeymapHandling;
} // namespace KEYMAP

namespace GAME
{
class CControllerActivity;

/*!
 * \ingroup games
 */
class CPortInput : public JOYSTICK::IInputHandler, public KEYMAP::IKeymapEnvironment
{
public:
  CPortInput(JOYSTICK::IInputHandler* gameInput);
  ~CPortInput() override;

  void RegisterInput(JOYSTICK::IInputProvider* provider);
  void UnregisterInput(JOYSTICK::IInputProvider* provider);

  // Input parameters
  float GetActivation() const;

  // Implementation of IInputHandler
  std::string ControllerID() const override;
  bool HasFeature(const std::string& feature) const override { return true; }
  bool AcceptsInput(const std::string& feature) const override;
  bool OnButtonPress(const std::string& feature, bool bPressed) override;
  void OnButtonHold(const std::string& feature, unsigned int holdTimeMs) override;
  bool OnButtonMotion(const std::string& feature,
                      float magnitude,
                      unsigned int motionTimeMs) override;
  bool OnAnalogStickMotion(const std::string& feature,
                           float x,
                           float y,
                           unsigned int motionTimeMs) override;
  bool OnAccelerometerMotion(const std::string& feature, float x, float y, float z) override;
  bool OnWheelMotion(const std::string& feature,
                     float position,
                     unsigned int motionTimeMs) override;
  bool OnThrottleMotion(const std::string& feature,
                        float position,
                        unsigned int motionTimeMs) override;
  void OnInputFrame() override;

  // Implementation of IKeymapEnvironment
  int GetWindowID() const override;
  void SetWindowID(int windowId) override {}
  int GetFallthrough(int windowId) const override { return -1; }
  bool UseGlobalFallthrough() const override { return false; }
  bool UseEasterEgg() const override { return false; }

private:
  // Construction parameters
  JOYSTICK::IInputHandler* const m_gameInput;

  // Handles input to Kodi
  std::unique_ptr<KEYMAP::CKeymapHandling> m_appInput;

  // Prevents input falling through to Kodi when not handled by the game
  std::unique_ptr<JOYSTICK::IInputHandler> m_inputSink;

  // Records controller activity
  std::unique_ptr<CControllerActivity> m_controllerActivity;
};
} // namespace GAME
} // namespace KODI
