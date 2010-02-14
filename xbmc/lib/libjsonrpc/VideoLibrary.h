#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "StdString.h"
#include "JSONRPC.h"
#include "FileItemHandler.h"

/*
  bool GetGenresNav(const CStdString& strBaseDir, CFileItemList& items, int idContent=-1);
  bool GetStudiosNav(const CStdString& strBaseDir, CFileItemList& items, int idContent=-1);
  bool GetActorsNav(const CStdString& strBaseDir, CFileItemList& items, int idContent=-1);
  bool GetDirectorsNav(const CStdString& strBaseDir, CFileItemList& items, int idContent=-1);
  bool GetWritersNav(const CStdString& strBaseDir, CFileItemList& items, int idContent=-1);
  bool GetYearsNav(const CStdString& strBaseDir, CFileItemList& items, int idContent=-1);
  bool GetSetsNav(const CStdString& strBaseDir, CFileItemList& items, int idContent=-1, const CStdString &where = "");
  bool GetMusicVideoAlbumsNav(const CStdString& strBaseDir, CFileItemList& items, int idArtist);

  bool GetMoviesNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre=-1, int idYear=-1, int idActor=-1, int idDirector=-1, int idStudio=-1, int idSet=-1);
  bool GetTvShowsNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre=-1, int idYear=-1, int idActor=-1, int idDirector=-1, int idStudio=-1);
  bool GetSeasonsNav(const CStdString& strBaseDir, CFileItemList& items, int idActor=-1, int idDirector=-1, int idGenre=-1, int idYear=-1, int idShow=-1);
  bool GetEpisodesNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre=-1, int idYear=-1, int idActor=-1, int idDirector=-1, int idShow=-1, int idSeason=-1);
  bool GetMusicVideosNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre=-1, int idYear=-1, int idArtist=-1, int idDirector=-1, int idStudio=-1, int idAlbum=-1);

  bool GetRecentlyAddedMoviesNav(const CStdString& strBaseDir, CFileItemList& items);
  bool GetRecentlyAddedEpisodesNav(const CStdString& strBaseDir, CFileItemList& items);
  bool GetRecentlyAddedMusicVideosNav(const CStdString& strBaseDir, CFileItemList& items);
*/

namespace JSONRPC
{
  class CVideoLibrary : public CFileItemHandler
  {
  public:
    static JSON_STATUS GetMovies(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result);

    static JSON_STATUS GetTVShows(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result);
    static JSON_STATUS GetSeasons(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result);
    static JSON_STATUS GetEpisodes(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result);

    static JSON_STATUS GetMusicVideoAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result);
    static JSON_STATUS GetMusicVideos(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result);

    static JSON_STATUS GetMovieInfo(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result);
    static JSON_STATUS GetTVShowInfo(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result);
    static JSON_STATUS GetEpisodeInfo(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result);
    static JSON_STATUS GetMusicVideoInfo(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result);
  };
}
