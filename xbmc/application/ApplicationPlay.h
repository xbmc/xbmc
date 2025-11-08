/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "application/IApplicationComponent.h"

#include <cstdint>
#include <memory>
#include <string>

class CServiceManager;
class CPlayerOptions;
class CVideoDatabase;
class CApplicationStackHelper;
class CFileItem;

enum class PlayMediaType : uint8_t
{
  MUSIC_PLAYLIST,
  VIDEO,
  VIDEO_PLAYLIST
};

class CApplicationPlay : public IApplicationComponent
{
public:
  /*!
   * \brief Resolves a vfs dynpath to an actual file path
   * \param item The CFileItem to resolve
   * \return true if resolved successfully, false otherwise
   */
  static bool ResolvePath(CFileItem& item);

  /*!
   * \brief Extracts a specific playable part from a stack://
   * \param item The CFileItem for the stack
   * \param player The player that to be used for playback (reset if stack resolved)
   * \param stackHelper The initialised application stack helper
   * \param bRestart Set to true if playback should restart from beginning
   * \return true if resolved successfully, false otherwise
   */
  static bool ResolveStack(CFileItem& item,
                           std::string& player,
                           const std::shared_ptr<CApplicationStackHelper>& stackHelper,
                           bool& bRestart);

  /*!
   * \brief Determines if there is a resume point for the item and updates the player options accordingly
   * Also resolves a removable media path if needed
   * \param item The CFileItem
   * \param options The player options to update
   * \param stackHelper The initialised application stack helper
   * \param bRestart Set to true if playback should restart from beginning
   */
  static void GetOptionsAndUpdateItem(CFileItem& item,
                                      CPlayerOptions& options,
                                      const std::shared_ptr<CApplicationStackHelper>& stackHelper,
                                      bool bRestart);

  /*!
   * \brief If the item is a bluray that has not been played before and simple menu is enabled,
   * then prompt user to select playlist
   * \param item The CFileItem for disc
   * \param options The player options to update
   * \param player The player that to be used for playback (reset if stack resolved)
   * \param serviceManager The initialised service manager
   * \return false if user cancels playlist selection, true otherwise
   */
  static bool GetPlaylistIfDisc(CFileItem& item,
                                CPlayerOptions& options,
                                const std::string& player,
                                const std::unique_ptr<CServiceManager>& serviceManager);

  /*!
   * \brief Determine if playback should go fullscreen based on media type and settings
   * \param item The CFileItem
   * \param options The player options to update with the outcome (options.fullscreen)
   * \param stackHelper The initialised application stack helper
   */
  static void DetermineFullScreen(const CFileItem& item,
                                  CPlayerOptions& options,
                                  const std::shared_ptr<CApplicationStackHelper>& stackHelper);
};
