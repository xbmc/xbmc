/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BlurayDiscCache.h"

#include "URL.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace XFILE;

CBlurayDiscCache::CDisc::CDisc(const BLURAY_DISC_INFO* discInfo)
{
  m_disc_info = *discInfo;
}

CBlurayDiscCache::CDisc::~CDisc() = default;

CBlurayDiscCache::CBlurayDiscCache()
{
}

CBlurayDiscCache::~CBlurayDiscCache() = default;

void CBlurayDiscCache::SetDisc(const std::string& path, const BLURAY_DISC_INFO* discInfo)
{
  std::unique_lock<CCriticalSection> lock(m_cs);

  // Get rid of any URL options, else the compare may be wrong
  std::string storedPath = CURL(path).GetWithoutOptions();
  URIUtils::RemoveSlashAtEnd(storedPath);

  ClearDisc(storedPath);

  CDisc disc(discInfo);
  m_cache.emplace(storedPath, std::move(disc));
}

void CBlurayDiscCache::SetPlaylistInfo(const std::string& path,
                                       unsigned int playlist,
                                       BLURAY_TITLE_INFO* playlistInfo)
{
  std::unique_lock<CCriticalSection> lock(m_cs);

  // Get rid of any URL options, else the compare may be wrong
  std::string storedPath = CURL(path).GetWithoutOptions();
  URIUtils::RemoveSlashAtEnd(storedPath);

  const auto i = m_cache.find(storedPath);
  if (i != m_cache.end())
  {
    auto& [_, disc] = *i;
    disc.m_playlists[playlist] = *playlistInfo;
  }
}

void CBlurayDiscCache::SetMaps(const std::string& path,
                               const PlaylistMap& playlistmap,
                               const ClipMap& clipmap)
{
  std::unique_lock<CCriticalSection> lock(m_cs);

  // Get rid of any URL options, else the compare may be wrong
  std::string storedPath = CURL(path).GetWithoutOptions();
  URIUtils::RemoveSlashAtEnd(storedPath);

  const auto i = m_cache.find(storedPath);
  if (i != m_cache.end())
  {
    auto& [_, disc] = *i;
    disc.m_playlistmap = playlistmap;
    disc.m_clipmap = clipmap;
    disc.m_mapsSet = true;
  }
}

bool CBlurayDiscCache::GetDiscInfo(const std::string& path, const BLURAY_DISC_INFO*& discInfo)
{
  std::unique_lock<CCriticalSection> lock(m_cs);

  // Get rid of any URL options, else the compare may be wrong
  std::string storedPath = CURL(path).GetWithoutOptions();
  URIUtils::RemoveSlashAtEnd(storedPath);

  const auto i = m_cache.find(storedPath);
  if (i != m_cache.end())
  {
    const auto& [_, disc] = *i;
    discInfo = &disc.m_disc_info;
    return true;
  }
  return false;
}

bool CBlurayDiscCache::GetPlaylistInfo(const std::string& path,
                                       unsigned int playlist,
                                       BLURAY_TITLE_INFO& playlistInfo)
{
  std::unique_lock<CCriticalSection> lock(m_cs);

  // Get rid of any URL options, else the compare may be wrong
  std::string storedPath = CURL(path).GetWithoutOptions();
  URIUtils::RemoveSlashAtEnd(storedPath);

  const auto i = m_cache.find(storedPath);
  if (i != m_cache.end())
  {
    const auto& [_, disc] = *i;
    const auto j = disc.m_playlists.find(playlist);
    if (j != disc.m_playlists.end())
    {
      const auto& [_, info] = *j;
      playlistInfo = info;
      return true;
    }
  }
  return false;
}

bool CBlurayDiscCache::GetMaps(const std::string& path, PlaylistMap& playlistmap, ClipMap& clipmap)
{
  std::unique_lock<CCriticalSection> lock(m_cs);

  // Get rid of any URL options, else the compare may be wrong
  std::string storedPath = CURL(path).GetWithoutOptions();
  URIUtils::RemoveSlashAtEnd(storedPath);

  const auto i = m_cache.find(storedPath);
  if (i != m_cache.end())
  {
    const auto& [_, disc] = *i;
    if (disc.m_mapsSet)
    {
      clipmap = disc.m_clipmap;
      playlistmap = disc.m_playlistmap;
      return true;
    }
  }
  return false;
}

void CBlurayDiscCache::ClearDisc(const std::string& path)
{
  std::unique_lock<CCriticalSection> lock(m_cs);

  // Get rid of any URL options, else the compare may be wrong
  std::string storedPath = CURL(path).GetWithoutOptions();
  URIUtils::RemoveSlashAtEnd(storedPath);

  m_cache.erase(storedPath);
}

void CBlurayDiscCache::Clear()
{
  std::unique_lock<CCriticalSection> lock(m_cs);
  m_cache.clear();
}