/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogSimpleMenu.h"

#include "FileItem.h"
#include "GUIDialogOK.h"
#include "GUIDialogSelect.h"
#include "GUIDialogYesNo.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "media/MediaType.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoManagerTypes.h"
#include "video/guilib/VideoGUIUtils.h"

#include <algorithm>
#include <memory>
#include <ranges>
#include <string>
#include <vector>

using namespace KODI;
namespace
{
void RetypeAsVersion(CFileItem& item)
{
  CVideoInfoTag* tag{item.GetVideoInfoTag()};
  tag->m_type = MediaTypeVideoVersion;
  tag->m_iDbId = tag->m_iFileId;
  tag->m_strTitle = tag->GetAssetInfo().GetTitle();
  item.SetTitle(tag->m_strTitle);
  item.SetLabel(tag->m_strTitle);
}

// Take the chosen playlist away from the library items that have it, reverting them to the disc base
bool ReassignPlaylist(const CFileItem& item,
                      const std::vector<CVideoDatabase::PlaylistInfo>& matchingPlaylists)
{
  CVideoDatabase db;
  if (!db.Open())
  {
    CLog::LogF(LOGERROR, "Failed to open video database");
    return false;
  }

  // This is the wrong way to reassign versions - that should be done in the version manager (either
  // changing type or adding/removing)
  int assignedMovie{-1}; // Old movie id
  if (item.HasVideoInfoTag())
  {
    const CVideoInfoTag* tag{item.GetVideoInfoTag()};
    if (tag->m_type == MediaTypeMovie) // In library view
      assignedMovie = tag->m_iDbId;
    else if (tag->m_type == MediaTypeVideoVersion) // From versions manager
      assignedMovie = db.GetVideoVersionInfo(item.GetDynPath()).m_idMedia;
  }
  if (assignedMovie >= 0 &&
      std::ranges::any_of(
          matchingPlaylists, [assignedMovie](const CVideoDatabase::PlaylistInfo& p)
          { return p.mediaType == VideoDbContentType::MOVIES && p.idMedia == assignedMovie; }))
  {
    CGUIDialogOK::ShowAndGetInput(
        CVariant{257}, CVariant{40047}); // This playlist belongs to another version of this movie
    return false;
  }

  // Show warning dialog if the new playlist will displace an existing item in the library
  const bool displacesMovie{
      std::ranges::any_of(matchingPlaylists, [](const CVideoDatabase::PlaylistInfo& p)
                          { return p.mediaType == VideoDbContentType::MOVIES; })};
  if (!CGUIDialogYesNo::ShowAndGetInput(
          CVariant{559}, CVariant{displacesMovie ? 40049 : 40048})) // Movie or episode
    return false;

  std::string base{item.GetDynPath()};
  if (URIUtils::IsBlurayPath(base))
    base = URIUtils::GetDiscFile(base);

  // Loop over all matching playlists and revert the file to the base file (BDMV/ISO)
  struct Displaced
  {
    CFileItem item;
    std::string oldPath;
    int oldFile;
    bool version;
    std::string mediaType;
    int idMedia;
  };
  std::vector<Displaced> displaced;

  db.BeginTransaction();

  for (const auto& it : matchingPlaylists)
  {
    const MediaType& mediaType{it.mediaType == VideoDbContentType::EPISODES ? MediaTypeEpisode
                                                                            : MediaTypeMovie};

    // History belongs to the playlist (watched counts etc.), so it is not carried over.
    // An item already at the base file keeps its own, as SetFileForMedia() rewrites the row
    CVideoInfoTag baseFile;
    const bool baseInUse{db.GetFileInfo(base, baseFile)};
    const int newIdFile{db.SetFileForMedia(
        base, it.mediaType, it.idMedia,
        CVideoDatabase::FileRecord{
            .m_idFile = it.idFile,
            .m_playCount = baseInUse ? baseFile.GetPlayCount() : -1,
            .m_lastPlayed = baseInUse ? baseFile.m_lastPlayed : CDateTime{},
            .m_dateAdded = baseInUse
                               ? baseFile.m_dateAdded
                               : it.dateAdded})}; // Update displaced item and create new idFile
    if (newIdFile <= 0)
    {
      CLog::LogF(LOGERROR, "Failed to move {} {} off playlist {}", mediaType, it.idMedia,
                 it.playlist);
      db.RollbackTransaction();
      return false;
    }
    if (CVideoInfoTag oldFile;
        it.mediaType == VideoDbContentType::MOVIES && db.GetFileInfo("", oldFile, it.idFile))
    {
      db.RollbackTransaction();
      CLog::LogF(LOGERROR, "File {} is still in use after moving {} {} to file {}", it.idFile,
                 mediaType, it.idMedia, newIdFile);
      CGUIDialogOK::ShowAndGetInput(CVariant{257},
                                    CVariant{40051}); // Still in use by another movie version
      return false;
    }

    // Get the details of the displaced item from the database
    CVideoInfoTag details;
    if (it.mediaType == VideoDbContentType::MOVIES)
    {
      if (!db.GetMovieInfo("", details, it.idMedia, -1, newIdFile))
      {
        CLog::LogF(LOGERROR, "Failed to read movie {} on file {}", it.idMedia, newIdFile);
        db.RollbackTransaction();
        return false;
      }
    }
    else
    {
      details = db.GetDetailsByTypeAndId(it.mediaType, it.idMedia);
      if (details.m_iDbId < 0)
      {
        CLog::LogF(LOGERROR, "Failed to read {} {}", mediaType, it.idMedia);
        db.RollbackTransaction();
        return false;
      }
    }

    CLog::LogF(LOGDEBUG, "{} {} moved from playlist {} to file {}", mediaType, it.idMedia,
               it.playlist, newIdFile);
    displaced.emplace_back(Displaced{.item = CFileItem{details},
                                     .oldPath = URIUtils::GetBlurayPlaylistPath(base, it.playlist),
                                     .oldFile = it.idFile,
                                     .version = it.mediaType == VideoDbContentType::MOVIES,
                                     .mediaType = mediaType,
                                     .idMedia = it.idMedia});
  }
  if (!db.CommitTransaction())
  {
    CLog::LogF(LOGERROR, "Failed to commit the playlist reassignment");
    db.RollbackTransaction();
    return false;
  }
  db.Close();

  for (auto& d : displaced)
  {
    d.item.SetDynPath(base);
    VIDEO::UTILS::NotifyItemPathChanged(d.item, d.oldPath, d.oldFile);
    if (d.version)
    {
      RetypeAsVersion(d.item); // Generate FileItem for version update
      VIDEO::UTILS::NotifyItemPathChanged(d.item, d.oldPath, d.oldFile);
    }
  }

  // Cached library listings still hold the old paths, and widgets reload on the announcement
  // Notify the rest of kodi
  CUtil::DeleteVideoDatabaseDirectoryCache();
  for (const auto& d : displaced)
    CVideoDatabase::AnnounceUpdate(d.mediaType, d.idMedia);

  return true;
}
} // namespace

