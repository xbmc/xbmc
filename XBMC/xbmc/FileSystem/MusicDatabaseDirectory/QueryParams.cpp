#include "../../stdafx.h"
#include "QueryParams.h"

using namespace DIRECTORY::MUSICDATABASEDIRECTORY;

CQueryParams::CQueryParams()
{
  m_idArtist=-1;
  m_idAlbum=-1;
  m_idGenre=-1;
}

void CQueryParams::SetQueryParam(NODE_TYPE NodeType, const CStdString& strNodeName)
{
  long idDb=atol(strNodeName.c_str());

  switch (NodeType)
  {
  case NODE_TYPE_GENRE:
    m_idGenre=idDb;
    break;
  case NODE_TYPE_ARTIST:
    m_idArtist=idDb;
    break;
  case NODE_TYPE_ALBUM:
  case NODE_TYPE_ALBUM_RECENTLY_ADDED:
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED:
  case NODE_TYPE_ALBUM_TOP100:
    m_idAlbum=idDb;
    break;
  }
}
