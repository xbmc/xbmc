/*
 *      Copyright (C) 2015-2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "games/controllers/ControllerTypes.h"
#include "input/joysticks/interfaces/IInputHandler.h"

#include <memory>

struct KodiToAddonFuncTable_Game;

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
    CGameClientJoystick(const CGameClient &addon,
                        const std::string &portAddress,
                        const ControllerPtr& controller,
                        const KodiToAddonFuncTable_Game &dllStruct);

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
    const CGameClient &m_gameClient;
    const std::string m_portAddress;
    const ControllerPtr       m_controller;
    const KodiToAddonFuncTable_Game &m_dllStruct;

    // Input parameters
    std::unique_ptr<CPort> m_port;
  };
}
}
