/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIViewStateAddonBrowser.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "filesystem/File.h"
#include "guilib/WindowIDs.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "view/ViewState.h"
#include "windowing/GraphicContext.h"

using namespace XFILE;
using namespace ADDON;

CGUIViewStateAddonBrowser::CGUIViewStateAddonBrowser(const CFileItemList& items)
  : CGUIViewState(items)
{
  if (URIUtils::PathEquals(items.GetPath(), "addons://"))
  {
    AddSortMethod(SortByNone, 551, LABEL_MASKS("%F", "", "%L", ""));
    SetSortMethod(SortByNone);
  }
  else if (URIUtils::PathEquals(items.GetPath(), "addons://recently_updated/", true))
  {
    AddSortMethod(SortByLastUpdated, 12014, LABEL_MASKS("%L", "%v", "%L", "%v"),
                  SortAttributeIgnoreFolders, SortOrderDescending);
  }
  else
  {
    AddSortMethod(SortByLabel, SortAttributeIgnoreFolders, 551,
                  LABEL_MASKS("%L", "%s", "%L", "%s"));

    if (StringUtils::StartsWith(items.GetPath(), "addons://sources/"))
      AddSortMethod(SortByLastUsed, 12012, LABEL_MASKS("%L", "%u", "%L", "%u"),
                    SortAttributeIgnoreFolders, SortOrderDescending); //Label, Last used

    if (StringUtils::StartsWith(items.GetPath(), "addons://user/") &&
        items.GetContent() == "addons")
      AddSortMethod(SortByInstallDate, 12013, LABEL_MASKS("%L", "%i", "%L", "%i"),
                    SortAttributeIgnoreFolders, SortOrderDescending);

    SetSortMethod(SortByLabel);
  }
  SetViewAsControl(DEFAULT_VIEW_AUTO);

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
