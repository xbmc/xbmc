/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RConsoleIDs.h"

#include <cstdint>
#include <string>

namespace KODI
{
namespace GAME
{
class CGameClient;
}

namespace RETRO
{
class CCheevos
{
public:
  CCheevos(GAME::CGameClient* gameClient,
           const std::string& userName,
           const std::string& loginToken);
  void ResetRuntime();
  void EnableRichPresence();
  std::string GetRichPresenceEvaluation();

private:
  bool LoadData();
  RConsoleID ConsoleID();

  GAME::CGameClient* const m_gameClient;
  std::string m_userName;
  std::string m_loginToken;
  std::string m_romHash;
  std::string m_richPresenceScript;
  uint32_t m_gameID{};
  RConsoleID m_consoleID = RConsoleID::RC_INVALID_ID;
  bool m_richPresenceLoaded{};
};
} // namespace RETRO
} // namespace KODI