bool CGUIDialogSimpleMenu::ShowPlaylistSelection(
    const CFileItem& item,
    CFileItem& selectedItem,
    const CFileItemList& items,
    const std::vector<CVideoDatabase::PlaylistInfo>& usedPlaylists)
{
  CGUIDialogSelect* dialog{CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
      WINDOW_DIALOG_SELECT)};

  dialog->Reset();
  dialog->SetHeading(CVariant{25006}); // Select playback item
  dialog->SetItems(items);
  dialog->SetUseDetails(true);
  dialog->Open();

  if (dialog->GetSelectedItem() < 0)
  {
    CLog::LogF(LOGDEBUG, "User aborted playlist selection");
    return false;
  }

  // If item is not folder (ie. all titles)
  selectedItem = *dialog->GetSelectedFileItem();
  if (selectedItem.IsFolder())
    return true;

  CLog::LogF(LOGDEBUG, "Playlist {} chosen for {}",
             selectedItem.GetProperty("bluray_playlist").asInteger32(-1),
             CURL::GetRedacted(item.GetDynPath()));

  // See if already selected
  if (usedPlaylists.empty())
    return true; // No playlists used yet

  // See if playlist already used
  const int newPlaylist{selectedItem.GetProperty("bluray_playlist").asInteger32(-1)};
  auto matching{usedPlaylists |
                std::views::filter([newPlaylist](const CVideoDatabase::PlaylistInfo& p)
                                   { return p.playlist == newPlaylist; })};
  if (std::ranges::empty(matching))
    return true; // Not used

  // A playlist already assigned to several episodes is a multi-episode playlist, so it is shared
  // rather than taken away
  if (std::ranges::count_if(matching, [](const CVideoDatabase::PlaylistInfo& p)
                            { return p.mediaType == VideoDbContentType::EPISODES; }) > 1)
    return CGUIDialogYesNo::ShowAndGetInput(CVariant{559}, CVariant{40050});

  return ReassignPlaylist(item, {matching.begin(), matching.end()});
}
