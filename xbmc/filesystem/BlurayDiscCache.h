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

#include <cstdint>
#include <map>
#include <optional>
#include <string>

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

  //! When this disc was last used, to decide which to drop when the cache is full. Mutable as
  //! recency is not part of what the cache holds, so reading a disc's information updates it too.
  mutable uint64_t lastUsed{0};
};

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

  //! Drop everything held for a disc, as its information no longer describes what is in the drive
  void ClearDisc(const std::string& path);

private:
  /*!
   \brief Find a disc, recording that it has been used. m_cs must be held.
   \return the disc, or nullptr if nothing is held for it
   */
  const Disc* Find(const std::string& path) const;

  /*!
   \brief Find a disc, adding it if nothing is held for it yet, and recording that it has been
   used. Adding may drop the least recently used disc. m_cs must be held.
   */
  Disc& FindOrCreate(const std::string& path);

  //! Drop the least recently used discs until the cache is within its limit. m_cs must be held.
  void Evict();

  CacheMap m_cache;

  //! Ticks on every use, to order the discs by how recently they were used
  mutable uint64_t m_useCounter{0};

  mutable CCriticalSection m_cs;
};
} // namespace XFILE
