/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

#include <libtorrent/libtorrent.hpp>

namespace KODI
{
namespace NETWORK
{
/*!
 * \brief Subsystem for handling libtorrent caching
 */
class CLibtorrentCache
{
public:
  CLibtorrentCache();
  ~CLibtorrentCache() = default;

  const std::string& GetCachePath() const { return m_cachePath; }

  std::string GetMetadataPath(const lt::sha256_hash& infoHash) const;

  void SaveMetadata(const lt::add_torrent_params& addTorrentParams);

private:
  void CreatePaths();

  const std::string m_cachePath;
};
} // namespace NETWORK
} // namespace KODI
