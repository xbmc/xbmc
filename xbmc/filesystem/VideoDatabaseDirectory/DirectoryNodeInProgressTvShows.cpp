/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeInProgressTvShows.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "video/VideoDatabase.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

CDirectoryNodeInProgressTvShows::CDirectoryNodeInProgressTvShows(const std::string& strName,
                                                                 CDirectoryNode* pParent)
  : CDirectoryNode(NodeType::INPROGRESS_TVSHOWS, strName, pParent)
{

}

NodeType CDirectoryNodeInProgressTvShows::GetChildType() const
{
  return NodeType::SEASONS;
}

std::string CDirectoryNodeInProgressTvShows::GetLocalizedName() const
{
  CVideoDatabase db;
  if (db.Open())
    return db.GetTvShowTitleById(GetID());
  return "";
}

bool CDirectoryNodeInProgressTvShows::GetContent(CFileItemList& items) const
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  int details = items.HasProperty("set_videodb_details")
                    ? items.GetProperty("set_videodb_details").asInteger32()
                    : VideoDbDetailsNone;
  bool bSuccess = videodatabase.GetInProgressTvShowsNav(BuildPath(), items, 0, details);

  videodatabase.Close();

  return bSuccess;
}
