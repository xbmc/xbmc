/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "DirectoryNodeEpisodes.h"
#include "QueryParams.h"
#include "video/VideoDatabase.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

CDirectoryNodeEpisodes::CDirectoryNodeEpisodes(const CStdString& strName, CDirectoryNode* pParent)
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

  bool bSuccess=videodatabase.GetEpisodesNav(BuildPath(), items, params.GetGenreId(), params.GetYear(), params.GetActorId(), params.GetDirectorId(), params.GetTvShowId(), season);

  videodatabase.Close();

  return bSuccess;
}
