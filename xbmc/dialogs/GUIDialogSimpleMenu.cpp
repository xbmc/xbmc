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
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoFileItemClassify.h"

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
  void Run() override { m_result = XFILE::CDirectory::GetDirectory(m_path, m_items, m_hints); }

  bool m_result;

protected:
  std::string m_path;
  CFileItemList& m_items;
  XFILE::CDirectory::CHints m_hints;
};

class CGetEpisodeDirectoryItems : public IRunnable
{
public:
  CGetEpisodeDirectoryItems(const std::string& path, CFileItemList& items, const CFileItem& item)
    : m_path(path), m_items(items), m_item(item)
  {
  }
  void Run() override { m_result = XFILE::CDirectory::GetDirectory(m_path, m_items, m_item); }

  bool m_result;

protected:
  std::string m_path;
  CFileItemList& m_items;
  CFileItem m_item;
};

class CGetMainItem : public IRunnable
{
public:
  CGetMainItem(const std::string& path, CFileItem& main, const CFileItem& item)
    : m_path(path), m_main(main), m_item(item)
  {
  }
  void Run() override { m_result = XFILE::CDirectory::GetDirectory(m_path, m_main, m_item); }

  bool m_result;

protected:
  std::string m_path;
  CFileItem& m_main;
  CFileItem m_item;
};
} // namespace

bool CGUIDialogSimpleMenu::ShowPlaySelection(CFileItem& item, bool forceSelection /* = false */)
{
  std::string directory{};

  if (forceSelection && (VIDEO::IsBlurayPlaylist(item) || VIDEO::IsDVDPlaylist(item)))
  {
    item.SetProperty("save_dyn_path", item.GetDynPath()); // save for screen refresh later
    if (VIDEO::IsBlurayPlaylist(item))
      item.SetDynPath(URIUtils::GetBlurayPath(item.GetDynPath()));
    else
      item.SetDynPath(URIUtils::GetDVDPath(item.GetDynPath()));
  }

  if (VIDEO::IsDVDFile(item))
  {
    std::string root = URIUtils::GetParentPath(item.GetDynPath());
    URIUtils::RemoveSlashAtEnd(root);
    if (URIUtils::GetFileName(root) == "VIDEO_TS")
    {
      CURL url("dvd://");
      url.SetHostName(URIUtils::GetParentPath(root));
      url.SetFileName("root");
      directory = url.Get();
    }
  }

  if (VIDEO::IsBDFile(item))
  {
    std::string root = URIUtils::GetParentPath(item.GetDynPath());
    URIUtils::RemoveSlashAtEnd(root);
    if (URIUtils::GetFileName(root) == "BDMV")
    {
      CURL url("bluray://");
      url.SetHostName(URIUtils::GetParentPath(root));
      url.SetFileName("root");
      directory = url.Get();
    }
  }

  if (item.IsDiscImage())
  {
    CURL url2("udf://");
    url2.SetHostName(item.GetDynPath());

    // Blu-ray ISO
    url2.SetFileName("BDMV/index.bdmv");
    if (CFileUtils::Exists(url2.Get()))
    {
      url2.SetFileName("");

      CURL url("bluray://");
      url.SetHostName(url2.Get());
      url.SetFileName("root");
      directory = url.Get();
    }

    // DVD ISO
    url2.SetFileName("VIDEO_TS/VIDEO_TS.IFO");
    if (CFileUtils::Exists(url2.Get()))
    {
      url2.SetFileName("");

      CURL url("dvd://");
      url.SetHostName(url2.Get());
      url.SetFileName("root");
      directory = url.Get();
    }
  }

  if (!directory.empty())
  {
    if ((URIUtils::IsProtocol(directory, "dvd") &&
         (forceSelection ||
          CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
              CSettings::SETTING_DVDS_PLAYBACK) == DVD_PLAYBACK_SIMPLE_MENU ||
          CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
              CSettings::SETTING_DVDS_PLAYBACK) == DVD_PLAYBACK_MAIN_TITLE)) ||
        (URIUtils::IsProtocol(directory, "bluray") &&
         (forceSelection ||
          CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
              CSettings::SETTING_DISC_PLAYBACK) == BD_PLAYBACK_SIMPLE_MENU ||
          CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
              CSettings::SETTING_DISC_PLAYBACK) == BD_PLAYBACK_MAIN_TITLE)))
      // Show simple menu selection
      return ShowPlaySelection(item, directory);

    if ((URIUtils::IsProtocol(directory, "dvd") &&
         CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
             CSettings::SETTING_DVDS_PLAYBACK) == DVD_PLAYBACK_MAIN_TITLE) ||
        (URIUtils::IsProtocol(directory, "bluray") &&
         CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
             CSettings::SETTING_DISC_PLAYBACK) == BD_PLAYBACK_MAIN_TITLE))
    {
      // Select main title (not for episodes)
      CFileItem main;
      GetMainItem(directory, main, item);
      const std::string original_path = item.GetDynPath();
      item.SetDynPath(main.GetDynPath());
      item.SetProperty("get_stream_details_from_player", true);
      item.SetProperty("original_listitem_url", original_path);
      return true;
    }
  }

  return true;
}

