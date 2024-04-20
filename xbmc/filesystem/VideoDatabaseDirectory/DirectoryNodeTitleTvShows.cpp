/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeTitleTvShows.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "QueryParams.h"
#include "video/VideoDatabase.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

CDirectoryNodeTitleTvShows::CDirectoryNodeTitleTvShows(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_TITLE_TVSHOWS, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeTitleTvShows::GetChildType() const
{
  return NODE_TYPE_SEASONS;
}

std::string CDirectoryNodeTitleTvShows::GetLocalizedName() const
{
  CVideoDatabase db;
  if (db.Open())
    return db.GetTvShowTitleById(GetID());
  return "";
}

bool CDirectoryNodeTitleTvShows::GetContent(CFileItemList& items) const
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  int details = items.HasProperty("set_videodb_details")
                    ? items.GetProperty("set_videodb_details").asInteger32()
                    : VideoDbDetailsNone;

  bool bSuccess = videodatabase.GetTvShowsNav(
      BuildPath(), items, params.GetGenreId(), params.GetYear(), params.GetActorId(),
      params.GetDirectorId(), params.GetStudioId(), params.GetTagId(), SortDescription(), details);

  videodatabase.Close();

  return bSuccess;
}
