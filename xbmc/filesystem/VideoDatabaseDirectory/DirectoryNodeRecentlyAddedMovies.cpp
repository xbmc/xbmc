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
#include "utils/ItemProperties.h"
#include "video/VideoDatabase.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

CDirectoryNodeRecentlyAddedMovies::CDirectoryNodeRecentlyAddedMovies(const std::string& strName,
                                                                     CDirectoryNode* pParent)
  : CDirectoryNode(NodeType::RECENTLY_ADDED_MOVIES, strName, pParent)
{

}

bool CDirectoryNodeRecentlyAddedMovies::GetContent(CFileItemList& items) const
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  int details = items.HasProperty(KODI::ITEM::PROPERTY::SET_VIDEODB_DETAILS)
                    ? items.GetProperty(KODI::ITEM::PROPERTY::SET_VIDEODB_DETAILS).asInteger32()
                    : VideoDbDetailsNone;
  bool bSuccess = videodatabase.GetRecentlyAddedMoviesNav(BuildPath(), items, 0, details);

  videodatabase.Close();

  return bSuccess;
}

NodeType CDirectoryNodeRecentlyAddedMovies::GetChildType() const
{
  return NodeType::MOVIE_ASSET_TYPES;
}
