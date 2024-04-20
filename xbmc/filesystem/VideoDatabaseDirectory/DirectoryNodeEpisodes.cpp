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
#include "video/VideoDatabase.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

CDirectoryNodeEpisodes::CDirectoryNodeEpisodes(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_EPISODES, strName, pParent)
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

  int details = items.HasProperty("set_videodb_details")
                    ? items.GetProperty("set_videodb_details").asInteger32()
                    : VideoDbDetailsNone;

  bool bSuccess = videodatabase.GetEpisodesNav(
      BuildPath(), items, params.GetGenreId(), params.GetYear(), params.GetActorId(),
      params.GetDirectorId(), params.GetTvShowId(), season, SortDescription(), details);

  videodatabase.Close();

  return bSuccess;
}

NODE_TYPE CDirectoryNodeEpisodes::GetChildType() const
{
  return NODE_TYPE_EPISODES;
}
