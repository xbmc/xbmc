/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIViewStatePrograms.h"
#include "GUIBaseContainer.h"
#include "FileItem.h"
#include "ViewState.h"
#include "GUISettings.h"
#include "AdvancedSettings.h"
#include "Settings.h"
#include "FileSystem/Directory.h"
#include "FileSystem/PluginDirectory.h"
#include "Util.h"
#include "LocalizeStrings.h"

using namespace DIRECTORY;

CGUIViewStateWindowPrograms::CGUIViewStateWindowPrograms(const CFileItemList& items) : CGUIViewState(items)
{
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%K", "%I", "%L", ""));  // Titel, Size | Foldername, empty
  else
    AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%K", "%I", "%L", ""));  // Titel, Size | Foldername, empty
  AddSortMethod(SORT_METHOD_DATE, 552, LABEL_MASKS("%K", "%J", "%L", "%J"));  // Titel, Date | Foldername, Date
  AddSortMethod(SORT_METHOD_PROGRAM_COUNT, 565, LABEL_MASKS("%K", "%C", "%L", ""));  // Titel, Count | Foldername, empty
  AddSortMethod(SORT_METHOD_SIZE, 553, LABEL_MASKS("%K", "%I", "%K", "%I"));  // Filename, Size | Foldername, Size
  AddSortMethod(SORT_METHOD_FILE, 561, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | FolderName, empty

  SetSortMethod(g_stSettings.m_viewStatePrograms.m_sortMethod);
  SetViewAsControl(g_stSettings.m_viewStatePrograms.m_viewMode);
  SetSortOrder(g_stSettings.m_viewStatePrograms.m_sortOrder);

  LoadViewState(items.m_strPath, WINDOW_PROGRAMS);
}

void CGUIViewStateWindowPrograms::SaveViewState()
{
  if (g_guiSettings.GetBool("programfiles.savefolderviews"))
    SaveViewToDb(m_items.m_strPath, WINDOW_PROGRAMS, &g_stSettings.m_viewStatePrograms);
  else
  {
    g_stSettings.m_viewStatePrograms = CViewState(GetViewAsControl(), GetSortMethod(), GetSortOrder());
    g_settings.Save();
  }
}

CStdString CGUIViewStateWindowPrograms::GetLockType()
{
  return "programs";
}

CStdString CGUIViewStateWindowPrograms::GetExtensions()
{
#ifdef _LINUX
  return "";
#endif
#ifdef _WIN32
    return ".exe|.lnk|.cmd|.bat";
#else
    return ".xbe|.cut";
#endif 
}

VECSOURCES& CGUIViewStateWindowPrograms::GetSources()
{
  bool bIsSourceName = true;
  // plugins share
  if (CPluginDirectory::HasPlugins("programs"))
  {
    CMediaSource share;
    share.strName = g_localizeStrings.Get(1043); // Program Plugins
    share.strPath = "plugin://programs/";
    share.m_strThumbnailImage = CUtil::GetDefaultFolderThumb("DefaultProgramPlugins.png");
    share.m_ignore= true;
    if (CUtil::GetMatchingSource(share.strName, g_settings.m_programSources, bIsSourceName) < 0)
      g_settings.m_programSources.push_back(share);
  }
  return g_settings.m_programSources;
}

CGUIViewStateWindowProgramNav::CGUIViewStateWindowProgramNav(const CFileItemList& items) : CGUIViewStateWindowPrograms(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_NONE, 551, LABEL_MASKS("%F", "%I", "%L", ""));  // Filename, Size | Foldername, empty
    SetSortMethod(SORT_METHOD_NONE);

    SetViewAsControl(DEFAULT_VIEW_LIST);

    SetSortOrder(SORT_ORDER_NONE);
  }
  LoadViewState(items.m_strPath, WINDOW_PROGRAM_NAV);
}

void CGUIViewStateWindowProgramNav::SaveViewState()
{
  SaveViewToDb(m_items.m_strPath, WINDOW_PROGRAM_NAV);
}

VECSOURCES& CGUIViewStateWindowProgramNav::GetSources()
{
  //  Setup shares we want to have
  m_sources.clear();

  // plugins share
  if (CPluginDirectory::HasPlugins("programs") && g_advancedSettings.m_bVirtualShares)
  {
    CMediaSource share;
    share.strName = g_localizeStrings.Get(1043);
    share.strPath = "plugin://programs/";
    share.m_strThumbnailImage = CUtil::GetDefaultFolderThumb("DefaultProgramPlugins.png");
    m_sources.push_back(share);
  }
 
  return m_sources;
}

