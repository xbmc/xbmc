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

#include "VideoLibrary.h"
#include "JSONUtils.h"
#include "video/VideoDatabase.h"
#include "Util.h"
#include "Application.h"

using namespace Json;
using namespace JSONRPC;

JSON_STATUS CVideoLibrary::GetMovies(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull()))
    return InvalidParams;

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

//  int genreID = parameterObject.get("genreid", -1).asInt();

  CFileItemList items;
  if (videodatabase.GetMoviesNav("videodb://", items))
    HandleFileItemList("movieid", true, "movies", items, parameterObject, result);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetMovieDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull() || parameterObject.isMember("movieid")))
    return InvalidParams;

  int id = parameterObject.get("movieid", -1).asInt();
  if (id < 0)
    return InvalidParams;

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CVideoInfoTag infos;
  videodatabase.GetMovieInfo("", infos, id);
  if (infos.m_iDbId <= 0)
  {
    videodatabase.Close();
    return InvalidParams;
  }

  Json::Value validFields = Value(arrayValue);
  MakeFieldsList(parameterObject, validFields);
  HandleFileItem("movieid", true, "moviedetails", CFileItemPtr(new CFileItem(infos)), parameterObject, validFields, result);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetTVShows(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull()))
    return InvalidParams;

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

//  int genreID = parameterObject.get("genreid", -1).asInt();

  CFileItemList items;
  if (videodatabase.GetTvShowsNav("videodb://", items))
    HandleFileItemList("tvshowid", false, "tvshows", items, parameterObject, result);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetTVShowDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull() || parameterObject.isMember("tvshowid")))
    return InvalidParams;

  int id = parameterObject.get("tvshowid", -1).asInt();
  if (id < 0)
    return InvalidParams;

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CVideoInfoTag infos;
  videodatabase.GetTvShowInfo("", infos, id);
  if (infos.m_iDbId <= 0)
  {
    videodatabase.Close();
    return InvalidParams;
  }

  Json::Value validFields = Value(arrayValue);
  MakeFieldsList(parameterObject, validFields);
  HandleFileItem("tvshowid", true, "tvshowdetails", CFileItemPtr(new CFileItem(infos)), parameterObject, validFields, result);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetSeasons(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull()))
    return InvalidParams;

  const Value param = ForceObject(parameterObject);
  if (!ParameterIntOrNull(param, "tvshowid"))
    return InvalidParams;

  int tvshowID = ParameterAsInt(param, -1, "tvshowid");

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CFileItemList items;
  if (videodatabase.GetSeasonsNav("videodb://", items, -1, -1, -1, -1, tvshowID))
    HandleFileItemList(NULL, false, "seasons", items, param, result);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetEpisodes(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull()))
    return InvalidParams;

  const Value param = ForceObject(parameterObject);
  if (!(ParameterIntOrNull(param, "tvshowid") || ParameterIntOrNull(param, "season")))
    return InvalidParams;

  int tvshowID = ParameterAsInt(param, -1, "tvshowid");
  int season   = ParameterAsInt(param, -1, "season");

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CFileItemList items;
  if (videodatabase.GetEpisodesNav("videodb://", items, -1, -1, -1, -1, tvshowID, season))
    HandleFileItemList("episodeid", true, "episodes", items, param, result);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetEpisodeDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull() || parameterObject.isMember("episodeid")))
    return InvalidParams;

  int id = parameterObject.get("episodeid", -1).asInt();
  if (id < 0)
    return InvalidParams;

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CVideoInfoTag infos;
  videodatabase.GetEpisodeInfo("", infos, id);
  if (infos.m_iDbId <= 0)
  {
    videodatabase.Close();
    return InvalidParams;
  }

  Json::Value validFields = Value(arrayValue);
  MakeFieldsList(parameterObject, validFields);
  HandleFileItem("episodeid", true, "episodedetails", CFileItemPtr(new CFileItem(infos)), parameterObject, validFields, result);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetMusicVideos(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull()))
    return InvalidParams;

  const Value param = ForceObject(parameterObject);
  if (!(ParameterIntOrNull(param, "artistid") || ParameterIntOrNull(param, "albumid")))
    return InvalidParams;

  int artistID = ParameterAsInt(param, -1, "artistid");
  int albumID  = ParameterAsInt(param, -1, "albumid");

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CFileItemList items;
  if (videodatabase.GetMusicVideosNav("videodb://", items, -1, -1, artistID, -1, -1, albumID))
    HandleFileItemList("musicvideoid", true, "musicvideos", items, param, result);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetMusicVideoDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull() || parameterObject.isMember("musicvideoid")))
    return InvalidParams;

  int id = parameterObject.get("musicvideoid", -1).asInt();
  if (id < 0)
    return InvalidParams;

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CVideoInfoTag infos;
  videodatabase.GetMusicVideoInfo("", infos, id);
  if (infos.m_iDbId <= 0)
  {
    videodatabase.Close();
    return InvalidParams;
  }

  Json::Value validFields = Value(arrayValue);
  MakeFieldsList(parameterObject, validFields);
  HandleFileItem("musicvideoid", true, "musicvideodetails", CFileItemPtr(new CFileItem(infos)), parameterObject, validFields, result);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetRecentlyAddedMovies(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CFileItemList items;
  if (videodatabase.GetRecentlyAddedMoviesNav("videodb://", items))
    HandleFileItemList("movieid", true, "movies", items, parameterObject, result);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetRecentlyAddedEpisodes(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CFileItemList items;
  if (videodatabase.GetRecentlyAddedEpisodesNav("videodb://", items))
    HandleFileItemList("episodeid", true, "episodes", items, parameterObject, result);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetRecentlyAddedMusicVideos(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CFileItemList items;
  if (videodatabase.GetRecentlyAddedMusicVideosNav("videodb://", items))
    HandleFileItemList("musicvideoid", true, "musicvideos", items, parameterObject, result);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::ScanForContent(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_application.getApplicationMessenger().ExecBuiltIn("updatelibrary(video)");
  return ACK;
}

bool CVideoLibrary::FillFileItemList(const Value &parameterObject, CFileItemList &list)
{
  CVideoDatabase videodatabase;
  if ((parameterObject["movieid"].isInt() || parameterObject["episodeid"].isInt() || parameterObject["musicvideoid"].isInt()) && videodatabase.Open())
  {
    int movieID       = ParameterAsInt(parameterObject, -1, "movieid");
    int episodeID     = ParameterAsInt(parameterObject, -1, "episodeid");
    int musicVideoID  = ParameterAsInt(parameterObject, -1, "musicvideoid");

    bool success = true;
    if (movieID > 0)
    {
      CVideoInfoTag details;
      videodatabase.GetMovieInfo("", details, movieID);
      if (!details.IsEmpty())
      {
        CFileItemPtr item = CFileItemPtr(new CFileItem(details));
        list.Add(item);
        success &= true;
      }
      success = false;
    }
    if (episodeID > 0)
    {
      CVideoInfoTag details;
      if (videodatabase.GetEpisodeInfo("", details, episodeID) && !details.IsEmpty())
      {
        CFileItemPtr item = CFileItemPtr(new CFileItem(details));
        list.Add(item);
        success &= true;
      }
      success = false;
    }
    if (musicVideoID > 0)
    {
      CVideoInfoTag details;
      videodatabase.GetMusicVideoInfo("", details, musicVideoID);
      if (!details.IsEmpty())
      {
        CFileItemPtr item = CFileItemPtr(new CFileItem(details));
        list.Add(item);
        success &= true;
      }
      success = false;
    }

    videodatabase.Close();
    return success;
  }

  return false;
}
