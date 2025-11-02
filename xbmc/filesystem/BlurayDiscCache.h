/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DiscDirectoryHelper.h"
#include "bluray/M2TSParser.h"
#include "bluray/PlaylistStructure.h"
#include "threads/CriticalSection.h"

#include <map>

struct Disc
{
  std::map<unsigned int, XFILE::BlurayPlaylistInformation, std::less<>> playlists;
  std::map<unsigned int, unsigned int, std::less<>> titleMap;

  bool mapsSet{false};
  XFILE::PlaylistMap playlistMap;
  XFILE::ClipMap clipMap;

  std::map<unsigned int, XFILE::StreamMap, std::less<>> streamMap;
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
                       const BlurayPlaylistInformation& playlistInfo);
  void SetMaps(const std::string& path, const PlaylistMap& playlistmap, const ClipMap& clipmap);
  void SetPlaylistStreamInfo(const std::string& path,
                             unsigned int playlist,
                             const StreamMap& streams);

  bool GetPlaylistInfo(const std::string& path,
                       unsigned int playlist,
                       BlurayPlaylistInformation& playlistInfo) const;
  bool GetMaps(const std::string& path, PlaylistMap& playlistmap, ClipMap& clipmap) const;
  bool GetPlaylistStreamInfo(const std::string& path,
                             unsigned int playlist,
                             StreamMap& streams) const;

  void ClearDisc(const std::string& path);

private:
  CacheMap m_cache;

  mutable CCriticalSection m_cs;
};
} // namespace XFILE
