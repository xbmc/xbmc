/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "GUIViewStatePVR.h"
#include "GUIWindowPVR.h"
#include "GUIWindowPVRCommon.h"
#include "guilib/GUIWindowManager.h"
#include "settings/Settings.h"

using namespace PVR;

CGUIViewStatePVR::CGUIViewStatePVR(const CFileItemList& items) :
  CGUIViewState(items)
{
  PVRWindow ActiveView = GetActiveView();
  if (ActiveView == PVR_WINDOW_RECORDINGS)
  {
    AddSortMethod(SortByLabel, 551, LABEL_MASKS("%L", "%I", "%L", ""), CSettings::Get().GetBool("filelists.ignorethewhensorting") ? SortAttributeIgnoreArticle : SortAttributeNone);  // FileName, Size | Foldername, empty
    AddSortMethod(SortBySize, 553, LABEL_MASKS("%L", "%I", "%L", "%I"));  // FileName, Size | Foldername, Size
    AddSortMethod(SortByDate, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // FileName, Date | Foldername, Date
    AddSortMethod(SortByFile, 561, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | FolderName, empty

    // Sort recordings view by date as default
    SetSortMethod(SortByDate);
  }

  LoadViewState(items.GetPath(), ActiveView == PVR_WINDOW_UNKNOWN ? WINDOW_PVR : WINDOW_PVR + 100 - ActiveView );
}

PVRWindow CGUIViewStatePVR::GetActiveView()
{
  PVRWindow returnWindow = PVR_WINDOW_UNKNOWN;

  int iActiveWindow = g_windowManager.GetActiveWindow();
  if (iActiveWindow == WINDOW_PVR)
  {
    CGUIWindowPVR *pWindow = (CGUIWindowPVR *) g_windowManager.GetWindow(WINDOW_PVR);
    CGUIWindowPVRCommon *pActiveView = NULL;
    if (pWindow && (pActiveView = pWindow->GetActiveView()) != NULL)
      returnWindow = pActiveView->GetWindowId();
  }

  return returnWindow;
}

void CGUIViewStatePVR::SaveViewState(void) 
{
  PVRWindow ActiveView = GetActiveView();
  SaveViewToDb(m_items.GetPath(), ActiveView == PVR_WINDOW_UNKNOWN ? WINDOW_PVR : WINDOW_PVR + 100 - ActiveView, NULL);
}

bool CGUIViewStatePVR::HideParentDirItems(void)
{
  return (CGUIViewState::HideParentDirItems() || PVR_WINDOW_RECORDINGS != GetActiveView() || m_items.GetPath() == "pvr://recordings/");
}
