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
#include "GUIViewStatePictures.h"
#include "GUIBaseContainer.h"
#include "FileItem.h"
#include "ViewState.h"
#include "Settings.h"
#include "FileSystem/Directory.h"
#include "FileSystem/PluginDirectory.h"
#include "Util.h"

using namespace DIRECTORY;

CGUIViewStateWindowPictures::CGUIViewStateWindowPictures(const CFileItemList& items) : CGUIViewState(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS());
    AddSortMethod(SORT_METHOD_DRIVE_TYPE, 564, LABEL_MASKS());
    SetSortMethod(SORT_METHOD_LABEL);

    SetViewAsControl(DEFAULT_VIEW_LIST);

    SetSortOrder(SORT_ORDER_ASC);
  }
  else
  {
    AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | Foldername, empty
    AddSortMethod(SORT_METHOD_SIZE, 553, LABEL_MASKS("%L", "%I", "%L", "%I"));  // Filename, Size | Foldername, Size
    AddSortMethod(SORT_METHOD_DATE, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // Filename, Date | Foldername, Date
    AddSortMethod(SORT_METHOD_FILE, 561, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | FolderName, empty

    SetSortMethod(g_stSettings.m_viewStatePictures.m_sortMethod);
    SetViewAsControl(g_stSettings.m_viewStatePictures.m_viewMode);
    SetSortOrder(g_stSettings.m_viewStatePictures.m_sortOrder);
  }
  LoadViewState(items.m_strPath, WINDOW_PICTURES);
}

void CGUIViewStateWindowPictures::SaveViewState()
{
  if (g_guiSettings.GetBool("pictures.savefolderviews"))
    SaveViewToDb(m_items.m_strPath, WINDOW_PICTURES, &g_stSettings.m_viewStatePictures);
  else
  {
    g_stSettings.m_viewStatePictures = CViewState(GetViewAsControl(), GetSortMethod(), GetSortOrder());
    g_settings.Save();
  }
}

CStdString CGUIViewStateWindowPictures::GetLockType()
{
  return "pictures";
}

bool CGUIViewStateWindowPictures::UnrollArchives()
{
  return g_guiSettings.GetBool("filelists.unrollarchives");
}

CStdString CGUIViewStateWindowPictures::GetExtensions()
{
  return g_stSettings.m_pictureExtensions;
}

VECSOURCES& CGUIViewStateWindowPictures::GetSources()
{
  bool bIsSourceName = true;
  // plugins share
  if (CPluginDirectory::HasPlugins("pictures") && g_advancedSettings.m_bVirtualShares)
  {
    CMediaSource share;
    share.strName = g_localizeStrings.Get(1039); // Picture Plugins
    share.strPath = "plugin://pictures/";
    share.m_ignore = true;
    if (CUtil::GetMatchingSource(share.strName, g_settings.m_pictureSources, bIsSourceName) < 0)
      g_settings.m_pictureSources.push_back(share);
  }
  return g_settings.m_pictureSources;
}

