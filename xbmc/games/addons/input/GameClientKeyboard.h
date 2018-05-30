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

#include "input/keyboard/interfaces/IKeyboardInputHandler.h"

struct KodiToAddonFuncTable_Game;

namespace KODI
{
namespace KEYBOARD
{
  class IKeyboardInputProvider;
}

namespace GAME
{
  class CGameClient;

  /*!
   * \ingroup games
   * \brief Handles keyboard events for games.
   *
   * Listens to keyboard events and forwards them to the games (as game_input_event).
   */
  class CGameClientKeyboard : public KEYBOARD::IKeyboardInputHandler
  {
  public:
    /*!
     * \brief Constructor registers for keyboard events at CInputManager.
     * \param gameClient The game client implementation.
     * \param controllerId The controller profile used for input
     * \param dllStruct The emulator or game to which the events are sent.
     * \param inputProvider The interface providing us with keyboard input.
     */
    CGameClientKeyboard(const CGameClient &gameClient,
                        std::string controllerId,
                        const KodiToAddonFuncTable_Game &dllStruct,
                        KEYBOARD::IKeyboardInputProvider *inputProvider);

    /*!
     * \brief Destructor unregisters from keyboard events from CInputManager.
     */
    virtual ~CGameClientKeyboard();

    // implementation of IKeyboardInputHandler
    std::string ControllerID() const override;
    bool HasKey(const KEYBOARD::KeyName &key) const override;
    bool OnKeyPress(const KEYBOARD::KeyName &key, KEYBOARD::Modifier mod, uint32_t unicode) override;
    void OnKeyRelease(const KEYBOARD::KeyName &key, KEYBOARD::Modifier mod, uint32_t unicode) override;

  private:
    // Construction parameters
    const CGameClient &m_gameClient;
    const std::string m_controllerId;
    const KodiToAddonFuncTable_Game &m_dllStruct;
    KEYBOARD::IKeyboardInputProvider *const m_inputProvider;
  };
}
}
