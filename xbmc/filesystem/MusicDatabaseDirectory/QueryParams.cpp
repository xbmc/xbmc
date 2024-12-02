/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "QueryParams.h"

#include "DirectoryNode.h"

#include <stdlib.h>

using namespace XFILE::MUSICDATABASEDIRECTORY;

CQueryParams::CQueryParams()
{
  m_idArtist=-1;
  m_idAlbum=-1;
  m_idGenre=-1;
  m_idSong=-1;
  m_year=-1;
  m_disc = -1;
}

void CQueryParams::SetQueryParam(NodeType nodeType, const std::string& strNodeName)
{
  int idDb = atoi(strNodeName.c_str());

  switch (nodeType)
  {
    case NodeType::GENRE:
      m_idGenre = idDb;
      break;
    case NodeType::YEAR:
      m_year = idDb;
      break;
    case NodeType::ARTIST:
      m_idArtist = idDb;
      break;
    case NodeType::DISC:
      m_disc = idDb;
      break;
    case NodeType::ALBUM_RECENTLY_PLAYED:
    case NodeType::ALBUM_RECENTLY_ADDED:
    case NodeType::ALBUM_TOP100:
    case NodeType::ALBUM:
      m_idAlbum = idDb;
      break;
    case NodeType::ALBUM_RECENTLY_ADDED_SONGS:
    case NodeType::ALBUM_RECENTLY_PLAYED_SONGS:
    case NodeType::ALBUM_TOP100_SONGS:
    case NodeType::SONG:
    case NodeType::SONG_TOP100:
      m_idSong = idDb;
    default:
      break;
  }
}
