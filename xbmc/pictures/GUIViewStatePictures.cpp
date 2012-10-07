/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIViewStatePictures.h"
#include "FileItem.h"
#include "ViewState.h"
#include "settings/GUISettings.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "filesystem/Directory.h"
#include "filesystem/PluginDirectory.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/Key.h"
#include "pictures/PictureDatabase.h"
#include "filesystem/PictureDatabaseDirectory.h"

using namespace XFILE;
using namespace PICTUREDATABASEDIRECTORY;

CGUIViewStateWindowPictures::CGUIViewStateWindowPictures(const CFileItemList& items) : CGUIViewState(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_NONE, 551, LABEL_MASKS("%F", "%I", "%L", ""));  // Filename, Size | Label, empty
    SetSortMethod(SORT_METHOD_NONE);

    SetViewAsControl(DEFAULT_VIEW_LIST);

    SetSortOrder(SortOrderNone);
  }
  else if (items.IsPictureDb())
  {
    NODE_TYPE NodeType = CPictureDatabaseDirectory::GetDirectoryChildType(items.GetPath());
    switch (NodeType)
    {
    case NODE_TYPE_FOLDER:
    case NODE_TYPE_YEAR:
    case NODE_TYPE_CAMERA:
    case NODE_TYPE_TAGS:
      AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%L", "", "%L", ""));  // Label, empty | Label, empty
      SetSortMethod(SORT_METHOD_LABEL);
      SetViewAsControl(DEFAULT_VIEW_LIST);
      SetSortOrder(SortOrderAscending);
      break;

    case NODE_TYPE_PICTURES:
      AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%L", "%J", "%L", ""));  // Filename, Date | Foldername, empty
      AddSortMethod(SORT_METHOD_DATE, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // Filename, Date | Foldername, Date
      AddSortMethod(SORT_METHOD_SIZE, 553, LABEL_MASKS("%L", "%I", "%L", "%I"));  // Filename, Size | Foldername, Size
      AddSortMethod(SORT_METHOD_FILE, 561, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | FolderName, empty

      SetSortMethod(g_settings.m_viewStatePictures.m_sortMethod);
      SetViewAsControl(g_settings.m_viewStatePictures.m_viewMode);
      SetSortOrder(g_settings.m_viewStatePictures.m_sortOrder);
      break;

    case NODE_TYPE_OVERVIEW:
    default:
      AddSortMethod(SORT_METHOD_NONE, 551, LABEL_MASKS("%F", "%I", "%L", ""));  // Filename, Size | Label, empty
      SetSortMethod(SORT_METHOD_NONE);
      SetViewAsControl(DEFAULT_VIEW_LIST);
      SetSortOrder(SortOrderNone);
      break;
    }
  }
  else
  {
    AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | Foldername, empty
    AddSortMethod(SORT_METHOD_SIZE, 553, LABEL_MASKS("%L", "%I", "%L", "%I"));  // Filename, Size | Foldername, Size
    AddSortMethod(SORT_METHOD_DATE, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // Filename, Date | Foldername, Date
    AddSortMethod(SORT_METHOD_FILE, 561, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | FolderName, empty

    SetSortMethod(g_settings.m_viewStatePictures.m_sortMethod);
    SetViewAsControl(g_settings.m_viewStatePictures.m_viewMode);
    SetSortOrder(g_settings.m_viewStatePictures.m_sortOrder);
  }
  LoadViewState(items.GetPath(), WINDOW_PICTURES);
}

void CGUIViewStateWindowPictures::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_PICTURES, &g_settings.m_viewStatePictures);
}

CStdString CGUIViewStateWindowPictures::GetLockType()
{
  return "pictures";
}

CStdString CGUIViewStateWindowPictures::GetExtensions()
{
  if (g_guiSettings.GetBool("pictures.showvideos"))
    return g_settings.m_pictureExtensions+"|"+g_settings.m_videoExtensions;

  return g_settings.m_pictureExtensions;
}

VECSOURCES& CGUIViewStateWindowPictures::GetSources()
{
  m_sources.clear();

  // Library items
  CPictureDatabase db;
  if (db.Open() && db.Count())
  {
    CFileItemList items;
    CDirectory::GetDirectory("picturedb://", items);
    for (int i = 0; i < items.Size(); i++)
    {
      CMediaSource share;
      share.strName = items[i]->GetLabel();
      share.strPath = items[i]->GetPath();
      share.m_strThumbnailImage = items[i]->GetIconImage();
      share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
      m_sources.push_back(share);
    }
  }

  // Files
  {
    CMediaSource share;
    share.strPath = "sources://pictures/";
    share.strName = g_localizeStrings.Get(744);
    share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    share.m_strThumbnailImage = "DefaultFolder.png";
    m_sources.push_back(share);
  }

  // Add-ons
  AddAddonsSource("image", g_localizeStrings.Get(1039), "DefaultAddonPicture.png");
  return CGUIViewState::GetSources();
}

