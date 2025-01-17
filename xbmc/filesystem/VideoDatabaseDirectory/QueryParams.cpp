/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "QueryParams.h"

#include "DirectoryNode.h"
#include "video/VideoDatabase.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

CQueryParams::CQueryParams()
{
  m_idMovie = -1;
  m_idGenre = -1;
  m_idCountry = -1;
  m_idYear = -1;
  m_idActor = -1;
  m_idDirector = -1;
  m_idContent = static_cast<long>(VideoDbContentType::UNKNOWN);
  m_idShow = -1;
  m_idSeason = -1;
  m_idEpisode = -1;
  m_idStudio = -1;
  m_idMVideo = -1;
  m_idAlbum = -1;
  m_idSet = -1;
  m_idTag = -1;
}

void CQueryParams::SetQueryParam(NodeType nodeType, const std::string& strNodeName)
{
  long idDb=atol(strNodeName.c_str());

  switch (nodeType)
  {
    case NodeType::OVERVIEW:
      if (strNodeName == "tvshows")
        m_idContent = static_cast<long>(VideoDbContentType::TVSHOWS);
      else if (strNodeName == "musicvideos")
        m_idContent = static_cast<long>(VideoDbContentType::MUSICVIDEOS);
      else
        m_idContent = static_cast<long>(VideoDbContentType::MOVIES);
      break;
    case NodeType::GENRE:
      m_idGenre = idDb;
      break;
    case NodeType::COUNTRY:
      m_idCountry = idDb;
      break;
    case NodeType::YEAR:
      m_idYear = idDb;
      break;
    case NodeType::ACTOR:
      m_idActor = idDb;
      break;
    case NodeType::DIRECTOR:
      m_idDirector = idDb;
      break;
    case NodeType::TITLE_MOVIES:
    case NodeType::RECENTLY_ADDED_MOVIES:
      m_idMovie = idDb;
      break;
    case NodeType::TITLE_TVSHOWS:
    case NodeType::INPROGRESS_TVSHOWS:
      m_idShow = idDb;
      break;
    case NodeType::SEASONS:
      m_idSeason = idDb;
      break;
    case NodeType::EPISODES:
    case NodeType::RECENTLY_ADDED_EPISODES:
      m_idEpisode = idDb;
      break;
    case NodeType::STUDIO:
      m_idStudio = idDb;
      break;
    case NodeType::TITLE_MUSICVIDEOS:
    case NodeType::RECENTLY_ADDED_MUSICVIDEOS:
      m_idMVideo = idDb;
      break;
    case NodeType::MUSICVIDEOS_ALBUM:
      m_idAlbum = idDb;
      break;
    case NodeType::SETS:
      m_idSet = idDb;
      break;
    case NodeType::TAGS:
      m_idTag = idDb;
      break;
    case NodeType::VIDEOVERSIONS:
      m_idVideoVersion = idDb;
      break;
    case NodeType::MOVIE_ASSET_TYPES:
      m_videoAssetType = idDb;
      break;
    case NodeType::MOVIE_ASSETS:
      m_idVideoAsset = idDb;
      break;
    default:
      break;
  }
}
