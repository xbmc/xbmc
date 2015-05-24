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

#include "GUIViewStatePictures.h"
#include "FileItem.h"
#include "view/ViewState.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "filesystem/Directory.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "view/ViewStateSettings.h"

using namespace XFILE;
using namespace ADDON;

CGUIViewStateWindowPictures::CGUIViewStateWindowPictures(const CFileItemList& items) : CGUIViewState(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SortByLabel, 551, LABEL_MASKS());
    AddSortMethod(SortByDriveType, 564, LABEL_MASKS());
    SetSortMethod(SortByLabel);

    SetViewAsControl(DEFAULT_VIEW_LIST);

    SetSortOrder(SortOrderAscending);
  }
  else
  {
    AddSortMethod(SortByLabel, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | Foldername, empty
    AddSortMethod(SortBySize, 553, LABEL_MASKS("%L", "%I", "%L", "%I"));  // Filename, Size | Foldername, Size
    AddSortMethod(SortByDate, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // Filename, Date | Foldername, Date
    AddSortMethod(SortByDateTaken, 577, LABEL_MASKS("%L", "%t", "%L", "%J"));  // Filename, DateTaken | Foldername, Date
    AddSortMethod(SortByFile, 561, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | FolderName, empty

    const CViewState *viewState = CViewStateSettings::Get().Get("pictures");
    SetSortMethod(viewState->m_sortDescription);
    SetViewAsControl(viewState->m_viewMode);
    SetSortOrder(viewState->m_sortDescription.sortOrder);
  }
  LoadViewState(items.GetPath(), WINDOW_PICTURES);
}

void CGUIViewStateWindowPictures::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_PICTURES, CViewStateSettings::Get().Get("pictures"));
}

std::string CGUIViewStateWindowPictures::GetLockType()
{
  return "pictures";
}

std::string CGUIViewStateWindowPictures::GetExtensions()
{
  std::string extensions = g_advancedSettings.m_pictureExtensions;
  if (CSettings::Get().GetBool("pictures.showvideos"))
    extensions += "|" + g_advancedSettings.m_videoExtensions;

  return extensions;
}

VECSOURCES& CGUIViewStateWindowPictures::GetSources()
{
  VECSOURCES *pictureSources = CMediaSourceSettings::Get().GetSources("pictures");
  AddAddonsSource("image", g_localizeStrings.Get(1039), "DefaultAddonPicture.png");
  AddOrReplace(*pictureSources, CGUIViewState::GetSources());
  return *pictureSources;
}

