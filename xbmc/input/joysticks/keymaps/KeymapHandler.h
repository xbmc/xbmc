/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/joysticks/JoystickTypes.h"
#include "input/joysticks/interfaces/IButtonSequence.h"
#include "input/joysticks/interfaces/IInputHandler.h"
#include "input/joysticks/interfaces/IKeymapHandler.h"

#include <map>
#include <memory>
#include <string>

class IActionListener;
class IKeymap;

namespace KODI
{
namespace JOYSTICK
{
class IKeyHandler;

/*!
 * \ingroup joystick
 * \brief
 */
class CKeymapHandler : public IKeymapHandler, public IInputHandler
{
public:
  CKeymapHandler(IActionListener* actionHandler, const IKeymap* keymap);

  ~CKeymapHandler() override = default;

  // implementation of IKeymapHandler
  bool HotkeysPressed(const std::set<std::string>& keyNames) const override;
  std::string GetLastPressed() const override { return m_lastPressed; }
  void OnPress(const std::string& keyName) override { m_lastPressed = keyName; }

  // implementation of IInputHandler
  std::string ControllerID() const override;
  bool HasFeature(const FeatureName& feature) const override { return true; }
  bool AcceptsInput(const FeatureName& feature) const override;
  bool OnButtonPress(const FeatureName& feature, bool bPressed) override;
  void OnButtonHold(const FeatureName& feature, unsigned int holdTimeMs) override;
  bool OnButtonMotion(const FeatureName& feature,
                      float magnitude,
                      unsigned int motionTimeMs) override;
  bool OnAnalogStickMotion(const FeatureName& feature,
                           float x,
                           float y,
                           unsigned int motionTimeMs) override;
  bool OnAccelerometerMotion(const FeatureName& feature, float x, float y, float z) override;
  bool OnWheelMotion(const FeatureName& feature,
                     float position,
                     unsigned int motionTimeMs) override;
  bool OnThrottleMotion(const FeatureName& feature,
                        float position,
                        unsigned int motionTimeMs) override;
  void OnInputFrame() override {}

protected:
  // Keep track of cheat code presses
  std::unique_ptr<IButtonSequence> m_easterEgg;

private:
  // Analog stick helper functions
  bool ActivateDirection(const FeatureName& feature,
                         float magnitude,
                         ANALOG_STICK_DIRECTION dir,
                         unsigned int motionTimeMs);
  void DeactivateDirection(const FeatureName& feature, ANALOG_STICK_DIRECTION dir);

  // Wheel helper functions
  bool ActivateDirection(const FeatureName& feature,
                         float magnitude,
                         WHEEL_DIRECTION dir,
                         unsigned int motionTimeMs);
  void DeactivateDirection(const FeatureName& feature, WHEEL_DIRECTION dir);

  // Throttle helper functions
  bool ActivateDirection(const FeatureName& feature,
                         float magnitude,
                         THROTTLE_DIRECTION dir,
                         unsigned int motionTimeMs);
  void DeactivateDirection(const FeatureName& feature, THROTTLE_DIRECTION dir);

  // Helper functions
  IKeyHandler* GetKeyHandler(const std::string& keyName);
  bool HasAction(const std::string& keyName) const;

  // Construction parameters
  IActionListener* const m_actionHandler;
  const IKeymap* const m_keymap;

  // Handlers for individual keys
  std::map<std::string, std::unique_ptr<IKeyHandler>> m_keyHandlers; // Key name -> handler

  // Last pressed key
  std::string m_lastPressed;
};
} // namespace JOYSTICK
} // namespace KODI
