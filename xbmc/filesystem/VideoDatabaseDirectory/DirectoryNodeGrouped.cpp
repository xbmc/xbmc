/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeGrouped.h"

#include "QueryParams.h"
#include "utils/ContentNames.h"
#include "video/VideoDatabase.h"
#include "video/VideoDbUrl.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

namespace CONTENT = KODI::MEDIA::CONTENT;

CDirectoryNodeGrouped::CDirectoryNodeGrouped(NodeType type,
                                             const std::string& strName,
                                             CDirectoryNode* pParent)
  : CDirectoryNode(type, strName, pParent)
{ }

NodeType CDirectoryNodeGrouped::GetChildType() const
{
  CQueryParams params;
  CollectQueryParams(params);

  VideoDbContentType type = static_cast<VideoDbContentType>(params.GetContentType());
  if (type == VideoDbContentType::MOVIES)
    return NodeType::TITLE_MOVIES;
  if (type == VideoDbContentType::MUSICVIDEOS)
  {
    if (GetType() == NodeType::ACTOR)
      return NodeType::MUSICVIDEOS_ALBUM;
    else
      return NodeType::TITLE_MUSICVIDEOS;
  }

  return NodeType::TITLE_TVSHOWS;
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
    case NodeType::GENRE:
      return CONTENT::GENRES;
    case NodeType::COUNTRY:
      return CONTENT::COUNTRIES;
    case NodeType::SETS:
      return CONTENT::SETS;
    case NodeType::TAGS:
      return CONTENT::TAGS;
    case NodeType::VIDEOVERSIONS:
      return CONTENT::VIDEOVERSIONS;
    case NodeType::YEAR:
      return CONTENT::YEARS;
    case NodeType::ACTOR:
      if (static_cast<VideoDbContentType>(params.GetContentType()) ==
          VideoDbContentType::MUSICVIDEOS)
        return CONTENT::ARTISTS;
      else
        return CONTENT::ACTORS;
    case NodeType::DIRECTOR:
      return CONTENT::DIRECTORS;
    case NodeType::STUDIO:
      return CONTENT::STUDIOS;
    case NodeType::MUSICVIDEOS_ALBUM:
      return CONTENT::ALBUMS;

    case NodeType::EPISODES:
    case NodeType::MOVIES_OVERVIEW:
    case NodeType::MUSICVIDEOS_OVERVIEW:
    case NodeType::NONE:
    case NodeType::OVERVIEW:
    case NodeType::RECENTLY_ADDED_EPISODES:
    case NodeType::RECENTLY_ADDED_MOVIES:
    case NodeType::RECENTLY_ADDED_MUSICVIDEOS:
    case NodeType::INPROGRESS_TVSHOWS:
    case NodeType::ROOT:
    case NodeType::SEASONS:
    case NodeType::TITLE_MOVIES:
    case NodeType::TITLE_MUSICVIDEOS:
    case NodeType::TITLE_TVSHOWS:
    case NodeType::TVSHOWS_OVERVIEW:
    default:
      break;
  }

  return "";
}
