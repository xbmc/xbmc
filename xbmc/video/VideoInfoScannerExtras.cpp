/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoInfoScannerExtras.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "InfoScanner.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "addons/Scraper.h"
#include "cores/VideoPlayer/DVDFileInfo.h"
#include "filesystem/Directory.h"
#include "filesystem/DiscDirectoryHelper.h"
#include "filesystem/StackDirectory.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/DiscsUtils.h"
#include "utils/FileExtensionProvider.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/Bookmark.h"
#include "video/VideoDatabase.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoScanner.h"
#include "video/VideoInfoScannerArt.h"
#include "video/VideoInfoTag.h"
#include "video/VideoManagerTypes.h"
#include "video/VideoUtils.h"
#include "video/dialogs/GUIDialogVideoManagerExtras.h"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <set>
#include <utility>
#include <vector>

using namespace XFILE;

namespace KODI::VIDEO
{
namespace
{
bool IsBluray(const std::string& path)
{
  return ::UTILS::DISCS::IsBlurayDiscImage(path) || URIUtils::IsBDFile(path);
}

//! The movie a bluray belongs to. GetMovieId() finds it from the disc only where the movie's
//! default version is on it, which is not so for an HD disc beside the movie's 4K one.
int GetDiscMovieId(CVideoDatabase& db, const std::string& disc)
{
  if (const int dbId{db.GetMovieId(disc)}; dbId >= 0)
    return dbId;

  std::set<int> movies;
  for (const auto& playlist : db.GetPlaylistsByPath(URIUtils::GetBlurayPlaylistPath(disc)))
  {
    if (playlist.mediaType == VideoDbContentType::MOVIES &&
        playlist.itemType == VideoAssetType::VERSION)
      movies.emplace(playlist.idMedia);
  }
  return movies.size() == 1 ? *movies.begin() : -1;
}

//! Whether a movie already has an extra on a bluray, either the whole disc or one of its playlists
bool HasExtraOnDisc(CVideoDatabase& db, const std::string& disc, int dbId)
{
  if (const VideoAssetInfo whole{db.GetVideoVersionInfo(disc)};
      whole.m_assetType == VideoAssetType::EXTRA && whole.m_idMedia == dbId)
    return true;

  return std::ranges::any_of(db.GetPlaylistsByPath(URIUtils::GetBlurayPlaylistPath(disc)),
                             [dbId](const CVideoDatabase::PlaylistInfo& playlist)
                             {
                               return playlist.mediaType == VideoDbContentType::MOVIES &&
                                      playlist.idMedia == dbId &&
                                      playlist.itemType == VideoAssetType::EXTRA;
                             });
}
} // namespace

CVideoInfoScannerExtras::CVideoInfoScannerExtras(CVideoDatabase& database,
                                                 const CVideoInfoScannerArt& art)
  : m_database(database),
    m_art(art)
{
}

bool CVideoInfoScannerExtras::AddVideoExtras(const CFileItemList& items, const std::string& path)
{
  int dbId = -1;

  // get the movie which was added previously
  for (const auto& item : items)
  {
    if (!item->IsFolder())
    {
      dbId = m_database.GetMovieId(item->GetPath());
      if (dbId != -1)
      {
        break;
      }
    }
  }

  if (dbId == -1)
  {
    CLog::Log(LOGERROR, "VideoInfoScanner: Failed to find the library item for video extras {}",
              CURL::GetRedacted(path));
    return false;
  }

  AddVideoExtras(dbId, path);
  return true;
}

void CVideoInfoScannerExtras::AddVideoExtras(int dbId, const std::string& path)
{
  // No need to check for .nomedia in the current directory, the caller already checked and this
  // function would not have been called if it existed.

  // Add video extras to library
  UTILS::EnumerateVideoExtras(
      path,
      [this, dbId, path](const std::shared_ptr<CFileItem>& item)
      {
        // A bluray is added as the extras it names, or failing that as a whole. One already
        // there may since have been given a playlist, so it is not added as a whole again.
        if (IsBluray(item->GetPath()) && (HasExtraOnDisc(m_database, item->GetPath(), dbId) ||
                                          AddDiscExtras(item->GetPath(), dbId)))
          return;

        // An extra already in the library keeps any name or art it has since been given. A version
        // is left one, as converting it would make the movie's own file the extra.
        if (const VideoAssetInfo asset{m_database.GetVideoVersionInfo(item->GetPath())};
            asset.m_assetType != VideoAssetType::UNKNOWN && asset.m_idMedia == dbId)
          return;

        const std::string extraTypeName =
            CGUIDialogVideoManagerExtras::GenerateVideoExtra(path, item->GetPath());

        const int idVideoAssetType = m_database.AddVideoVersionType(
            extraTypeName, VideoAssetTypeOwner::AUTO, VideoAssetType::EXTRA);

        // the video may have been added to the library as a movie earlier (different settings)
        const int idMovie{m_database.GetMovieId(item->GetPath())};

        if (idMovie <= 0)
        {
          if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                  CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS))
          {
            CDVDFileInfo::GetFileStreamDetails(item.get());
            CLog::Log(LOGDEBUG, "VideoInfoScanner: Extracted filestream details from video file {}",
                      CURL::GetRedacted(item->GetPath()));
          }

          m_art.GetArtwork(item.get(), ADDON::ContentType::MOVIES, true, true, "");

          if (m_database.AddVideoAsset(VideoDbContentType::MOVIES, dbId, idVideoAssetType,
                                       VideoAssetType::EXTRA, *item.get()))
          {
            CLog::Log(LOGDEBUG, "VideoInfoScanner: Added video extra {}",
                      CURL::GetRedacted(item->GetPath()));
          }
          else
          {
            CLog::Log(LOGERROR, "VideoInfoScanner: Failed to add video extra {}",
                      CURL::GetRedacted(item->GetPath()));
          }
        }
        else
        {
          m_database.ConvertVideoToVersion(VideoDbContentType::MOVIES, idMovie, dbId,
                                           idVideoAssetType, VideoAssetType::EXTRA);
        }
      });
}

