/*
 *      Copyright (C) 2012-2017 Team Kodi
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

#include "GameTypes.h"
#include "addons/Addon.h"
#include "addons/IAddon.h"

#include <set>
#include <string>

class CFileItem;
class CURL;

namespace GAME
{
  /*!
   * \ingroup games
   * \brief Game related utilities.
   */
  class CGameUtils
  {
  public:
    /*!
     * \brief Select a game client, possibly via prompt, for the given game
     *
     * \param file The game being played
     *
     * \return A game client ready to be initialized for playback
     */
    static GameClientPtr OpenGameClient(const CFileItem& file);

    /*!
     * \brief Check if the file extension is supported by an add-on in
     *        a local or remote repository
     *
     * \param path The path of the game file
     *
     * \return true if the path's extension is supported by a known game client
     */
    static bool HasGameExtension(const std::string& path);

    static std::set<std::string> GetGameExtensions();

    /*!
     * \brief Check if game script or game add-on can be launched directly
     *
     * \return true if the add-on can be launched, false otherwise
     */
    static bool IsStandaloneGame(const ADDON::AddonPtr& addon);

  private:
    static void GetGameClients(const CFileItem& file, GameClientVector& candidates, GameClientVector& installable, bool& bHasVfsGameClient);
    static void GetGameClients(const ADDON::VECADDONS& addons, const CURL& translatedUrl, GameClientVector& candidates, bool& bHasVfsGameClient);
  };
} // namespace GAME
