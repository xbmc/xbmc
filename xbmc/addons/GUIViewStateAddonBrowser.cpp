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
#include "addons/Addon.h"
#include "addons/AddonManager.h"
#include "FileItem.h"
#include "filesystem/File.h"
#include "guilib/GraphicContext.h"
#include "guilib/WindowIDs.h"
#include "view/ViewState.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"

using namespace XFILE;
using namespace ADDON;

CGUIViewStateAddonBrowser::CGUIViewStateAddonBrowser(const CFileItemList& items) : CGUIViewState(items)
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
    AddSortMethod(SortByLabel, SortAttributeIgnoreFolders, 551, LABEL_MASKS("%L", "%s", "%L", "%s"));

    if (StringUtils::StartsWith(items.GetPath(), "addons://sources/"))
      AddSortMethod(SortByLastUsed, 12012, LABEL_MASKS("%L", "%u", "%L", "%u"),
          SortAttributeIgnoreFolders, SortOrderDescending); //Label, Last used

    if (StringUtils::StartsWith(items.GetPath(), "addons://user/") && items.GetContent() == "addons")
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
