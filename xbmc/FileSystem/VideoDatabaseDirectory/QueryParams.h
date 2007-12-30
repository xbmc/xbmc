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
      long GetContentType() const { return m_idContent; }
      long GetMovieId() const { return m_idMovie; }
      long GetYear() const { return m_idYear; }
      long GetGenreId() const { return m_idGenre; }
      long GetActorId() const { return m_idActor; }
      long GetDirectorId() const { return m_idDirector; }
      long GetTvShowId() const { return m_idShow; }
      long GetSeason() const { return m_idSeason; }
      long GetEpisodeId() const { return m_idEpisode; }
      long GetStudioId() const { return m_idStudio; }
      long GetMVideoId() const { return m_idMVideo; }

    protected:
      void SetQueryParam(NODE_TYPE NodeType, const CStdString& strNodeName);

      friend class CDirectoryNode;
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
      long m_idMVideo;
    };
  }
}
