/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/ControllerTypes.h"
#include "input/joysticks/interfaces/IInputHandler.h"
#include "peripherals/PeripheralTypes.h"

#include <memory>

namespace KODI
{
namespace JOYSTICK
{
class IInputProvider;
}

namespace GAME
{
class CGameClient;
class CPortInput;

/*!
 * \ingroup games
 *
 * \brief Handles game controller events for games.
 *
 * Listens to game controller events and forwards them to the games (as game_input_event).
 */
class CGameClientJoystick : public JOYSTICK::IInputHandler
{
public:
  /*!
   * \brief Constructor.
   * \param addon The game client implementation.
   * \param port The port this game controller is associated with.
   * \param controller The game controller which is used (for controller mapping).
   * \param dllStruct The emulator or game to which the events are sent.
   */
  CGameClientJoystick(CGameClient& addon,
                      const std::string& portAddress,
                      const ControllerPtr& controller);

  ~CGameClientJoystick() override;

  void RegisterInput(JOYSTICK::IInputProvider* inputProvider);
  void UnregisterInput(JOYSTICK::IInputProvider* inputProvider);

  // Implementation of IInputHandler
  std::string ControllerID() const override;
  bool HasFeature(const std::string& feature) const override;
  bool AcceptsInput(const std::string& feature) const override;
  bool OnButtonPress(const std::string& feature, bool bPressed) override;
  void OnButtonHold(const std::string& feature, unsigned int holdTimeMs) override {}
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
  void OnInputFrame() override {}

  // Input accessors
  const std::string& GetPortAddress() const { return m_portAddress; }
  ControllerPtr GetController() const { return m_controller; }
  std::string GetControllerAddress() const;
  PERIPHERALS::PeripheralPtr GetSource() const { return m_sourcePeripheral; }
  std::string GetSourceLocation() const;
  float GetActivation() const;

  // Input mutators
  void SetSource(PERIPHERALS::PeripheralPtr sourcePeripheral);
  void ClearSource();

  // Input handlers
  bool SetRumble(const std::string& feature, float magnitude);

private:
  // Construction parameters
  CGameClient& m_gameClient;
  const std::string m_portAddress;
  const ControllerPtr m_controller;

  // Input parameters
  std::unique_ptr<CPortInput> m_portInput;
  PERIPHERALS::PeripheralPtr m_sourcePeripheral;
};
} // namespace GAME
} // namespace KODI
