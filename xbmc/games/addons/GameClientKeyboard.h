/*
 *      Copyright (C) 2015-2016 Team Kodi
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

#include "input/keyboard/IKeyboardHandler.h"

struct KodiToAddonFuncTable_Game;

namespace GAME
{
  class CGameClient;

  /*!
   * \ingroup games
   * \brief Handles keyboard events for games.
   *
   * Listens to keyboard events and forwards them to the games (as game_input_event).
   */
  class CGameClientKeyboard : public KODI::KEYBOARD::IKeyboardHandler
  {
  public:
    /*!
     * \brief Constructor registers for keyboard events at CInputManager.
     * \param gameClient The game client implementation.
     * \param dllStruct The emulator or game to which the events are sent.
     */
    CGameClientKeyboard(const CGameClient* gameClient, const KodiToAddonFuncTable_Game* dllStruct);

    /*!
     * \brief Destructor unregisters from keyboard events from CInputManager.
     */
    ~CGameClientKeyboard();

    // implementation of IKeyboardHandler
    virtual bool OnKeyPress(const CKey& key) override;
    virtual void OnKeyRelease(const CKey& key) override;

  private:
    // Construction parameters
    const CGameClient* const m_gameClient;
    const KodiToAddonFuncTable_Game* const m_dllStruct;
  };
}
