#pragma once
#include "DirectoryNode.h"

namespace DIRECTORY
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

    protected:
      void SetQueryParam(NODE_TYPE NodeType, const CStdString& strNodeName);

      friend class CDirectoryNode;
    private:
      long m_idArtist;
      long m_idAlbum;
      long m_idGenre;
      long m_idSong;
      long m_year;
    };
  }
}

