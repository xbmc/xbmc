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

#ifndef ADDONS_GUIVIEWSTATEADDONBROWSER_H_INCLUDED
#define ADDONS_GUIVIEWSTATEADDONBROWSER_H_INCLUDED
#include "GUIViewStateAddonBrowser.h"
#endif

#ifndef ADDONS_FILEITEM_H_INCLUDED
#define ADDONS_FILEITEM_H_INCLUDED
#include "FileItem.h"
#endif

#ifndef ADDONS_GUILIB_GRAPHICCONTEXT_H_INCLUDED
#define ADDONS_GUILIB_GRAPHICCONTEXT_H_INCLUDED
#include "guilib/GraphicContext.h"
#endif

#ifndef ADDONS_GUILIB_WINDOWIDS_H_INCLUDED
#define ADDONS_GUILIB_WINDOWIDS_H_INCLUDED
#include "guilib/WindowIDs.h"
#endif

#ifndef ADDONS_VIEW_VIEWSTATE_H_INCLUDED
#define ADDONS_VIEW_VIEWSTATE_H_INCLUDED
#include "view/ViewState.h"
#endif

#ifndef ADDONS_ADDONS_ADDON_H_INCLUDED
#define ADDONS_ADDONS_ADDON_H_INCLUDED
#include "addons/Addon.h"
#endif

#ifndef ADDONS_ADDONS_ADDONMANAGER_H_INCLUDED
#define ADDONS_ADDONS_ADDONMANAGER_H_INCLUDED
#include "addons/AddonManager.h"
#endif

#ifndef ADDONS_ADDONDATABASE_H_INCLUDED
#define ADDONS_ADDONDATABASE_H_INCLUDED
#include "AddonDatabase.h"
#endif


using namespace XFILE;
using namespace ADDON;

CGUIViewStateAddonBrowser::CGUIViewStateAddonBrowser(const CFileItemList& items) : CGUIViewState(items)
{
  AddSortMethod(SortByLabel, SortAttributeIgnoreFolders, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | Foldername, empty
  AddSortMethod(SortByDate, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // Filename, Date | Foldername, Date
  SetSortMethod(SortByLabel, SortAttributeIgnoreFolders);

  SetViewAsControl(DEFAULT_VIEW_AUTO);

  SetSortOrder(SortOrderAscending);
  LoadViewState(items.GetPath(), WINDOW_ADDON_BROWSER);
}

void CGUIViewStateAddonBrowser::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_ADDON_BROWSER);
}

CStdString CGUIViewStateAddonBrowser::GetExtensions()
{
  return "";
}

VECSOURCES& CGUIViewStateAddonBrowser::GetSources()
{
  m_sources.clear();

  // we always have some enabled addons
  {
    CMediaSource share;
    share.strPath = "addons://enabled/";
    share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    share.strName = g_localizeStrings.Get(24062);
    m_sources.push_back(share);
  }
  CAddonDatabase db;
  if (db.Open() && db.HasDisabledAddons())
  {
    CMediaSource share;
    share.strPath = "addons://disabled/";
    share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    share.strName = g_localizeStrings.Get(24039);
    m_sources.push_back(share);
  }
  if (CAddonMgr::Get().HasOutdatedAddons())
  {
    CMediaSource share;
    share.strPath = "addons://outdated/";
    share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    share.strName = g_localizeStrings.Get(24043);
    m_sources.push_back(share);
  }
  if (CAddonMgr::Get().HasAddons(ADDON_REPOSITORY,true))
  {
    CMediaSource share;
    share.strPath = "addons://repos/";
    share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    share.strName = g_localizeStrings.Get(24033);
    m_sources.push_back(share);
  }
  // add "install from zip"
  {
    CMediaSource share;
    share.strPath = "addons://install/";
    share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    share.strName = g_localizeStrings.Get(24041);
    m_sources.push_back(share);
  }
  // add "search"
  {
    CMediaSource share;
    share.strPath = "addons://search/";
    share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    share.strName = g_localizeStrings.Get(137);
    m_sources.push_back(share);
  }

  return CGUIViewState::GetSources();
}

