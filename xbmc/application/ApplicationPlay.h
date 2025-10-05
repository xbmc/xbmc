/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItem.h"
#include "cores/IPlayer.h"

#include <string>

class CApplicationStackHelper;

/*!
 * \brief Helper class to gather all playback details for a file item.
 * Usage: Construct with stack helper → call GatherPlaybackDetails() → retrieve results via getters
 */
class CApplicationPlay
{
public:
  explicit CApplicationPlay(CApplicationStackHelper& stackHelper) : m_stackHelper(stackHelper) {}

  enum class GatherPlaybackDetailsResult
  {
    RESULT_SUCCESS, // all details gathered
    RESULT_ERROR, // not all details gathered
    RESULT_NO_PLAYLIST_SELECTED, // playlist required, but none selected
  };

  /*!
   * \brief Gathers all details needed to play the given item.
   * This includes resolving the item to get the item that actually shall be played, determining
   * the player to be used and the player options.
   * \param item The file item to play
   * \param player The default player to be used for playback
   * \param restart A flag indicating whether file shall be played from the beginning, ignoring
   * possibly existing resume points.
   * \return the result
   */
  GatherPlaybackDetailsResult GatherPlaybackDetails(const CFileItem& item,
                                                    std::string player,
                                                    bool restart);

  /*!
   * \brief Get the resolved item, that is to be used for playback.
   * \return the item.
   */
  const CFileItem& GetResolvedItem() const { return m_item; }

  /*!
   * \brief Get the player for playback of the resolved item.
   * \return the player.
   */
  const std::string& GetResolvedPlayer() const { return m_player; }

  /*!
   * \brief Get the player options for playback of the resolved item.
   * \return the options.
   */
  const CPlayerOptions& GetPlayerOptions() const { return m_options; }

private:
  CApplicationPlay() = delete;

  /*!
   * \brief Resolves m_item's vfs dynpath to an actual file path.
   * \return true if resolved successfully, false otherwise
   */
  bool ResolvePath();

  /*!
   * \brief Extracts a specific playable part from m_item's path, if m_item has a stack:// path.
   * \return true if resolved successfully, false otherwise
   */
  bool ResolveStack();

  /*!
   * \brief Determines if there is a resume point for m_item, updates the player options accordingly.
   * Also resolves a removable media path if needed
   * \param restart Set to true if playback should restart from beginning
   */
  void GetOptionsAndUpdateItem(bool restart);

  /*!
   * \brief If m_item is a bluray that has not been played before and simple menu is enabled, then
   * prompt user to select playlist.
   * \return false if user cancels playlist selection, true otherwise
   */
  bool GetPlaylistIfDisc();

  /*!
   * \brief Determine if playback should go fullscreen based on media type and settings
   */
  void DetermineFullScreen();

  CApplicationStackHelper& m_stackHelper;
  CFileItem m_item;
  std::string m_player;
  CPlayerOptions m_options;
};
