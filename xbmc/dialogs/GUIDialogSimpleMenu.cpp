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
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "media/MediaType.h"
#include "utils/ItemProperties.h"
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
  tag->SetMediaType(MEDIA::TYPE::VIDEO_VERSION);
  tag->m_iDbId = tag->m_iFileId;
  tag->m_strTitle = tag->GetAssetInfo().GetTitle();
  item.SetTitle(tag->m_strTitle);
  item.SetLabel(tag->m_strTitle);
}

// Take the chosen playlist away from the library items that have it, reverting them to the disc base
bool ReassignPlaylist(const CFileItem& item,
                      const std::vector<KODI::VIDEO::DiscPlaylist>& matchingPlaylists)
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
    if (tag->GetMediaType() == MEDIA::TYPE::MOVIE) // In library view
      assignedMovie = tag->m_iDbId;
    else if (tag->GetMediaType() == MEDIA::TYPE::VIDEO_VERSION) // From versions manager
      assignedMovie = db.GetVideoVersionInfo(item.GetDynPath()).m_idMedia;
  }
  const auto own{std::ranges::find_if(
      matchingPlaylists, [assignedMovie](const KODI::VIDEO::DiscPlaylist& p)
      { return p.mediaType == VideoDbContentType::MOVIES && p.idMedia == assignedMovie; })};
  if (assignedMovie >= 0 && own != matchingPlaylists.end())
  {
    // This playlist belongs to (another version of) this movie
    const bool isExtra{item.GetVideoInfoTag()->GetAssetInfo().GetType() == VideoAssetType::EXTRA ||
                       own->itemType == VideoAssetType::EXTRA};
    CGUIDialogOK::ShowAndGetInput(CVariant{257}, CVariant{isExtra ? 40055 : 40047});
    return false;
  }

  // Show warning dialog if the new playlist will displace an existing item in the library
  const auto displacedMovie{std::ranges::find(matchingPlaylists, VideoDbContentType::MOVIES,
                                              &KODI::VIDEO::DiscPlaylist::mediaType)};
  const int question{displacedMovie == matchingPlaylists.end()           ? 40048 // Episode
                     : displacedMovie->itemType == VideoAssetType::EXTRA ? 40056
                                                                         : 40049};
  if (!CGUIDialogYesNo::ShowAndGetInput(CVariant{559}, CVariant{question}))
    return false;

  std::string base{item.GetDynPath()};
  if (URIUtils::IsBlurayPath(base))
    base = URIUtils::GetDiscFile(base);

  std::vector<CVideoDatabase::PlaylistRevert> reverted;
  switch (db.RevertPlaylistsToDisc(base, matchingPlaylists, reverted))
  {
    case CVideoDatabase::PlaylistRevertResult::DONE:
      break;
    case CVideoDatabase::PlaylistRevertResult::IN_USE:
      CGUIDialogOK::ShowAndGetInput(CVariant{257},
                                    CVariant{40051}); // Still in use by another movie version
      return false;
    case CVideoDatabase::PlaylistRevertResult::FAILED:
      return false;
  }
  db.Close();

  for (const auto& r : reverted)
  {
    CFileItem moved{r.details};
    moved.SetDynPath(base);
    const std::string oldPath{URIUtils::GetBlurayPlaylistPath(base, r.playlist)};
    VIDEO::UTILS::NotifyItemPathChanged(moved, oldPath, r.oldIdFile);
    if (r.details.GetMediaType() == MEDIA::TYPE::MOVIE)
    {
      RetypeAsVersion(moved); // Generate FileItem for version update
      VIDEO::UTILS::NotifyItemPathChanged(moved, oldPath, r.oldIdFile);
    }
  }

  return true;
}
} // namespace

bool CGUIDialogSimpleMenu::ShowPlaylistSelection(
    const CFileItem& item,
    CFileItem& selectedItem,
    const CFileItemList& items,
    const std::vector<KODI::VIDEO::DiscPlaylist>& usedPlaylists)
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
             selectedItem.GetProperty(ITEM::PROPERTY::BLURAY_PLAYLIST).asInteger32(-1),
             CURL::GetRedacted(item.GetDynPath()));

  // See if already selected
  if (usedPlaylists.empty())
    return true; // No playlists used yet

  // See if playlist already used
  const int newPlaylist{selectedItem.GetProperty(ITEM::PROPERTY::BLURAY_PLAYLIST).asInteger32(-1)};
  auto matching{usedPlaylists | std::views::filter([newPlaylist](const KODI::VIDEO::DiscPlaylist& p)
                                                   { return p.playlist == newPlaylist; })};
  if (std::ranges::empty(matching))
    return true; // Not used

  // A playlist already assigned to several episodes is a multi-episode playlist, so it is shared
  // rather than taken away
  if (std::ranges::count_if(matching, [](const KODI::VIDEO::DiscPlaylist& p)
                            { return p.mediaType == VideoDbContentType::EPISODES; }) > 1)
    return CGUIDialogYesNo::ShowAndGetInput(CVariant{559}, CVariant{40050});

  return ReassignPlaylist(item, {matching.begin(), matching.end()});
}
