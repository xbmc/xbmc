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
#include "utils/URIUtils.h"
#include "Application.h"

using namespace JSONRPC;

JSON_STATUS CVideoLibrary::GetMovies(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CFileItemList items;
  JSON_STATUS ret = OK;
  if (videodatabase.GetMoviesByWhere("videodb://1/", "", "", items))
    ret = GetAdditionalMovieDetails(parameterObject, items, result, videodatabase);

  videodatabase.Close();
  return ret;
}

JSON_STATUS CVideoLibrary::GetMovieDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["movieid"].asInteger();

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

  HandleFileItem("movieid", true, "moviedetails", CFileItemPtr(new CFileItem(infos)), parameterObject, parameterObject["properties"], result, false);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetMovieSets(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CFileItemList items;
  if (videodatabase.GetSetsNav("videodb://1/7/", items, VIDEODB_CONTENT_MOVIES))
    HandleFileItemList("setid", false, "sets", items, parameterObject, result);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetMovieSetDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["setid"].asInteger();

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  // Get movie set details
  CVideoInfoTag infos;
  videodatabase.GetSetInfo(id, infos);
  if (infos.m_iDbId <= 0)
  {
    videodatabase.Close();
    return InvalidParams;
  }

  HandleFileItem("setid", false, "setdetails", CFileItemPtr(new CFileItem(infos)), parameterObject, parameterObject["properties"], result, false);

  // Get movies from the set
  CFileItemList items;
  JSON_STATUS ret = OK;
  if (videodatabase.GetMoviesNav("", items, -1, -1, -1, -1, -1, -1, id))
    ret = GetAdditionalMovieDetails(parameterObject["movies"], items, result["setdetails"]["items"], videodatabase);

  videodatabase.Close();
  return ret;
}

