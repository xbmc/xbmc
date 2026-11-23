/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoLibrary.h"

#include <optional>

namespace KODI::VIDEO::LIBRARY
{

void AddBookMarkForEpisode(const CVideoInfoTag& tag, const CBookmark& bookmark)
{
  WithDatabase([&](CVideoDatabase& db) { db.AddBookMarkForEpisode(tag, bookmark); });
}

bool AddBookMarkToFile(const std::string& strFilenameAndPath,
                       const CBookmark& bookmark,
                       CBookmark::EType type)
{
  return WithDatabase([&](CVideoDatabase& db)
                      { return db.AddBookMarkToFile(strFilenameAndPath, bookmark, type); });
}

int AddTag(const std::string& tag)
{
  return WithDatabase([&](CVideoDatabase& db) -> std::optional<int> { return db.AddTag(tag); })
      .value_or(-1);
}

void DeleteTag(int idTag, VideoDbContentType mediaType)
{
  WithDatabase([&](CVideoDatabase& db) { db.DeleteTag(idTag, mediaType); });
}

void EraseVideoSettings(const CFileItem& item)
{
  WithDatabase([&](CVideoDatabase& db) { db.EraseVideoSettings(item); });
}

void ExportToXML(
    const std::string& path, bool singleFile, bool images, bool actorThumbs, bool overwrite)
{
  WithDatabase([&](CVideoDatabase& db)
               { db.ExportToXML(path, singleFile, images, actorThumbs, overwrite); });
}

bool GetArtForItem(int mediaId, KODI::MEDIA::TYPE mediaType, KODI::ART::Artwork& art)
{
  return WithDatabase([&](CVideoDatabase& db)
                      { return db.GetArtForItem(mediaId, mediaType, art); });
}

std::string GetArtForItem(int mediaId, const std::string& mediaType, const std::string& artType)
{
  return WithDatabase([&](CVideoDatabase& db)
                      { return db.GetArtForItem(mediaId, mediaType, artType); });
}

CVideoInfoTag GetDetailsByTypeAndId(VideoDbContentType type, int id)
{
  return WithDatabase([&](CVideoDatabase& db) { return db.GetDetailsByTypeAndId(type, id); });
}

bool GetEpisodeMap(int idShow, EpisodeFileMap& fileMap, int idFile)
{
  return WithDatabase([&](CVideoDatabase& db)
                      { return db.GetEpisodeMap(idShow, fileMap, idFile); });
}

void GetEpisodesByBasePath(const std::string& path,
                           std::vector<CVideoInfoTag>& episodes,
                           int idShow)
{
  WithDatabase([&](CVideoDatabase& db) { db.GetEpisodesByBasePath(path, episodes, idShow); });
}

void GetEpisodesByBlurayPath(const std::string& path, std::vector<CVideoInfoTag>& episodes)
{
  WithDatabase([&](CVideoDatabase& db) { db.GetEpisodesByBlurayPath(path, episodes); });
}

void GetEpisodesByFile(const std::string& strFilenameAndPath, std::vector<CVideoInfoTag>& episodes)
{
  WithDatabase([&](CVideoDatabase& db) { db.GetEpisodesByFile(strFilenameAndPath, episodes); });
}

bool GetFileInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details, int idFile)
{
  return WithDatabase([&](CVideoDatabase& db)
                      { return db.GetFileInfo(strFilenameAndPath, details, idFile); });
}

void GetFilePathById(int idMovie, std::string& filePath, VideoDbContentType iType)
{
  WithDatabase([&](CVideoDatabase& db) { db.GetFilePathById(idMovie, filePath, iType); });
}

bool GetGenresNav(const std::string& strBaseDir, CFileItemList& items, VideoDbContentType idContent)
{
  return WithDatabase([&](CVideoDatabase& db)
                      { return db.GetGenresNav(strBaseDir, items, idContent); });
}

int GetMatchingMusicVideo(const std::string& strArtist,
                          const std::string& strAlbum,
                          const std::string& strTitle)
{
  return WithDatabase([&](CVideoDatabase& db) -> std::optional<int>
                      { return db.GetMatchingMusicVideo(strArtist, strAlbum, strTitle); })
      .value_or(-1);
}

bool GetMoviesNav(const std::string& strBaseDir,
                  CFileItemList& items,
                  int idGenre,
                  int idYear,
                  int idActor,
                  int idDirector,
                  int idStudio,
                  int idCountry,
                  int idSet,
                  int idTag,
                  const SortDescription& sortDescription,
                  int getDetails)
{
  return WithDatabase(
      [&](CVideoDatabase& db)
      {
        return db.GetMoviesNav(strBaseDir, items, idGenre, idYear, idActor, idDirector, idStudio,
                               idCountry, idSet, idTag, sortDescription, getDetails);
      });
}

bool GetPathsForCleaning(const std::string& directory,
                         const std::string& content,
                         std::set<int>& paths)
{
  return WithDatabase([&](CVideoDatabase& db)
                      { return db.GetPathsForCleaning(directory, content, paths); });
}

std::string GetRemovableBlurayPath(const std::string& originalPath)
{
  return WithDatabase([&](CVideoDatabase& db) { return db.GetRemovableBlurayPath(originalPath); });
}

