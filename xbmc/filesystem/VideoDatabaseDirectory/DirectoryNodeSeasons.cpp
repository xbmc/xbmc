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

#ifndef VIDEODATABASEDIRECTORY_DIRECTORYNODESEASONS_H_INCLUDED
#define VIDEODATABASEDIRECTORY_DIRECTORYNODESEASONS_H_INCLUDED
#include "DirectoryNodeSeasons.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_QUERYPARAMS_H_INCLUDED
#define VIDEODATABASEDIRECTORY_QUERYPARAMS_H_INCLUDED
#include "QueryParams.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_VIDEO_VIDEODATABASE_H_INCLUDED
#define VIDEODATABASEDIRECTORY_VIDEO_VIDEODATABASE_H_INCLUDED
#include "video/VideoDatabase.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_VIDEO_VIDEODBURL_H_INCLUDED
#define VIDEODATABASEDIRECTORY_VIDEO_VIDEODBURL_H_INCLUDED
#include "video/VideoDbUrl.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_SETTINGS_MEDIASETTINGS_H_INCLUDED
#define VIDEODATABASEDIRECTORY_SETTINGS_MEDIASETTINGS_H_INCLUDED
#include "settings/MediaSettings.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_SETTINGS_SETTINGS_H_INCLUDED
#define VIDEODATABASEDIRECTORY_SETTINGS_SETTINGS_H_INCLUDED
#include "settings/Settings.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_FILEITEM_H_INCLUDED
#define VIDEODATABASEDIRECTORY_FILEITEM_H_INCLUDED
#include "FileItem.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_UTILS_VARIANT_H_INCLUDED
#define VIDEODATABASEDIRECTORY_UTILS_VARIANT_H_INCLUDED
#include "utils/Variant.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_UTILS_STRINGUTILS_H_INCLUDED
#define VIDEODATABASEDIRECTORY_UTILS_STRINGUTILS_H_INCLUDED
#include "utils/StringUtils.h"
#endif


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
    CStdString season = StringUtils::Format(g_localizeStrings.Get(20358), GetID()); // Season <season>
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
