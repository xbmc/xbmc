/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"

#include <span>
#include <string>
#include <vector>

static constexpr unsigned int MIN_EPISODE_DURATION = 10 * 60; // 10 minutes
static constexpr unsigned int MAX_EPISODE_DIFFERENCE = 30; // 30 seconds
static constexpr unsigned int MIN_SPECIAL_DURATION = 5 * 60; // 5 minutes
static constexpr unsigned int MAIN_TITLE_LENGTH_PERCENT = 70;

class CFileItem;
class CFileItemList;
class CURL;
class CVideoInfoTag;

class CDiscDirectoryHelper
{
public:
  static void GetEpisodeTitles(CURL url,
                               const CFileItem& episode,
                               CFileItemList& items,
                               std::vector<CVideoInfoTag> episodesOnDisc,
                               const ClipMap& clips,
                               const PlaylistMap& playlists);
  static void GenerateItem(const CURL& url,
                           const std::shared_ptr<CFileItem>& item,
                           unsigned int playlist,
                           const PlaylistMap& playlists,
                           const CVideoInfoTag& tag,
                           bool isSpecial = false);
  static void AddRootOptions(CURL url, CFileItemList& items, bool addMenuOption);

  static std::string HexToString(std::span<const uint8_t> buf, int count);

  static std::string GetEpisodesLabel(CFileItem& newItem, const CFileItem& item);
};