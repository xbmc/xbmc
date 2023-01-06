/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeSeasons.h"

#include "FileItem.h"
#include "QueryParams.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "video/VideoDatabase.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

CDirectoryNodeSeasons::CDirectoryNodeSeasons(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_SEASONS, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeSeasons::GetChildType() const
{
  return NODE_TYPE_EPISODES;
}

std::string CDirectoryNodeSeasons::GetLocalizedName() const
{
  switch (GetID())
  {
    case 0:
      return g_localizeStrings.Get(20381); // Specials
    case -1:
      return g_localizeStrings.Get(20366); // All Seasons
    case -2:
    {
      CDirectoryNode* pParent = GetParent();
      if (pParent)
        return pParent->GetLocalizedName();
      return "";
    }
    default:
      return GetSeasonTitle();
  }
}

std::string CDirectoryNodeSeasons::GetSeasonTitle() const
{
  std::string season;
  CVideoDatabase db;
  if (db.Open())
  {
    CQueryParams params;
    CollectQueryParams(params);

    season = db.GetTvShowNamedSeasonById(params.GetTvShowId(), params.GetSeason());
  }
  if (season.empty())
    season = StringUtils::Format(g_localizeStrings.Get(20358), GetID()); // Season <n>

  return season;
}

bool CDirectoryNodeSeasons::GetContent(CFileItemList& items) const
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  bool bSuccess=videodatabase.GetSeasonsNav(BuildPath(), items, params.GetActorId(), params.GetDirectorId(), params.GetGenreId(), params.GetYear(), params.GetTvShowId());

  videodatabase.Close();

  return bSuccess;
}
