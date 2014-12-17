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

#include "DirectoryNodeGrouped.h"
#include "QueryParams.h"
#include "video/VideoDatabase.h"
#include "video/VideoDbUrl.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

CDirectoryNodeGrouped::CDirectoryNodeGrouped(NODE_TYPE type, const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(type, strName, pParent)
{ }

NODE_TYPE CDirectoryNodeGrouped::GetChildType() const
{
  CQueryParams params;
  CollectQueryParams(params);

  if (params.GetContentType() == VIDEODB_CONTENT_MOVIES)
    return NODE_TYPE_TITLE_MOVIES;
  if (params.GetContentType() == VIDEODB_CONTENT_MUSICVIDEOS)
  {
    if (GetType() == NODE_TYPE_ACTOR)
      return NODE_TYPE_MUSICVIDEOS_ALBUM;
    else
      return NODE_TYPE_TITLE_MUSICVIDEOS;
  }

  return NODE_TYPE_TITLE_TVSHOWS;
}

std::string CDirectoryNodeGrouped::GetLocalizedName() const
{
  CVideoDatabase db;
  if (db.Open())
    return db.GetItemById(GetContentType(), GetID());

  return "";
}

bool CDirectoryNodeGrouped::GetContent(CFileItemList& items) const
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  std::string itemType = GetContentType(params);
  if (itemType.empty())
    return false;

  // make sure to translate all IDs in the path into URL parameters
  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(BuildPath()))
    return false;

  return videodatabase.GetItems(videoUrl.ToString(), (VIDEODB_CONTENT_TYPE)params.GetContentType(), itemType, items);
}

std::string CDirectoryNodeGrouped::GetContentType() const
{
  CQueryParams params;
  CollectQueryParams(params);

  return GetContentType(params);
}

std::string CDirectoryNodeGrouped::GetContentType(const CQueryParams &params) const
{
  switch (GetType())
  {
    case NODE_TYPE_GENRE:
      return "genres";
    case NODE_TYPE_COUNTRY:
      return "countries";
    case NODE_TYPE_SETS:
      return "sets";
    case NODE_TYPE_TAGS:
      return "tags";
    case NODE_TYPE_YEAR:
      return "years";
    case NODE_TYPE_ACTOR:
      if ((VIDEODB_CONTENT_TYPE)params.GetContentType() == VIDEODB_CONTENT_MUSICVIDEOS)
        return "artists";
      else
        return "actors";
    case NODE_TYPE_DIRECTOR:
      return "directors";
    case NODE_TYPE_STUDIO:
      return "studios";
    case NODE_TYPE_MUSICVIDEOS_ALBUM:
      return "albums";

    case NODE_TYPE_EPISODES:
    case NODE_TYPE_MOVIES_OVERVIEW:
    case NODE_TYPE_MUSICVIDEOS_OVERVIEW:
    case NODE_TYPE_NONE:
    case NODE_TYPE_OVERVIEW:
    case NODE_TYPE_RECENTLY_ADDED_EPISODES:
    case NODE_TYPE_RECENTLY_ADDED_MOVIES:
    case NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS:
    case NODE_TYPE_ROOT:
    case NODE_TYPE_SEASONS:
    case NODE_TYPE_TITLE_MOVIES:
    case NODE_TYPE_TITLE_MUSICVIDEOS:
    case NODE_TYPE_TITLE_TVSHOWS:
    case NODE_TYPE_TVSHOWS_OVERVIEW:
    default:
      break;
  }

  return "";
}
