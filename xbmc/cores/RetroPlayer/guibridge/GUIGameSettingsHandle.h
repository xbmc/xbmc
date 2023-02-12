/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI
{
namespace RETRO
{
class CGUIGameRenderManager;
class IPlayback;

class CGUIGameSettingsHandle
{
public:
  CGUIGameSettingsHandle(CGUIGameRenderManager& renderManager);
  virtual ~CGUIGameSettingsHandle();

  /*!
   * \brief Get the ID of the active game client
   *
   * \return The ID of the active game client, or empty string if a game is
   * not playing
   */
  std::string GameClientID();

  /*!
   * \brief Get the full path of the game being played
   *
   * \return The game's path, or empty string if a game is not playing
   */
  std::string GetPlayingGame();

  /*!
   * \brief Create a savestate of the current game being played
   *
   * \param autosave True if the save was invoked automatically, or false if
   * the save was invoked by a player
   *
   * \return The path to the created savestate file, or empty string on
   * failure or if a game is not playing
   */
  std::string CreateSavestate(bool autosave);

  /*!
   * \brief Update a savestate for the current game being played
   *
   * \param savestatePath The path to the created savestate file returned by
   * CreateSavestate()
   *
   * \return True if the savestate was updated successfully, false otherwise
   */
  bool UpdateSavestate(const std::string& savestatePath);

  /*!
   * \brief Load a savestate for the current game being played
   *
   * \param savestatePath The path to the created savestate file returned by
   * CreateSavestate()
   *
   * \return True if the savestate was loaded successfully, false otherwise
   */
  bool LoadSavestate(const std::string& savestatePath);

  /*!
   * \brief Clear the video frame stored for the given statestate
   *
   * Useful to reclaim memory if a savestate has been deleted.
   *
   * \param savestatePath The path to the savestate file
   */
  void FreeSavestateResources(const std::string& savestatePath);

  /*!
   * \brief Close the in-game OSD
   */
  void CloseOSD();

private:
  // Construction parameters
  CGUIGameRenderManager& m_renderManager;
};
} // namespace RETRO
} // namespace KODI
