/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "XbtDirectory.h"

#include "FileItem.h"
#include "URL.h"
#include "filesystem/Directorization.h"
#include "filesystem/XbtManager.h"
#include "guilib/XBTF.h"

#include <stdio.h>

namespace XFILE
{

static CFileItemPtr XBTFFileToFileItem(const CXBTFFile& entry, const std::string& label, const std::string& path, bool isFolder)
{
  CFileItemPtr item(new CFileItem(label));
  if (!isFolder)
    item->m_dwSize = static_cast<int64_t>(entry.GetUnpackedSize());

  return item;
}

CXbtDirectory::CXbtDirectory() = default;

CXbtDirectory::~CXbtDirectory() = default;

bool CXbtDirectory::GetDirectory(const CURL& urlOrig, CFileItemList& items)
{
  CURL urlXbt(urlOrig);

  // if this isn't a proper xbt:// path, assume it's the path to a xbt file
  if (!urlOrig.IsProtocol("xbt"))
    urlXbt = URIUtils::CreateArchivePath("xbt", urlOrig);

  CURL url(urlXbt);
  url.SetOptions(""); // delete options to have a clean path to add stuff too
  url.SetFileName(""); // delete filename too as our names later will contain it

  std::vector<CXBTFFile> files;
  if (!CXbtManager::GetInstance().GetFiles(url, files))
    return false;

  // prepare the files for directorization
  DirectorizeEntries<CXBTFFile> entries;
  entries.reserve(files.size());
  for (const auto& file : files)
    entries.emplace_back(file.GetPath(), file);

  Directorize(urlXbt, entries, XBTFFileToFileItem, items);

  return true;
}

bool CXbtDirectory::ContainsFiles(const CURL& url)
{
  return CXbtManager::GetInstance().HasFiles(url);
}

}
