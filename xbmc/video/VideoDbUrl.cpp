/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoDbUrl.h"

#include "filesystem/VideoDatabaseDirectory.h"
#include "playlists/SmartPlayList.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

using namespace XFILE;

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
  VIDEODATABASEDIRECTORY::NODE_TYPE dirType = CVideoDatabaseDirectory::GetDirectoryType(path);
  VIDEODATABASEDIRECTORY::NODE_TYPE childType = CVideoDatabaseDirectory::GetDirectoryChildType(path);

  switch (dirType)
  {
    case VIDEODATABASEDIRECTORY::NODE_TYPE_MOVIES_OVERVIEW:
    case VIDEODATABASEDIRECTORY::NODE_TYPE_RECENTLY_ADDED_MOVIES:
    case VIDEODATABASEDIRECTORY::NODE_TYPE_TITLE_MOVIES:
    case VIDEODATABASEDIRECTORY::NODE_TYPE_SETS:
      m_type = "movies";
      break;

    case VIDEODATABASEDIRECTORY::NODE_TYPE_TVSHOWS_OVERVIEW:
    case VIDEODATABASEDIRECTORY::NODE_TYPE_TITLE_TVSHOWS:
    case VIDEODATABASEDIRECTORY::NODE_TYPE_SEASONS:
    case VIDEODATABASEDIRECTORY::NODE_TYPE_EPISODES:
    case VIDEODATABASEDIRECTORY::NODE_TYPE_RECENTLY_ADDED_EPISODES:
    case VIDEODATABASEDIRECTORY::NODE_TYPE_INPROGRESS_TVSHOWS:
      m_type = "tvshows";
      break;


    case VIDEODATABASEDIRECTORY::NODE_TYPE_MUSICVIDEOS_OVERVIEW:
    case VIDEODATABASEDIRECTORY::NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS:
    case VIDEODATABASEDIRECTORY::NODE_TYPE_TITLE_MUSICVIDEOS:
    case VIDEODATABASEDIRECTORY::NODE_TYPE_MUSICVIDEOS_ALBUM:
      m_type = "musicvideos";

    default:
      break;
  }

  switch (childType)
  {
    case VIDEODATABASEDIRECTORY::NODE_TYPE_MOVIES_OVERVIEW:
    case VIDEODATABASEDIRECTORY::NODE_TYPE_TITLE_MOVIES:
    case VIDEODATABASEDIRECTORY::NODE_TYPE_RECENTLY_ADDED_MOVIES:
      m_type = "movies";
      m_itemType = "movies";
      break;

    case VIDEODATABASEDIRECTORY::NODE_TYPE_TVSHOWS_OVERVIEW:
    case VIDEODATABASEDIRECTORY::NODE_TYPE_TITLE_TVSHOWS:
    case VIDEODATABASEDIRECTORY::NODE_TYPE_INPROGRESS_TVSHOWS:
      m_type = "tvshows";
      m_itemType = "tvshows";
      break;

    case VIDEODATABASEDIRECTORY::NODE_TYPE_SEASONS:
      m_type = "tvshows";
      m_itemType = "seasons";
      break;

    case VIDEODATABASEDIRECTORY::NODE_TYPE_EPISODES:
    case VIDEODATABASEDIRECTORY::NODE_TYPE_RECENTLY_ADDED_EPISODES:
      m_type = "tvshows";
      m_itemType = "episodes";
      break;

    case VIDEODATABASEDIRECTORY::NODE_TYPE_MUSICVIDEOS_OVERVIEW:
    case VIDEODATABASEDIRECTORY::NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS:
    case VIDEODATABASEDIRECTORY::NODE_TYPE_TITLE_MUSICVIDEOS:
      m_type = "musicvideos";
      m_itemType = "musicvideos";
      break;

    case VIDEODATABASEDIRECTORY::NODE_TYPE_GENRE:
      m_itemType = "genres";
      break;

    case VIDEODATABASEDIRECTORY::NODE_TYPE_ACTOR:
      m_itemType = "actors";
      break;

    case VIDEODATABASEDIRECTORY::NODE_TYPE_YEAR:
      m_itemType = "years";
      break;

    case VIDEODATABASEDIRECTORY::NODE_TYPE_DIRECTOR:
      m_itemType = "directors";
      break;

    case VIDEODATABASEDIRECTORY::NODE_TYPE_STUDIO:
      m_itemType = "studios";
      break;

    case VIDEODATABASEDIRECTORY::NODE_TYPE_COUNTRY:
      m_itemType = "countries";
      break;

    case VIDEODATABASEDIRECTORY::NODE_TYPE_SETS:
      m_itemType = "sets";
      break;

    case VIDEODATABASEDIRECTORY::NODE_TYPE_MUSICVIDEOS_ALBUM:
      m_type = "musicvideos";
      m_itemType = "albums";
      break;

    case VIDEODATABASEDIRECTORY::NODE_TYPE_TAGS:
      m_itemType = "tags";
      break;

    case VIDEODATABASEDIRECTORY::NODE_TYPE_VIDEOVERSIONS:
      m_itemType = "videoversions";
      break;

    case VIDEODATABASEDIRECTORY::NODE_TYPE_ROOT:
    case VIDEODATABASEDIRECTORY::NODE_TYPE_OVERVIEW:
    default:
      return false;
  }

  if (m_type.empty() || m_itemType.empty())
    return false;

  // parse query params
  VIDEODATABASEDIRECTORY::CQueryParams queryParams;
  if (!CVideoDatabaseDirectory::GetQueryParams(path, queryParams))
    return false;

  // retrieve and parse all options
  AddOptions(m_url.GetOptions());

  // add options based on the QueryParams
  if (queryParams.GetActorId() != -1)
  {
    std::string optionName = "actorid";
    if (m_type == "musicvideos")
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

  CSmartPlaylist xspFilter;
  if (!xspFilter.LoadFromJson(value.asString()))
    return false;

  // check if the filter playlist matches the item type
  return (xspFilter.GetType() == m_itemType ||
         (xspFilter.GetType() == "movies" && m_itemType == "sets"));
}
