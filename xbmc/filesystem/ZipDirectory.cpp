/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ZipDirectory.h"

#include "File.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Zip.h"
#include "cache/CacheComponent.h"
#include "cache/FileSystemCache.h"
#include "filesystem/Directorization.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <cstdint>
#include <cstring>
#include <vector>

namespace XFILE
{

namespace
{
std::shared_ptr<CFileItem> ZipEntryToFileItem(const SZipEntry& entry,
                                              const std::string& label,
                                              const std::string& path,
                                              bool isFolder)
{
  std::shared_ptr<CFileItem> item(new CFileItem(label));
  if (!isFolder)
  {
    item->SetSize(static_cast<int64_t>(entry.usize));
    item->SetDepth(entry.method);
  }

  return item;
}

bool GetZipEntries(const CURL& urlZip, std::vector<SZipEntry>& zipEntries)
{
  auto& cache = CServiceBroker::GetCacheComponent()->GetZipCache();
  const std::string strFile = urlZip.GetHostName();

  struct __stat64 statData = {};
  if (CFile::Stat(strFile, &statData))
    return false;

  if (!cache.GetCachedList(strFile, statData.st_mtime, zipEntries))
  {
    if (!Zip::ParseZipCentralDirectory(strFile, zipEntries))
      return false;
    cache.StoreList(strFile, statData.st_mtime, zipEntries);
  }

  return true;
}
} // namespace

  CZipDirectory::CZipDirectory() = default;

  CZipDirectory::~CZipDirectory() = default;

  bool CZipDirectory::GetDirectory(const CURL& urlOrig, CFileItemList& items)
  {
    CURL urlZip(urlOrig);

    /* if this isn't a proper archive path, assume it's the path to an archive file */
    if (!urlOrig.IsProtocol("zip"))
      urlZip = URIUtils::CreateArchivePath("zip", urlOrig);

    std::vector<SZipEntry> zipEntries;
    if (!GetZipEntries(urlZip, zipEntries))
      return false;

    // prepare the ZIP entries for directorization
    DirectorizeEntries<SZipEntry> entries;
    entries.reserve(zipEntries.size());
    for (const auto& zipEntry : zipEntries)
      entries.emplace_back(zipEntry.name, zipEntry);

    // directorize the ZIP entries into files and directories
    Directorize(urlZip, entries, ZipEntryToFileItem, items);

    return true;
  }

  bool CZipDirectory::ContainsFiles(const CURL& url)
  {
    std::vector<SZipEntry> items;
    if (!GetZipEntries(url, items))
      return false;

    return items.size() > 1;
  }

  bool CZipDirectory::ExtractArchive(const std::string& strArchive, const std::string& strPath)
  {
    const CURL pathToUrl(strArchive);
    return ExtractArchive(pathToUrl, strPath);
  }

  bool CZipDirectory::ExtractArchive(const CURL& archive, const std::string& strPath)
  {
    CURL url = URIUtils::CreateArchivePath("zip", archive);

    std::vector<SZipEntry> entries;
    if (!GetZipEntries(url, entries))
      return false;

    for (const auto& it : entries)
    {
      if (it.name[strlen(it.name) - 1] == '/') // skip dirs
        continue;
      std::string strFilePath(it.name);

      CURL zipPath = URIUtils::CreateArchivePath("zip", archive, strFilePath);
      const CURL pathToUrl(strPath + strFilePath);
      if (!CFile::Copy(zipPath, pathToUrl))
        return false;
    }
    return true;
  }
  } // namespace XFILE
