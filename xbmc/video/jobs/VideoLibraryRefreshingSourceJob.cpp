/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoLibraryRefreshingSourceJob.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/StackDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "media/MediaType.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/Artwork.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoManagerTypes.h"
#include "video/jobs/VideoLibraryRefreshingJob.h"

#include <cstring>
#include <memory>
#include <set>
#include <utility>
#include <vector>

using namespace XFILE;

CVideoLibraryRefreshingSourceJob::CVideoLibraryRefreshingSourceJob(std::string sourcePath)
  : CVideoLibraryProgressJob(nullptr),
    m_sourcePath(std::move(sourcePath))
{
}

bool CVideoLibraryRefreshingSourceJob::Cancel()
{
  m_cancelled = true;
  return true;
}

bool CVideoLibraryRefreshingSourceJob::Equals(const CJob* job) const
{
  if (strcmp(job->GetType(), GetType()) != 0)
    return false;

  const auto* refreshingJob{dynamic_cast<const CVideoLibraryRefreshingSourceJob*>(job)};
  return refreshingJob != nullptr && m_sourcePath == refreshingJob->m_sourcePath;
}

bool CVideoLibraryRefreshingSourceJob::Work(CVideoDatabase& db)
{
  m_started = true;
  if (m_cancelled)
    return true;

  std::vector<std::string> paths;
  if (URIUtils::IsMultiPath(m_sourcePath))
    CMultiPathDirectory::GetPaths(m_sourcePath, paths);
  else
    paths.emplace_back(m_sourcePath);

  // includes the encoded bluray://, udf:// and archive paths that disc images and rips are stored
  // under, which do not start with the source path
  std::set<int> pathIds;
  for (const std::string& path : paths)
  {
    std::vector<std::pair<int, std::string>> subPaths;
    db.GetSubPaths(path, subPaths, false);
    for (const auto& subPath : subPaths)
      pathIds.insert(subPath.first);
  }
  if (pathIds.empty())
    return true;

  std::vector<std::string> pathIdStrings;
  for (const int pathId : pathIds)
    pathIdStrings.emplace_back(std::to_string(pathId));
  const std::string pathIdList{StringUtils::Join(pathIdStrings, ",")};

  // a movie is in the source if any of its versions is
  CDatabase::Filter movieFilter{db.PrepareSQL(
      "idMovie IN (SELECT idMedia FROM videoversion JOIN files ON "
      "files.idFile = videoversion.idFile WHERE videoversion.media_type = 'movie' AND "
      "videoversion.itemType = %i AND files.idPath IN (%s))",
      static_cast<int>(VideoAssetType::VERSION), pathIdList.c_str())};
  movieFilter.AppendWhere("isDefaultVersion = 1");
  CFileItemList movies;
  db.GetMoviesByWhere("videodb://movies/titles/", movieFilter, movies, SortDescription(),
                      VideoDbDetailsAll);

  // a tvshow may have several paths, so match on any of them
  const CDatabase::Filter tvshowFilter{
      "idShow IN (SELECT idShow FROM tvshowlinkpath WHERE idPath IN (" + pathIdList + "))"};
  CFileItemList tvshows;
  db.GetTvShowsByWhere("videodb://tvshows/titles/", tvshowFilter, tvshows, SortDescription(),
                       VideoDbDetailsAll);

  const CDatabase::Filter musicvideoFilter{"idFile IN (SELECT idFile FROM files WHERE idPath IN (" +
                                           pathIdList + "))"};
  CFileItemList musicvideos;
  db.GetMusicVideosByWhere("videodb://musicvideos/titles/", musicvideoFilter, musicvideos, true,
                           SortDescription(), VideoDbDetailsAll);

  // tvshows are refreshed including their episodes
  std::vector<std::pair<std::shared_ptr<CFileItem>, bool>> itemsToRefresh;
  for (const auto& movie : movies)
    itemsToRefresh.emplace_back(movie, false);
  for (const auto& musicvideo : musicvideos)
    itemsToRefresh.emplace_back(musicvideo, false);
  for (const auto& tvshow : tvshows)
    itemsToRefresh.emplace_back(tvshow, true);

  if (itemsToRefresh.empty())
    return true;

  if (auto* dialog{
          CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogExtendedProgressBar>(
              WINDOW_DIALOG_EXT_PROGRESS)})
    SetProgressBar(
        dialog->GetHandle(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(13363)));

  const auto total{static_cast<unsigned int>(itemsToRefresh.size())};
  for (unsigned int i = 0; i < total; ++i)
  {
    // stopped before the next item, as the refresh of an item can't be interrupted
    if (m_cancelled)
      return true;
    SetProgress(static_cast<int>(i), static_cast<int>(total));

    const auto& [item, refreshAll] = itemsToRefresh[i];
    const CVideoInfoTag& listed{*item->GetVideoInfoTag()};

    // refreshing an earlier item may have merged it with this one as a version, changing its
    // default version, or removed this one
    CVideoInfoTag tag;
    bool found{false};
    if (listed.m_type == MediaTypeMovie)
      found = db.GetMovieInfo({}, tag, listed.m_iDbId);
    else if (listed.m_type == MediaTypeTvShow)
      found = db.GetTvShowInfo({}, tag, listed.m_iDbId);
    else
      found = db.GetMusicVideoInfo({}, tag, listed.m_iDbId);
    if (!found)
      continue;
    SetText(tag.GetTitle());

    // A refresh would remove an item whose media is missing, so it is left to library cleaning.
    // Its media is found as cleaning finds it.
    std::string mediaPath{refreshAll ? tag.m_strPath : tag.m_strFileNameAndPath};
    if (URIUtils::IsStack(mediaPath))
      mediaPath = CStackDirectory::GetFirstStackedFile(mediaPath);
    if (URIUtils::IsInArchive(mediaPath))
      mediaPath = CURL(mediaPath).GetHostName();
    if (URIUtils::IsBlurayPath(mediaPath))
      mediaPath = URIUtils::GetDiscFile(mediaPath);
    if (!URIUtils::IsPlugin(mediaPath) &&
        !(refreshAll ? CDirectory::Exists(mediaPath) : CFile::Exists(mediaPath)))
    {
      CLog::LogF(LOGWARNING, "Not refreshing {} with database id {} as {} is missing", tag.m_type,
                 tag.m_iDbId, CURL::GetRedacted(mediaPath));
      continue;
    }

    auto refreshItem{std::make_shared<CFileItem>(tag)};
    if (!db.GetScraperForPath(refreshItem->GetPath()))
    {
      CLog::LogF(LOGWARNING, "No scraper for {} with database id {}", tag.m_type, tag.m_iDbId);
      continue;
    }

    KODI::ART::Artwork art;
    if (!db.GetArtForItem(tag.m_iDbId, tag.m_type, art))
    {
      CLog::LogF(LOGERROR, "Failed to load artwork for {} with database id {}", tag.m_type,
                 tag.m_iDbId);
      continue;
    }
    refreshItem->SetArt(art);

    // as the info dialog does when refreshing a tvshow including its episodes
    if (refreshAll)
      db.SetPathHash(tag.m_strPath, "");

    CVideoLibraryRefreshingJob refreshingJob(std::move(refreshItem), true, refreshAll);
    refreshingJob.DoWork();
  }

  return true;
}
