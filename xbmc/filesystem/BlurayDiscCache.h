/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "BlurayDirectory.h"
#include "DiscDirectoryHelper.h"
#include "threads/CriticalSection.h"

#include <map>

struct Disc
{
  std::map<unsigned int, XFILE::PlaylistInformation> m_playlists;
  std::map<unsigned int, unsigned int> m_titleMap;

  bool m_mapsSet{false};
  XFILE::PlaylistMap m_playlistmap;
  XFILE::ClipMap m_clipmap;
};

using CacheMapEntry = std::pair<std::string, Disc>;
using CacheMap = std::map<std::string, Disc, std::less<>>;

namespace XFILE
{
class CBlurayDiscCache
{
public:
  CBlurayDiscCache() = default;
  ~CBlurayDiscCache() = default;
  CBlurayDiscCache(const CBlurayDiscCache&) = delete;
  CBlurayDiscCache& operator=(const CBlurayDiscCache&) = delete;
  void Clear();

  CacheMap::iterator SetDisc(const std::string& path);
  void SetPlaylistInfo(const std::string& path,
                       unsigned int playlist,
                       const PlaylistInformation& playlistInfo);
  void SetMaps(const std::string& path, const PlaylistMap& playlistmap, const ClipMap& clipmap);

  bool GetPlaylistInfo(const std::string& path,
                       unsigned int playlist,
                       PlaylistInformation& playlistInfo) const;
  bool GetMaps(const std::string& path, PlaylistMap& playlistmap, ClipMap& clipmap) const;

  void ClearDisc(const std::string& path);

private:
  CacheMap m_cache;

  mutable CCriticalSection m_cs;
};
} // namespace XFILE
