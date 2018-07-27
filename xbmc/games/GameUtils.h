/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GameTypes.h"
#include "addons/Addon.h"
#include "addons/IAddon.h"

#include <set>
#include <string>

class CFileItem;
class CURL;

namespace KODI
{
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
     * \brief Set the game client property, possibly via prompt, for the given item
     *
     * \param item The item with or without a game client in its info tag
     * \param prompt If true and no game client was resolved, prompt the user for one
     *
     * \return True if the item has a valid game client ID in its info tag
     */
    static bool FillInGameClient(CFileItem &item, bool bPrompt);

    /*!
     * \brief Check if the file extension is supported by an add-on in
     *        a local or remote repository
     *
     * \param path The path of the game file
     *
     * \return true if the path's extension is supported by a known game client
     */
    static bool HasGameExtension(const std::string& path);

    /*!
     * \brief Get all game extensions
     */
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
}
