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
#include "input/joysticks/IInputHandler.h"

struct KodiToAddonFuncTable_Game;

namespace KODI
{
namespace GAME
{
  class CGameClient;

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
    CGameClientJoystick(CGameClient* addon, int port, const ControllerPtr& controller, const KodiToAddonFuncTable_Game* dllStruct);

    virtual ~CGameClientJoystick() = default;

    // Implementation of IInputHandler
    virtual std::string ControllerID(void) const override;
    virtual bool HasFeature(const std::string& feature) const override;
    virtual bool AcceptsInput(void) override;
    virtual unsigned int GetDelayMs(const std::string& feature) const override { return 0; }
    virtual bool OnButtonPress(const std::string& feature, bool bPressed) override;
    virtual void OnButtonHold(const std::string& feature, unsigned int holdTimeMs) override { }
    virtual bool OnButtonMotion(const std::string& feature, float magnitude, unsigned int motionTimeMs) override;
    virtual bool OnAnalogStickMotion(const std::string& feature, float x, float y, unsigned int motionTimeMs) override;
    virtual bool OnAccelerometerMotion(const std::string& feature, float x, float y, float z) override;

    bool SetRumble(const std::string& feature, float magnitude);

  private:
    const CGameClient* const  m_gameClient;
    const int                 m_port;
    const ControllerPtr       m_controller;
    const KodiToAddonFuncTable_Game* const m_dllStruct;
  };
}
}
