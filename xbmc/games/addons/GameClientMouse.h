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

#include "input/mouse/IMouseInputHandler.h"

struct KodiToAddonFuncTable_Game;

namespace GAME
{
  class CGameClient;

  /*!
   * \ingroup games
   * \brief Handles mouse events for games.
   *
   * Listens to mouse events and forwards them to the games (as game_input_event).
   */
  class CGameClientMouse : public KODI::MOUSE::IMouseInputHandler
  {
  public:
    /*!
     * \brief Constructor registers for mouse events at CInputManager.
     * \param gameClient The game client implementation.
     * \param dllStruct The emulator or game to which the events are sent.
     */
    CGameClientMouse(const CGameClient* gameClient, const KodiToAddonFuncTable_Game* dllStruct);

    /*!
     * \brief Destructor unregisters from mouse events from CInputManager.
     */
    virtual ~CGameClientMouse();

    // implementation of IMouseInputHandler
    virtual std::string ControllerID(void) const override;
    virtual bool OnMotion(const std::string& relpointer, int dx, int dy) override;
    virtual bool OnButtonPress(const std::string& button) override;
    virtual void OnButtonRelease(const std::string& button) override;

  private:
    // Construction parameters
    const CGameClient* const m_gameClient;
    const KodiToAddonFuncTable_Game* const m_dllStruct;
    const std::string m_controllerId;
  };
}
