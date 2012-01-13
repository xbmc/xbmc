/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "DirectoryNodeSeasons.h"
#include "QueryParams.h"
#include "video/VideoDatabase.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "utils/Variant.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

CDirectoryNodeSeasons::CDirectoryNodeSeasons(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_SEASONS, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeSeasons::GetChildType() const
{
  return NODE_TYPE_EPISODES;
}

CStdString CDirectoryNodeSeasons::GetLocalizedName() const
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
    CStdString season;
    season.Format(g_localizeStrings.Get(20358), GetID()); // Season <season>
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

  int iFlatten = g_guiSettings.GetInt("videolibrary.flattentvshows");
  bool bSuccess=videodatabase.GetSeasonsNav(BuildPath(), items, params.GetActorId(), params.GetDirectorId(), params.GetGenreId(), params.GetYear(), params.GetTvShowId());
  bool bFlatten = (items.GetObjectCount() == 1 && iFlatten == 1) || iFlatten == 2;
  if (items.GetObjectCount() == 2 && iFlatten == 1)
    if (items[0]->GetVideoInfoTag()->m_iSeason == 0 || items[1]->GetVideoInfoTag()->m_iSeason == 0)
      bFlatten = true; // flatten if one season + specials

  if (!bFlatten && g_settings.GetWatchMode("tvshows") == VIDEO_SHOW_UNWATCHED) 
  {
    int count = 0;
    for(int i = 0; i < items.Size(); i++) 
    {
      if (items[i]->GetProperty("unwatchedepisodes").asInteger() != 0 && items[i]->GetVideoInfoTag()->m_iSeason != 0)
        count++;
    }
    bFlatten = (count < 2); // flatten if there is only 1 unwatched season (not counting specials)
  }

  if (bFlatten)
  { // flatten if one season or flatten always
    items.Clear();
    bSuccess=videodatabase.GetEpisodesNav(BuildPath()+"-2/",items,params.GetGenreId(),params.GetYear(),params.GetActorId(),params.GetDirectorId(),params.GetTvShowId());
    items.SetPath(BuildPath()+"-2/");
  }

  videodatabase.Close();

  return bSuccess;
}
