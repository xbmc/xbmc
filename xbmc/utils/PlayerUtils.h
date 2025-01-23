/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

class CApplicationPlayer;
class CFileItem;

namespace KODI::PLAYLIST
{
enum class Id;

} // namespace KODI::PLAYLIST

enum class TempoStepChange
{
  INCREASE,
  DECREASE,
};

class CPlayerUtils
{
public:
  static bool IsItemPlayable(const CFileItem& item);
  static void AdvanceTempoStep(const std::shared_ptr<CApplicationPlayer>& appPlayer,
                               TempoStepChange change);

  /*!
   \brief Get the players available for the given file item.
   \param item The item
   \return the players
   */
  static std::vector<std::string> GetPlayersForItem(const CFileItem& item);

  /*!
   \brief Check whether multiple players are available for the given item.
   \param item The item
   \return True if multiple players are available, false otherwise
   */
  static bool HasItemMultiplePlayers(const CFileItem& item);

  /*!
   \brief Play the media represented by the given item.
   \param item The item
   \param player The player to use or empty string for default player
   \param playlistId The id of the playlist to add the item to.
   \return True on success, false otherwise
   */
  static bool PlayMedia(const std::shared_ptr<CFileItem>& item,
                        const std::string& player,
                        KODI::PLAYLIST::Id playlistId);
};
