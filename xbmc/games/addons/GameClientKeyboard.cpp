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

#include "GameClientKeyboard.h"
#include "GameClient.h"
#include "GameClientTranslator.h"
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_game_types.h"
#include "input/InputManager.h"
#include "input/Key.h"
#include "utils/log.h"

using namespace KODI;
using namespace GAME;

#define BUTTON_INDEX_MASK  0x01ff

CGameClientKeyboard::CGameClientKeyboard(const CGameClient* gameClient, const KodiToAddonFuncTable_Game* dllStruct) :
  m_gameClient(gameClient),
  m_dllStruct(dllStruct)
{
  CInputManager::GetInstance().RegisterKeyboardHandler(this);
}

CGameClientKeyboard::~CGameClientKeyboard()
{
  CInputManager::GetInstance().UnregisterKeyboardHandler(this);
}

bool CGameClientKeyboard::OnKeyPress(const CKey& key)
{
  // Only allow activated input in fullscreen game
  if (!m_gameClient->AcceptsInput())
  {
    CLog::Log(LOGDEBUG, "GAME: key press ignored, not in fullscreen game");
    return false;
  }

  bool bHandled = false;

  game_input_event event;

  event.type            = GAME_INPUT_EVENT_KEY;
  event.port            = GAME_INPUT_PORT_KEYBOARD;
  event.controller_id   = ""; //! @todo
  event.feature_name    = ""; //! @todo
  event.key.pressed     = true;
  event.key.character   = static_cast<XBMCVKey>(key.GetButtonCode() & BUTTON_INDEX_MASK);
  event.key.modifiers   = CGameClientTranslator::GetModifiers(static_cast<CKey::Modifier>(key.GetModifiers()));

  if (event.key.character != 0)
  {
    try
    {
      bHandled = m_dllStruct->InputEvent(&event);
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "GAME: %s: exception caught in InputEvent()", m_gameClient->ID().c_str());
    }
  }

  return bHandled;
}

void CGameClientKeyboard::OnKeyRelease(const CKey& key)
{
  game_input_event event;

  event.type            = GAME_INPUT_EVENT_KEY;
  event.port            = GAME_INPUT_PORT_KEYBOARD;
  event.controller_id   = ""; //! @todo
  event.feature_name    = ""; //! @todo
  event.key.pressed     = false;
  event.key.character   = static_cast<XBMCVKey>(key.GetButtonCode() & BUTTON_INDEX_MASK);
  event.key.modifiers   = CGameClientTranslator::GetModifiers(static_cast<CKey::Modifier>(key.GetModifiers()));

  if (event.key.character != 0)
  {
    try
    {
      m_dllStruct->InputEvent(&event);
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "GAME: %s: exception caught in InputEvent()", m_gameClient->ID().c_str());
    }
  }
}
