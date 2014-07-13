#pragma once
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
    };
  }
}
