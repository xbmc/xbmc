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

#include "FileItem.h"
#include "settings/Settings.h"
#include "view/ViewStateSettings.h"

using namespace PVR;

CGUIViewStateWindowPVRChannels::CGUIViewStateWindowPVRChannels(const int windowId, const CFileItemList& items) : CGUIViewStatePVR(windowId, items)
{
  AddSortMethod(SortByChannelNumber, 549, LABEL_MASKS("%L", "%I", "%L", ""));
  AddSortMethod(SortByLabel, 551, LABEL_MASKS("%L", "%I", "%L", ""));

  // Default sorting
  SetSortMethod(SortByChannelNumber);

  LoadViewState("pvr://channels/", m_windowId);
}

void CGUIViewStateWindowPVRChannels::SaveViewState(void)
{
  SaveViewToDb("pvr://channels/", m_windowId, CViewStateSettings::Get().Get("pvrchannels"));
}

CGUIViewStateWindowPVRRecordings::CGUIViewStateWindowPVRRecordings(const int windowId, const CFileItemList& items) : CGUIViewStatePVR(windowId, items)
{
  AddSortMethod(SortByLabel, 551, LABEL_MASKS("%L", "%I", "%L", ""), CSettings::Get().GetBool("filelists.ignorethewhensorting") ? SortAttributeIgnoreArticle : SortAttributeNone);  // FileName, Size | Foldername, empty
  AddSortMethod(SortByDate, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // FileName, Date | Foldername, Date
  AddSortMethod(SortByTime, 180, LABEL_MASKS("%T", "%D"));
  AddSortMethod(SortByFile, 561, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | FolderName, empty

  // Default sorting
  SetSortMethod(SortByDate);

  LoadViewState(items.GetPath(), m_windowId);
}

void CGUIViewStateWindowPVRRecordings::SaveViewState(void)
{
  SaveViewToDb(m_items.GetPath(), m_windowId, CViewStateSettings::Get().Get("pvrrecordings"));
}

bool CGUIViewStateWindowPVRRecordings::HideParentDirItems(void)
{
  return (CGUIViewState::HideParentDirItems() || m_items.GetPath() == "pvr://recordings/active/" || m_items.GetPath() == "pvr://recordings/deleted/");
}

CGUIViewStateWindowPVRGuide::CGUIViewStateWindowPVRGuide(const int windowId, const CFileItemList& items) : CGUIViewStatePVR(windowId, items)
{
  LoadViewState("pvr://guide/", m_windowId);
}

void CGUIViewStateWindowPVRGuide::SaveViewState(void)
{
  SaveViewToDb("pvr://guide/", m_windowId, CViewStateSettings::Get().Get("pvrguide"));
}

CGUIViewStateWindowPVRTimers::CGUIViewStateWindowPVRTimers(const int windowId, const CFileItemList& items) : CGUIViewStatePVR(windowId, items)
{
  AddSortMethod(SortByLabel, 551, LABEL_MASKS("%L", "%I", "%L", ""));   // FileName, Size | Foldername, empty
  AddSortMethod(SortByDate, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // FileName, Date | Foldername, Date

  // Default sorting
  SetSortMethod(SortByDate);

  LoadViewState("pvr://timers/", m_windowId);
}

void CGUIViewStateWindowPVRTimers::SaveViewState(void)
{
  SaveViewToDb("pvr://timers/", m_windowId, CViewStateSettings::Get().Get("pvrtimers"));
}

CGUIViewStateWindowPVRSearch::CGUIViewStateWindowPVRSearch(const int windowId, const CFileItemList& items) : CGUIViewStatePVR(windowId, items)
{
  AddSortMethod(SortByLabel, 551, LABEL_MASKS("%L", "%I", "%L", ""));   // FileName, Size | Foldername, empty
  AddSortMethod(SortByDate, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // FileName, Date | Foldername, Date

  // Default sorting
  SetSortMethod(SortByDate);

  LoadViewState("pvr://search/", m_windowId);
}

void CGUIViewStateWindowPVRSearch::SaveViewState(void)
{
  SaveViewToDb("pvr://search/", m_windowId, CViewStateSettings::Get().Get("pvrsearch"));
}
