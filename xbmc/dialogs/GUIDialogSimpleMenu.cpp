/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogSimpleMenu.h"

#include "FileItem.h"
#include "GUIDialogSelect.h"
#include "GUIDialogYesNo.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "media/MediaType.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/guilib/VideoGUIUtils.h"

#include <memory>
#include <ranges>
#include <utility>
#include <vector>

using namespace KODI;

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
  if (!selectedItem.IsFolder())
  {
    // See if already selected
    if (!usedPlaylists.empty())
    {
      // See if playlist already used
      const int newPlaylist{selectedItem.GetProperty("bluray_playlist").asInteger32(-1)};
      auto matchingPlaylists{usedPlaylists |
                             std::views::filter([newPlaylist](const CVideoDatabase::PlaylistInfo& p)
                                                { return p.playlist == newPlaylist; })};

      if (std::ranges::distance(matchingPlaylists) > 0)
      {
        // Warn that this playlist is already associated with an episode
        if (!CGUIDialogYesNo::ShowAndGetInput(CVariant{559}, CVariant{25015}))
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

        std::vector<std::pair<std::string, int>> reverted;
        for (const auto& it : matchingPlaylists)
        {
          // Revert file to base file (BDMV/ISO). Play history belongs to the playlist, so it is
          // not carried over to the disc.
          db.BeginTransaction();
          const int newIdFile{db.SetFileForMedia(
              base, it.mediaType, it.idMedia,
              CVideoDatabase::FileRecord{.m_idFile = it.idFile, .m_dateAdded = it.dateAdded})};
          if (newIdFile <= 0)
          {
            db.RollbackTransaction();
            continue;
          }
          db.CommitTransaction();

          // Listings holding the displaced item still show it on the playlist, so it is looked
          // up by that. The disc path alone would also match other unresolved episodes on it.
          reverted.emplace_back(it.mediaType == VideoDbContentType::EPISODES ? MediaTypeEpisode
                                                                             : MediaTypeMovie,
                                it.idMedia);

          CFileItem displaced{db.GetDetailsByTypeAndId(it.mediaType, it.idMedia)};
          if (it.mediaType == VideoDbContentType::MOVIES &&
              displaced.GetVideoInfoTag()->m_iFileId != newIdFile)
          {
            // The details are the movie's default version, but another version was displaced.
            // A version is known by its file, so the id listings hold for it is the old one.
            CVideoInfoTag version;
            if (!db.GetFileInfo("", version, newIdFile))
            {
              CLog::LogF(LOGERROR, "Unable to read file {} for the displaced version", newIdFile);
              continue;
            }
            version.m_type = MediaTypeVideoVersion;
            version.m_iDbId = it.idFile;
            displaced = CFileItem{version};
          }
          displaced.SetDynPath(base);
          KODI::VIDEO::UTILS::NotifyItemPathChanged(
              displaced, URIUtils::GetBlurayPlaylistPath(base, it.playlist));
        }
        db.Close();

        // Cached library listings still hold the old paths, and widgets reload on the announcement
        if (!reverted.empty())
        {
          CUtil::DeleteVideoDatabaseDirectoryCache();
          for (const auto& [type, id] : reverted)
            CVideoDatabase::AnnounceUpdate(type, id);
        }
      }
    }
  }
  return true;
}
