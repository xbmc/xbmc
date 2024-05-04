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
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"

using namespace KODI;

namespace
{
class CGetDirectoryItems : public IRunnable
{
public:
  CGetDirectoryItems(const std::string &path, CFileItemList &items, const XFILE::CDirectory::CHints &hints)
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

bool CGUIDialogSimpleMenu::ShowPlaySelection(CFileItem& item, bool forceSelection /* = false */)
{
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_DISC_PLAYBACK) != BD_PLAYBACK_SIMPLE_MENU)
    return true;

  if (forceSelection && VIDEO::IsBlurayPlaylist(item))
  {
    item.SetProperty("save_dyn_path", item.GetDynPath()); // save for screen refresh later
    item.SetDynPath(item.GetBlurayPath());
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
      return ShowPlaySelection(item, url.Get());
    }
  }

  if (item.IsDiscImage())
  {
    CURL url2("udf://");
    url2.SetHostName(item.GetDynPath());
    url2.SetFileName("BDMV/index.bdmv");
    if (CFileUtils::Exists(url2.Get()))
    {
      url2.SetFileName("");

      CURL url("bluray://");
      url.SetHostName(url2.Get());
      url.SetFileName("root");
      return ShowPlaySelection(item, url.Get());
    }
  }
  return true;
}

bool CGUIDialogSimpleMenu::ShowPlaySelection(CFileItem& item, const std::string& directory)
{

  CFileItemList items;

  if (!GetDirectoryItems(directory, items, XFILE::CDirectory::CHints()))
  {
    CLog::Log(LOGERROR,
              "CGUIWindowVideoBase::ShowPlaySelection - Failed to get play directory for {}",
              directory);
    return true;
  }

  if (items.IsEmpty())
  {
    CLog::Log(LOGERROR, "CGUIWindowVideoBase::ShowPlaySelection - Failed to get any items {}",
              directory);
    return true;
  }

  CGUIDialogSelect* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
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
      return true;
    }

    items.Clear();
    if (!GetDirectoryItems(item_new->GetDynPath(), items, XFILE::CDirectory::CHints()) || items.IsEmpty())
    {
      CLog::Log(LOGERROR, "CGUIWindowVideoBase::ShowPlaySelection - Failed to get any items {}",
                item_new->GetPath());
      break;
    }
  }

  return false;
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
