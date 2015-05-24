/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "GUIViewStateAddonBrowser.h"
#include "FileItem.h"
#include "filesystem/File.h"
#include "guilib/GraphicContext.h"
#include "guilib/WindowIDs.h"
#include "view/ViewState.h"
#include "addons/Addon.h"
#include "addons/AddonManager.h"

using namespace XFILE;
using namespace ADDON;

CGUIViewStateAddonBrowser::CGUIViewStateAddonBrowser(const CFileItemList& items) : CGUIViewState(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SortByNone, 551, LABEL_MASKS("%F", "", "%L", ""));
    SetSortMethod(SortByNone);
  }
  else
  {
    AddSortMethod(SortByLabel, SortAttributeIgnoreFolders, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | Foldername, empty
    SetSortMethod(SortByLabel);
  }
  SetViewAsControl(DEFAULT_VIEW_AUTO);

  SetSortOrder(SortOrderAscending);
  LoadViewState(items.GetPath(), WINDOW_ADDON_BROWSER);
}

void CGUIViewStateAddonBrowser::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_ADDON_BROWSER);
}

std::string CGUIViewStateAddonBrowser::GetExtensions()
{
  return "";
}

VECSOURCES& CGUIViewStateAddonBrowser::GetSources()
{
  m_sources.clear();
  {
    CMediaSource share;
    share.strPath = "addons://user/";
    share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    share.strName = g_localizeStrings.Get(24998);
    m_sources.push_back(share);
  }
  if (CAddonMgr::Get().HasOutdatedAddons())
  {
    CMediaSource share;
    share.strPath = "addons://outdated/";
    share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    share.strName = g_localizeStrings.Get(24043); // "Available updates"
    m_sources.push_back(share);
  }
  if (CAddonMgr::Get().HasAddons(ADDON_REPOSITORY, true))
  {
    CMediaSource share;
    share.strPath = "addons://repos/";
    share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    share.strName = g_localizeStrings.Get(24033);
    m_sources.push_back(share);
  }
  {
    CMediaSource share;
    share.strPath = "addons://install/";
    share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    share.strName = g_localizeStrings.Get(24041);
    m_sources.push_back(share);
  }
  {
    CMediaSource share;
    share.strPath = "addons://search/";
    share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    share.strName = g_localizeStrings.Get(137);
    m_sources.push_back(share);
  }
  {
    CMediaSource share;
    share.strPath = "addons://manage/";
    share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    share.strName = g_localizeStrings.Get(24992);
    m_sources.push_back(share);
  }

  return CGUIViewState::GetSources();
}

