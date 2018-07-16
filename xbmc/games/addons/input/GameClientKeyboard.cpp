/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientKeyboard.h"
#include "GameClientInput.h"
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_game_types.h"
#include "games/addons/GameClient.h"
#include "games/addons/GameClientTranslator.h"
#include "input/keyboard/interfaces/IKeyboardInputProvider.h"
#include "input/Key.h"
#include "utils/log.h"

#include <utility>

using namespace KODI;
using namespace GAME;

#define BUTTON_INDEX_MASK  0x01ff

CGameClientKeyboard::CGameClientKeyboard(const CGameClient &gameClient,
                                         std::string controllerId,
                                         const KodiToAddonFuncTable_Game &dllStruct,
                                         KEYBOARD::IKeyboardInputProvider *inputProvider) :
  m_gameClient(gameClient),
  m_controllerId(std::move(controllerId)),
  m_dllStruct(dllStruct),
  m_inputProvider(inputProvider)
{
  m_inputProvider->RegisterKeyboardHandler(this, false);
}

CGameClientKeyboard::~CGameClientKeyboard()
{
  m_inputProvider->UnregisterKeyboardHandler(this);
}

std::string CGameClientKeyboard::ControllerID() const
{
  return m_controllerId;
}

bool CGameClientKeyboard::HasKey(const KEYBOARD::KeyName &key) const
{
  try
  {
    return m_dllStruct.HasFeature(ControllerID().c_str(), key.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GAME: %s: exception caught in HasFeature()", m_gameClient.ID().c_str());
  }

  return false;
}

bool CGameClientKeyboard::OnKeyPress(const KEYBOARD::KeyName &key, KEYBOARD::Modifier mod, uint32_t unicode)
{
  // Only allow activated input in fullscreen game
  if (!m_gameClient.Input().AcceptsInput())
  {
    CLog::Log(LOGDEBUG, "GAME: key press ignored, not in fullscreen game");
    return false;
  }

  bool bHandled = false;

  game_input_event event;

  event.type            = GAME_INPUT_EVENT_KEY;
  event.controller_id   = m_controllerId.c_str();
  event.port_type       = GAME_PORT_KEYBOARD;
  event.port_address    = ""; // Not used
  event.feature_name    = key.c_str();
  event.key.pressed     = true;
  event.key.unicode     = unicode;
  event.key.modifiers   = CGameClientTranslator::GetModifiers(mod);

  try
  {
    bHandled = m_dllStruct.InputEvent(&event);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GAME: %s: exception caught in InputEvent()", m_gameClient.ID().c_str());
  }

  return bHandled;
}

void CGameClientKeyboard::OnKeyRelease(const KEYBOARD::KeyName &key, KEYBOARD::Modifier mod, uint32_t unicode)
{
  game_input_event event;

  event.type            = GAME_INPUT_EVENT_KEY;
  event.controller_id   = m_controllerId.c_str();
  event.port_type       = GAME_PORT_KEYBOARD;
  event.port_address    = ""; // Not used
  event.feature_name    = key.c_str();
  event.key.pressed     = false;
  event.key.unicode     = unicode;
  event.key.modifiers   = CGameClientTranslator::GetModifiers(mod);

  try
  {
    m_dllStruct.InputEvent(&event);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GAME: %s: exception caught in InputEvent()", m_gameClient.ID().c_str());
  }
}
