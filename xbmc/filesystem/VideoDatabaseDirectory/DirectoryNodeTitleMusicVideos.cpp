/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeTitleMusicVideos.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "QueryParams.h"
#include "utils/ItemProperties.h"
#include "video/VideoDatabase.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

CDirectoryNodeTitleMusicVideos::CDirectoryNodeTitleMusicVideos(const std::string& strName,
                                                               CDirectoryNode* pParent)
  : CDirectoryNode(NodeType::TITLE_MUSICVIDEOS, strName, pParent)
{

}

bool CDirectoryNodeTitleMusicVideos::GetContent(CFileItemList& items) const
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  int details = items.HasProperty(KODI::ITEM::PROPERTY::SET_VIDEODB_DETAILS)
                    ? items.GetProperty(KODI::ITEM::PROPERTY::SET_VIDEODB_DETAILS).asInteger32()
                    : VideoDbDetailsNone;
  bool bSuccess = videodatabase.GetMusicVideosNav(
      BuildPath(), items, params.GetGenreId(), params.GetYear(), params.GetActorId(),
      params.GetDirectorId(), params.GetStudioId(), params.GetAlbumId(), params.GetTagId(),
      SortDescription(), details);

  videodatabase.Close();

  return bSuccess;
}