JSON_STATUS CVideoLibrary::GetTVShows(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CFileItemList items;
  if (videodatabase.GetTvShowsNav("videodb://2/", items))
  {
    bool additionalInfo = false;
    for (CVariant::const_iterator_array itr = parameterObject["properties"].begin_array(); itr != parameterObject["properties"].end_array(); itr++)
    {
      CStdString fieldValue = itr->asString();
      if (fieldValue == "cast")
        additionalInfo = true;
    }

    if (additionalInfo)
    {
      for (int index = 0; index < items.Size(); index++)
        videodatabase.GetTvShowInfo("", *(items[index]->GetVideoInfoTag()), items[index]->GetVideoInfoTag()->m_iDbId);
    }
    HandleFileItemList("tvshowid", true, "tvshows", items, parameterObject, result);
  }

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetTVShowDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["tvshowid"].asInteger();

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

  HandleFileItem("tvshowid", true, "tvshowdetails", CFileItemPtr(new CFileItem(infos)), parameterObject, parameterObject["properties"], result, false);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetSeasons(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int tvshowID = (int)parameterObject["tvshowid"].asInteger();

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CStdString strPath;
  strPath.Format("videodb://2/2/%i/", tvshowID);
  CFileItemList items;
  if (videodatabase.GetSeasonsNav(strPath, items, -1, -1, -1, -1, tvshowID))
    HandleFileItemList(NULL, false, "seasons", items, parameterObject, result);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetEpisodes(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int tvshowID = (int)parameterObject["tvshowid"].asInteger();
  int season   = (int)parameterObject["season"].asInteger();

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CStdString strPath;
  strPath.Format("videodb://2/2/%i/%i/", tvshowID, season);
  CFileItemList items;
  if (videodatabase.GetEpisodesNav(strPath, items, -1, -1, -1, -1, tvshowID, season))
    GetAdditionalEpisodeDetails(parameterObject, items, result, videodatabase);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetEpisodeDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["episodeid"].asInteger();

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
  CFileItemPtr pItem = CFileItemPtr(new CFileItem(infos));
  // We need to set the correct base path to get the valid fanart
  int tvshowid = infos.m_iIdShow;
  if (tvshowid <= 0)
    tvshowid = videodatabase.GetTvShowForEpisode(id);
  CStdString basePath; basePath.Format("videodb://2/2/%ld/%ld/%ld", tvshowid, infos.m_iSeason, id);
  pItem->SetPath(basePath);

  HandleFileItem("episodeid", true, "episodedetails", pItem, parameterObject, parameterObject["properties"], result, false);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetMusicVideos(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int artistID = (int)parameterObject["artistid"].asInteger();
  int albumID  = (int)parameterObject["albumid"].asInteger();

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CFileItemList items;
  if (videodatabase.GetMusicVideosNav("videodb://3/", items, -1, -1, artistID, -1, -1, albumID))
    GetAdditionalMusicVideoDetails(parameterObject, items, result, videodatabase);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetMusicVideoDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["musicvideoid"].asInteger();

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

  HandleFileItem("musicvideoid", true, "musicvideodetails", CFileItemPtr(new CFileItem(infos)), parameterObject, parameterObject["properties"], result, false);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetRecentlyAddedMovies(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CFileItemList items;
  if (videodatabase.GetRecentlyAddedMoviesNav("videodb://4/", items))
    GetAdditionalMovieDetails(parameterObject, items, result, videodatabase);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetRecentlyAddedEpisodes(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CFileItemList items;
  if (videodatabase.GetRecentlyAddedEpisodesNav("videodb://5/", items))
    GetAdditionalEpisodeDetails(parameterObject, items, result, videodatabase);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetRecentlyAddedMusicVideos(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CFileItemList items;
  if (videodatabase.GetRecentlyAddedMusicVideosNav("videodb://6/", items))
    GetAdditionalMusicVideoDetails(parameterObject, items, result, videodatabase);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetGenres(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CStdString media = parameterObject["type"].asString();
  media = media.ToLower();
  int idContent = -1;

  CStdString strPath = "videodb://";
  /* select which video content to get genres from*/
  if (media.Equals("movie"))
  {
    idContent = VIDEODB_CONTENT_MOVIES;
    strPath += "1";
  }
  else if (media.Equals("tvshow"))
  {
    idContent = VIDEODB_CONTENT_TVSHOWS;
    strPath += "2";
  }
  else if (media.Equals("musicvideo"))
  {
    idContent = VIDEODB_CONTENT_MUSICVIDEOS;
    strPath += "3";
  }
  strPath += "/1/";
 
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CFileItemList items;
  if (videodatabase.GetGenresNav(strPath, items, idContent))
  {
    /* need to set strTitle in each item*/
    for (unsigned int i = 0; i < (unsigned int)items.Size(); i++)
      items[i]->GetVideoInfoTag()->m_strTitle = items[i]->GetLabel();
 
    HandleFileItemList("genreid", false, "genres", items, parameterObject, result);
  }

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::Scan(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  g_application.getApplicationMessenger().ExecBuiltIn("updatelibrary(video)");
  return ACK;
}

JSON_STATUS CVideoLibrary::Export(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CStdString cmd;
  if (parameterObject["options"].isMember("path"))
    cmd.Format("exportlibrary(video, false, %s)", parameterObject["options"]["path"].asString());
  else
    cmd.Format("exportlibrary(video, true, %s, %s, %s)",
      parameterObject["options"]["images"].asBoolean() ? "true" : "false",
      parameterObject["options"]["overwrite"].asBoolean() ? "true" : "false",
      parameterObject["options"]["actorthumbs"].asBoolean() ? "true" : "false");

  g_application.getApplicationMessenger().ExecBuiltIn(cmd);
  return ACK;
}

JSON_STATUS CVideoLibrary::Clean(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  g_application.getApplicationMessenger().ExecBuiltIn("cleanlibrary(video)");
  return ACK;
}

bool CVideoLibrary::FillFileItem(const CStdString &strFilename, CFileItem &item)
{
  CVideoDatabase videodatabase;
  bool status = false;
  if (!strFilename.empty() && videodatabase.Open())
  {
    CVideoInfoTag details;
    if (videodatabase.LoadVideoInfo(strFilename, details))
    {
      item = CFileItem(details);
      status = true;
    }

    videodatabase.Close();
  }

  return status;
}

bool CVideoLibrary::FillFileItemList(const CVariant &parameterObject, CFileItemList &list)
{
  CVideoDatabase videodatabase;
  if (videodatabase.Open())
  {
    CStdString file = parameterObject["file"].asString();
    int movieID = (int)parameterObject["movieid"].asInteger(-1);
    int episodeID = (int)parameterObject["episodeid"].asInteger(-1);
    int musicVideoID = (int)parameterObject["musicvideoid"].asInteger(-1);

    bool success = false;
    CFileItem fileItem;
    if (FillFileItem(file, fileItem))
    {
      success = true;
      list.Add(CFileItemPtr(new CFileItem(fileItem)));
    }

    if (movieID > 0)
    {
      CVideoInfoTag details;
      videodatabase.GetMovieInfo("", details, movieID);
      if (!details.IsEmpty())
      {
        list.Add(CFileItemPtr(new CFileItem(details)));
        success = true;
      }
    }
    if (episodeID > 0)
    {
      CVideoInfoTag details;
      if (videodatabase.GetEpisodeInfo("", details, episodeID) && !details.IsEmpty())
      {
        list.Add(CFileItemPtr(new CFileItem(details)));
        success = true;
      }
    }
    if (musicVideoID > 0)
    {
      CVideoInfoTag details;
      videodatabase.GetMusicVideoInfo("", details, musicVideoID);
      if (!details.IsEmpty())
      {
        list.Add(CFileItemPtr(new CFileItem(details)));
        success = true;
      }
    }

    videodatabase.Close();
    return success;
  }

  return false;
}

JSON_STATUS CVideoLibrary::GetAdditionalMovieDetails(const CVariant &parameterObject, CFileItemList &items, CVariant &result, CVideoDatabase &videodatabase)
{
  if (!videodatabase.Open())
    return InternalError;

  bool additionalInfo = false;
  for (CVariant::const_iterator_array itr = parameterObject["properties"].begin_array(); itr != parameterObject["properties"].end_array(); itr++)
  {
    CStdString fieldValue = itr->asString();
    if (fieldValue == "cast" || fieldValue == "set" || fieldValue == "setid" || fieldValue == "showlink" || fieldValue == "resume")
      additionalInfo = true;
  }

  if (additionalInfo)
  {
    for (int index = 0; index < items.Size(); index++)
      videodatabase.GetMovieInfo("", *(items[index]->GetVideoInfoTag()), items[index]->GetVideoInfoTag()->m_iDbId);
  }
  HandleFileItemList("movieid", true, "movies", items, parameterObject, result);

  return OK;
}

JSON_STATUS CVideoLibrary::GetAdditionalEpisodeDetails(const CVariant &parameterObject, CFileItemList &items, CVariant &result, CVideoDatabase &videodatabase)
{
  if (!videodatabase.Open())
    return InternalError;

  bool additionalInfo = false;
  for (CVariant::const_iterator_array itr = parameterObject["properties"].begin_array(); itr != parameterObject["properties"].end_array(); itr++)
  {
    CStdString fieldValue = itr->asString();
    if (fieldValue == "cast" || fieldValue == "resume")
      additionalInfo = true;
  }

  if (additionalInfo)
  {
    for (int index = 0; index < items.Size(); index++)
      videodatabase.GetEpisodeInfo("", *(items[index]->GetVideoInfoTag()), items[index]->GetVideoInfoTag()->m_iDbId);
  }
  HandleFileItemList("episodeid", true, "episodes", items, parameterObject, result);

  return OK;
}

JSON_STATUS CVideoLibrary::GetAdditionalMusicVideoDetails(const CVariant &parameterObject, CFileItemList &items, CVariant &result, CVideoDatabase &videodatabase)
{
  if (!videodatabase.Open())
    return InternalError;

  bool additionalInfo = false;
  for (CVariant::const_iterator_array itr = parameterObject["properties"].begin_array(); itr != parameterObject["properties"].end_array(); itr++)
  {
    CStdString fieldValue = itr->asString();
    if (fieldValue == "resume")
      additionalInfo = true;
  }

  if (additionalInfo)
  {
    for (int index = 0; index < items.Size(); index++)
      videodatabase.GetMusicVideoInfo("", *(items[index]->GetVideoInfoTag()), items[index]->GetVideoInfoTag()->m_iDbId);
  }
  HandleFileItemList("musicvideoid", true, "musicvideos", items, parameterObject, result);

  return OK;
}
