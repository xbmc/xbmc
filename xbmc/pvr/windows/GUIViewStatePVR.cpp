/*
 *      Copyright (C) 2012 Team XBMC
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

#include "GUIViewStatePVR.h"
#include "GUIWindowPVR.h"
#include "GUIWindowPVRCommon.h"
#include "guilib/GUIWindowManager.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"

using namespace PVR;

CGUIViewStatePVR::CGUIViewStatePVR(const CFileItemList& items) :
  CGUIViewState(items)
{
  PVRWindow ActiveView = GetActiveView();
  if (ActiveView == PVR_WINDOW_RECORDINGS)
  {
    if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
      AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // FileName, Size | Foldername, e
    else
      AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // FileName, Size | Foldername, empty
    AddSortMethod(SORT_METHOD_SIZE, 553, LABEL_MASKS("%L", "%I", "%L", "%I"));  // FileName, Size | Foldername, Size
    AddSortMethod(SORT_METHOD_DATE, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // FileName, Date | Foldername, Date
    AddSortMethod(SORT_METHOD_FILE, 561, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | FolderName, empty
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
