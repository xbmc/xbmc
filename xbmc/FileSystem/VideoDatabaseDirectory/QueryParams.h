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
      long GetContentType() { return m_idContent; }
      long GetMovieId() { return m_idMovie; }
      long GetYear() { return m_idYear; }
      long GetGenreId() { return m_idGenre; }
      long GetActorId() { return m_idActor; }
      long GetDirectorId() { return m_idDirector; }
      long GetTvShowId() { return m_idShow; }
      long GetSeason() { return m_idSeason; }
      long GetEpisodeId() { return m_idEpisode; }
      long GetStudioId() { return m_idStudio; }

    protected:
      void SetQueryParam(NODE_TYPE NodeType, const CStdString& strNodeName);

      friend CDirectoryNode;
    private:
      long m_idContent;
      long m_idMovie;
      long m_idGenre;
      long m_idYear;
      long m_idActor;
      long m_idDirector;
      long m_idShow;
      long m_idSeason;
      long m_idEpisode;
      long m_idStudio;
    };
  };
};