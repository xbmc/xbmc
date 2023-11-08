/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GameTypes.h"

#include <set>
#include <string>

class CFileItem;
class CURL;

namespace ADDON
{
class IAddon;
using AddonPtr = std::shared_ptr<IAddon>;
using VECADDONS = std::vector<AddonPtr>;
} // namespace ADDON

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 *
 * \brief Game related utilities.
 */
class CGameUtils
{
public:
  /*!
   * \brief Set the game client property, prompt the user for a savestate if there are any
   * (savestates store the information of which game client created it).
   * If there are no savestates or the user wants a new savestate, prompt the user
   * for a game client.
   *
   * \param item The item with or without a game client in its info tag
   * \param savestatePath Output. The path to the savestate selected. Empty if new savestate was
   * selected
   *
   * \return True if the item has a valid game client ID in its info tag
   */
  static bool FillInGameClient(CFileItem& item, std::string& savestatePath);

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
  static void GetGameClients(const CFileItem& file,
                             GameClientVector& candidates,
                             GameClientVector& installable,
                             bool& bHasVfsGameClient);
  static void GetGameClients(const ADDON::VECADDONS& addons,
                             const CURL& translatedUrl,
                             GameClientVector& candidates,
                             bool& bHasVfsGameClient);

  /*!
   * \brief Install the specified game client
   *
   * If the game client is not installed, a model dialog is shown installing
   * the game client. If the installation fails, an error dialog is shown.
   *
   * \param gameClient The game client to install
   *
   * \return True if the game client is installed, false otherwise
   */
  static bool Install(const std::string& gameClient);

  /*!
   * \brief Enable the specified game client
   *
   * \param gameClient the game client to enable
   *
   * \return True if the game client is enabled, false otherwise
   */
  static bool Enable(const std::string& gameClient);
};
} // namespace GAME
} // namespace KODI
