/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogSimpleMenu.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIDialogSelect.h"
#include "GUIDialogYesNo.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/StringUtils.h"
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
    const CFileItemList& playlistItems,
    const std::vector<CVideoDatabase::PlaylistInfo>& usedPlaylists)
{
  CGUIDialogSelect* dialog{CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
      WINDOW_DIALOG_SELECT)};

  if (!dialog)
  {
    CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_SELECT instance");
    return false;
  }

  CFileItemList items;
  items.Copy(playlistItems);

  bool chapters{false};
  while (true)
  {
    dialog->Reset();
    dialog->SetHeading(CVariant{25006}); // Select playback item
    dialog->SetItems(items);
    dialog->SetUseDetails(true);
    if (chapters)
    {
      dialog->SetMultiSelection(true);
      dialog->SetEnforceContiguousSelection(true);
    }
    else
    {
      dialog->EnableButton2(true, 21486); // Chapters
      dialog->SetSelectChapters(true);
      dialog->SetUseExtraAsOK(true);
    }
    dialog->Open();

    if (dialog->GetSelectedItem() < 0)
    {
      CLog::LogF(LOGDEBUG, "User aborted playlist selection");
      return false;
    }

    selectedItem = *dialog->GetSelectedFileItem();
    if (selectedItem.IsFolder())
      return true; // If item is a folder (ie. all titles)

    if (chapters)
    {
      // Generate bluray:// path from selected chapters
      std::vector<int> selectedItems{dialog->GetSelectedItems()};
      int startChapter{items[selectedItems.front()]->GetProperty("chapter").asInteger32()};
      int endChapter{items[selectedItems.back()]->GetProperty("chapter").asInteger32()};

      CURL url{selectedItem.GetPath()};
      std::string file{url.GetFileName()};
      file = file.substr(0, file.find("/chapter/"));
      url.SetFileName(file);
      url.SetOptions("?chapters=" + std::to_string(startChapter) + "-" +
                     std::to_string(endChapter));
      selectedItem.SetPath(url.Get());
      return true;
    }

    if (dialog->IsButton2Pressed()) // Chapter
    {
      // Generate chapter list
      items.Clear();
      for (const auto& chapter : selectedItem.GetVideoInfoTag()->GetChapters())
      {
        const auto chapterItem{std::make_shared<CFileItem>(
            URIUtils::AddFileToFolder(selectedItem.GetDynPath(), "chapter",
                                      std::to_string(chapter.chapter)),
            false)};
        chapterItem->SetLabel(
            CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(21396) /* Chapter */ +
            " " + std::to_string(chapter.chapter));
        chapterItem->SetLabel2(
            CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(335) /* Start */ +
            " " +
            StringUtils::SecondsToTimeString(static_cast<int>(chapter.start.count() / 1000),
                                             TIME_FORMAT_HH_MM_SS) +
            " - " +
            CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(180) /* Duration */ +
            " " +
            StringUtils::SecondsToTimeString(static_cast<int>(chapter.duration.count() / 1000),
                                             TIME_FORMAT_HH_MM_SS));
        chapterItem->GetVideoInfoTag()->m_streamDetails =
            selectedItem.GetVideoInfoTag()->m_streamDetails;
        chapterItem->GetVideoInfoTag()->SetChapters(selectedItem.GetVideoInfoTag()->GetChapters());
        chapterItem->SetArt("icon", "DefaultStudios.png");
        chapterItem->SetProperty("chapter", chapter.chapter);
        items.Add(chapterItem);
      }
      chapters = true;
      continue;
    }

    // See if menu selected
    if (URIUtils::GetFileName(selectedItem.GetPath()) == "menu")
      return true;

    if (usedPlaylists.empty())
      return true; // No used playlists

    // See if playlist already used
    const int newPlaylist{selectedItem.GetProperty("bluray_playlist").asInteger32(0)};
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

      for (const auto& it : matchingPlaylists)
      {
        // Revert file to base file (BDMV/ISO)
        db.BeginTransaction();
        if (db.SetFileForMedia(
                base, it.mediaType, it.idMedia,
                CVideoDatabase::FileRecord{.m_idFile = it.idFile,
                                           .m_dateAdded = item.GetVideoInfoTag()->m_dateAdded}) > 0)
          db.CommitTransaction();
        else
          db.RollbackTransaction();
      }
      db.Close();
    }

    return true;
  }
}
