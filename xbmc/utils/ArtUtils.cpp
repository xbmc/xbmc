/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ArtUtils.h"

#include "FileItem.h"
#include "filesystem/File.h"
#include "filesystem/StackDirectory.h"
#include "utils/URIUtils.h"

using namespace XFILE;

namespace KODI::ART
{

// Gets the .tbn filename from a file or folder name.
// <filename>.ext -> <filename>.tbn
// <foldername>/ -> <foldername>.tbn
std::string GetTBNFile(const CFileItem& item)
{
  std::string thumbFile;
  std::string file = item.GetPath();

  if (item.IsStack())
  {
    std::string path, returnPath;
    URIUtils::GetParentPath(item.GetPath(), path);
    CFileItem item(CStackDirectory::GetFirstStackedFile(file), false);
    const std::string TBNFile = GetTBNFile(item);
    returnPath = URIUtils::AddFileToFolder(path, URIUtils::GetFileName(TBNFile));
    if (CFile::Exists(returnPath))
      return returnPath;

    const std::string& stackPath = CStackDirectory::GetStackedTitlePath(file);
    file = URIUtils::AddFileToFolder(path, URIUtils::GetFileName(stackPath));
  }

  if (URIUtils::IsInRAR(file) || URIUtils::IsInZIP(file))
  {
    const std::string path = URIUtils::GetDirectory(file);
    std::string parent;
    URIUtils::GetParentPath(path, parent);
    file = URIUtils::AddFileToFolder(parent, URIUtils::GetFileName(item.GetPath()));
  }

  CURL url(file);
  file = url.GetFileName();

  if (item.m_bIsFolder && !item.IsFileFolder())
    URIUtils::RemoveSlashAtEnd(file);

  if (!file.empty())
  {
    if (item.m_bIsFolder && !item.IsFileFolder())
      thumbFile = file + ".tbn"; // folder, so just add ".tbn"
    else
      thumbFile = URIUtils::ReplaceExtension(file, ".tbn");
    url.SetFileName(thumbFile);
    thumbFile = url.Get();
  }
  return thumbFile;
}

} // namespace KODI::ART
