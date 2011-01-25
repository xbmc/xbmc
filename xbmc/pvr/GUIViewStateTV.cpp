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

#include "GUIViewStateTV.h"
#include "PlayListPlayer.h"
#include "guilib/GUIBaseContainer.h" // for VIEW_TYPE_*
#include "video/VideoDatabase.h"
#include "settings/GUISettings.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "guilib/Key.h"
#include "Util.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"

CGUIViewStateWindowTV::CGUIViewStateWindowTV(const CFileItemList& items) : CGUIViewState(items)
{
  if (items.m_strPath.Left(17) == "pvr://recordings/")
  {
    if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
      AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // FileName, Size | Foldername, e
    else
      AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // FileName, Size | Foldername, empty
    AddSortMethod(SORT_METHOD_SIZE, 553, LABEL_MASKS("%L", "%I", "%L", "%I"));  // FileName, Size | Foldername, Size
    AddSortMethod(SORT_METHOD_DATE, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // FileName, Date | Foldername, Date
    AddSortMethod(SORT_METHOD_FILE, 561, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | FolderName, empty

    SetSortMethod(g_settings.m_viewStateVideoFiles.m_sortMethod);
    SetViewAsControl(g_settings.m_viewStateVideoFiles.m_viewMode);
    SetSortOrder(g_settings.m_viewStateVideoFiles.m_sortOrder);
  }
  LoadViewState(items.m_strPath, WINDOW_TV);
}

bool CGUIViewStateWindowTV::AutoPlayNextItem()
{
  return false;
}

//CStdString CGUIViewStateWindowTV::GetExtensions()
//{
//  return  ".pvr|.timer"";
//}

bool CGUIViewStateWindowTV::HideParentDirItems()
{
  return false;
}

void CGUIViewStateWindowTV::SaveViewState()
{

}
