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
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace KODI::VIDEO::LIBRARY
{
/*!
 * \brief Run \p work on a video database connection of its own, opened for it and closed after.
 * \param caller the function a failure to open is logged under
 * \return what \p work returns, or a value-initialised one when the database cannot be opened
 */
template<typename Work>
auto WithDatabase(Work&& work, const std::source_location& caller = std::source_location::current())
    -> std::invoke_result_t<Work, CVideoDatabase&>
{
  using Result = std::invoke_result_t<Work, CVideoDatabase&>;
  CVideoDatabase db;
  if (!db.Open())
  {
    std::string_view name{caller.function_name()};
    name = name.substr(0, name.find('('));
    name = name.substr(name.rfind(':') + 1);
    CLog::Log(LOGERROR, "{}: could not open the video database", name);
    if constexpr (std::is_void_v<Result>)
      return;
    else
      return Result{};
  }
  return std::forward<Work>(work)(db);
}

void AddBookMarkForEpisode(const CVideoInfoTag& tag, const CBookmark& bookmark);

bool AddBookMarkToFile(const std::string& strFilenameAndPath,
                       const CBookmark& bookmark,
                       CBookmark::EType type);

int AddTag(const std::string& tag);

void DeleteTag(int idTag, VideoDbContentType mediaType);

void EraseVideoSettings(const CFileItem& item);

void ExportToXML(
    const std::string& path, bool singleFile, bool images, bool actorThumbs, bool overwrite);

bool GetArtForItem(int mediaId, KODI::MEDIA::TYPE mediaType, KODI::ART::Artwork& art);

std::string GetArtForItem(int mediaId, const std::string& mediaType, const std::string& artType);

CVideoInfoTag GetDetailsByTypeAndId(VideoDbContentType type, int id);

bool GetEpisodeMap(int idShow, EpisodeFileMap& fileMap, int idFile = -1);

void GetEpisodesByBasePath(const std::string& path,
                           std::vector<CVideoInfoTag>& episodes,
                           int idShow);

void GetEpisodesByBlurayPath(const std::string& path, std::vector<CVideoInfoTag>& episodes);

void GetEpisodesByFile(const std::string& strFilenameAndPath, std::vector<CVideoInfoTag>& episodes);

bool GetFileInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details, int idFile);

void GetFilePathById(int idMovie, std::string& filePath, VideoDbContentType iType);

bool GetGenresNav(const std::string& strBaseDir,
                  CFileItemList& items,
                  VideoDbContentType idContent);

//! -1 when the database cannot be opened
int GetMatchingMusicVideo(const std::string& strArtist,
                          const std::string& strAlbum = "",
                          const std::string& strTitle = "");

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

bool GetPathsForCleaning(const std::string& directory,
                         const std::string& content,
                         std::set<int>& paths);

std::string GetRemovableBlurayPath(const std::string& originalPath);

bool GetResumeBookMark(const std::string& strFilenameAndPath, CBookmark& bookmark);

ADDON::ScraperPtr GetScraperForPath(const std::string& strPath);

bool GetSeasonsByWhere(const std::string& strBaseDir,
                       const CDatabase::Filter& filter,
                       CFileItemList& items,
                       bool appendFullShowPath);

std::string GetSetById(int id);

bool GetSortedVideos(KODI::MEDIA::TYPE mediaType,
                     const std::string& strBaseDir,
                     const SortDescription& sortDescription,
                     CFileItemList& items);

bool GetStackTimes(const std::string& filePath, std::vector<std::chrono::milliseconds>& times);

bool GetTagsNav(const std::string& strBaseDir, CFileItemList& items, VideoDbContentType idContent);

bool GetTvShowInfo(const std::string& strPath, CVideoInfoTag& details, int idTvShow);

bool GetTvShowsByWhere(const std::string& strBaseDir,
                       const CDatabase::Filter& filter,
                       CFileItemList& items,
                       const SortDescription& sortDescription,
                       int getDetails);

bool GetUseAllExternalAudioForVideo(const std::string& videoPath);

bool HasContent(VideoDbContentType type);

void ImportFromXML(const std::string& path);

bool LoadVideoInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details);

bool ScraperInUse(const std::string& scraperID);

bool SetPathHash(const std::string& path, const std::string& hash);

void SetStackTimes(const std::string& filePath,
                   const std::vector<std::chrono::milliseconds>& times);

void SetTrailerForMovie(int idMovie, const std::string& trailer);

void SetVideoSettings(const CFileItem& item, const CVideoSettings& settings);

bool SetVideoUserRating(int dbId, int rating, KODI::MEDIA::TYPE mediaType);

//! Error when the database cannot be opened
CDatabase::GetResult TryGetDetailsByTypeAndId(KODI::MEDIA::TYPE type,
                                              int id,
                                              CVideoInfoTag& details,
                                              CFileItem* item,
                                              int getDetails,
                                              int idVersion,
                                              int idFile);

bool UpdateVideoSortTitle(int idDb, const std::string& strNewSortTitle, VideoDbContentType iType);
} // namespace KODI::VIDEO::LIBRARY
