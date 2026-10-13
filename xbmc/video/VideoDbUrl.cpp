/*
 *  Copyright (C) 2016-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoDbUrl.h"

#include "filesystem/VideoDatabaseDirectory.h"
#include "filesystem/VideoDatabaseDirectory/DirectoryNode.h"
#include "filesystem/VideoDatabaseDirectory/QueryParams.h"
#include "playlists/SmartPlayList.h"
#include "utils/ContentNames.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

using namespace KODI;
using namespace XFILE;
using namespace VIDEODATABASEDIRECTORY;

namespace CONTENT = KODI::MEDIA::CONTENT;

CVideoDbUrl::CVideoDbUrl()
  : CDbUrl()
{ }

CVideoDbUrl::~CVideoDbUrl() = default;

bool CVideoDbUrl::parse()
{
  // the URL must start with videodb://
  if (!m_url.IsProtocol("videodb") || m_url.GetFileName().empty())
    return false;

  std::string path = m_url.Get();
  const auto dirType = CVideoDatabaseDirectory::GetDirectoryType(path);
  const auto childType = CVideoDatabaseDirectory::GetDirectoryChildType(path);

  switch (dirType)
  {
    case NodeType::MOVIES_OVERVIEW:
    case NodeType::RECENTLY_ADDED_MOVIES:
    case NodeType::TITLE_MOVIES:
    case NodeType::SETS:
      m_type = CONTENT::MOVIES;
      break;

    // Leaf node
    case NodeType::MOVIE_ASSETS:
      m_type = CONTENT::MOVIES;
      m_itemType = CONTENT::MOVIES;
      break;

    case NodeType::TVSHOWS_OVERVIEW:
    case NodeType::TITLE_TVSHOWS:
    case NodeType::SEASONS:
    case NodeType::EPISODES:
    case NodeType::RECENTLY_ADDED_EPISODES:
    case NodeType::INPROGRESS_TVSHOWS:
      m_type = CONTENT::TVSHOWS;
      break;

    case NodeType::MUSICVIDEOS_OVERVIEW:
    case NodeType::RECENTLY_ADDED_MUSICVIDEOS:
    case NodeType::TITLE_MUSICVIDEOS:
    case NodeType::MUSICVIDEOS_ALBUM:
      m_type = CONTENT::MUSICVIDEOS;
      break;

    default:
      break;
  }

  switch (childType)
  {
    case NodeType::MOVIES_OVERVIEW:
    case NodeType::TITLE_MOVIES:
    case NodeType::RECENTLY_ADDED_MOVIES:
    case NodeType::MOVIE_ASSET_TYPES:
    case NodeType::MOVIE_ASSETS:
    case NodeType::MOVIE_ASSETS_VERSIONS:
    case NodeType::MOVIE_ASSETS_EXTRAS:
      m_type = CONTENT::MOVIES;
      m_itemType = CONTENT::MOVIES;
      break;

    case NodeType::TVSHOWS_OVERVIEW:
    case NodeType::TITLE_TVSHOWS:
    case NodeType::INPROGRESS_TVSHOWS:
      m_type = CONTENT::TVSHOWS;
      m_itemType = CONTENT::TVSHOWS;
      break;

    case NodeType::SEASONS:
      m_type = CONTENT::TVSHOWS;
      m_itemType = CONTENT::SEASONS;
      break;

    case NodeType::EPISODES:
    case NodeType::RECENTLY_ADDED_EPISODES:
      m_type = CONTENT::TVSHOWS;
      m_itemType = CONTENT::EPISODES;
      break;

    case NodeType::MUSICVIDEOS_OVERVIEW:
    case NodeType::RECENTLY_ADDED_MUSICVIDEOS:
    case NodeType::TITLE_MUSICVIDEOS:
      m_type = CONTENT::MUSICVIDEOS;
      m_itemType = CONTENT::MUSICVIDEOS;
      break;

    case NodeType::GENRE:
      m_itemType = CONTENT::GENRES;
      break;

    case NodeType::ACTOR:
      m_itemType = CONTENT::ACTORS;
      break;

    case NodeType::YEAR:
      m_itemType = CONTENT::YEARS;
      break;

    case NodeType::DIRECTOR:
      m_itemType = CONTENT::DIRECTORS;
      break;

    case NodeType::STUDIO:
      m_itemType = CONTENT::STUDIOS;
      break;

    case NodeType::COUNTRY:
      m_itemType = CONTENT::COUNTRIES;
      break;

    case NodeType::SETS:
      m_itemType = CONTENT::SETS;
      break;

    case NodeType::MUSICVIDEOS_ALBUM:
      m_type = CONTENT::MUSICVIDEOS;
      m_itemType = CONTENT::ALBUMS;
      break;

    case NodeType::TAGS:
      m_itemType = CONTENT::TAGS;
      break;

    case NodeType::VIDEOVERSIONS:
      m_itemType = CONTENT::VIDEOVERSIONS;
      break;

    case NodeType::NONE:
      if (m_type.empty() || m_itemType.empty())
        return false;
      break;

    case NodeType::ROOT:
    case NodeType::OVERVIEW:
    default:
      return false;
  }

  if (m_type.empty() || m_itemType.empty())
    return false;

  // parse query params
  CQueryParams queryParams;
  if (!CVideoDatabaseDirectory::GetQueryParams(path, queryParams))
    return false;

  // retrieve and parse all options
  AddOptions(m_url.GetOptions());

  // add options based on the QueryParams
  if (queryParams.GetActorId() != -1)
  {
    std::string optionName = "actorid";
    if (m_type == CONTENT::MUSICVIDEOS)
      optionName = "artistid";

    AddOption(optionName, (int)queryParams.GetActorId());
  }
  if (queryParams.GetAlbumId() != -1)
    AddOption("albumid", (int)queryParams.GetAlbumId());
  if (queryParams.GetCountryId() != -1)
    AddOption("countryid", (int)queryParams.GetCountryId());
  if (queryParams.GetDirectorId() != -1)
    AddOption("directorid", (int)queryParams.GetDirectorId());
  if (queryParams.GetEpisodeId() != -1)
    AddOption("episodeid", (int)queryParams.GetEpisodeId());
  if (queryParams.GetGenreId() != -1)
    AddOption("genreid", (int)queryParams.GetGenreId());
  if (queryParams.GetMovieId() != -1)
    AddOption("movieid", (int)queryParams.GetMovieId());
  if (queryParams.GetMVideoId() != -1)
    AddOption("musicvideoid", (int)queryParams.GetMVideoId());
  if (queryParams.GetSeason() != -1 && queryParams.GetSeason() >= -2)
    AddOption("season", (int)queryParams.GetSeason());
  if (queryParams.GetSetId() != -1)
    AddOption("setid", (int)queryParams.GetSetId());
  if (queryParams.GetStudioId() != -1)
    AddOption("studioid", (int)queryParams.GetStudioId());
  if (queryParams.GetTvShowId() != -1)
    AddOption("tvshowid", (int)queryParams.GetTvShowId());
  if (queryParams.GetYear() != -1)
    AddOption("year", (int)queryParams.GetYear());
  if (queryParams.GetVideoVersionId() != -1)
    AddOption("videoversionid", (int)queryParams.GetVideoVersionId());
  if (queryParams.GetVideoAssetType() != -1)
    AddOption("assetType", static_cast<int>(queryParams.GetVideoAssetType()));
  if (queryParams.GetVideoAssetId() != -1)
    AddOption("assetid", static_cast<int>(queryParams.GetVideoAssetId()));

  return true;
}

bool CVideoDbUrl::validateOption(const std::string &key, const CVariant &value)
{
  if (!CDbUrl::validateOption(key, value))
    return false;

  // if the value is empty it will remove the option which is ok
  // otherwise we only care about the "filter" option here
  if (value.empty() || !StringUtils::EqualsNoCase(key, "filter"))
    return true;

  if (!value.isString())
    return false;

  PLAYLIST::CSmartPlaylist xspFilter;
  if (!xspFilter.LoadFromJson(value.asString()))
    return false;

  // check if the filter playlist matches the item type
  return (xspFilter.GetType() == m_itemType ||
         (xspFilter.GetType() == CONTENT::MOVIES && m_itemType == CONTENT::SETS));
}
