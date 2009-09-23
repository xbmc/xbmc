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

#include "GUIViewStateScripts.h"
#include "GUIBaseContainer.h"
#include "FileItem.h"
#include "Key.h"
#include "ViewState.h"
#include "Settings.h"
#include "FileSystem/Directory.h"

using namespace DIRECTORY;

CGUIViewStateWindowScripts::CGUIViewStateWindowScripts(const CFileItemList& items) : CGUIViewState(items)
{
  AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | Foldername, empty
  AddSortMethod(SORT_METHOD_DATE, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // Filename, Date | Foldername, Date
  AddSortMethod(SORT_METHOD_SIZE, 553, LABEL_MASKS("%L", "%I", "%L", "%I"));  // Filename, Size | Foldername, Size
  AddSortMethod(SORT_METHOD_FILE, 561, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | FolderName, empty
  SetSortMethod(SORT_METHOD_LABEL);

  SetViewAsControl(DEFAULT_VIEW_LIST);

  SetSortOrder(SORT_ORDER_ASC);
  LoadViewState(items.m_strPath, WINDOW_SCRIPTS);
}

void CGUIViewStateWindowScripts::SaveViewState()
{
  SaveViewToDb(m_items.m_strPath, WINDOW_SCRIPTS);
}

CStdString CGUIViewStateWindowScripts::GetExtensions()
{
  return ".py";
}

VECSOURCES& CGUIViewStateWindowScripts::GetSources()
{
  m_sources.clear();

  CMediaSource share;
  if (g_settings.m_vecProfiles.size() > 1)
  {
    if (CDirectory::Exists("special://profile/scripts"))
    {
      CMediaSource share2;
      share2.strName = "Profile Scripts";
      share2.strPath = "special://profile/scripts";
      share2.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
      m_sources.push_back(share2);
    }
    share.strName = "Shared Scripts";
  }
  else
    share.strName = "Scripts";

  share.strPath = "special://home/scripts";
  if (!CDirectory::Exists(share.strPath))
    share.strPath = "special://xbmc/scripts";
  share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
  m_sources.push_back(share);

  return CGUIViewState::GetSources();
}

