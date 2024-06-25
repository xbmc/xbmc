/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ArtUtils.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/StackDirectory.h"
#include "network/NetworkFileItemClassify.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/FileExtensionProvider.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"

using namespace XFILE;

namespace KODI::ART
{

std::string GetLocalFanart(const CFileItem& item)
{
  if (VIDEO::IsVideoDb(item))
  {
    if (!item.HasVideoInfoTag())
      return ""; // nothing can be done
    CFileItem dbItem(item.m_bIsFolder ? item.GetVideoInfoTag()->m_strPath
                                      : item.GetVideoInfoTag()->m_strFileNameAndPath,
                     item.m_bIsFolder);
    return GetLocalFanart(dbItem);
  }

  std::string strFile2;
  std::string strFile = item.GetPath();
  if (item.IsStack())
  {
    std::string strPath;
    URIUtils::GetParentPath(item.GetPath(), strPath);
    CStackDirectory dir;
    std::string strPath2;
    strPath2 = dir.GetStackedTitlePath(strFile);
    strFile = URIUtils::AddFileToFolder(strPath, URIUtils::GetFileName(strPath2));
    CFileItem fan_item(dir.GetFirstStackedFile(item.GetPath()), false);
    std::string strTBNFile(URIUtils::ReplaceExtension(GetTBNFile(fan_item), "-fanart"));
    strFile2 = URIUtils::AddFileToFolder(strPath, URIUtils::GetFileName(strTBNFile));
  }

  if (URIUtils::IsInRAR(strFile) || URIUtils::IsInZIP(strFile))
  {
    std::string strPath = URIUtils::GetDirectory(strFile);
    std::string strParent;
    URIUtils::GetParentPath(strPath, strParent);
    strFile = URIUtils::AddFileToFolder(strParent, URIUtils::GetFileName(item.GetPath()));
  }

  // no local fanart available for these
  if (NETWORK::IsInternetStream(item) || URIUtils::IsUPnP(strFile) || URIUtils::IsBluray(strFile) ||
      item.IsLiveTV() || item.IsPlugin() || item.IsAddonsPath() || item.IsDVD() ||
      (URIUtils::IsFTP(strFile) &&
       !CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bFTPThumbs) ||
      item.GetPath().empty())
    return "";

  std::string strDir = URIUtils::GetDirectory(strFile);

  if (strDir.empty())
    return "";

  CFileItemList items;
  CDirectory::GetDirectory(strDir, items,
                           CServiceBroker::GetFileExtensionProvider().GetPictureExtensions(),
                           DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO);
  if (item.IsOpticalMediaFile())
  { // grab from the optical media parent folder as well
    CFileItemList moreItems;
    CDirectory::GetDirectory(item.GetLocalMetadataPath(), moreItems,
                             CServiceBroker::GetFileExtensionProvider().GetPictureExtensions(),
                             DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO);
    items.Append(moreItems);
  }

  std::vector<std::string> fanarts = {"fanart"};

  strFile = URIUtils::ReplaceExtension(strFile, "-fanart");
  fanarts.insert(item.m_bIsFolder ? fanarts.end() : fanarts.begin(),
                 URIUtils::GetFileName(strFile));

  if (!strFile2.empty())
    fanarts.insert(item.m_bIsFolder ? fanarts.end() : fanarts.begin(),
                   URIUtils::GetFileName(strFile2));

  for (const auto& fanart : fanarts)
  {
    for (const auto& item : items)
    {
      std::string strCandidate = URIUtils::GetFileName(item->GetPath());
      URIUtils::RemoveExtension(strCandidate);
      std::string strFanart = fanart;
      URIUtils::RemoveExtension(strFanart);
      if (StringUtils::EqualsNoCase(strCandidate, strFanart))
        return item->GetPath();
    }
  }

  return "";
}

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
