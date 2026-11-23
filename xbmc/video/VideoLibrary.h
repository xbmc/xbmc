/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/log.h"
#include "video/VideoDatabase.h"

#include <chrono>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace KODI::VIDEO::LIBRARY
{
/*!
 * \brief Run \p work on a video database connection of its own, opened for it and closed after.
 * \return what \p work returns, or a value-initialised one when the database cannot be opened
 */
template<typename Work>
auto WithDatabase(Work&& work) -> std::invoke_result_t<Work, CVideoDatabase&>
{
  using Result = std::invoke_result_t<Work, CVideoDatabase&>;
  CVideoDatabase db;
  if (!db.Open())
  {
    CLog::LogF(LOGERROR, "could not open the video database");
    if constexpr (std::is_void_v<Result>)
      return;
    else
      return Result{};
  }
  return std::forward<Work>(work)(db);
}

//! CVideoDatabase::AddBookMarkForEpisode on a connection of its own
void AddBookMarkForEpisode(const CVideoInfoTag& tag, const CBookmark& bookmark);

//! CVideoDatabase::AddBookMarkToFile on a connection of its own
bool AddBookMarkToFile(const std::string& strFilenameAndPath,
                       const CBookmark& bookmark,
                       CBookmark::EType type);

//! CVideoDatabase::AddTag on a connection of its own
int AddTag(const std::string& tag);

//! CVideoDatabase::DeleteTag on a connection of its own
void DeleteTag(int idTag, VideoDbContentType mediaType);

//! CVideoDatabase::EraseVideoSettings on a connection of its own
void EraseVideoSettings(const CFileItem& item);

//! CVideoDatabase::ExportToXML on a connection of its own
void ExportToXML(
    const std::string& path, bool singleFile, bool images, bool actorThumbs, bool overwrite);

//! CVideoDatabase::GetArtForItem on a connection of its own
bool GetArtForItem(int mediaId, KODI::MEDIA::TYPE mediaType, KODI::ART::Artwork& art);

//! CVideoDatabase::GetArtForItem on a connection of its own
std::string GetArtForItem(int mediaId, const std::string& mediaType, const std::string& artType);

//! CVideoDatabase::GetDetailsByTypeAndId on a connection of its own
CVideoInfoTag GetDetailsByTypeAndId(VideoDbContentType type, int id);

//! CVideoDatabase::GetEpisodeMap on a connection of its own
bool GetEpisodeMap(int idShow, EpisodeFileMap& fileMap, int idFile = -1);

//! CVideoDatabase::GetEpisodesByBasePath on a connection of its own
void GetEpisodesByBasePath(const std::string& path,
                           std::vector<CVideoInfoTag>& episodes,
                           int idShow);

//! CVideoDatabase::GetEpisodesByBlurayPath on a connection of its own
void GetEpisodesByBlurayPath(const std::string& path, std::vector<CVideoInfoTag>& episodes);

//! CVideoDatabase::GetEpisodesByFile on a connection of its own
void GetEpisodesByFile(const std::string& strFilenameAndPath, std::vector<CVideoInfoTag>& episodes);

//! CVideoDatabase::GetFileInfo on a connection of its own
bool GetFileInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details, int idFile);

//! CVideoDatabase::GetFilePathById on a connection of its own
void GetFilePathById(int idMovie, std::string& filePath, VideoDbContentType iType);

//! CVideoDatabase::GetGenresNav on a connection of its own
bool GetGenresNav(const std::string& strBaseDir,
                  CFileItemList& items,
                  VideoDbContentType idContent);

//! CVideoDatabase::GetMatchingMusicVideo on a connection of its own; -1 when it cannot be opened
int GetMatchingMusicVideo(const std::string& strArtist,
                          const std::string& strAlbum = "",
                          const std::string& strTitle = "");

//! CVideoDatabase::GetMoviesNav on a connection of its own
bool GetMoviesNav(const std::string& strBaseDir,
                  CFileItemList& items,
                  int idGenre = -1,
                  int idYear = -1,
                  int idActor = -1,
                  int idDirector = -1,
                  int idStudio = -1,
                  int idCountry = -1,
                  int idSet = -1,
                  int idTag = -1,
                  const SortDescription& sortDescription = SortDescription(),
                  int getDetails = VideoDbDetailsNone);

