/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlaylistDirectory.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "application/ApplicationPlayLists.h"
#include "playlists/PlayList.h"
#include "utils/ItemProperties.h"

#include <map>
#include <vector>

using namespace KODI;
using namespace XFILE;

CPlaylistDirectory::CPlaylistDirectory() = default;

CPlaylistDirectory::~CPlaylistDirectory() = default;

namespace
{
constexpr char MUSIC_PROTOCOL[] = "playlistmusic";
constexpr char VIDEO_PROTOCOL[] = "playlistvideo";
} // namespace

std::string CPlaylistDirectory::PathOf(PLAYLIST::Type type)
{
  return std::string{type == PLAYLIST::Audio ? MUSIC_PROTOCOL : VIDEO_PROTOCOL} + "://";
}

std::optional<PLAYLIST::Type> CPlaylistDirectory::TypeOf(const CURL& url)
{
  if (url.IsProtocol(MUSIC_PROTOCOL))
    return PLAYLIST::Audio;
  if (url.IsProtocol(VIDEO_PROTOCOL))
    return PLAYLIST::Video;
  return std::nullopt;
}

bool CPlaylistDirectory::GetDirectory(const CURL& url, CFileItemList& items)
{
  const std::optional<PLAYLIST::Type> type{TypeOf(url)};
  if (!type)
    return false;

  const PLAYLIST::CPlayList& playList = CServiceBroker::GetPlayLists()->GetPlayList(*type);
  const std::vector<PLAYLIST::PlayListEntry> entries = playList.GetEntries();
  std::map<PLAYLIST::EntryId, int> positions;
  for (int i = 0; i < static_cast<int>(entries.size()); ++i)
    positions.emplace(entries[i].id, i);
  items.Reserve(static_cast<int>(entries.size()));

  // Rows are listed in play order and carry the entry's position in the list, which is what acts
  // on it. Each row is a copy: what a window loads or formats onto its rows stays off the
  // playlist, and an item on the playlist twice lists twice.
  int displayOrder = 0;
  for (const PLAYLIST::EntryId id : playList.GetPlayOrder())
  {
    const auto position = positions.find(id);
    if (position == positions.end())
      continue;
    auto row = std::make_shared<CFileItem>(*entries[position->second].item);
    row->SetProperty(ITEM::PROPERTY::PLAYLIST_ENTRY, id);
    row->SetProperty(ITEM::PROPERTY::PLAYLIST_POSITION, position->second);
    row->SetProperty(ITEM::PROPERTY::PLAYLIST_DISPLAY_ORDER, displayOrder++);
    row->SetProperty("playlisttype", static_cast<int>(*type));
    items.Add(std::move(row));
  }

  return true;
}
