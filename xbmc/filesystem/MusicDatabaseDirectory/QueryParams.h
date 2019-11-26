/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DirectoryNode.h"

namespace XFILE
{
  namespace MUSICDATABASEDIRECTORY
  {
    class CQueryParams
    {
    public:
      CQueryParams();
      long GetArtistId() { return m_idArtist; }
      long GetAlbumId() { return m_idAlbum; }
      long GetGenreId() { return m_idGenre; }
      long GetSongId() { return m_idSong; }
      long GetYear() { return m_year; }
      long GetDisc() { return m_disc; }

    protected:
      void SetQueryParam(NODE_TYPE NodeType, const std::string& strNodeName);

      friend class CDirectoryNode;
    private:
      long m_idArtist;
      long m_idAlbum;
      long m_idGenre;
      long m_idSong;
      long m_year;
      long m_disc;
    };
  }
}


