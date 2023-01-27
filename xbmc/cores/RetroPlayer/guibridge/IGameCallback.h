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
class IPlayback;

class IGameCallback
{
public:
  virtual ~IGameCallback() = default;

  /*!
   * \brief Get the game client being used to play the game
   *
   * \return The game client's ID, or empty if no game is being played
   */
  virtual std::string GameClientID() const = 0;

  /*!
   * \brief Get the game that is being played
   *
   * \return The path to the game, or empty if no game is being played
   */
  virtual std::string GetPlayingGame() const = 0;

  /*!
   * \brief Creates a savestate
   *
   * \param autosave Whether the save type is auto
   *
   * \return The path to the created savestate, or empty on error
   */
  virtual std::string CreateSavestate(bool autosave) = 0;

  /*!
   * \brief Updates a savestate with the current game being played
   *
   * \param savestatePath The path to the savestate
   *
   * \return True if the savestate was updated, false on error
   */
  virtual bool UpdateSavestate(const std::string& savestatePath) = 0;

  /*!
   * \brief Loads a savestate
   *
   * \param savestatePath The path to the savestate
   *
   * \return True if the savestate was loaded, false on error
   */
  virtual bool LoadSavestate(const std::string& savestatePath) = 0;

  /*!
   * \brief Frees resources allocated to the savestate, such as its video thumbnail
   *
   * \param savestatePath The path to the savestate
   */
  virtual void FreeSavestateResources(const std::string& savestatePath) = 0;

  /*!
   * \brief Closes the OSD
   */
  virtual void CloseOSDCallback() = 0;
};
} // namespace RETRO
} // namespace KODI
