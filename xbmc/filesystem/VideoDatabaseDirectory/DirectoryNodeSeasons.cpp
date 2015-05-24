/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DirectoryNodeSeasons.h"
#include "QueryParams.h"
#include "video/VideoDatabase.h"
#include "FileItem.h"
#include "utils/StringUtils.h"

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
    CDirectoryNode *pParent = GetParent();
    if (pParent)
      return pParent->GetLocalizedName();
    return "";
  }
  default:
    std::string season = StringUtils::Format(g_localizeStrings.Get(20358).c_str(), GetID()); // Season <season>
    return season;
  }
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
