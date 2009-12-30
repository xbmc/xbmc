#include "VideoLibrary.h"
#include "../VideoDatabase.h"
#include "../Util.h"

using namespace Json;

JSON_STATUS CVideoLibrary::GetMovies(const CStdString &method, const Value& parameterObject, Value &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

//  int genreID = parameterObject.get("genreid", -1).asInt();

  CFileItemList items;
  if (videodatabase.GetMoviesNav("", items))
  {
    unsigned start, end;
    HandleFileItemList("movieid", "movies", items, start, end, parameterObject, result);
  }

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetTVShows(const CStdString &method, const Value& parameterObject, Value &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

//  int genreID = parameterObject.get("genreid", -1).asInt();

  CFileItemList items;
  if (videodatabase.GetTvShowsNav("", items))
  {
    unsigned start, end;
    HandleFileItemList("tvshowid", "tvshows", items, start, end, parameterObject, result);
  }

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetSeasons(const CStdString &method, const Value& parameterObject, Value &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  int tvshowID = parameterObject.get("tvshowid", -1).asInt();

  CFileItemList items;
  if (videodatabase.GetSeasonsNav("", items, -1, -1, -1, -1, tvshowID))
  {
    unsigned start, end;
    HandleFileItemList("seasonid", "seasons", items, start, end, parameterObject, result);
  }

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetEpisodes(const CStdString &method, const Value& parameterObject, Value &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  int tvshowID = parameterObject.get("tvshowid", -1).asInt();
  int seasonID = parameterObject.get("seasonid", -1).asInt();

  CFileItemList items;
  if (videodatabase.GetEpisodesNav("", items, -1, -1, -1, -1, tvshowID, seasonID))
  {
    unsigned start, end;
    HandleFileItemList("episodeid", "episodes", items, start, end, parameterObject, result);
  }

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetMusicVideoAlbums(const CStdString &method, const Value& parameterObject, Value &result)
{
/*  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  int artistID = parameterObject.get("artistid", -1).asInt();
//  bool GetMusicVideosNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre=-1, int idYear=-1, int idArtist=-1, int idDirector=-1, int idStudio=-1, int idAlbum=-1);
  CFileItemList items;
//  bool GetMusicVideosNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre=-1, int idYear=-1, int idArtist=-1, int idDirector=-1, int idStudio=-1, int idAlbum=-1);
  if (videodatabase.GetMusicVideoAlbumsNav("", items, -1, -1, artistID, -1, -1, -1, -1))
  {
    unsigned start, end;
    HandleFileItemList("albumid", "albums", items, start, end, parameterObject, result);
  }

  videodatabase.Close();*/
  return OK;
}

JSON_STATUS CVideoLibrary::GetMusicVideos(const CStdString &method, const Value& parameterObject, Value &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  int artistID = parameterObject.get("artistid", -1).asInt();
  int albumID  = parameterObject.get("albumid",  -1).asInt();

  CFileItemList items;
  if (videodatabase.GetMusicVideosNav("", items, -1, -1, artistID, -1, -1, albumID))
  {
    unsigned start, end;
    HandleFileItemList("musicvideoid", "musicvideos", items, start, end, parameterObject, result);
  }

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetMovieInfo(const CStdString &method, const Value& parameterObject, Value &result)
{
  if (!parameterObject.isMember("movieid") || parameterObject.get("movieid", -1).asInt() < 0 || !parameterObject.isMember("fields"))
    return InvalidParams;

  int movieID = parameterObject.get("movieid", -1).asInt();

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CStdString filepath;
  CVideoInfoTag videoInfo;
  videodatabase.GetMovieInfo(filepath, videoInfo, movieID);
  FillVideoDetails(&videoInfo, parameterObject, result);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetTVShowInfo(const CStdString &method, const Value& parameterObject, Value &result)
{
/*  if (!parameterObject.isMember("tvshowid") || parameterObject.get("tvshowid", -1).asInt() < 0 || !parameterObject.isMember("fields"))
    return InvalidParams;

  int tvshowID = parameterObject.get("tvshowid", -1).asInt();

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CStdString filepath;
  CVideoInfoTag videoInfo;
  videodatabase.GetTvShowInfo(filepath, videoInfo, tvshowID);
  FillVideoDetails(&videoInfo, parameterObject, result);

  videodatabase.Close();*/
  return OK;
}

JSON_STATUS CVideoLibrary::GetEpisodeInfo(const CStdString &method, const Value& parameterObject, Value &result)
{
/*  if (!parameterObject.has<long>("episodeid") || parameterObject.get<long>("episodeid") < 0 || !parameterObject.has<Array>("fields"))
  if (!parameterObject.isMember("episodeid") || parameterObject.get("episodeid", -1).asInt() < 0 || !parameterObject.isMember("fields"))
    return InvalidParams;

  int episodeID  = (int)parameterObject.get<long>("episodeid");

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CStdString filepath;
  CVideoInfoTag videoInfo;
  videodatabase.GetEpisodeInfo(filepath, videoInfo, episodeID);
  FillVideoDetails(&videoInfo, parameterObject, result);

  videodatabase.Close();*/
  return OK;
}

JSON_STATUS CVideoLibrary::GetMusicVideoInfo(const CStdString &method, const Value& parameterObject, Value &result)
{
/*  if (!parameterObject.has<long>("musicvideoid") || parameterObject.get<long>("musicvideoid") < 0 || !parameterObject.has<Array>("fields"))
    return InvalidParams;

  int musicID  = (int)parameterObject.get<long>("musicvideoid");

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CStdString filepath;
  CVideoInfoTag videoInfo;
  videodatabase.GetMusicVideoInfo(filepath, videoInfo, musicID);
  FillVideoDetails(&videoInfo, parameterObject, result);

  videodatabase.Close();*/
  return OK;
}
