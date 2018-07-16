/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "QueryParams.h"
#include <stdlib.h>

using namespace XFILE::MUSICDATABASEDIRECTORY;

CQueryParams::CQueryParams()
{
  m_idArtist=-1;
  m_idAlbum=-1;
  m_idGenre=-1;
  m_idSong=-1;
  m_year=-1;
}

void CQueryParams::SetQueryParam(NODE_TYPE NodeType, const std::string& strNodeName)
{
  long idDb=atol(strNodeName.c_str());

  switch (NodeType)
  {
  case NODE_TYPE_GENRE:
    m_idGenre=idDb;
    break;
  case NODE_TYPE_YEAR:
    m_year=idDb;
    break;
  case NODE_TYPE_ARTIST:
    m_idArtist=idDb;
    break;
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED:
  case NODE_TYPE_ALBUM_RECENTLY_ADDED:
  case NODE_TYPE_ALBUM_COMPILATIONS:
  case NODE_TYPE_ALBUM_TOP100:
  case NODE_TYPE_ALBUM:
  case NODE_TYPE_YEAR_ALBUM:
    m_idAlbum=idDb;
    break;
  case NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS:
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS:
  case NODE_TYPE_ALBUM_TOP100_SONGS:
  case NODE_TYPE_ALBUM_COMPILATIONS_SONGS:
  case NODE_TYPE_YEAR_SONG:
  case NODE_TYPE_SONG:
  case NODE_TYPE_SONG_TOP100:
    m_idSong=idDb;
  default:
    break;
  }
}
