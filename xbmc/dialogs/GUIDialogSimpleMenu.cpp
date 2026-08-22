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
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

#include <memory>
#include <ranges>
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

        const std::string& base{item.GetDynPath()};

        CVideoDatabase db;
        if (!db.Open())
        {
          CLog::LogF(LOGERROR, "Failed to open video database");
          return false;
        }

        // The displaced items are reverted together. A playlist belongs to the one title, so
        // leaving any of them holding it would hand it to two at once - the selection is
        // abandoned rather than half made.
        db.BeginTransaction();
        for (const auto& it : matchingPlaylists)
        {
          // Revert the displaced item to a select path. The playlist belongs to it alone, so its
          // file record is simply renamed and keeps its idFile - and with it the item's bookmarks,
          // stream details and settings.
          const std::string revertPath{
              it.mediaType == VideoDbContentType::EPISODES
                  ? URIUtils::GetBluraySelectPath(base, it.season, it.episode)
                  : URIUtils::GetBluraySelectPath(base)};
          if (revertPath.empty() || !db.RenameFile(it.idFile, revertPath))
          {
            db.RollbackTransaction();
            db.Close();
            CLog::LogF(LOGERROR,
                       "Unable to revert file {} of {} to a select path, so playlist {} is left "
                       "where it is",
                       it.idFile, CURL::GetRedacted(base), newPlaylist);

            // The user has just agreed to the change, so say that it hasn't happened rather than
            // leave the dialog closing as though it had
            CGUIDialogOK::ShowAndGetInput(CVariant{559}, CVariant{25021});
            return false;
          }
        }
        db.CommitTransaction();
        db.Close();
      }
    }
  }
  return true;
}
