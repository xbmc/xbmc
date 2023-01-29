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
#include <map>
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

  const std::map<std::string, RConsoleID> m_extensionToConsole = {
      {".a26", RConsoleID::RC_CONSOLE_ATARI_2600},
      {".a78", RConsoleID::RC_CONSOLE_ATARI_7800},
      {".agb", RConsoleID::RC_CONSOLE_GAMEBOY_ADVANCE},
      {".cdi", RConsoleID::RC_CONSOLE_DREAMCAST},
      {".cdt", RConsoleID::RC_CONSOLE_AMSTRAD_PC},
      {".cgb", RConsoleID::RC_CONSOLE_GAMEBOY_COLOR},
      {".chd", RConsoleID::RC_CONSOLE_DREAMCAST},
      {".cpr", RConsoleID::RC_CONSOLE_AMSTRAD_PC},
      {".d64", RConsoleID::RC_CONSOLE_COMMODORE_64},
      {".gb", RConsoleID::RC_CONSOLE_GAMEBOY},
      {".gba", RConsoleID::RC_CONSOLE_GAMEBOY_ADVANCE},
      {".gbc", RConsoleID::RC_CONSOLE_GAMEBOY_COLOR},
      {".gdi", RConsoleID::RC_CONSOLE_DREAMCAST},
      {".j64", RConsoleID::RC_CONSOLE_ATARI_JAGUAR},
      {".jag", RConsoleID::RC_CONSOLE_ATARI_JAGUAR},
      {".lnx", RConsoleID::RC_CONSOLE_ATARI_LYNX},
      {".mds", RConsoleID::RC_CONSOLE_SATURN},
      {".min", RConsoleID::RC_CONSOLE_POKEMON_MINI},
      {".mx1", RConsoleID::RC_CONSOLE_MSX},
      {".mx2", RConsoleID::RC_CONSOLE_MSX},
      {".n64", RConsoleID::RC_CONSOLE_NINTENDO_64},
      {".ndd", RConsoleID::RC_CONSOLE_NINTENDO_64},
      {".nds", RConsoleID::RC_CONSOLE_NINTENDO_DS},
      {".nes", RConsoleID::RC_CONSOLE_NINTENDO},
      {".o", RConsoleID::RC_CONSOLE_ATARI_LYNX},
      {".pce", RConsoleID::RC_CONSOLE_PC_ENGINE},
      {".sfc", RConsoleID::RC_CONSOLE_SUPER_NINTENDO},
      {".sgx", RConsoleID::RC_CONSOLE_PC_ENGINE},
      {".smc", RConsoleID::RC_CONSOLE_SUPER_NINTENDO},
      {".sna", RConsoleID::RC_CONSOLE_AMSTRAD_PC},
      {".tap", RConsoleID::RC_CONSOLE_AMSTRAD_PC},
      {".u1", RConsoleID::RC_CONSOLE_NINTENDO_64},
      {".v64", RConsoleID::RC_CONSOLE_NINTENDO_64},
      {".vb", RConsoleID::RC_CONSOLE_VIRTUAL_BOY},
      {".vboy", RConsoleID::RC_CONSOLE_VIRTUAL_BOY},
      {".vec", RConsoleID::RC_CONSOLE_VECTREX},
      {".voc", RConsoleID::RC_CONSOLE_AMSTRAD_PC},
      {".z64", RConsoleID::RC_CONSOLE_NINTENDO_64}};
};
} // namespace RETRO
} // namespace KODI
