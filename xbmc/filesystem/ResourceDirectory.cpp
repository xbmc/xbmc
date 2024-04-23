/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ResourceDirectory.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "filesystem/Directory.h"
#include "filesystem/ResourceFile.h"
#include "utils/URIUtils.h"

using namespace XFILE;

CResourceDirectory::CResourceDirectory() = default;

CResourceDirectory::~CResourceDirectory() = default;

bool CResourceDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  const std::string pathToUrl(url.Get());
  std::string translatedPath;
  if (!CResourceFile::TranslatePath(url, translatedPath))
    return false;

  if (CDirectory::GetDirectory(translatedPath, items, m_strFileMask, m_flags | DIR_FLAG_GET_HIDDEN))
  { // replace our paths as necessary
    items.SetURL(url);
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      if (URIUtils::PathHasParent(item->GetPath(), translatedPath))
        item->SetPath(URIUtils::AddFileToFolder(pathToUrl, item->GetPath().substr(translatedPath.size())));
    }

    return true;
  }

  return false;
}

std::string CResourceDirectory::TranslatePath(const CURL &url)
{
  std::string translatedPath;
  if (!CResourceFile::TranslatePath(url, translatedPath))
    return "";

  return translatedPath;
}
