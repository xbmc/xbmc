/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DiscDirectoryHelper.h"
#include "FileItemList.h"
#include "bluray/M2TSParser.h"
#include "bluray/PlaylistStructure.h"
#include "threads/CriticalSection.h"

#include <map>
#include <optional>

struct Disc
{
  std::map<unsigned int, XFILE::BlurayPlaylistInformation, std::less<>> playlists;
  std::map<unsigned int, unsigned int, std::less<>> titleMap;

  bool mapsSet{false};
  XFILE::PlaylistMap playlistMap;
  XFILE::ClipMap clipMap;
  CFileItemList itemMap;

  std::map<unsigned int, XFILE::StreamMap, std::less<>> streamMap;

  std::optional<bool> menuSupport;
  std::optional<int> mainPlaylist;
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
  void SetMaps(const std::string& path,
               const PlaylistMap& playlistmap,
               const ClipMap& clipmap,
               const CFileItemList& itemmap);
  void SetPlaylistStreamInfo(const std::string& path,
                             unsigned int playlist,
                             const StreamMap& streams);
  void SetMenuSupport(const std::string& path, bool menuSupport);
  void SetMainPlaylist(const std::string& path, int mainPlaylist);

  bool GetPlaylistInfo(const std::string& path,
                       unsigned int playlist,
                       BlurayPlaylistInformation& playlistInfo) const;
  bool GetMaps(const std::string& path,
               PlaylistMap& playlistmap,
               ClipMap& clipmap,
               CFileItemList& itemmap) const;
  bool GetPlaylistStreamInfo(const std::string& path,
                             unsigned int playlist,
                             StreamMap& streams) const;

  /*!
   \brief Get whether this disc supports menus, if already determined.
   \param[out] menuSupport set only when true is returned
   \return true if the disc has been probed for menu support before
   */
  bool GetMenuSupport(const std::string& path, bool& menuSupport) const;

  /*!
   \brief Get the main playlist named in the disc's disc.inf, if already looked for.
   \param[out] mainPlaylist set only when true is returned, -1 when the disc names no playlist
   \return true if the disc has been examined for a main playlist before
   */
  bool GetMainPlaylist(const std::string& path, int& mainPlaylist) const;

  void ClearDisc(const std::string& path);

private:
  CacheMap m_cache;

  mutable CCriticalSection m_cs;
};
} // namespace XFILE
