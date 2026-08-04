/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BlurayDiscCache.h"

#include "URL.h"
#include "bluray/PlaylistStructure.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <ranges>

using namespace XFILE;

namespace
{
/*!
 \brief Number of discs to keep information for.
 Holds - a playlist for every title on the disc. Each playlist entry holds information for each clip.
       - a FileItem for most playlists
 The total cache size for each disc could be 2-3MB
 Only the disc in use is normally wanted; the allowance is for going back and forth between the
 discs of a set.
 */
constexpr size_t MAX_CACHED_DISCS{10};
static_assert(MAX_CACHED_DISCS >= 1);

//! Discs are keyed on their path without URL options or a trailing slash, else the compare may be
//! wrong
std::string GetDiscKey(const std::string& path)
{
  std::string key{CURL(path).GetWithoutOptions()};
  URIUtils::RemoveSlashAtEnd(key);
  return key;
}
} // namespace

const Disc* CBlurayDiscCache::Find(const std::string& path) const
{
  const auto it{m_cache.find(GetDiscKey(path))};
  if (it == m_cache.end())
    return nullptr;

  it->second.lastUsed = ++m_useCounter;

  return &it->second;
}

Disc& CBlurayDiscCache::FindOrCreate(const std::string& path)
{
  const auto [it, inserted]{m_cache.try_emplace(GetDiscKey(path))};
  it->second.lastUsed = ++m_useCounter;

  // The disc just used is the most recent, so is never the one dropped
  if (inserted)
    Evict();

  return it->second;
}

void CBlurayDiscCache::Evict()
{
  while (m_cache.size() > MAX_CACHED_DISCS)
  {
    const auto oldest{std::ranges::min_element(m_cache, {}, [](const CacheMap::value_type& entry)
                                               { return entry.second.lastUsed; })};

    CLog::LogF(LOGDEBUG, "Dropping cached information for {}", CURL::GetRedacted(oldest->first));
    m_cache.erase(oldest);
  }
}

void CBlurayDiscCache::SetPlaylistInfo(const std::string& path,
                                       unsigned int playlist,
                                       const BlurayPlaylistInformation& playlistInfo)
{
  std::unique_lock lock(m_cs);

  FindOrCreate(path).playlists[playlist] = playlistInfo;
}

void CBlurayDiscCache::SetMaps(const std::string& path,
                               const PlaylistMap& playlistmap,
                               const ClipMap& clipmap,
                               const CFileItemList& itemmap)
{
  std::unique_lock lock(m_cs);

  Disc& disc{FindOrCreate(path)};
  disc.playlistMap = playlistmap;
  disc.clipMap = clipmap;
  disc.itemMap.Copy(itemmap);
  disc.mapsSet = true;
}

void CBlurayDiscCache::SetPlaylistStreamInfo(const std::string& path,
                                             unsigned int playlist,
                                             const StreamMap& streams)
{
  std::unique_lock lock(m_cs);

  FindOrCreate(path).streamMap[playlist] = streams;
}

void CBlurayDiscCache::SetMenuSupport(const std::string& path, bool menuSupport)
{
  std::unique_lock lock(m_cs);

  FindOrCreate(path).menuSupport = menuSupport;
}

void CBlurayDiscCache::SetMainPlaylist(const std::string& path, int mainPlaylist)
{
  std::unique_lock lock(m_cs);

  FindOrCreate(path).mainPlaylist = mainPlaylist;
}

bool CBlurayDiscCache::GetPlaylistInfo(const std::string& path,
                                       unsigned int playlist,
                                       BlurayPlaylistInformation& playlistInfo) const
{
  std::unique_lock lock(m_cs);

  if (const Disc * disc{Find(path)}; disc)
  {
    if (const auto& it{disc->playlists.find(playlist)}; it != disc->playlists.end())
    {
      playlistInfo = it->second;
      return true;
    }
  }
  return false;
}

bool CBlurayDiscCache::GetMaps(const std::string& path,
                               PlaylistMap& playlistmap,
                               ClipMap& clipmap,
                               CFileItemList& itemmap) const
{
  std::unique_lock lock(m_cs);

  if (const Disc * disc{Find(path)}; disc && disc->mapsSet)
  {
    clipmap = disc->clipMap;
    playlistmap = disc->playlistMap;
    itemmap.Copy(disc->itemMap);
    return true;
  }
  return false;
}

bool CBlurayDiscCache::GetPlaylistStreamInfo(const std::string& path,
                                             unsigned int playlist,
                                             StreamMap& streams) const
{
  std::unique_lock lock(m_cs);

  if (const Disc * disc{Find(path)}; disc)
  {
    if (const auto& it{disc->streamMap.find(playlist)}; it != disc->streamMap.end())
    {
      streams = it->second;
      return true;
    }
  }
  return false;
}

bool CBlurayDiscCache::GetMenuSupport(const std::string& path, bool& menuSupport) const
{
  std::unique_lock lock(m_cs);

  if (const Disc * disc{Find(path)}; disc && disc->menuSupport)
  {
    menuSupport = *disc->menuSupport;
    return true;
  }
  return false;
}

bool CBlurayDiscCache::GetMainPlaylist(const std::string& path, int& mainPlaylist) const
{
  std::unique_lock lock(m_cs);

  if (const Disc * disc{Find(path)}; disc && disc->mainPlaylist)
  {
    mainPlaylist = *disc->mainPlaylist;
    return true;
  }
  return false;
}

void CBlurayDiscCache::ClearDisc(const std::string& path)
{
  std::unique_lock lock(m_cs);

  m_cache.erase(GetDiscKey(path));
}

void CBlurayDiscCache::Clear()
{
  std::unique_lock lock(m_cs);

  m_cache.clear();
}
