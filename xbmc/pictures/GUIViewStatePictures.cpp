/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIViewStatePictures.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/FileExtensionProvider.h"
#include "view/ViewState.h"
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

    const CViewState *viewState = CViewStateSettings::GetInstance().Get("pictures");
    SetSortMethod(viewState->m_sortDescription);
    SetViewAsControl(viewState->m_viewMode);
    SetSortOrder(viewState->m_sortDescription.sortOrder);
  }
  LoadViewState(items.GetPath(), WINDOW_PICTURES);
}

void CGUIViewStateWindowPictures::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_PICTURES, CViewStateSettings::GetInstance().Get("pictures"));
}

std::string CGUIViewStateWindowPictures::GetLockType()
{
  return "pictures";
}

std::string CGUIViewStateWindowPictures::GetExtensions()
{
  std::string extensions = CServiceBroker::GetFileExtensionProvider().GetPictureExtensions();
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_PICTURES_SHOWVIDEOS))
    extensions += "|" + CServiceBroker::GetFileExtensionProvider().GetVideoExtensions();

  return extensions;
}

VECSOURCES& CGUIViewStateWindowPictures::GetSources()
{
  VECSOURCES *pictureSources = CMediaSourceSettings::GetInstance().GetSources("pictures");

  // Guard against source type not existing
  if (pictureSources == nullptr)
  {
    static VECSOURCES empty;
    return empty;
  }

  return *pictureSources;
}