bool GetResumeBookMark(const std::string& strFilenameAndPath, CBookmark& bookmark)
{
  return WithDatabase([&](CVideoDatabase& db)
                      { return db.GetResumeBookMark(strFilenameAndPath, bookmark); });
}

ADDON::ScraperPtr GetScraperForPath(const std::string& strPath)
{
  return WithDatabase([&](CVideoDatabase& db) { return db.GetScraperForPath(strPath); });
}

bool GetSeasonsByWhere(const std::string& strBaseDir,
                       const CDatabase::Filter& filter,
                       CFileItemList& items,
                       bool appendFullShowPath)
{
  return WithDatabase(
      [&](CVideoDatabase& db)
      { return db.GetSeasonsByWhere(strBaseDir, filter, items, appendFullShowPath); });
}

std::string GetSetById(int id)
{
  return WithDatabase([&](CVideoDatabase& db) { return db.GetSetById(id); });
}

bool GetSortedVideos(KODI::MEDIA::TYPE mediaType,
                     const std::string& strBaseDir,
                     const SortDescription& sortDescription,
                     CFileItemList& items)
{
  return WithDatabase(
      [&](CVideoDatabase& db)
      { return db.GetSortedVideos(mediaType, strBaseDir, sortDescription, items); });
}

bool GetStackTimes(const std::string& filePath, std::vector<std::chrono::milliseconds>& times)
{
  return WithDatabase([&](CVideoDatabase& db) { return db.GetStackTimes(filePath, times); });
}

bool GetTagsNav(const std::string& strBaseDir, CFileItemList& items, VideoDbContentType idContent)
{
  return WithDatabase([&](CVideoDatabase& db)
                      { return db.GetTagsNav(strBaseDir, items, idContent); });
}

bool GetTvShowInfo(const std::string& strPath, CVideoInfoTag& details, int idTvShow)
{
  return WithDatabase([&](CVideoDatabase& db)
                      { return db.GetTvShowInfo(strPath, details, idTvShow); });
}

bool GetTvShowsByWhere(const std::string& strBaseDir,
                       const CDatabase::Filter& filter,
                       CFileItemList& items,
                       const SortDescription& sortDescription,
                       int getDetails)
{
  return WithDatabase(
      [&](CVideoDatabase& db)
      { return db.GetTvShowsByWhere(strBaseDir, filter, items, sortDescription, getDetails); });
}

bool GetUseAllExternalAudioForVideo(const std::string& videoPath)
{
  return WithDatabase([&](CVideoDatabase& db)
                      { return db.GetUseAllExternalAudioForVideo(videoPath); });
}

bool HasContent(VideoDbContentType type)
{
  return WithDatabase([&](CVideoDatabase& db) { return db.HasContent(type); });
}

void ImportFromXML(const std::string& path)
{
  WithDatabase([&](CVideoDatabase& db) { db.ImportFromXML(path); });
}

bool LoadVideoInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details)
{
  return WithDatabase([&](CVideoDatabase& db)
                      { return db.LoadVideoInfo(strFilenameAndPath, details); });
}

bool ScraperInUse(const std::string& scraperID)
{
  return WithDatabase([&](CVideoDatabase& db) { return db.ScraperInUse(scraperID); });
}

bool SetPathHash(const std::string& path, const std::string& hash)
{
  return WithDatabase([&](CVideoDatabase& db) { return db.SetPathHash(path, hash); });
}

void SetStackTimes(const std::string& filePath, const std::vector<std::chrono::milliseconds>& times)
{
  WithDatabase([&](CVideoDatabase& db) { db.SetStackTimes(filePath, times); });
}

void SetTrailerForMovie(int idMovie, const std::string& trailer)
{
  WithDatabase([&](CVideoDatabase& db) { db.SetTrailerForMovie(idMovie, trailer); });
}

void SetVideoSettings(const CFileItem& item, const CVideoSettings& settings)
{
  WithDatabase([&](CVideoDatabase& db) { db.SetVideoSettings(item, settings); });
}

bool SetVideoUserRating(int dbId, int rating, KODI::MEDIA::TYPE mediaType)
{
  return WithDatabase([&](CVideoDatabase& db)
                      { return db.SetVideoUserRating(dbId, rating, mediaType); });
}

CDatabase::GetResult TryGetDetailsByTypeAndId(KODI::MEDIA::TYPE type,
                                              int id,
                                              CVideoInfoTag& details,
                                              CFileItem* item,
                                              int getDetails,
                                              int idVersion,
                                              int idFile)
{
  return WithDatabase(
             [&](CVideoDatabase& db) -> std::optional<CDatabase::GetResult>
             {
               return db.TryGetDetailsByTypeAndId(type, id, details, item, getDetails, idVersion,
                                                  idFile);
             })
      .value_or(CDatabase::GetResult::Error);
}

bool UpdateVideoSortTitle(int idDb, const std::string& strNewSortTitle, VideoDbContentType iType)
{
  return WithDatabase([&](CVideoDatabase& db)
                      { return db.UpdateVideoSortTitle(idDb, strNewSortTitle, iType); });
}

} // namespace KODI::VIDEO::LIBRARY
