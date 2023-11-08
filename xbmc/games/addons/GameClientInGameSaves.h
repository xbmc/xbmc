/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/addon-instance/Game.h"

#include <string>

struct GameClient;

namespace KODI
{
namespace GAME
{
class CGameClient;

/*!
 * \ingroup games
 *
 * \brief This class implements in-game saves.
 *
 * \details Some games do not implement state persistence on their own, but rely on the frontend for
 * saving their current memory state to disk. This is mostly the case for emulators for SRAM
 * (battery backed up ram on cartridges) or memory cards.
 *
 * Differences to save states:
 * - Works only for supported games (e.g. emulated games with SRAM support)
 * - Often works emulator independent (and can be used to start a game with one emulator and
 * continue with another)
 * - Visible in-game (e.g. in-game save game selection menus)
 */
class CGameClientInGameSaves
{
public:
  /*!
   * \brief Constructor.
   * \param addon The game client implementation.
   * \param dllStruct The emulator or game for which the in-game saves are processed.
   */
  CGameClientInGameSaves(CGameClient* addon, const AddonInstance_Game* dllStruct);

  /*!
   * \brief Load in-game data.
   */
  void Load();

  /*!
   * \brief Save in-game data.
   */
  void Save();

private:
  std::string GetPath(GAME_MEMORY memoryType);

  void Load(GAME_MEMORY memoryType);
  void Save(GAME_MEMORY memoryType);

  const CGameClient* const m_gameClient;
  const AddonInstance_Game* const m_dllStruct;
};
} // namespace GAME
} // namespace KODI
