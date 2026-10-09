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
#include "utils/Artwork.h"
#include "utils/DiscsUtils.h"
#include "utils/FileExtensionProvider.h"
#include "utils/RegExp.h"
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
#include "video/tags/IVideoInfoTagLoader.h"
#include "video/tags/VideoInfoTagLoaderFactory.h"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <set>
#include <string_view>
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

//! The loader of what an export wrote beside a video in an extras folder, read with its scraper
std::unique_ptr<IVideoInfoTagLoader> CreateExportLoader(CVideoDatabase& db,
                                                        const std::string& video,
                                                        const std::string& folder)
{
  const ADDON::ScraperPtr info{db.GetScraperForPath(folder)};
  if (!info)
    return nullptr;
  return std::unique_ptr<IVideoInfoTagLoader>{
      CVideoInfoTagLoaderFactory::CreateLoader(CFileItem{video, false}, info, false)};
}

//! Whether an export records a video as a version or an extra of its movie
bool IsExportedAsset(const CVideoInfoTag& tag)
{
  const VideoAssetType type{tag.GetAssetInfo().GetType()};
  return (type == VideoAssetType::VERSION || type == VideoAssetType::EXTRA) &&
         !tag.GetAssetInfo().GetTitle().empty();
}

//! A version or an extra an export wrote beside it, or false where there is none
bool GetExportedAsset(CVideoDatabase& db,
                      const std::string& video,
                      const std::string& folder,
                      CVideoInfoTag& tag)
{
  const std::unique_ptr<IVideoInfoTagLoader> loader{CreateExportLoader(db, video, folder)};
  return loader && loader->Load(tag, false) == CInfoScanner::InfoType::FULL && IsExportedAsset(tag);
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
        // A movie restored from its nfos has only the extras an export records
        const bool restored{m_restoredFromNfo.contains(dbId)};

        // A bluray is added as the versions and extras its exported nfo records, or as the extras
        // it names, or failing that as a whole. One already there may since have been given a
        // playlist, so it is not added as a whole again.
        if (IsBluray(item->GetPath()) &&
            (HasExtraOnDisc(m_database, item->GetPath(), dbId) ||
             AddNfoDiscExtras(item->GetPath(), path, dbId) || restored ||
             AddDiscExtras(item->GetPath(), dbId)))
          return;

        // An extra already in the library keeps any name or art it has since been given. A version
        // is left one, as converting it would make the movie's own file the extra.
        if (const VideoAssetInfo asset{m_database.GetVideoVersionInfo(item->GetPath())};
            asset.m_assetType != VideoAssetType::UNKNOWN && asset.m_idMedia == dbId)
          return;

        // An export keeps the extra's name and play state in an nfo beside it, and whether it is
        // a version of the movie instead
        std::string extraTypeName;
        VideoAssetType assetType{VideoAssetType::EXTRA};
        if (CVideoInfoTag exported; GetExportedAsset(m_database, item->GetPath(), path, exported))
        {
          extraTypeName = exported.GetAssetInfo().GetTitle();
          assetType = exported.GetAssetInfo().GetType();
          CVideoInfoTag& tag{*item->GetVideoInfoTag()};
          tag.m_dateAdded = exported.m_dateAdded;
          const std::shared_ptr<CAdvancedSettings> advancedSettings{
              CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()};
          if (advancedSettings->m_bVideoLibraryImportWatchedState)
          {
            tag.SetPlayCount(exported.GetPlayCount());
            tag.m_lastPlayed = exported.m_lastPlayed;
          }
          if (advancedSettings->m_bVideoLibraryImportResumePoint)
            tag.SetResumePoint(exported.GetResumePoint());
        }
        else if (restored)
        {
          CLog::LogF(LOGDEBUG, "Extra {} is not in the nfos of its movie",
                     CURL::GetRedacted(item->GetPath()));
          return;
        }
        else
          extraTypeName = CGUIDialogVideoManagerExtras::GenerateVideoExtra(path, item->GetPath());

        const int idVideoAssetType{
            m_database.AddOrValidateVideoVersionType(extraTypeName, assetType)};
        const std::string_view kind{assetType == VideoAssetType::VERSION ? "version" : "extra"};

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
                                       assetType, *item.get()))
          {
            CLog::Log(LOGDEBUG, "VideoInfoScanner: Added video {} {}", kind,
                      CURL::GetRedacted(item->GetPath()));
            const CBookmark& resume{item->GetVideoInfoTag()->GetResumePoint()};
            if (resume.IsSet())
              m_database.AddBookMarkToFile(item->GetPath(), resume, CBookmark::RESUME);
          }
          else
          {
            CLog::Log(LOGERROR, "VideoInfoScanner: Failed to add video {} {}", kind,
                      CURL::GetRedacted(item->GetPath()));
          }
        }
        else
        {
          m_database.ConvertVideoToVersion(VideoDbContentType::MOVIES, idMovie, dbId,
                                           idVideoAssetType, assetType);
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

    // A movie restored from its nfos has the extras they record, not those its discs name
    if (m_restoredFromNfo.contains(dbId))
    {
      CLog::LogF(LOGDEBUG, "Movie {} has the extras of its nfos, not those {} names", dbId,
                 CURL::GetRedacted(path));
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
                                                       const std::vector<std::string>& regexps,
                                                       bool rescan /* = false */)
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
    if (std::string dbHash; !rescan && !hash.empty() && m_database.GetPathHash(folder, dbHash) &&
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

void CVideoInfoScannerExtras::AddMovieExtras(int dbId,
                                             bool useFolderNames,
                                             const std::vector<std::string>& regexps)
{
  CFileItemList versions;
  m_database.GetVideoVersions(VideoDbContentType::MOVIES, dbId, versions, VideoAssetType::VERSION);

  // A disc holding several versions is read once
  std::set<std::string> discs;
  std::set<std::string> folders;
  std::vector<CRegExp> folderStacks{
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_folderStackRegExps};
  for (const auto& version : versions)
  {
    // A movie on several discs is stored under its stack, whose discs are read together
    const std::string& file{version->GetDynPath()};
    const std::string disc{URIUtils::IsBlurayPath(file) ? URIUtils::GetDiscFile(file) : file};
    if (discs.insert(disc).second)
      AddMovieDiscExtras(CFileItem{disc, false});

    std::vector<std::string> paths{disc};
    if (URIUtils::IsStack(disc))
    {
      paths.clear();
      CStackDirectory::GetPaths(disc, paths);
    }
    for (std::string path : paths)
    {
      // The folder holding the movie's file, disc structure or archive
      if (URIUtils::IsBlurayPath(path))
        path = URIUtils::GetDiscFile(path);
      if (URIUtils::IsInArchive(path))
        path = CURL(path).GetHostName();
      std::string folder{URIUtils::IsOpticalMediaFile(path) ? URIUtils::RemoveDiscPath(path)
                                                            : URIUtils::GetDirectory(path)};
      URIUtils::AddSlashAtEnd(folder);
      folders.insert(folder);

      // The extras folders of a movie on several discs (Disc 1, Disc 2) are beside them
      std::string name{folder};
      StringUtils::ToLower(name);
      URIUtils::RemoveSlashAtEnd(name);
      if (std::ranges::any_of(folderStacks,
                              [&name](CRegExp& regExp) { return regExp.RegFind(name) != -1; }))
        folders.insert(URIUtils::GetParentPath(folder));
    }
  }

  if (!useFolderNames)
    return;
  for (const std::string& folder : folders)
    AddVideoExtrasBesideDisc(folder, true, regexps, true);
}

bool CVideoInfoScannerExtras::AddNfoDiscExtras(const std::string& disc,
                                               const std::string& folder,
                                               int dbId)
{
  const std::unique_ptr<IVideoInfoTagLoader> loader{CreateExportLoader(m_database, disc, folder)};
  if (!loader)
    return false;

  const std::shared_ptr<CAdvancedSettings> advancedSettings{
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()};

  // The export writes each playlist of the disc as a <movie> of the nfo, and the disc as a whole as
  // one without a playlist
  bool recorded{false};
  CVideoInfoTag tag;
  int index{1};
  for (CInfoScanner::InfoType result{loader->Load(tag, false)};
       result != CInfoScanner::InfoType::NONE; result = loader->LoadVersion(++index, tag))
  {
    const int playlist{loader->GetBlurayPlaylist()};
    const std::string title{tag.GetAssetInfo().GetTitle()};
    const VideoAssetType assetType{tag.GetAssetInfo().GetType()};
    if (result != CInfoScanner::InfoType::FULL || !IsExportedAsset(tag))
    {
      tag.Reset();
      continue;
    }
    recorded = true;
    const std::string_view kind{assetType == VideoAssetType::VERSION ? "version" : "extra"};

    const std::string path{playlist < 0 ? disc : URIUtils::GetBlurayPlaylistPath(disc, playlist)};
    CFileItem asset{path, false};
    *asset.GetVideoInfoTag() = tag;
    asset.GetVideoInfoTag()->m_strFileNameAndPath = path;
    tag.Reset();

    if (m_database.GetVideoVersionInfo(path).m_assetTypeId != -1)
    {
      CLog::LogF(LOGDEBUG, "The {} '{}' ({}) is already in the library", kind, title,
                 CURL::GetRedacted(path));
      continue;
    }

    // Keep the play state only if advancedsettings.xml says so, as for a movie's nfo
    CVideoInfoTag& assetTag{*asset.GetVideoInfoTag()};
    if (!advancedSettings->m_bVideoLibraryImportWatchedState)
      assetTag.ResetPlayCount();
    if (!advancedSettings->m_bVideoLibraryImportResumePoint)
      assetTag.SetResumePoint(CBookmark());
    if (!assetTag.HasStreamDetails() && playlist >= 0)
      CDiscDirectoryHelper::ReadResolvedPlaylist(asset);

    // The art the export wrote beside the disc is an extra's own, the nfo's the movie's. A
    // version's nfo holds its own, as for one beside the movie.
    if (assetType == VideoAssetType::EXTRA)
    {
      assetTag.m_strPictureURL.Clear();
      assetTag.m_fanart.Clear();
    }
    m_art.GetArtwork(&asset, ADDON::ContentType::MOVIES, false, true, "");
    ART::Artwork art{asset.GetArt()};
    std::erase_if(art, [](const auto& image) { return image.second.starts_with("image://video"); });
    asset.SetArt(art);

    const int idType{m_database.AddOrValidateVideoVersionType(title, assetType)};
    if (idType < 0 ||
        !m_database.AddVideoAsset(VideoDbContentType::MOVIES, dbId, idType, assetType, asset))
    {
      CLog::LogF(LOGERROR, "Failed to add {} '{}' ({})", kind, title, CURL::GetRedacted(path));
      continue;
    }

    CLog::LogF(LOGDEBUG, "Added {} '{}' ({}) from its nfo", kind, title, CURL::GetRedacted(path));
    if (assetTag.GetResumePoint().IsSet())
      m_database.AddBookMarkToFile(path, assetTag.GetResumePoint(), CBookmark::RESUME);
  }
  return recorded;
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
