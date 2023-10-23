/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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

  VideoDbContentType type = static_cast<VideoDbContentType>(params.GetContentType());
  if (type == VideoDbContentType::MOVIES)
    return NODE_TYPE_TITLE_MOVIES;
  if (type == VideoDbContentType::MUSICVIDEOS)
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

  return videodatabase.GetItems(videoUrl.ToString(),
                                static_cast<VideoDbContentType>(params.GetContentType()), itemType,
                                items);
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
    case NODE_TYPE_VIDEOVERSIONS:
      return "videoversions";
    case NODE_TYPE_YEAR:
      return "years";
    case NODE_TYPE_ACTOR:
      if (static_cast<VideoDbContentType>(params.GetContentType()) ==
          VideoDbContentType::MUSICVIDEOS)
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
    case NODE_TYPE_INPROGRESS_TVSHOWS:
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
