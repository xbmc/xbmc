/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/keyboard/interfaces/IKeyboardInputHandler.h"

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
    CGameClientKeyboard(CGameClient &gameClient,
                        std::string controllerId,
                        KEYBOARD::IKeyboardInputProvider *inputProvider);

    /*!
     * \brief Destructor unregisters from keyboard events from CInputManager.
     */
    ~CGameClientKeyboard() override;

    // implementation of IKeyboardInputHandler
    std::string ControllerID() const override;
    bool HasKey(const KEYBOARD::KeyName &key) const override;
    bool OnKeyPress(const KEYBOARD::KeyName &key, KEYBOARD::Modifier mod, uint32_t unicode) override;
    void OnKeyRelease(const KEYBOARD::KeyName &key, KEYBOARD::Modifier mod, uint32_t unicode) override;

  private:
    // Construction parameters
    CGameClient &m_gameClient;
    const std::string m_controllerId;
    KEYBOARD::IKeyboardInputProvider *const m_inputProvider;
  };
}
}
