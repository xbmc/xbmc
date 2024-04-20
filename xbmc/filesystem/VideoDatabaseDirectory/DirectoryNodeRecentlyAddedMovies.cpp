/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeRecentlyAddedMovies.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "video/VideoDatabase.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

CDirectoryNodeRecentlyAddedMovies::CDirectoryNodeRecentlyAddedMovies(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_RECENTLY_ADDED_MOVIES, strName, pParent)
{

}

bool CDirectoryNodeRecentlyAddedMovies::GetContent(CFileItemList& items) const
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  int details = items.HasProperty("set_videodb_details")
                    ? items.GetProperty("set_videodb_details").asInteger32()
                    : VideoDbDetailsNone;
  bool bSuccess = videodatabase.GetRecentlyAddedMoviesNav(BuildPath(), items, 0, details);

  videodatabase.Close();

  return bSuccess;
}
