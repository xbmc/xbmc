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
#include "GUIDialogSelect.h"
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

#include <algorithm>
#include <memory>

using namespace KODI;

namespace
{
class CGetDirectoryItems : public IRunnable
{
public:
  CGetDirectoryItems(const std::string& path, CFileItemList& items, const XFILE::CDirectory::CHints& hints)
    : m_path(path), m_items(items), m_hints(hints)
  {
  }
  void Run() override
  {
    m_result = XFILE::CDirectory::GetDirectory(m_path, m_items, m_hints);
  }
  bool m_result{false};

protected:
  std::string m_path;
  CFileItemList &m_items;
  XFILE::CDirectory::CHints m_hints;
};
} // namespace

bool CGUIDialogSimpleMenu::GetOrShowPlaylistSelection(
    CFileItem& item,
    PLAYLIST::ForcePlaylistSelection forceSelection /* = DONT_FORCE_PLAYLIST_SELECTION */,
    PLAYLIST::ExcludeUsedPlaylists excludeUsedPlaylists /* = DONT_EXCLUDE_USED_PLAYLISTS */)
{
  if (forceSelection != PLAYLIST::ForcePlaylistSelection::FORCE_PLAYLIST_SELECTION &&
      CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
          CSettings::SETTING_DISC_PLAYBACK) != BD_PLAYBACK_SIMPLE_MENU)
    return true;

  const std::string originalDynPath{
      item.GetDynPath()}; // Overwritten by dialog selection. Needed for screen refresh.

  std::string directory{[&item, &originalDynPath]
                        {
                          if (item.GetVideoContentType() == VideoDbContentType::EPISODES)
                          {
                            if (item.m_multipleTitles)
                              return URIUtils::GetBlurayAllEpisodesPath(originalDynPath);
                            const CVideoInfoTag* tag{item.GetVideoInfoTag()};
                            return URIUtils::GetBlurayEpisodePath(originalDynPath, tag->m_iSeason,
                                                                  tag->m_iEpisode);
                          }
                          return URIUtils::GetBlurayRootPath(originalDynPath);
                        }()};

  // Get playlists that are already used (to avoid duplicates in file table)
  std::optional<std::vector<int>> excludePlaylists;
  if (excludeUsedPlaylists == PLAYLIST::ExcludeUsedPlaylists::EXCLUDE_USED_PLAYLISTS)
  {
    CVideoDatabase database;
    if (!database.Open())
    {
      CLog::LogF(LOGERROR, "Failed to open video database");
      return false;
    }
    excludePlaylists =
        database.GetPlaylistsByPath(URIUtils::GetBlurayPlaylistPath(originalDynPath));

    // If replacing existing playlist (FORCE_PLAYLIST_SELECTION), remove it from exclude list
    // as user could choose the same playlist again
    if (forceSelection == PLAYLIST::ForcePlaylistSelection::FORCE_PLAYLIST_SELECTION)
    {
      CRegExp regex{true, CRegExp::autoUtf8, R"(\/(\d+).mpls$)"};
      if (regex.RegFind(originalDynPath) != -1)
      {
        const int playlist{std::stoi(regex.GetMatch(1))};
        std::erase_if(*excludePlaylists, [&playlist](int p) { return p == playlist; });
      }
    }
  }

  CFileItemList items;
  if (!GetItems(item, items, directory, excludePlaylists) ||
      items.Size() <=
          2) // Only main menu and all titles (others removed as already used, or nothing else found)))
  {

    directory = URIUtils::GetBlurayTitlesPath(originalDynPath); // /root/titles - all playlists
    if (!GetItems(item, items, directory, excludePlaylists) ||
        items.Size() <= 2) // Still no playlists found
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

    if (!item_new->m_bIsFolder)
    {
      item.SetDynPath(item_new->GetDynPath());
      item.GetVideoInfoTag()->m_streamDetails =
          item_new->GetVideoInfoTag()
              ->m_streamDetails; // Basic stream details from BLURAY_TITLE INFO
      item.SetProperty("get_stream_details_from_player", true); // Overwrite when played
      item.SetProperty("original_listitem_url", originalDynPath);
      return true;
    }

    if (!GetItems(item, items, item_new->GetDynPath(),
                  excludePlaylists)) // Get selected (usually all) titles
      return true;
  }

  return false;
}

bool CGUIDialogSimpleMenu::GetItems(const CFileItem& item,
                                    CFileItemList& items,
                                    const std::string& directory,
                                    const std::optional<std::vector<int>>& excludePlaylists)
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

  // Remove playlists that are already used
  if (!item.m_multipleTitles && excludePlaylists.has_value() && !excludePlaylists->empty())
  {
    for (const int playlist : *excludePlaylists)
    {
      items.erase(std::remove_if(items.begin(), items.end(), [&playlist](const auto& i)
                                 { return i->GetVideoInfoTag()->m_iTrack == playlist; }),
                  items.end());
    }
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