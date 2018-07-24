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
  class CPort;

  /*!
   * \ingroup games
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
    CGameClientJoystick(CGameClient &addon,
                        const std::string &portAddress,
                        const ControllerPtr& controller);

    ~CGameClientJoystick() override;

    void RegisterInput(JOYSTICK::IInputProvider *inputProvider);
    void UnregisterInput(JOYSTICK::IInputProvider *inputProvider);

    // Implementation of IInputHandler
    virtual std::string ControllerID(void) const override;
    virtual bool HasFeature(const std::string& feature) const override;
    virtual bool AcceptsInput(const std::string &feature) const override;
    virtual bool OnButtonPress(const std::string& feature, bool bPressed) override;
    virtual void OnButtonHold(const std::string& feature, unsigned int holdTimeMs) override { }
    virtual bool OnButtonMotion(const std::string& feature, float magnitude, unsigned int motionTimeMs) override;
    virtual bool OnAnalogStickMotion(const std::string& feature, float x, float y, unsigned int motionTimeMs) override;
    virtual bool OnAccelerometerMotion(const std::string& feature, float x, float y, float z) override;
    virtual bool OnWheelMotion(const std::string& feature, float position, unsigned int motionTimeMs) override;
    virtual bool OnThrottleMotion(const std::string& feature, float position, unsigned int motionTimeMs) override;

    bool SetRumble(const std::string& feature, float magnitude);

  private:
    // Construction parameters
    CGameClient &m_gameClient;
    const std::string m_portAddress;
    const ControllerPtr       m_controller;

    // Input parameters
    std::unique_ptr<CPort> m_port;
  };
}
}
