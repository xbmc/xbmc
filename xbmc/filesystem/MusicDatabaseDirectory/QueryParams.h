/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace XFILE
{
  namespace MUSICDATABASEDIRECTORY
  {

  enum class NodeType;

  class CQueryParams
  {
  public:
    CQueryParams();
    int GetArtistId() { return m_idArtist; }
    int GetAlbumId() { return m_idAlbum; }
    int GetGenreId() { return m_idGenre; }
    int GetSongId() { return m_idSong; }
    int GetYear() { return m_year; }
    int GetDisc() { return m_disc; }

  protected:
    void SetQueryParam(NodeType NodeType, const std::string& strNodeName);

    friend class CDirectoryNode;

  private:
    int m_idArtist;
    int m_idAlbum;
    int m_idGenre;
    int m_idSong;
    int m_year;
    int m_disc;
  };
  }
}