bool CGUIDialogSimpleMenu::ShowPlaySelection(CFileItem& item, const std::string& directory)
{

  CFileItemList items;

  if (item.GetVideoContentType() == VideoDbContentType::EPISODES)
    // Try to show episodes instead of titles
    GetEpisodeDirectoryItems(directory, items, item);

  if (items.IsEmpty())
  {
    // Not episode or new episode search failed, so do it the old way
    if (!GetDirectoryItems(directory, items, XFILE::CDirectory::CHints()))
    {
      CLog::LogF(LOGERROR, "Failed to get play directory for {}", directory);
      return true;
    }
  }

  if (items.IsEmpty())
  {
    CLog::LogF(LOGERROR, "Failed to get any items {}", directory);
    return true;
  }

  CGUIDialogSelect* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
          WINDOW_DIALOG_SELECT);
  while (true)
  {
    dialog->Reset();
    dialog->SetHeading(CVariant{25006}); // Select playback item
    dialog->SetItems(items);
    dialog->SetUseDetails(true);
    dialog->Open();

    CFileItemPtr item_new = dialog->GetSelectedFileItem();
    if (!item_new || dialog->GetSelectedItem() < 0)
    {
      CLog::Log(LOGDEBUG, "CGUIWindowVideoBase::ShowPlaySelection - User aborted {}", directory);
      break;
    }

    if (item_new->m_bIsFolder == false)
    {
      std::string path;
      if (item.HasProperty("save_dyn_path"))
        path = item.GetProperty("save_dyn_path").asString();
      else
        path = item.GetDynPath(); // If not set above (choose playlist selected)
      item.SetDynPath(item_new->GetDynPath());
      item.SetProperty("get_stream_details_from_player", true);
      item.SetProperty("original_listitem_url", path);
      if (item_new->HasProperty("chapter_duration"))
        item.SetProperty("chapter_duration",
                         item_new->GetProperty("chapter_duration").asUnsignedInteger());
      return true;
    }

    items.Clear();
    if (!GetDirectoryItems(item_new->GetDynPath(), items, XFILE::CDirectory::CHints()) ||
        items.IsEmpty())
    {
      CLog::Log(LOGERROR, "CGUIWindowVideoBase::ShowPlaySelection - Failed to get any items {}",
                item_new->GetPath());
      break;
    }
  }

  return false;
}

bool CGUIDialogSimpleMenu::GetDirectoryItems(const std::string& path,
                                             CFileItemList& items,
                                             const XFILE::CDirectory::CHints& hints)
{
  CGetDirectoryItems getItems(path, items, hints);
  if (!CGUIDialogBusy::Wait(&getItems, 100, true))
  {
    return false;
  }
  return getItems.m_result;
}

bool CGUIDialogSimpleMenu::GetEpisodeDirectoryItems(const std::string& path,
                                                    CFileItemList& items,
                                                    const CFileItem& item)
{
  CGetEpisodeDirectoryItems getItems(path, items, item);
  if (!CGUIDialogBusy::Wait(&getItems, 100, true))
  {
    return false;
  }
  return getItems.m_result;
}

bool CGUIDialogSimpleMenu::GetMainItem(const std::string& path,
                                       CFileItem& main,
                                       const CFileItem& item)
{
  CGetMainItem getItems(path, main, item);
  if (!CGUIDialogBusy::Wait(&getItems, 100, true))
  {
    return false;
  }
  return getItems.m_result;
}