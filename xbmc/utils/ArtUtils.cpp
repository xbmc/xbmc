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
  std::string strFile = item.GetPath();

  if (item.IsStack())
  {
    std::string strPath, strReturn;
    URIUtils::GetParentPath(item.GetPath(), strPath);
    CFileItem item(CStackDirectory::GetFirstStackedFile(strFile), false);
    std::string strTBNFile = GetTBNFile(item);
    strReturn = URIUtils::AddFileToFolder(strPath, URIUtils::GetFileName(strTBNFile));
    if (CFile::Exists(strReturn))
      return strReturn;

    strFile = URIUtils::AddFileToFolder(
        strPath, URIUtils::GetFileName(CStackDirectory::GetStackedTitlePath(strFile)));
  }

  if (URIUtils::IsInRAR(strFile) || URIUtils::IsInZIP(strFile))
  {
    std::string strPath = URIUtils::GetDirectory(strFile);
    std::string strParent;
    URIUtils::GetParentPath(strPath, strParent);
    strFile = URIUtils::AddFileToFolder(strParent, URIUtils::GetFileName(item.GetPath()));
  }

  CURL url(strFile);
  strFile = url.GetFileName();

  if (item.m_bIsFolder && !item.IsFileFolder())
    URIUtils::RemoveSlashAtEnd(strFile);

  if (!strFile.empty())
  {
    if (item.m_bIsFolder && !item.IsFileFolder())
      thumbFile = strFile + ".tbn"; // folder, so just add ".tbn"
    else
      thumbFile = URIUtils::ReplaceExtension(strFile, ".tbn");
    url.SetFileName(thumbFile);
    thumbFile = url.Get();
  }
  return thumbFile;
}

} // namespace KODI::ART
