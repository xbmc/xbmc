#include "stdafx.h"
#include "QueryParams.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CQueryParams::CQueryParams()
{
  m_idMovie = -1;
  m_idGenre = -1;
  m_idYear = -1;
  m_idActor = -1;
  m_idDirector = -1;
  m_idContent = -1;
  m_idShow = -1;
  m_idSeason = -1;
  m_idEpisode = -1;
  m_idStudio = -1;
  m_idMVideo = -1;
}

void CQueryParams::SetQueryParam(NODE_TYPE NodeType, const CStdString& strNodeName)
{
  long idDb=atol(strNodeName.c_str());

  switch (NodeType)
  {
  case NODE_TYPE_OVERVIEW:
    m_idContent = idDb;
    break;
  case NODE_TYPE_GENRE:
    m_idGenre = idDb;
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
    m_idMovie = idDb;
    break;
  case NODE_TYPE_TITLE_TVSHOWS:
    m_idShow = idDb;
    break;
  case NODE_TYPE_SEASONS:
    m_idSeason = idDb;
    break;
  case NODE_TYPE_EPISODES:
    m_idEpisode = idDb;
    break;
  case NODE_TYPE_STUDIO:
    m_idStudio = idDb;
    break;
  case NODE_TYPE_TITLE_MUSICVIDEOS:
    m_idMVideo = idDb;
    break;
  }
}
