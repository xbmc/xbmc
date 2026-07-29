/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "MPLSParser.h"

#include <algorithm>
#include <chrono>
#include <ranges>
#include <string>
#include <vector>

class CFileItem;
class CFileItemList;

namespace XFILE
{
using namespace std::chrono_literals;

struct BlurayPlaylistInformation
{
  unsigned int playlist{0};
  std::string version;
  std::chrono::milliseconds duration{0ms};
  BLURAY_PLAYBACK_TYPE playbackType{0};
  unsigned int playbackCount{0};
  std::vector<PlayItemInformation> playItems;
  std::vector<ClipInformation> clips;
  std::vector<SubPlayItemInformation> subPlayItems;
  std::vector<SubPlayItemInformation> extensionSubPlayItems;
  std::vector<PlaylistMarkInformation> playlistMarks;
  std::vector<ChapterInformation> chapters;
};

// The first angle clip of the longest play item, or nullptr if the playlist has no play items or
// the longest play item has no clips.
// A playlist may contain several clips with differing streams, so both the M2TS analysis and the
// stream information taken from the CLPI must use this same clip for the packet identifiers (and
// hence the stream details) to correspond.
inline const ClipInformation* GetLongestPlayItemClip(const BlurayPlaylistInformation& playlist)
{
  const auto it{std::ranges::max_element(playlist.playItems, {},
                                         [](const PlayItemInformation& playItem)
                                         { return playItem.outTime - playItem.inTime; })};
  if (it == playlist.playItems.end() || it->angleClips.empty())
    return nullptr;

  return &it->angleClips.front();
}
} // namespace XFILE
