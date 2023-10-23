/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "QueryParams.h"

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

void CQueryParams::SetQueryParam(NODE_TYPE NodeType, const std::string& strNodeName)
{
  long idDb=atol(strNodeName.c_str());

  switch (NodeType)
  {
  case NODE_TYPE_OVERVIEW:
    if (strNodeName == "tvshows")
      m_idContent = static_cast<long>(VideoDbContentType::TVSHOWS);
    else if (strNodeName == "musicvideos")
      m_idContent = static_cast<long>(VideoDbContentType::MUSICVIDEOS);
    else
      m_idContent = static_cast<long>(VideoDbContentType::MOVIES);
    break;
  case NODE_TYPE_GENRE:
    m_idGenre = idDb;
    break;
  case NODE_TYPE_COUNTRY:
    m_idCountry = idDb;
    break;
  case NODE_TYPE_YEAR:
    m_idYear = idDb;
    break;
  case NODE_TYPE_ACTOR:
    m_idActor = idDb;
    break;
  case NODE_TYPE_DIRECTOR:
    m_idDirector = idDb;
    break;
  case NODE_TYPE_TITLE_MOVIES:
  case NODE_TYPE_RECENTLY_ADDED_MOVIES:
    m_idMovie = idDb;
    break;
  case NODE_TYPE_TITLE_TVSHOWS:
  case NODE_TYPE_INPROGRESS_TVSHOWS:
    m_idShow = idDb;
    break;
  case NODE_TYPE_SEASONS:
    m_idSeason = idDb;
    break;
  case NODE_TYPE_EPISODES:
  case NODE_TYPE_RECENTLY_ADDED_EPISODES:
    m_idEpisode = idDb;
    break;
  case NODE_TYPE_STUDIO:
    m_idStudio = idDb;
    break;
  case NODE_TYPE_TITLE_MUSICVIDEOS:
  case NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS:
    m_idMVideo = idDb;
    break;
  case NODE_TYPE_MUSICVIDEOS_ALBUM:
    m_idAlbum = idDb;
    break;
  case NODE_TYPE_SETS:
    m_idSet = idDb;
    break;
  case NODE_TYPE_TAGS:
    m_idTag = idDb;
    break;
  case NODE_TYPE_VIDEOVERSIONS:
    m_idVideoVersion = idDb;
    break;
  default:
    break;
  }
}
