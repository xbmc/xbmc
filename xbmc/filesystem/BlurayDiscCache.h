/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DiscDirectoryHelper.h"
#include "threads/CriticalSection.h"

#include <map>

#include <libbluray/bluray.h>

struct Disc
{
  std::map<unsigned int, BLURAY_TITLE_INFO> m_playlists;
  std::map<unsigned int, unsigned int> m_titleMap;

  bool m_mapsSet{false};
  XFILE::PlaylistMap m_playlistmap;
  XFILE::ClipMap m_clipmap;
};

using CacheMapEntry = std::pair<std::string, Disc>;
using CacheMap = std::map<std::string, Disc>;

namespace XFILE
{
class CBlurayDiscCache
{
public:
  CBlurayDiscCache() = default;
  ~CBlurayDiscCache() = default;
  void Clear();

  CacheMap::iterator SetDisc(const std::string& path);
  void SetTitleInfo(const std::string& path,
                    unsigned int title,
                    const BLURAY_TITLE_INFO* playlistInfo);
  void SetPlaylistInfo(const std::string& path,
                       unsigned int playlist,
                       const BLURAY_TITLE_INFO* playlistInfo);
  void SetMaps(const std::string& path, const PlaylistMap& playlistmap, const ClipMap& clipmap);

  bool GetPlaylistInfo(const std::string& path,
                       unsigned int playlist,
                       BLURAY_TITLE_INFO& playlistInfo) const;
  bool GetTitleInfo(const std::string& path,
                    unsigned int title,
                    BLURAY_TITLE_INFO& playlistInfo) const;
  bool GetMaps(const std::string& path, PlaylistMap& playlistmap, ClipMap& clipmap) const;

  void ClearDisc(const std::string& path);

protected:
  CacheMap m_cache;

  mutable CCriticalSection m_cs;
};
} // namespace XFILE
