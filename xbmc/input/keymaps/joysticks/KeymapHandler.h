/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/joysticks/JoystickTypes.h"
#include "input/joysticks/interfaces/IButtonSequence.h"
#include "input/joysticks/interfaces/IInputHandler.h"
#include "input/keymaps/interfaces/IKeymapHandler.h"

#include <map>
#include <memory>
#include <string>

namespace KODI
{
namespace ACTION
{
class IActionListener;
} // namespace ACTION

namespace KEYMAP
{
class IKeyHandler;
class IKeymap;

/*!
 * \ingroup keymap
 */
class CKeymapHandler : public IKeymapHandler, public JOYSTICK::IInputHandler
{
public:
  CKeymapHandler(ACTION::IActionListener* actionHandler, const IKeymap* keymap);

  ~CKeymapHandler() override = default;

  // implementation of IKeymapHandler
  bool HotkeysPressed(const std::set<std::string>& keyNames) const override;
  std::string GetLastPressed() const override { return m_lastPressed; }
  void OnPress(const std::string& keyName) override { m_lastPressed = keyName; }

  // implementation of IInputHandler
  std::string ControllerID() const override;
  bool HasFeature(const JOYSTICK::FeatureName& feature) const override { return true; }
  bool AcceptsInput(const JOYSTICK::FeatureName& feature) const override;
  bool OnButtonPress(const JOYSTICK::FeatureName& feature, bool bPressed) override;
  void OnButtonHold(const JOYSTICK::FeatureName& feature, unsigned int holdTimeMs) override;
  bool OnButtonMotion(const JOYSTICK::FeatureName& feature,
                      float magnitude,
                      unsigned int motionTimeMs) override;
  bool OnAnalogStickMotion(const JOYSTICK::FeatureName& feature,
                           float x,
                           float y,
                           unsigned int motionTimeMs) override;
  bool OnAccelerometerMotion(const JOYSTICK::FeatureName& feature,
                             float x,
                             float y,
                             float z) override;
  bool OnWheelMotion(const JOYSTICK::FeatureName& feature,
                     float position,
                     unsigned int motionTimeMs) override;
  bool OnThrottleMotion(const JOYSTICK::FeatureName& feature,
                        float position,
                        unsigned int motionTimeMs) override;
  void OnInputFrame() override {}

protected:
  // Keep track of cheat code presses
  std::unique_ptr<JOYSTICK::IButtonSequence> m_easterEgg;

private:
  // Analog stick helper functions
  bool ActivateDirection(const JOYSTICK::FeatureName& feature,
                         float magnitude,
                         JOYSTICK::ANALOG_STICK_DIRECTION dir,
                         unsigned int motionTimeMs);
  void DeactivateDirection(const JOYSTICK::FeatureName& feature,
                           JOYSTICK::ANALOG_STICK_DIRECTION dir);

  // Wheel helper functions
  bool ActivateDirection(const JOYSTICK::FeatureName& feature,
                         float magnitude,
                         JOYSTICK::WHEEL_DIRECTION dir,
                         unsigned int motionTimeMs);
  void DeactivateDirection(const JOYSTICK::FeatureName& feature, JOYSTICK::WHEEL_DIRECTION dir);

  // Throttle helper functions
  bool ActivateDirection(const JOYSTICK::FeatureName& feature,
                         float magnitude,
                         JOYSTICK::THROTTLE_DIRECTION dir,
                         unsigned int motionTimeMs);
  void DeactivateDirection(const JOYSTICK::FeatureName& feature, JOYSTICK::THROTTLE_DIRECTION dir);

  // Helper functions
  IKeyHandler* GetKeyHandler(const std::string& keyName);
  bool HasAction(const std::string& keyName) const;

  // Construction parameters
  ACTION::IActionListener* const m_actionHandler;
  const IKeymap* const m_keymap;

  // Handlers for individual keys
  std::map<std::string, std::unique_ptr<IKeyHandler>> m_keyHandlers; // Key name -> handler

  // Last pressed key
  std::string m_lastPressed;
};
} // namespace KEYMAP
} // namespace KODI
