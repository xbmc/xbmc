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

// Take the chosen playlist away from the library items that have it, reverting them to the disc bast
bool ReassignPlaylist(const CFileItem& item,
                      const std::vector<CVideoDatabase::PlaylistInfo>& matchingPlaylists)
{
  // This is the wrong way to reassign versions - that should be done in the version manager (either
  // changing type or adding/removing)
  if (item.HasVideoInfoTag() && item.GetVideoInfoTag()->m_type == MediaTypeMovie)
  {
    const int assignedMovie{item.GetVideoInfoTag()->m_iDbId}; // Old movie id
    if (std::ranges::any_of(
            matchingPlaylists, [assignedMovie](const CVideoDatabase::PlaylistInfo& p)
            { return p.mediaType == VideoDbContentType::MOVIES && p.idMedia == assignedMovie; }))
    {
      CGUIDialogOK::ShowAndGetInput(
          CVariant{257}, CVariant{40047}); // This playlist belongs to another version of this movie
      return false;
    }
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

  CVideoDatabase db;
  if (!db.Open())
  {
    CLog::LogF(LOGERROR, "Failed to open video database");
    return false;
  }

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
    // History belongs to the playlist (watched counts etc.), so it is not carried over
    const int newIdFile{db.SetFileForMedia(
        base, it.mediaType, it.idMedia,
        CVideoDatabase::FileRecord{
            .m_idFile = it.idFile,
            .m_dateAdded = it.dateAdded})}; // Update displaced item and create new idFile
    if (newIdFile <= 0)
    {
      db.RollbackTransaction();
      return false;
    }
    if (CVideoInfoTag oldFile;
        it.mediaType == VideoDbContentType::MOVIES && db.GetFileInfo("", oldFile, it.idFile))
    {
      db.RollbackTransaction();
      CGUIDialogOK::ShowAndGetInput(
          CVariant{257}, CVariant{40047}); // This playlist belongs to another version of this movie
      return false;
    }

    // Get the details of the displaced item from the database
    CVideoInfoTag details;
    if (it.mediaType == VideoDbContentType::MOVIES)
    {
      if (!db.GetMovieInfo("", details, it.idMedia, -1, newIdFile))
      {
        db.RollbackTransaction();
        return false;
      }
    }
    else
      details = db.GetDetailsByTypeAndId(it.mediaType, it.idMedia);

    CLog::LogF(LOGDEBUG, "{} {} moved from playlist {} to file {}",
               it.mediaType == VideoDbContentType::EPISODES ? MediaTypeEpisode : MediaTypeMovie,
               it.idMedia, it.playlist, newIdFile);
    displaced.emplace_back(Displaced{.item = CFileItem{details},
                                     .oldPath = URIUtils::GetBlurayPlaylistPath(base, it.playlist),
                                     .oldFile = it.idFile,
                                     .version = it.mediaType == VideoDbContentType::MOVIES,
                                     .mediaType = it.mediaType == VideoDbContentType::EPISODES
                                                      ? MediaTypeEpisode
                                                      : MediaTypeMovie,
                                     .idMedia = it.idMedia});
  }
  db.CommitTransaction();
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

  if (selectedItem.HasVideoInfoTag() && selectedItem.GetVideoInfoTag()->m_EpBookmark.IsSet())
    return true; // Several episodes share a multi-episode playlist

  // See if playlist already used
  const int newPlaylist{selectedItem.GetProperty("bluray_playlist").asInteger32(-1)};
  const int ownFile{item.HasVideoInfoTag() ? item.GetVideoInfoTag()->m_iFileId : -1};
  auto matching{usedPlaylists |
                std::views::filter([newPlaylist, ownFile](const CVideoDatabase::PlaylistInfo& p)
                                   { return p.playlist == newPlaylist && p.idFile != ownFile; })};
  if (std::ranges::empty(matching))
    return true; // Not used

  return ReassignPlaylist(item, {matching.begin(), matching.end()});
}
