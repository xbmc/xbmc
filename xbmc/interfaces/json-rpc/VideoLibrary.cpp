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
#include "Application.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"

using namespace JSONRPC;

JSONRPC_STATUS CVideoLibrary::GetMovies(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CFileItemList items;
  JSONRPC_STATUS ret = OK;
  if (videodatabase.GetMoviesByWhere("videodb://1/2/", "", items))
    ret = GetAdditionalMovieDetails(parameterObject, items, result, videodatabase);

  videodatabase.Close();
  return ret;
}

JSONRPC_STATUS CVideoLibrary::GetMovieDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSONRPC_STATUS CVideoLibrary::GetMovieSets(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSONRPC_STATUS CVideoLibrary::GetMovieSetDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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
  JSONRPC_STATUS ret = OK;
  if (videodatabase.GetMoviesNav("videodb://2/2/", items, -1, -1, -1, -1, -1, -1, id))
    ret = GetAdditionalMovieDetails(parameterObject["movies"], items, result["setdetails"]["items"], videodatabase);

  videodatabase.Close();
  return ret;
}

JSONRPC_STATUS CVideoLibrary::GetTVShows(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CFileItemList items;
  if (videodatabase.GetTvShowsNav("videodb://2/2/", items))
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

JSONRPC_STATUS CVideoLibrary::GetTVShowDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSONRPC_STATUS CVideoLibrary::GetSeasons(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSONRPC_STATUS CVideoLibrary::GetEpisodes(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSONRPC_STATUS CVideoLibrary::GetEpisodeDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSONRPC_STATUS CVideoLibrary::GetMusicVideos(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int artistID = (int)parameterObject["artistid"].asInteger();
  int albumID  = (int)parameterObject["albumid"].asInteger();

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CFileItemList items;
  if (videodatabase.GetMusicVideosNav("videodb://3/2/", items, -1, -1, artistID, -1, -1, albumID))
    GetAdditionalMusicVideoDetails(parameterObject, items, result, videodatabase);

  videodatabase.Close();
  return OK;
}

JSONRPC_STATUS CVideoLibrary::GetMusicVideoDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSONRPC_STATUS CVideoLibrary::GetRecentlyAddedMovies(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSONRPC_STATUS CVideoLibrary::GetRecentlyAddedEpisodes(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSONRPC_STATUS CVideoLibrary::GetRecentlyAddedMusicVideos(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSONRPC_STATUS CVideoLibrary::GetGenres(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSONRPC_STATUS CVideoLibrary::SetMovieDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

  std::map<std::string, std::string> artwork;
  videodatabase.GetArtForItem(infos.m_iDbId, infos.m_type, artwork);

  int playcount = infos.m_playCount;
  CDateTime lastPlayed = infos.m_lastPlayed;

  UpdateVideoTag(parameterObject, infos, artwork);

  if (videodatabase.SetDetailsForMovie(infos.m_strFileNameAndPath, infos, artwork, id) > 0)
  {
    if (playcount != infos.m_playCount || lastPlayed != infos.m_lastPlayed)
      videodatabase.SetPlayCount(CFileItem(infos), infos.m_playCount, infos.m_lastPlayed);
    return ACK;
  }

  return InternalError;
}

JSONRPC_STATUS CVideoLibrary::SetTVShowDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

  std::map<std::string, std::string> artwork;
  videodatabase.GetArtForItem(infos.m_iDbId, infos.m_type, artwork);

  std::map<int, std::string> seasonArt;
  videodatabase.GetTvShowSeasonArt(infos.m_iDbId, seasonArt);

  int playcount = infos.m_playCount;
  CDateTime lastPlayed = infos.m_lastPlayed;

  UpdateVideoTag(parameterObject, infos, artwork);

  if (videodatabase.SetDetailsForTvShow(infos.m_strFileNameAndPath, infos, artwork, seasonArt, id) > 0)
  {
    if (playcount != infos.m_playCount || lastPlayed != infos.m_lastPlayed)
      videodatabase.SetPlayCount(CFileItem(infos), infos.m_playCount, infos.m_lastPlayed);
    return ACK;
  }

  return InternalError;
}

JSONRPC_STATUS CVideoLibrary::SetEpisodeDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

  int tvshowid = videodatabase.GetTvShowForEpisode(id);
  if (tvshowid <= 0)
  {
    videodatabase.Close();
    return InvalidParams;
  }

  std::map<std::string, std::string> artwork;
  videodatabase.GetArtForItem(infos.m_iDbId, infos.m_type, artwork);

  int playcount = infos.m_playCount;
  CDateTime lastPlayed = infos.m_lastPlayed;

  UpdateVideoTag(parameterObject, infos, artwork);

  if (videodatabase.SetDetailsForEpisode(infos.m_strFileNameAndPath, infos, artwork, tvshowid, id) > 0)
  {
    if (playcount != infos.m_playCount || lastPlayed != infos.m_lastPlayed)
      videodatabase.SetPlayCount(CFileItem(infos), infos.m_playCount, infos.m_lastPlayed);
    return ACK;
  }

  return InternalError;
}

JSONRPC_STATUS CVideoLibrary::SetMusicVideoDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

  std::map<std::string, std::string> artwork;
  videodatabase.GetArtForItem(infos.m_iDbId, infos.m_type, artwork);

  int playcount = infos.m_playCount;
  CDateTime lastPlayed = infos.m_lastPlayed;

  UpdateVideoTag(parameterObject, infos, artwork);

  if (videodatabase.SetDetailsForMusicVideo(infos.m_strFileNameAndPath, infos, artwork, id) > 0)
  {
    if (playcount != infos.m_playCount || lastPlayed != infos.m_lastPlayed)
      videodatabase.SetPlayCount(CFileItem(infos), infos.m_playCount, infos.m_lastPlayed);
    return ACK;
  }

  return InternalError;
}

JSONRPC_STATUS CVideoLibrary::RemoveMovie(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return RemoveVideo(parameterObject);
}

JSONRPC_STATUS CVideoLibrary::RemoveTVShow(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return RemoveVideo(parameterObject);
}

JSONRPC_STATUS CVideoLibrary::RemoveEpisode(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return RemoveVideo(parameterObject);
}

JSONRPC_STATUS CVideoLibrary::RemoveMusicVideo(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return RemoveVideo(parameterObject);
}

JSONRPC_STATUS CVideoLibrary::Scan(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string directory = parameterObject["directory"].asString();
  CStdString cmd;
  if (directory.empty())
    cmd = "updatelibrary(video)";
  else
    cmd.Format("updatelibrary(video, %s)", directory.c_str());

  g_application.getApplicationMessenger().ExecBuiltIn(cmd);
  return ACK;
}

JSONRPC_STATUS CVideoLibrary::Export(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSONRPC_STATUS CVideoLibrary::Clean(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSONRPC_STATUS CVideoLibrary::GetAdditionalMovieDetails(const CVariant &parameterObject, CFileItemList &items, CVariant &result, CVideoDatabase &videodatabase)
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

JSONRPC_STATUS CVideoLibrary::GetAdditionalEpisodeDetails(const CVariant &parameterObject, CFileItemList &items, CVariant &result, CVideoDatabase &videodatabase)
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

JSONRPC_STATUS CVideoLibrary::GetAdditionalMusicVideoDetails(const CVariant &parameterObject, CFileItemList &items, CVariant &result, CVideoDatabase &videodatabase)
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

JSONRPC_STATUS CVideoLibrary::RemoveVideo(const CVariant &parameterObject)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  if (parameterObject.isMember("movieid"))
    videodatabase.DeleteMovie((int)parameterObject["movieid"].asInteger());
  else if (parameterObject.isMember("tvshowid"))
    videodatabase.DeleteTvShow((int)parameterObject["tvshowid"].asInteger());
  else if (parameterObject.isMember("episodeid"))
    videodatabase.DeleteEpisode((int)parameterObject["episodeid"].asInteger());
  else if (parameterObject.isMember("musicvideoid"))
    videodatabase.DeleteMusicVideo((int)parameterObject["musicvideoid"].asInteger());
  return ACK;
}

void CVideoLibrary::UpdateVideoTag(const CVariant &parameterObject, CVideoInfoTag& details, std::map<std::string, std::string> &artwork)
{
  if (ParameterNotNull(parameterObject, "title"))
    details.m_strTitle = parameterObject["title"].asString();
  if (ParameterNotNull(parameterObject, "playcount"))
    details.m_playCount = (int)parameterObject["playcount"].asInteger();
  if (ParameterNotNull(parameterObject, "runtime"))
    details.m_strRuntime = parameterObject["runtime"].asString();
  if (ParameterNotNull(parameterObject, "director"))
    CopyStringArray(parameterObject["director"], details.m_director);
  if (ParameterNotNull(parameterObject, "studio"))
    CopyStringArray(parameterObject["studio"], details.m_studio);
  if (ParameterNotNull(parameterObject, "year"))
    details.m_iYear = (int)parameterObject["year"].asInteger();
  if (ParameterNotNull(parameterObject, "plot"))
    details.m_strPlot = parameterObject["plot"].asString();
  if (ParameterNotNull(parameterObject, "album"))
    details.m_strAlbum = parameterObject["album"].asString();
  if (ParameterNotNull(parameterObject, "artist"))
    CopyStringArray(parameterObject["artist"], details.m_artist);
  if (ParameterNotNull(parameterObject, "genre"))
    CopyStringArray(parameterObject["genre"], details.m_genre);
  if (ParameterNotNull(parameterObject, "track"))
    details.m_iTrack = (int)parameterObject["track"].asInteger();
  if (ParameterNotNull(parameterObject, "rating"))
    details.m_fRating = parameterObject["rating"].asFloat();
  if (ParameterNotNull(parameterObject, "mpaa"))
    details.m_strMPAARating = parameterObject["mpaa"].asString();
  if (ParameterNotNull(parameterObject, "imdbnumber"))
    details.m_strIMDBNumber = parameterObject["imdbnumber"].asString();
  if (ParameterNotNull(parameterObject, "premiered"))
    details.m_premiered.SetFromDBDate(parameterObject["premiered"].asString());
  if (ParameterNotNull(parameterObject, "votes"))
    details.m_strVotes = parameterObject["votes"].asString();
  if (ParameterNotNull(parameterObject, "lastplayed"))
    details.m_lastPlayed.SetFromDBDateTime(parameterObject["lastplayed"].asString());
  if (ParameterNotNull(parameterObject, "firstaired"))
    details.m_firstAired.SetFromDBDateTime(parameterObject["firstaired"].asString());
  if (ParameterNotNull(parameterObject, "productioncode"))
    details.m_strProductionCode = parameterObject["productioncode"].asString();
  if (ParameterNotNull(parameterObject, "season"))
    details.m_iSeason = (int)parameterObject["season"].asInteger();
  if (ParameterNotNull(parameterObject, "episode"))
    details.m_iEpisode = (int)parameterObject["episode"].asInteger();
  if (ParameterNotNull(parameterObject, "originaltitle"))
    details.m_strOriginalTitle = parameterObject["originaltitle"].asString();
  if (ParameterNotNull(parameterObject, "trailer"))
    details.m_strTrailer = parameterObject["trailer"].asString();
  if (ParameterNotNull(parameterObject, "tagline"))
    details.m_strTagLine = parameterObject["tagline"].asString();
  if (ParameterNotNull(parameterObject, "plotoutline"))
    details.m_strPlotOutline = parameterObject["plotoutline"].asString();
  if (ParameterNotNull(parameterObject, "writer"))
    CopyStringArray(parameterObject["writer"], details.m_writingCredits);
  if (ParameterNotNull(parameterObject, "country"))
    CopyStringArray(parameterObject["country"], details.m_country);
  if (ParameterNotNull(parameterObject, "top250"))
    details.m_iTop250 = (int)parameterObject["top250"].asInteger();
  if (ParameterNotNull(parameterObject, "sorttitle"))
    details.m_strSortTitle = parameterObject["sorttitle"].asString();
  if (ParameterNotNull(parameterObject, "episodeguide"))
    details.m_strEpisodeGuide = parameterObject["episodeguide"].asString();
  if (ParameterNotNull(parameterObject, "set"))
    CopyStringArray(parameterObject["set"], details.m_set);
  if (ParameterNotNull(parameterObject, "showlink"))
    CopyStringArray(parameterObject["showlink"], details.m_showLink);
  if (ParameterNotNull(parameterObject, "thumbnail"))
    artwork["thumb"] = parameterObject["thumbnail"].asString();
  if (ParameterNotNull(parameterObject, "fanart"))
    artwork["fanart"] = parameterObject["fanart"].asString();
}
