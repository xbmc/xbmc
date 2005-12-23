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
      void SetQueryParam(NODE_TYPE NodeType, const CStdString& strNodeName);

      long GetArtistId() { return m_idArtist; }
      long GetAlbumId() { return m_idAlbum; }
      long GetGenreId() { return m_idGenre; }
    private:
      long m_idArtist;
      long m_idAlbum;
      long m_idGenre;
    };
  };
};

