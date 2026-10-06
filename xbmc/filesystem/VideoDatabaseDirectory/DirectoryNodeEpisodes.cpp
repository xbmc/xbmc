/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeEpisodes.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "QueryParams.h"
#include "utils/ItemProperties.h"
#include "video/VideoDatabase.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

CDirectoryNodeEpisodes::CDirectoryNodeEpisodes(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NodeType::EPISODES, strName, pParent)
{

}

bool CDirectoryNodeEpisodes::GetContent(CFileItemList& items) const
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  int season = (int)params.GetSeason();
  if (season == -2)
    season = -1;

  int details = items.HasProperty(KODI::ITEM::PROPERTY::SET_VIDEODB_DETAILS)
                    ? items.GetProperty(KODI::ITEM::PROPERTY::SET_VIDEODB_DETAILS).asInteger32()
                    : VideoDbDetailsNone;

  bool bSuccess = videodatabase.GetEpisodesNav(
      BuildPath(), items, params.GetGenreId(), params.GetYear(), params.GetActorId(),
      params.GetDirectorId(), params.GetTvShowId(), season, SortDescription(), details);

  videodatabase.Close();

  return bSuccess;
}

NodeType CDirectoryNodeEpisodes::GetChildType() const
{
  return NodeType::EPISODES;
}