void CVideoInfoScannerExtras::AddMovieDiscExtras(const CFileItem& item)
{
  std::vector<std::string> paths{item.GetDynPath()};
  int stackDbId{-1};
  if (URIUtils::IsStack(paths.front()))
  {
    const std::string stack{paths.front()};
    paths.clear();
    CStackDirectory::GetPaths(stack, paths);

    // A movie on several discs is stored under its stack, not under any one disc
    stackDbId = m_database.GetMovieId(stack);
  }

  for (const std::string& path : paths)
  {
    if (!IsBluray(path) && !URIUtils::IsBlurayPath(path))
      continue;

    const int dbId{stackDbId >= 0 ? stackDbId : GetDiscMovieId(m_database, path)};
    if (dbId < 0)
    {
      CLog::LogF(LOGDEBUG, "No movie found for {} to add its extras to", CURL::GetRedacted(path));
      continue;
    }

    // A disc already giving the movie its extras is not read again on every scan
    if (HasExtraOnDisc(m_database, path, dbId))
      continue;

    AddDiscExtras(path, dbId);
  }
}

void CVideoInfoScannerExtras::AddVideoExtrasBesideDisc(const std::string& discFolder,
                                                       bool discChanged,
                                                       const std::vector<std::string>& regexps)
{
  // A new extras folder changes the disc's folder, so is found by listing it. A file added to an
  // extras folder changes only that, so those already known are each looked at.
  std::vector<std::string> folders;
  if (discChanged)
  {
    CFileItemList items;
    if (!CDirectory::GetDirectory(discFolder, items, "/", DIR_FLAG_DEFAULTS))
      return;
    for (const auto& item : items)
    {
      if (IsVideoExtrasFolder(*item))
        folders.emplace_back(item->GetPath());
    }
  }
  else
  {
    std::vector<std::pair<int, std::string>> paths;
    m_database.GetSubPaths(discFolder, paths);
    for (const auto& [idPath, path] : paths)
    {
      if (URIUtils::PathEquals(URIUtils::GetParentPath(path), discFolder, true) &&
          IsVideoExtrasFolderName(URIUtils::GetFileOrFolderName(path)))
        folders.emplace_back(path);
    }
  }

  const bool useFastHash{
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bVideoLibraryUseFastHash};
  for (const std::string& folder : folders)
  {
    // Leave a folder with content set on it (a source root) alone
    SScanSettings settings;
    bool foundDirectly{false};
    if (m_database.GetScraperForPath(folder, settings, foundDirectly) && foundDirectly)
      continue;

    // Without fast hashing the folder's listing is hashed, as the scanner does for its folders
    std::string hash;
    if (useFastHash)
      hash = UTILS::GetFastHash(folder, regexps);
    else
    {
      CFileItemList items;
      CDirectory::GetDirectory(folder, items,
                               CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(),
                               DIR_FLAG_DEFAULTS);
      items.Sort(SortBy::FILE, SortOrder::ASCENDING, SortAttributeNone);
      UTILS::GetPathHash(items, hash);
    }
    if (std::string dbHash; !hash.empty() && m_database.GetPathHash(folder, dbHash) &&
                            StringUtils::EqualsNoCase(hash, dbHash))
      continue;

    if (CInfoScanner::HasNoMedia(folder))
      continue;

    const int dbId{m_database.GetMovieIdInFolder(discFolder, folder)};
    if (dbId < 0)
    {
      CLog::Log(LOGDEBUG, "VideoInfoScanner: No single movie found for video extras {}",
                CURL::GetRedacted(folder));
      continue;
    }

    AddVideoExtras(dbId, folder);
    CLog::Log(LOGDEBUG, "VideoInfoScanner: Finished adding video extras from dir {}",
              CURL::GetRedacted(folder));

    if (!hash.empty())
      m_database.SetPathHash(folder, hash);
  }
}

