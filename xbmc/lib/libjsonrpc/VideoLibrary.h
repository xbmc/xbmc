#pragma once
#include "StdString.h"
#include "JSONRPC.h"
#include "LibraryBase.h"
#include "../FileItem.h"

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

class CVideoLibrary : public CLibraryBase
{
public:
  static JSON_STATUS GetMovies(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);

  static JSON_STATUS GetTVShows(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS GetSeasons(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS GetEpisodes(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);

  static JSON_STATUS GetMusicVideoAlbums(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS GetMusicVideos(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);

  static JSON_STATUS GetMovieInfo(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS GetTVShowInfo(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS GetEpisodeInfo(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS GetMusicVideoInfo(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
};
