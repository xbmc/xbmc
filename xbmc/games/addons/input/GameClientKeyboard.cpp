/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientKeyboard.h"

#include "GameClientInput.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/Game.h"
#include "games/addons/GameClient.h"
#include "games/addons/GameClientTranslator.h"
#include "games/controllers/input/ControllerActivity.h"
#include "input/keyboard/interfaces/IKeyboardInputProvider.h"
#include "utils/log.h"

#include <utility>

using namespace KODI;
using namespace GAME;

#define BUTTON_INDEX_MASK 0x01ff

CGameClientKeyboard::CGameClientKeyboard(CGameClient& gameClient,
                                         std::string controllerId,
                                         KEYBOARD::IKeyboardInputProvider* inputProvider)
  : m_gameClient(gameClient),
    m_controllerId(std::move(controllerId)),
    m_inputProvider(inputProvider),
    m_keyboardActivity(std::make_unique<CControllerActivity>())
{
  m_inputProvider->RegisterKeyboardHandler(this, false, false);
}

CGameClientKeyboard::~CGameClientKeyboard()
{
  m_inputProvider->UnregisterKeyboardHandler(this);
}

std::string CGameClientKeyboard::ControllerID() const
{
  return m_controllerId;
}

bool CGameClientKeyboard::HasKey(const KEYBOARD::KeyName& key) const
{
  return m_gameClient.Input().HasFeature(ControllerID(), key);
}

bool CGameClientKeyboard::OnKeyPress(const KEYBOARD::KeyName& key,
                                     KEYBOARD::Modifier mod,
                                     uint32_t unicode)
{
  m_keyboardActivity->OnKeyPress(key);
  m_keyboardActivity->OnInputFrame();

  // Only allow activated input in fullscreen game
  if (!m_gameClient.Input().AcceptsInput())
  {
    CLog::Log(LOGDEBUG, "GAME: key press ignored, not in fullscreen game");
    return false;
  }

  game_input_event event;

  event.type = GAME_INPUT_EVENT_KEY;
  event.controller_id = m_controllerId.c_str();
  event.port_type = GAME_PORT_KEYBOARD;
  event.port_address = KEYBOARD_PORT_ADDRESS;
  event.feature_name = key.c_str();
  event.key.pressed = true;
  event.key.unicode = unicode;
  event.key.modifiers = CGameClientTranslator::GetModifiers(mod);

  return m_gameClient.Input().InputEvent(event);
}

void CGameClientKeyboard::OnKeyRelease(const KEYBOARD::KeyName& key,
                                       KEYBOARD::Modifier mod,
                                       uint32_t unicode)
{
  m_keyboardActivity->OnKeyRelease(key);
  m_keyboardActivity->OnInputFrame();

  game_input_event event;

  event.type = GAME_INPUT_EVENT_KEY;
  event.controller_id = m_controllerId.c_str();
  event.port_type = GAME_PORT_KEYBOARD;
  event.port_address = KEYBOARD_PORT_ADDRESS;
  event.feature_name = key.c_str();
  event.key.pressed = false;
  event.key.unicode = unicode;
  event.key.modifiers = CGameClientTranslator::GetModifiers(mod);

  m_gameClient.Input().InputEvent(event);
}

float CGameClientKeyboard::GetActivation() const
{
  return m_keyboardActivity->GetActivation();
}

void CGameClientKeyboard::SetSource(PERIPHERALS::PeripheralPtr sourcePeripheral)
{
  m_sourcePeripheral = std::move(sourcePeripheral);
}

void CGameClientKeyboard::ClearSource()
{
  m_sourcePeripheral.reset();
}
