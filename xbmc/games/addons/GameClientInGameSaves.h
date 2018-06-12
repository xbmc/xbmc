/*
 *      Copyright (C) 2016-2017 Team Kodi
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

#include "addons/kodi-addon-dev-kit/include/kodi/kodi_game_types.h"

#include <string>

struct GameClient;

namespace KODI
{
namespace GAME
{
  class CGameClient;

  /*!
   * \brief This class implements in-game saves.
   *
   * \details Some games do not implement state persistence on their own, but rely on the frontend for saving their current
   * memory state to disk. This is mostly the case for emulators for SRAM (battery backed up ram on cartridges) or
   * memory cards.
   *
   * Differences to save states:
   * - Works only for supported games (e.g. emulated games with SRAM support)
   * - Often works emulator independent (and can be used to start a game with one emulator and continue with another)
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
    CGameClientInGameSaves(CGameClient* addon, const KodiToAddonFuncTable_Game* dllStruct);

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
    const KodiToAddonFuncTable_Game* const m_dllStruct;
  };
}
}
