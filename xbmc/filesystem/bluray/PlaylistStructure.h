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

  //! Whether the clips carry the stream information from their .clpi (see CMPLSParser::ReadMPLS)
  bool clipStreamsRead{false};
};

// The longest play item of the playlist, or nullptr if the playlist has no play items.
// A playlist may contain several clips with differing streams, so the M2TS analysis and the stream
// information - whether taken from the CLPI or from the play item's stream number table - must all
// describe this same play item for the packet identifiers (and hence the stream details) to
// correspond.
inline const PlayItemInformation* GetLongestPlayItem(const BlurayPlaylistInformation& playlist)
{
  const auto it{std::ranges::max_element(playlist.playItems, {},
                                         [](const PlayItemInformation& playItem)
                                         { return playItem.outTime - playItem.inTime; })};
  if (it == playlist.playItems.end())
    return nullptr;

  return &*it;
}

// The first angle clip of the longest play item, or nullptr if the playlist has no play items or
// the longest play item has no clips.
inline const ClipInformation* GetLongestPlayItemClip(const BlurayPlaylistInformation& playlist)
{
  const PlayItemInformation* playItem{GetLongestPlayItem(playlist)};
  if (!playItem || playItem->angleClips.empty())
    return nullptr;

  return &playItem->angleClips.front();
}
} // namespace XFILE
