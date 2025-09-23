/*
 *  Copyright (C) 2005-2025 Team Kodi
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

using namespace XFILE;

CacheMap::iterator CBlurayDiscCache::SetDisc(const std::string& path)
{
  std::unique_lock lock(m_cs);

  // Get rid of any URL options, else the compare may be wrong
  std::string storedPath{CURL(path).GetWithoutOptions()};
  URIUtils::RemoveSlashAtEnd(storedPath);

  const auto [it, _]{m_cache.try_emplace(storedPath)};
  return it;
}

void CBlurayDiscCache::SetPlaylistInfo(const std::string& path,
                                       unsigned int playlist,
                                       const BlurayPlaylistInformation& playlistInfo)
{
  std::unique_lock lock(m_cs);

  // Get rid of any URL options, else the compare may be wrong
  std::string storedPath{CURL(path).GetWithoutOptions()};
  URIUtils::RemoveSlashAtEnd(storedPath);

  auto i{m_cache.find(storedPath)};
  if (i == m_cache.end())
    i = SetDisc(path);
  auto& [_, disc] = *i;
  disc.playlists[playlist] = playlistInfo;
}

void CBlurayDiscCache::SetMaps(const std::string& path,
                               const PlaylistMap& playlistmap,
                               const ClipMap& clipmap)
{
  std::unique_lock lock(m_cs);

  // Get rid of any URL options, else the compare may be wrong
  std::string storedPath{CURL(path).GetWithoutOptions()};
  URIUtils::RemoveSlashAtEnd(storedPath);

  auto i{m_cache.find(storedPath)};
  if (i == m_cache.end())
    i = SetDisc(path);
  auto& [_, disc] = *i;
  disc.playlistMap = playlistmap;
  disc.clipMap = clipmap;
  disc.mapsSet = true;
}

void CBlurayDiscCache::SetPlaylistStreamInfo(const std::string& path,
                                             unsigned int playlist,
                                             const StreamMap& streams)
{
  std::unique_lock lock(m_cs);

  // Get rid of any URL options, else the compare may be wrong
  std::string storedPath{CURL(path).GetWithoutOptions()};
  URIUtils::RemoveSlashAtEnd(storedPath);

  auto i{m_cache.find(storedPath)};
  if (i == m_cache.end())
    i = SetDisc(path);
  auto& [_, disc] = *i;
  disc.streamMap[playlist] = streams;
}

bool CBlurayDiscCache::GetPlaylistInfo(const std::string& path,
                                       unsigned int playlist,
                                       BlurayPlaylistInformation& playlistInfo) const
{
  std::unique_lock lock(m_cs);

  // Get rid of any URL options, else the compare may be wrong
  std::string storedPath{CURL(path).GetWithoutOptions()};
  URIUtils::RemoveSlashAtEnd(storedPath);

  if (const auto& i{m_cache.find(storedPath)}; i != m_cache.end())
  {
    const auto& [_, disc] = *i;
    const auto& j{disc.playlists.find(playlist)};
    if (j != disc.playlists.end())
    {
      const auto& [_, info] = *j;
      playlistInfo = info;
      return true;
    }
  }
  return false;
}

bool CBlurayDiscCache::GetMaps(const std::string& path,
                               PlaylistMap& playlistmap,
                               ClipMap& clipmap) const
{
  std::unique_lock lock(m_cs);

  // Get rid of any URL options, else the compare may be wrong
  std::string storedPath{CURL(path).GetWithoutOptions()};
  URIUtils::RemoveSlashAtEnd(storedPath);

  if (const auto& i{m_cache.find(storedPath)}; i != m_cache.end())
  {
    const auto& [_, disc] = *i;
    if (disc.mapsSet)
    {
      clipmap = disc.clipMap;
      playlistmap = disc.playlistMap;
      return true;
    }
  }
  return false;
}

bool CBlurayDiscCache::GetPlaylistStreamInfo(const std::string& path,
                                             unsigned int playlist,
                                             StreamMap& streams) const
{
  std::unique_lock lock(m_cs);

  // Get rid of any URL options, else the compare may be wrong
  std::string storedPath{CURL(path).GetWithoutOptions()};
  URIUtils::RemoveSlashAtEnd(storedPath);

  if (const auto& i{m_cache.find(storedPath)}; i != m_cache.end())
  {
    const auto& [_, disc] = *i;
    const auto& j{disc.streamMap.find(playlist)};
    if (j != disc.streamMap.end())
    {
      const auto& [_, info] = *j;
      streams = info;
      return true;
    }
  }
  return false;
}

void CBlurayDiscCache::ClearDisc(const std::string& path)
{
  std::unique_lock lock(m_cs);

  // Get rid of any URL options, else the compare may be wrong
  std::string storedPath{CURL(path).GetWithoutOptions()};
  URIUtils::RemoveSlashAtEnd(storedPath);

  m_cache.erase(storedPath);
}

void CBlurayDiscCache::Clear()
{
  std::unique_lock lock(m_cs);
  m_cache.clear();
}
