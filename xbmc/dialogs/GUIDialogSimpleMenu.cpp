/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogSimpleMenu.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIDialogOK.h"
#include "GUIDialogSelect.h"
#include "GUIDialogYesNo.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "dialogs/GUIDialogBusy.h"
#include "filesystem/Directory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "settings/DiscSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/IRunnable.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

#include <memory>
#include <ranges>
#include <vector>

using namespace KODI;

namespace
{
class CGetDirectoryItems : public IRunnable
{
public:
  CGetDirectoryItems(const std::string& path,
                     CFileItemList& items,
                     const XFILE::CDirectory::CHints& hints)
    : m_path(path), m_items(items), m_hints(hints)
  {
  }
  void Run() override
  {
    m_result = XFILE::CDirectory::GetDirectory(m_path, m_items, m_hints);
  }
  bool m_result;
protected:
  std::string m_path;
  CFileItemList &m_items;
  XFILE::CDirectory::CHints m_hints;
};
}

bool CGUIDialogSimpleMenu::ShowPlaylistSelection(CFileItem& item)
{
  const bool forceSelection{item.GetProperty("force_playlist_selection").asBoolean(false)};
  if (!forceSelection && CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
                             CSettings::SETTING_DISC_PLAYBACK) != BD_PLAYBACK_SIMPLE_MENU)
    return true;

  const std::string originalDynPath{
      item.GetDynPath()}; // Overwritten by dialog selection. Needed for screen refresh.

  const std::string directory{
      [&item, &originalDynPath]
      {
        if (item.GetVideoContentType() == VideoDbContentType::EPISODES)
        {
          const CVideoInfoTag* tag{item.GetVideoInfoTag()};
          return URIUtils::GetBlurayEpisodePath(originalDynPath, tag->m_iSeason, tag->m_iEpisode);
        }
        return URIUtils::GetBlurayRootPath(originalDynPath);
      }()};

  // Get playlists that are already used (for warning after selection to avoid duplicates in file table)
  std::vector<CVideoDatabase::PlaylistInfo> usedPlaylists{};
  CVideoDatabase database;
  if (!database.Open())
  {
    CLog::LogF(LOGERROR, "Failed to open video database");
    return false;
  }
  usedPlaylists = database.GetPlaylistsByPath(URIUtils::GetBlurayPlaylistPath(originalDynPath));

  // If replacing existing playlist (FORCE_PLAYLIST_SELECTION), remove it from exclude list
  // as user could choose the same playlist again
  if (forceSelection)
  {
    CRegExp regex{true, CRegExp::autoUtf8, R"(\/(\d{5}).mpls$)"};
    if (regex.RegFind(originalDynPath) != -1)
    {
      const int playlist{std::stoi(regex.GetMatch(1))};
      std::erase_if(usedPlaylists, [&playlist](const CVideoDatabase::PlaylistInfo& p)
                    { return p.playlist == playlist; });
    }
  }

  // Get items
  CFileItemList items;
  if (!GetItems(item, items, directory))
  {
    // No main movie or episode playlist found
    CGUIDialogOK::ShowAndGetInput(
        CVariant{257},
        CVariant{item.GetVideoContentType() == VideoDbContentType::EPISODES ? 25017 : 25016});
    return false;
  }

  CGUIDialogSelect* dialog{CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
      WINDOW_DIALOG_SELECT)};
  while (true)
  {
    dialog->Reset();
    dialog->SetHeading(CVariant{25006}); // Select playback item
    dialog->SetItems(items);
    dialog->SetUseDetails(true);
    dialog->Open();

    const std::shared_ptr<CFileItem> item_new{dialog->GetSelectedFileItem()};
    if (!item_new || dialog->GetSelectedItem() < 0)
    {
      CLog::LogF(LOGDEBUG, "User aborted {}", directory);
      break;
    }

    // If item is not folder (ie. all titles)
    if (!item_new->IsFolder())
    {
      if (!usedPlaylists.empty())
      {
        // See if playlist already used
        const int newPlaylist{item_new->GetProperty("bluray_playlist").asInteger32(0)};
        auto matchingPlaylists{
            usedPlaylists | std::views::filter([newPlaylist](const CVideoDatabase::PlaylistInfo& p)
                                               { return p.playlist == newPlaylist; })};

        if (std::ranges::distance(matchingPlaylists) > 0)
        {
          // Warn that this playlist is already associated with an episode
          if (!CGUIDialogYesNo::ShowAndGetInput(CVariant{559}, CVariant{25015}))
            return false;

          std::string base{originalDynPath};
          if (URIUtils::IsBlurayPath(base))
            base = URIUtils::GetDiscFile(base);

          for (const auto& it : matchingPlaylists)
          {
            // Revert file to base file (BDMV/ISO)
            database.BeginTransaction();
            if (database.SetFileForMedia(base, it.mediaType, it.idMedia,
                                         CVideoDatabase::FileRecord{
                                             .m_idFile = it.idFile,
                                             .m_dateAdded = item.GetVideoInfoTag()->m_dateAdded}) >
                0)
              database.CommitTransaction();
            else
              database.RollbackTransaction();
          }
        }
      }

      item.SetDynPath(item_new->GetDynPath());
      item.GetVideoInfoTag()->m_streamDetails =
          item_new->GetVideoInfoTag()
              ->m_streamDetails; // Basic stream details from BLURAY_TITLE INFO
      item.SetProperty("get_stream_details_from_player", true); // Overwrite when played
      item.SetProperty("original_listitem_url", originalDynPath);
      return true;
    }

    if (!GetItems(item, items, item_new->GetDynPath())) // Get selected (usually all) titles
      return true;
  }

  return false;
}

bool CGUIDialogSimpleMenu::GetItems(const CFileItem& item,
                                    CFileItemList& items,
                                    const std::string& directory)
{
  items.Clear();
  if (!GetDirectoryItems(directory, items, XFILE::CDirectory::CHints()))
  {
    CLog::LogF(LOGERROR, "Failed to get play directory for {}", directory);
    return false;
  }

  if (items.IsEmpty())
  {
    CLog::LogF(LOGERROR, "Failed to get any items in {}", directory);
    return false;
  }

  return true;
}

bool CGUIDialogSimpleMenu::GetDirectoryItems(const std::string &path, CFileItemList &items,
                                             const XFILE::CDirectory::CHints &hints)
{
  CGetDirectoryItems getItems(path, items, hints);
  if (!CGUIDialogBusy::Wait(&getItems, 100, true))
  {
    return false;
  }
  return getItems.m_result;
}