const CFileItem* FindExtraOnAnotherDisc(const CFileItemList& existing, const CFileItem& extra)
{
  const auto letters{[](const std::string& name)
                     {
                       std::string title{StringUtils::ToLower(name)};
                       std::erase_if(title,
                                     [](char c) { return !StringUtils::isasciialphanum(c); });
                       return title;
                     }};

  // What one playing all of a kind is called, as GetExtraTitle() names it
  const CLocalizeStrings& strings{CServiceBroker::GetResourcesComponent().GetLocalizeStrings()};
  const std::string playAll{strings.Get(40508)};
  const std::string playAllOf{strings.Get(40708)};
  const size_t name{playAllOf.find("{0:s}")};
  const auto isPlayAll{[&playAll, &playAllOf, name](const std::string& title)
                       {
                         if (title == playAll)
                           return true;
                         if (name == std::string::npos)
                           return false;
                         const std::string before{playAllOf.substr(0, name)};
                         const std::string after{playAllOf.substr(name + 5)};
                         return title.size() > before.size() + after.size() &&
                                title.starts_with(before) && title.ends_with(after);
                       }};

  const CVideoInfoTag& tag{*extra.GetVideoInfoTag()};
  const std::string title{letters(tag.GetAssetInfo().GetTitle())};
  const bool playsAll{isPlayAll(tag.GetAssetInfo().GetTitle())};
  const auto same{std::ranges::find_if(
      existing,
      [&tag, &title, playsAll, &letters, &isPlayAll](const auto& other)
      {
        const CVideoInfoTag& otherTag{*other->GetVideoInfoTag()};
        if (std::abs(static_cast<int>(otherTag.GetDuration()) -
                     static_cast<int>(tag.GetDuration())) > 2)
          return false;

        // Each disc may code a kind differently, so playing all of a kind is told by its length
        if (playsAll && isPlayAll(otherTag.GetAssetInfo().GetTitle()))
          return true;

        const std::string otherTitle{letters(otherTag.GetAssetInfo().GetTitle())};
        return !title.empty() && !otherTitle.empty() &&
               (otherTitle.find(title) != std::string::npos ||
                title.find(otherTitle) != std::string::npos);
      })};
  return same != existing.end() ? same->get() : nullptr;
}

