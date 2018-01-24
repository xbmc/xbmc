/*
 *      Copyright (C) 2015 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>

#include "XbtDirectory.h"
#include "FileItem.h"
#include "URL.h"
#include "filesystem/Directorization.h"
#include "filesystem/XbtManager.h"
#include "guilib/XBTF.h"

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
    entries.push_back(DirectorizeEntry<CXBTFFile>(file.GetPath(), file));

  Directorize(urlXbt, entries, XBTFFileToFileItem, items);

  return true;
}

bool CXbtDirectory::ContainsFiles(const CURL& url)
{
  return CXbtManager::GetInstance().HasFiles(url);
}

}
