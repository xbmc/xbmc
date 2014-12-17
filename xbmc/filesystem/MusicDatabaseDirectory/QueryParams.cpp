/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
