#pragma once
#include "DirectoryNode.h"

namespace DIRECTORY
{
  namespace VIDEODATABASEDIRECTORY
  {
    class CQueryParams
    {
    public:
      CQueryParams();
      long GetMovieId() { return m_idMovie; }
      long GetYear() { return m_idYear; }
      long GetGenreId() { return m_idGenre; }
      long GetActorId() { return m_idActor; }
      long GetDirectorId() { return m_idDirector; }

    protected:
      void SetQueryParam(NODE_TYPE NodeType, const CStdString& strNodeName);

      friend CDirectoryNode;
    private:
      long m_idMovie;
      long m_idGenre;
      long m_idYear;
      long m_idActor;
      long m_idDirector;
    };
  };
};