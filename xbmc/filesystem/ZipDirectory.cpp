/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ZipDirectory.h"

#include "FileItem.h"
#include "URL.h"
#include "ZipManager.h"
#include "filesystem/Directorization.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <vector>

namespace XFILE
{

  static CFileItemPtr ZipEntryToFileItem(const SZipEntry& entry, const std::string& label, const std::string& path, bool isFolder)
  {
    CFileItemPtr item(new CFileItem(label));
    if (!isFolder)
    {
      item->m_dwSize = entry.usize;
      item->m_idepth = entry.method;
    }

    return item;
  }

  CZipDirectory::CZipDirectory() = default;

  CZipDirectory::~CZipDirectory() = default;

  bool CZipDirectory::GetDirectory(const CURL& urlOrig, CFileItemList& items)
  {
    CURL urlZip(urlOrig);

    /* if this isn't a proper archive path, assume it's the path to a archive file */
    if (!urlOrig.IsProtocol("zip"))
      urlZip = URIUtils::CreateArchivePath("zip", urlOrig);

    std::vector<SZipEntry> zipEntries;
    if (!g_ZipManager.GetZipList(urlZip, zipEntries))
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
    g_ZipManager.GetZipList(url, items);
    if (items.size())
    {
      if (items.size() > 1)
        return true;

      return false;
    }

    return false;
  }
}

