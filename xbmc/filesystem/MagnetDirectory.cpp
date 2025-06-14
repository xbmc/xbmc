/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MagnetDirectory.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "network/Network.h"
#include "network/NetworkServices.h"
#include "network/torrent/Libtorrent.h"
#include "network/torrent/TorrentUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <cstring>

#include <libtorrent/libtorrent.hpp>

using namespace KODI;
using namespace NETWORK;
using namespace XFILE;

bool CMagnetDirectory::GetDirectory(const CURL& urlOrig, CFileItemList& items)
{
  CNetworkServices& networkServices = CServiceBroker::GetNetwork().GetServices();
  CLibtorrent* libtorrent = dynamic_cast<CLibtorrent*>(networkServices.GetLibtorrent());
  if (libtorrent == nullptr)
    return false;

  std::string magnetUri;
  std::string subdirectory;
  std::tie(magnetUri, subdirectory) = CTorrentUtils::SplitMagnetURL(urlOrig);

  lt::torrent_handle torrentHandle = libtorrent->AddTorrent(magnetUri);
  if (!torrentHandle.is_valid())
    return false;

  std::shared_ptr<const lt::torrent_info> torrentInfo = torrentHandle.torrent_file();
  if (!torrentInfo)
    return false;

  const lt::file_storage& files = torrentInfo->files();

  if (files.num_files() == 0)
  {
    CLog::Log(LOGERROR, "Torrent has no files!");
    return false;
  }

  // Split subdirectory into parts
  std::vector<std::string> subdirectoryParts = URIUtils::SplitPath(subdirectory);

  // Create a unique set of folders to browse deeper into
  std::set<std::string> folders;

  // Loop through files and select ones that match the subdirectory
  for (int i = 0; i < files.num_files(); ++i)
  {
    std::string filePath = files.file_path(static_cast<lt::file_index_t>(i));
    std::vector<std::string> fileParts = URIUtils::SplitPath(filePath);

    if (fileParts.empty() || fileParts.size() - 1 < subdirectoryParts.size())
      continue;

    // Loop through subdirectory parts and check if they match
    bool inFolder = true;
    for (size_t j = 0; j < subdirectoryParts.size(); ++j)
    {
      if (fileParts[j] != subdirectoryParts[j])
      {
        inFolder = false;
        break;
      }
    }
    if (!inFolder)
      continue;

    // Check if this is a file or a folder
    if (fileParts.size() - subdirectoryParts.size() == 1)
    {
      // This is a file in the subdirectory
      std::time_t modificationTime = files.mtime(static_cast<lt::file_index_t>(i));
      std::string filePath = files.file_path(static_cast<lt::file_index_t>(i));
      std::int64_t fileSize = files.file_size(static_cast<lt::file_index_t>(i));

      CURL url = urlOrig;
      url.SetFileName(filePath);

      CFileItemPtr item = std::make_shared<CFileItem>(URIUtils::GetFileName(filePath));
      item->SetURL(url);
      item->m_dateTime = modificationTime;
      item->m_dwSize = fileSize;

      items.Add(std::move(item));
    }
    else if (fileParts.size() - subdirectoryParts.size() > 1)
    {
      // Record subdirectory to browse deeper into the torrent
      std::string folder = fileParts[subdirectoryParts.size()];
      folders.insert(folder);
    }
  }

  // Now add unique subdirectories
  for (const std::string& folder : folders)
  {
    std::string folderPath = URIUtils::AddFileToFolder(subdirectory, folder);

    CURL url = urlOrig;
    url.SetFileName(folderPath);

    CFileItemPtr item = std::make_shared<CFileItem>(folder);
    item->SetURL(url);
    item->m_bIsFolder = true;

    items.Add(std::move(item));
  }

  if (!subdirectoryParts.empty())
    items.SetLabel(subdirectoryParts.back());

  return true;
}

bool CMagnetDirectory::Exists(const CURL& url)
{
  CNetworkServices& networkServices = CServiceBroker::GetNetwork().GetServices();
  CLibtorrent* libtorrent = dynamic_cast<CLibtorrent*>(networkServices.GetLibtorrent());
  if (libtorrent == nullptr)
    return false;

  std::string magnetUri;
  std::string subdirectory;
  std::tie(magnetUri, subdirectory) = CTorrentUtils::SplitMagnetURL(url);

  lt::torrent_handle torrentHandle = libtorrent->AddTorrent(magnetUri);
  if (!torrentHandle.is_valid())
    return false;

  std::shared_ptr<const lt::torrent_info> torrentInfo = torrentHandle.torrent_file();
  if (!torrentInfo)
    return false;

  const lt::file_storage& files = torrentInfo->files();

  // Split subdirectory into parts
  std::vector<std::string> subdirectoryParts = URIUtils::SplitPath(subdirectory);

  // Loop through files looking for one that matches the subdirectory
  for (int i = 0; i < files.num_files(); ++i)
  {
    std::string filePath = files.file_path(static_cast<lt::file_index_t>(i));
    std::vector<std::string> fileParts = URIUtils::SplitPath(filePath);

    if (fileParts.empty())
      continue;

    // Loop through subdirectory parts and check if they match
    bool hasFolder = true;
    for (size_t j = 0; j < subdirectoryParts.size(); ++j)
    {
      if (subdirectoryParts[j] != fileParts[j])
      {
        hasFolder = false;
        break;
      }
    }
    if (hasFolder)
      return true;
  }

  return false;
}

bool CMagnetDirectory::ContainsFiles(const CURL& url)
{
  //! @todo
  return false;
}
