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

#include "FileItem.h"
#include "settings/Settings.h"
#include "view/ViewStateSettings.h"

#include "pvr/recordings/PVRRecordingsPath.h"
#include "pvr/timers/PVRTimers.h"

#include "GUIViewStatePVR.h"

using namespace PVR;

CGUIViewStateWindowPVRChannels::CGUIViewStateWindowPVRChannels(const int windowId, const CFileItemList& items) : CGUIViewStatePVR(windowId, items)
{
  AddSortMethod(SortByChannelNumber, 549, LABEL_MASKS("%L", "", "%L", ""));      // "Number"      : Filename, empty | Foldername, empty
  AddSortMethod(SortByLabel,         551, LABEL_MASKS("%L", "", "%L", ""));      // "Name"        : Filename, empty | Foldername, empty
  AddSortMethod(SortByLastPlayed,    568, LABEL_MASKS( "%L", "%p", "%L", "%p")); // "Last played" : Filename, LastPlayed | Foldername, LastPlayed

  // Default sorting
  SetSortMethod(SortByChannelNumber);

  LoadViewState("pvr://channels/", m_windowId);
}

void CGUIViewStateWindowPVRChannels::SaveViewState(void)
{
  SaveViewToDb("pvr://channels/", m_windowId, CViewStateSettings::GetInstance().Get("pvrchannels"));
}

CGUIViewStateWindowPVRRecordings::CGUIViewStateWindowPVRRecordings(const int windowId, const CFileItemList& items) : CGUIViewStatePVR(windowId, items)
{
  AddSortMethod(SortByLabel, 551, LABEL_MASKS("%L", "%d", "%L", ""),    // "Name"     : Filename, DateTime | Foldername, empty
                CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING)
                  ? SortAttributeIgnoreArticle
                  : SortAttributeNone);
  AddSortMethod(SortByDate,  552, LABEL_MASKS("%L", "%d", "%L", "%d")); // "Date"     : Filename, DateTime | Foldername, DateTime
  AddSortMethod(SortByTime,  180, LABEL_MASKS("%L", "%D", "%L", ""));   // "Duration" : Filename, Duration | Foldername, empty
  AddSortMethod(SortByFile,  561, LABEL_MASKS("%L", "%d", "%L", ""));   // "File"     : Filename, DateTime | Foldername, empty

  // Default sorting
  SetSortMethod(SortByDate);

  LoadViewState(items.GetPath(), m_windowId);
}

void CGUIViewStateWindowPVRRecordings::SaveViewState(void)
{
  SaveViewToDb(m_items.GetPath(), m_windowId, CViewStateSettings::GetInstance().Get("pvrrecordings"));
}

bool CGUIViewStateWindowPVRRecordings::HideParentDirItems(void)
{
  return (CGUIViewState::HideParentDirItems() || CPVRRecordingsPath(m_items.GetPath()).IsRecordingsRoot());
}

CGUIViewStateWindowPVRGuide::CGUIViewStateWindowPVRGuide(const int windowId, const CFileItemList& items) : CGUIViewStatePVR(windowId, items)
{
  LoadViewState("pvr://guide/", m_windowId);
}

void CGUIViewStateWindowPVRGuide::SaveViewState(void)
{
  SaveViewToDb("pvr://guide/", m_windowId, CViewStateSettings::GetInstance().Get("pvrguide"));
}

CGUIViewStateWindowPVRTimers::CGUIViewStateWindowPVRTimers(const int windowId, const CFileItemList& items) : CGUIViewStatePVR(windowId, items)
{
  int sortAttributes(CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
  sortAttributes |= SortAttributeIgnoreFolders;
  AddSortMethod(SortByLabel, static_cast<SortAttribute>(sortAttributes), 551, LABEL_MASKS("%L", "", "%L", ""));     // "Name" : Filename, empty | Foldername, empty
  AddSortMethod(SortByDate,  static_cast<SortAttribute>(sortAttributes), 552, LABEL_MASKS("%L", "%d", "%L", "%d")); // "Date" : Filename, DateTime | Foldername, DateTime

  // Default sorting
  SetSortMethod(SortByDate);

  LoadViewState("pvr://timers/", m_windowId);
}

void CGUIViewStateWindowPVRTimers::SaveViewState(void)
{
  SaveViewToDb("pvr://timers/", m_windowId, CViewStateSettings::GetInstance().Get("pvrtimers"));
}

bool CGUIViewStateWindowPVRTimers::HideParentDirItems(void)
{
  return (CGUIViewState::HideParentDirItems() || CPVRTimersPath(m_items.GetPath()).IsTimersRoot());
}

CGUIViewStateWindowPVRSearch::CGUIViewStateWindowPVRSearch(const int windowId, const CFileItemList& items) : CGUIViewStatePVR(windowId, items)
{
  AddSortMethod(SortByLabel, 551, LABEL_MASKS("%L", "", "%L", ""));     // "Name" : Filename, empty | Foldername, empty
  AddSortMethod(SortByDate,  552, LABEL_MASKS("%L", "%d", "%L", "%d")); // "Date" : Filename, DateTime | Foldername, DateTime

  // Default sorting
  SetSortMethod(SortByDate);

  LoadViewState("pvr://search/", m_windowId);
}

void CGUIViewStateWindowPVRSearch::SaveViewState(void)
{
  SaveViewToDb("pvr://search/", m_windowId, CViewStateSettings::GetInstance().Get("pvrsearch"));
}
