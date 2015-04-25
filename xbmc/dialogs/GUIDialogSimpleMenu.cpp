/*
 *      Copyright (C) 2005-2015 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */


#include "GUIDialogSimpleMenu.h"
#include "guilib/GUIWindowManager.h"
#include "GUIDialogSelect.h"
#include "settings/DiscSettings.h"
#include "settings/Settings.h"
#include "utils/URIUtils.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"
#include "URL.h"

bool CGUIDialogSimpleMenu::ShowPlaySelection(CFileItem& item)
{
  /* if asked to resume somewhere, we should not show anything */
  if (item.m_lStartOffset || (item.HasVideoInfoTag() && item.GetVideoInfoTag()->m_iBookmarkId > 0))
    return true;

  if (CSettings::Get().GetInt("disc.playback") != BD_PLAYBACK_SIMPLE_MENU)
    return true;

  std::string path;
  if (item.IsVideoDb())
    path = item.GetVideoInfoTag()->m_strFileNameAndPath;
  else
    path = item.GetPath();

  if (item.IsBDFile())
  {
    std::string root = URIUtils::GetParentPath(path);
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
    url2.SetHostName(item.GetPath());
    url2.SetFileName("BDMV/index.bdmv");
    if (XFILE::CFile::Exists(url2.Get()))
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

  if (!XFILE::CDirectory::GetDirectory(directory, items, XFILE::CDirectory::CHints(), true))
  {
    CLog::Log(LOGERROR, "CGUIWindowVideoBase::ShowPlaySelection - Failed to get play directory for %s", directory.c_str());
    return true;
  }

  if (items.IsEmpty())
  {
    CLog::Log(LOGERROR, "CGUIWindowVideoBase::ShowPlaySelection - Failed to get any items %s", directory.c_str());
    return true;
  }

  CGUIDialogSelect* dialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  while (true)
  {
    dialog->Reset();
    dialog->SetHeading(25006 /* Select playback item */);
    dialog->SetItems(&items);
    dialog->SetUseDetails(true);
    dialog->DoModal();

    CFileItemPtr item_new = dialog->GetSelectedItem();
    if (!item_new || dialog->GetSelectedLabel() < 0)
    {
      CLog::Log(LOGDEBUG, "CGUIWindowVideoBase::ShowPlaySelection - User aborted %s", directory.c_str());
      break;
    }

    if (item_new->m_bIsFolder == false)
    {
      std::string original_path = item.GetPath();
      item.Reset();
      item = *item_new;
      item.SetProperty("original_listitem_url", original_path);
      return true;
    }

    items.Clear();
    if (!XFILE::CDirectory::GetDirectory(item_new->GetPath(), items, XFILE::CDirectory::CHints(), true) || items.IsEmpty())
    {
      CLog::Log(LOGERROR, "CGUIWindowVideoBase::ShowPlaySelection - Failed to get any items %s", item_new->GetPath().c_str());
      break;
    }
  }

  return false;
}
