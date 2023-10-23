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
      long GetCountryId() const { return m_idCountry; }
      long GetActorId() const { return m_idActor; }
      long GetAlbumId() const { return m_idAlbum; }
      long GetDirectorId() const { return m_idDirector; }
      long GetTvShowId() const { return m_idShow; }
      long GetSeason() const { return m_idSeason; }
      long GetEpisodeId() const { return m_idEpisode; }
      long GetStudioId() const { return m_idStudio; }
      long GetMVideoId() const { return m_idMVideo; }
      long GetSetId() const { return m_idSet; }
      long GetTagId() const { return m_idTag; }
      long GetVideoVersionId() const { return m_idVideoVersion; }

    protected:
      void SetQueryParam(NODE_TYPE NodeType, const std::string& strNodeName);

      friend class CDirectoryNode;
    private:
      long m_idContent;
      long m_idMovie;
      long m_idGenre;
      long m_idCountry;
      long m_idYear;
      long m_idActor;
      long m_idDirector;
      long m_idShow;
      long m_idSeason;
      long m_idEpisode;
      long m_idStudio;
      long m_idMVideo;
      long m_idAlbum;
      long m_idSet;
      long m_idTag;
      long m_idVideoVersion{-1};
    };
  }
}
