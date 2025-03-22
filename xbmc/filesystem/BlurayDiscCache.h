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
  std::map<unsigned int, PlaylistInformation> m_playlists;
  std::map<unsigned int, unsigned int> m_titleMap;

  bool m_mapsSet{false};
  PlaylistMap m_playlistmap;
  ClipMap m_clipmap;
};

using CacheMapEntry = std::pair<std::string, Disc>;
using CacheMap = std::map<std::string, Disc>;

namespace XFILE
{
class CBlurayDiscCache
{
public:
  CBlurayDiscCache();
  virtual ~CBlurayDiscCache();
  void Clear();

  CacheMap::iterator SetDisc(const std::string& path);
  void SetPlaylistInfo(const std::string& path,
                       unsigned int playlist,
                       const PlaylistInformation& playlistInfo);
  void SetMaps(const std::string& path, const PlaylistMap& playlistmap, const ClipMap& clipmap);

  bool GetPlaylistInfo(const std::string& path,
                       unsigned int playlist,
                       PlaylistInformation& playlistInfo);
  bool GetMaps(const std::string& path, PlaylistMap& playlistmap, ClipMap& clipmap);

  void ClearDisc(const std::string& path);

protected:
  CacheMap m_cache;

  mutable CCriticalSection m_cs;
};
} // namespace XFILE