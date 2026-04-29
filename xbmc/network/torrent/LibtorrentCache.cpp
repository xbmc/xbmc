/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LibtorrentCache.h"

#include "URL.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <sstream>

using namespace KODI;
using namespace NETWORK;

CLibtorrentCache::CLibtorrentCache()
  : m_cachePath(CSpecialProtocol::TranslatePath("special://home/cache/kademlia"))
{
  CreatePaths();
}

std::string CLibtorrentCache::GetMetadataPath(const lt::sha256_hash& infoHash) const
{
  std::stringstream str;
  str << infoHash;

  return URIUtils::AddFileToFolder(m_cachePath, str.str() + ".torrent");
}

void CLibtorrentCache::SaveMetadata(const lt::add_torrent_params& addTorrentParams)
{
  const std::string torrentPath = GetMetadataPath(addTorrentParams.info_hashes.v2);

  // Encode metadata
  std::vector<char> buffer;
  try
  {
    buffer = lt::write_torrent_file_buf(addTorrentParams, {});
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "Failed to encode torrent file: {}", e.what());
    return;
  }

  // Write metadata to cache
  XFILE::CFile file;
  if (!file.OpenForWrite(torrentPath, true))
  {
    CLog::Log(LOGERROR, "Failed to open torrent file for writing: {}",
              CURL::GetRedacted(torrentPath));
    return;
  }

  file.Write(buffer.data(), buffer.size());
}

void CLibtorrentCache::CreatePaths()
{
  // Create folder if it doesn't exist
  if (!XFILE::CDirectory::Exists(m_cachePath))
  {
    CLog::Log(LOGDEBUG, "Creating Kademlia cache path: {}", CURL::GetRedacted(m_cachePath));
    if (!XFILE::CDirectory::Create(m_cachePath))
      CLog::Log(LOGERROR, "Failed to create Kademlia cache path");
  }
}