//! CVideoDatabase::GetPathsForCleaning on a connection of its own
bool GetPathsForCleaning(const std::string& directory,
                         const std::string& content,
                         std::set<int>& paths);

//! CVideoDatabase::GetRemovableBlurayPath on a connection of its own
std::string GetRemovableBlurayPath(const std::string& originalPath);

//! CVideoDatabase::GetResumeBookMark on a connection of its own
bool GetResumeBookMark(const std::string& strFilenameAndPath, CBookmark& bookmark);

//! CVideoDatabase::GetScraperForPath on a connection of its own
ADDON::ScraperPtr GetScraperForPath(const std::string& strPath);

//! CVideoDatabase::GetSeasonsByWhere on a connection of its own
bool GetSeasonsByWhere(const std::string& strBaseDir,
                       const CDatabase::Filter& filter,
                       CFileItemList& items,
                       bool appendFullShowPath);

//! CVideoDatabase::GetSetById on a connection of its own
std::string GetSetById(int id);

//! CVideoDatabase::GetSortedVideos on a connection of its own
bool GetSortedVideos(KODI::MEDIA::TYPE mediaType,
                     const std::string& strBaseDir,
                     const SortDescription& sortDescription,
                     CFileItemList& items);

//! CVideoDatabase::GetStackTimes on a connection of its own
bool GetStackTimes(const std::string& filePath, std::vector<std::chrono::milliseconds>& times);

//! CVideoDatabase::GetTagsNav on a connection of its own
bool GetTagsNav(const std::string& strBaseDir, CFileItemList& items, VideoDbContentType idContent);

//! CVideoDatabase::GetTvShowInfo on a connection of its own
bool GetTvShowInfo(const std::string& strPath, CVideoInfoTag& details, int idTvShow);

//! CVideoDatabase::GetTvShowsByWhere on a connection of its own
bool GetTvShowsByWhere(const std::string& strBaseDir,
                       const CDatabase::Filter& filter,
                       CFileItemList& items,
                       const SortDescription& sortDescription,
                       int getDetails);

//! CVideoDatabase::GetUseAllExternalAudioForVideo on a connection of its own
bool GetUseAllExternalAudioForVideo(const std::string& videoPath);

//! CVideoDatabase::HasContent on a connection of its own
bool HasContent(VideoDbContentType type);

//! CVideoDatabase::ImportFromXML on a connection of its own
void ImportFromXML(const std::string& path);

//! CVideoDatabase::LoadVideoInfo on a connection of its own
bool LoadVideoInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details);

//! CVideoDatabase::ScraperInUse on a connection of its own
bool ScraperInUse(const std::string& scraperID);

//! CVideoDatabase::SetPathHash on a connection of its own
bool SetPathHash(const std::string& path, const std::string& hash);

//! CVideoDatabase::SetStackTimes on a connection of its own
void SetStackTimes(const std::string& filePath,
                   const std::vector<std::chrono::milliseconds>& times);

//! CVideoDatabase::SetTrailerForMovie on a connection of its own
void SetTrailerForMovie(int idMovie, const std::string& trailer);

//! CVideoDatabase::SetVideoSettings on a connection of its own
void SetVideoSettings(const CFileItem& item, const CVideoSettings& settings);

//! CVideoDatabase::SetVideoUserRating on a connection of its own
bool SetVideoUserRating(int dbId, int rating, KODI::MEDIA::TYPE mediaType);

//! CVideoDatabase::TryGetDetailsByTypeAndId on a connection of its own; Error if it cannot be had
CDatabase::GetResult TryGetDetailsByTypeAndId(KODI::MEDIA::TYPE type,
                                              int id,
                                              CVideoInfoTag& details,
                                              CFileItem* item,
                                              int getDetails,
                                              int idVersion,
                                              int idFile);

//! CVideoDatabase::UpdateVideoSortTitle on a connection of its own
bool UpdateVideoSortTitle(int idDb, const std::string& strNewSortTitle, VideoDbContentType iType);
} // namespace KODI::VIDEO::LIBRARY
