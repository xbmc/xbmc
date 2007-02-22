#include "../../stdafx.h"
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
  case NODE_TYPE_TITLE:
    m_idMovie = idDb;
  }
}