bool CVideoInfoScannerExtras::AddDiscExtras(const std::string& disc, int dbId)
{
  CFileItem item{disc, false};
  item.GetVideoInfoTag()->GetAssetInfo().SetType(VideoAssetType::EXTRA);

  CFileItemList extras;
  if (!CDiscDirectoryHelper::GetOrShowPlaylistSelection(item, extras, MenuDecision::SILENT) ||
      extras.IsEmpty())
  {
    CLog::LogF(LOGDEBUG, "No extras named on {}", CURL::GetRedacted(disc));
    return false;
  }

  // Those of this disc are already told apart, so only those of other discs are compared
  CFileItemList existing;
  m_database.GetVideoVersions(VideoDbContentType::MOVIES, dbId, existing, VideoAssetType::EXTRA);

  for (const auto& extra : extras)
  {
    const std::string& path{extra->GetDynPath()};
    const std::string& title{extra->GetVideoInfoTag()->GetAssetInfo().GetTitle()};
    if (m_database.GetVideoVersionInfo(path).m_assetTypeId != -1)
    {
      CLog::LogF(LOGDEBUG, "Extra '{}' ({}) is already in the library", title,
                 CURL::GetRedacted(path));
      continue;
    }
    if (const auto* repeat{FindExtraOnAnotherDisc(existing, *extra)})
    {
      // The extra is kept in its better copy, as a 4K disc's may be in 4K
      const CVideoInfoTag& other{*repeat->GetVideoInfoTag()};
      if (extra->GetVideoInfoTag()->m_streamDetails.GetVideoHeight() <=
          other.m_streamDetails.GetVideoHeight())
      {
        CLog::LogF(LOGDEBUG, "Extra '{}' ({}) is on another disc of the movie", title,
                   CURL::GetRedacted(path));
        continue;
      }

      m_database.BeginTransaction();

      // Deleting the old file deletes its bookmarks, so they are carried over
      VECBOOKMARKS bookmarks;
      m_database.GetBookMarksForFile(other.m_strFileNameAndPath, bookmarks);
      m_database.GetBookMarksForFile(other.m_strFileNameAndPath, bookmarks, CBookmark::RESUME,
                                     true);
      const int idFile{
          m_database.SetFileForMedia(path, VideoDbContentType::MOVIES, dbId,
                                     CVideoDatabase::FileRecord{.m_idFile = other.m_iFileId,
                                                                .m_playCount = other.GetPlayCount(),
                                                                .m_lastPlayed = other.m_lastPlayed,
                                                                .m_dateAdded = other.m_dateAdded})};
      if (idFile > 0 &&
          m_database.SetStreamDetailsForFileId(extra->GetVideoInfoTag()->m_streamDetails, idFile) &&
          std::ranges::all_of(bookmarks,
                              [this, &path](const CBookmark& bookmark)
                              {
                                return m_database.AddBookMarkToFile(path, bookmark,
                                                                    bookmark.type);
                              }))
      {
        m_database.CommitTransaction();
        CLog::LogF(LOGDEBUG, "Extra '{}' moved to its better copy on this disc ({})",
                   other.GetAssetInfo().GetTitle(), CURL::GetRedacted(path));
      }
      else
      {
        m_database.RollbackTransaction();
        CLog::LogF(LOGERROR, "Failed to move extra '{}' to its better copy ({})",
                   other.GetAssetInfo().GetTitle(), CURL::GetRedacted(path));
      }
      continue;
    }

    const int idType{
        m_database.AddVideoVersionType(title, VideoAssetTypeOwner::AUTO, VideoAssetType::EXTRA)};
    if (idType < 0 || !m_database.AddVideoAsset(VideoDbContentType::MOVIES, dbId, idType,
                                                VideoAssetType::EXTRA, *extra))
    {
      CLog::LogF(LOGERROR, "Failed to add extra '{}' ({})", title, CURL::GetRedacted(path));
      continue;
    }

    CLog::LogF(LOGDEBUG, "Added extra '{}' ({})", title, CURL::GetRedacted(path));
  }
  return true;
}
} // namespace KODI::VIDEO
