/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "stdafx.h"
#include "GUIWindowProgramNav.h"
#include "Util.h"
#include "utils/GUIInfoManager.h"
#include "GUIPassword.h"
#include "GUIDialogFileBrowser.h"
#include "GUIDialogContentSettings.h"
#include "Picture.h"
#include "PlayListFactory.h"
#include "GUIWindowManager.h"
#include "GUIDialogOK.h"
#include "GUIDialogKeyboard.h"
#include "GUIEditControl.h"
#include "FileSystem/File.h"
#include "FileItem.h"
#include "Application.h"
#include "Application.h"
#include "Settings.h"
#include "AdvancedSettings.h"
#include "GUISettings.h"
#include "LocalizeStrings.h"

using namespace std;
using namespace DIRECTORY;
using namespace PLAYLIST;

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_BTNTYPE            5
#define CONTROL_LABELFILES        12

#define CONTROL_SEARCH             8
#define CONTROL_FILTER            15
#define CONTROL_BTN_FILTER        19
#define CONTROL_LABELEMPTY        18

CGUIWindowProgramNav::CGUIWindowProgramNav(void)
    : CGUIMediaWindow(WINDOW_PROGRAM_NAV, "MyProgramsNav.xml")
{
  m_vecItems->m_strPath = "?";
  m_bDisplayEmptyDatabaseMessage = false;
  m_unfilteredItems = new CFileItemList;
  m_searchWithEdit = false;
}

CGUIWindowProgramNav::~CGUIWindowProgramNav(void)
{
  delete m_unfilteredItems;
}

bool CGUIWindowProgramNav::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_RESET:
    m_vecItems->m_strPath = "?";
    break;
  case GUI_MSG_WINDOW_DEINIT:
      m_programdatabase.Close();
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      m_programdatabase.Open();
/* We don't want to show Autosourced items (ie removable pendrives, memorycards) in Library mode */
      m_rootDir.AllowNonLocalSources(false);
      // check for valid quickpath parameter
      CStdStringArray params;
      StringUtils::SplitString(message.GetStringParam(), ",", params);
      bool returning = params.size() > 1 && params[1].Equals("return");

      CStdString strDestination = params.size() ? params[0] : "";
      if (!strDestination.IsEmpty())
      {
        message.SetStringParam("");
        CLog::Log(LOGINFO, "Attempting to quickpath to: %s", strDestination.c_str());
      }

      // is this the first time the window is opened?
      if (m_vecItems->m_strPath == "?" && strDestination.IsEmpty())
      {
        strDestination = g_settings.m_defaultProgramLibSource;
        m_vecItems->m_strPath = strDestination;
        CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
      }

      CStdString destPath;
      if (!strDestination.IsEmpty())
      {
        if (strDestination.Equals("$ROOT") || strDestination.Equals("Root"))
          destPath = "";
/*        else if (strDestination.Equals("Genres"))
          destPath = "musicdb://1/";
        else if (strDestination.Equals("Artists"))
          destPath = "musicdb://2/";
        else if (strDestination.Equals("Albums"))
          destPath = "musicdb://3/";
        else if (strDestination.Equals("Songs"))
          destPath = "musicdb://4/";
        else if (strDestination.Equals("Top100"))
          destPath = "musicdb://5/";
        else if (strDestination.Equals("Top100Songs"))
          destPath = "musicdb://5/2/";
        else if (strDestination.Equals("Top100Albums"))
          destPath = "musicdb://5/1/";
        else if (strDestination.Equals("RecentlyAddedAlbums"))
          destPath = "musicdb://6/";
        else if (strDestination.Equals("RecentlyPlayedAlbums"))
          destPath = "musicdb://7/";
        else if (strDestination.Equals("Compilations"))
          destPath = "musicdb://8/";
        else if (strDestination.Equals("Playlists"))
          destPath = "special://musicplaylists/";
        else if (strDestination.Equals("Years"))
          destPath = "musicdb://9/";*/
        else if (strDestination.Equals("Plugins"))
          destPath = "plugin://programs/";
        else
        {
          CLog::Log(LOGWARNING, "Warning, destination parameter (%s) may not be valid", strDestination.c_str());
          destPath = strDestination;
        }
        if (!returning || m_vecItems->m_strPath.Left(destPath.GetLength()) != destPath)
        { // we're not returning to the same path, so set our directory to the requested path
          m_vecItems->m_strPath = destPath;
        }
        SetHistoryForPath(m_vecItems->m_strPath);
      }

      DisplayEmptyDatabaseMessage(false); // reset message state

      if (message.GetParam1() != WINDOW_INVALID)
      { // first time to this window - make sure we set the root path
        m_startDirectory = returning ? destPath : "";
      }

      //  base class has opened the database, do our check
      DisplayEmptyDatabaseMessage(m_programdatabase.GetProgramsCount() <= 0);

      if (m_bDisplayEmptyDatabaseMessage)
      {
        // no library - make sure we focus on a known control, and default to the root.
        SET_CONTROL_FOCUS(CONTROL_BTNTYPE, 0);
        m_vecItems->m_strPath = "";
        SetHistoryForPath("");
        Update("");
      }

      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTN_FILTER)
      {
        if (GetControl(iControl)->GetControlType() == CGUIControl::GUICONTROL_EDIT)
        { // filter updated
          CGUIMessage selected(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_BTN_FILTER);
          OnMessage(selected);
          m_filter = selected.GetLabel();
          OnFilterItems();
          return true;
        }
        if (m_filter.IsEmpty())
          CGUIDialogKeyboard::ShowAndGetFilter(m_filter, false);
        else
        {
          m_filter.Empty();
          OnFilterItems();
        }
        return true;
      }
      else if (iControl == CONTROL_SEARCH)
      {
        if (m_searchWithEdit)
        {
          // search updated - reset timer
          m_searchTimer.StartZero();
          // grab our search string
          CGUIMessage selected(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_SEARCH);
          OnMessage(selected);
          m_search = selected.GetLabel();
          return true;
        }
        CGUIDialogKeyboard::ShowAndGetFilter(m_search, true);
        return true;
      }
    }
    break;
  case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_FILTER_ITEMS && IsActive())
      {
        if (message.GetParam2() == 1) // append
          m_filter += message.GetStringParam();
        else if (message.GetParam2() == 2)
        { // delete
          if (m_filter.size())
            m_filter = m_filter.Left(m_filter.size() - 1);
        }
        else
          m_filter = message.GetStringParam();
        OnFilterItems();
        return true;
      }
      if (message.GetParam1() == GUI_MSG_SEARCH_UPDATE && IsActive())
      {
        // search updated - reset timer
        m_searchTimer.StartZero();
        m_search = message.GetStringParam();
      }
    }
  }
  return CGUIMediaWindow::OnMessage(message);
}

bool CGUIWindowProgramNav::OnAction(const CAction& action)
{
  if (action.wID == ACTION_PARENT_DIR)
  {
    if (g_advancedSettings.m_bUseEvilB && m_vecItems->m_strPath == m_startDirectory)
    {
      m_gWindowManager.PreviousWindow();
      return true;
    }
  }

  return CGUIMediaWindow::OnAction(action);
}

CStdString CGUIWindowProgramNav::GetQuickpathName(const CStdString& strPath) const
{
/*  if (strPath.Equals("musicdb://1/"))
    return "Genres";
  else if (strPath.Equals("musicdb://2/"))
    return "Artists";
  else if (strPath.Equals("musicdb://3/"))
    return "Albums";
  else if (strPath.Equals("musicdb://4/"))
    return "Songs";
  else if (strPath.Equals("musicdb://5/"))
    return "Top100";
  else if (strPath.Equals("musicdb://5/2/"))
    return "Top100Songs";
  else if (strPath.Equals("musicdb://5/1/"))
    return "Top100Albums";
  else if (strPath.Equals("musicdb://6/"))
    return "RecentlyAddedAlbums";
  else if (strPath.Equals("musicdb://7/"))
    return "RecentlyPlayedAlbums";
  else if (strPath.Equals("musicdb://8/"))
    return "Compilations";
  else if (strPath.Equals("musicdb://9/"))
    return "Years";
  else if (strPath.Equals("special://musicplaylists/"))
    return "Playlists";
  else*/
  {
    CLog::Log(LOGERROR, "  %s: Unknown parameter (%s)", __FUNCTION__,strPath.c_str());
    return strPath;
  }
}

bool CGUIWindowProgramNav::OnClick(int iItem)
{
  if (iItem < 0 || iItem >= m_vecItems->Size()) return false;

  CFileItemPtr item = m_vecItems->Get(iItem);
  if (item->m_strPath.Left(14) == "programsearch://")
  {
    if (m_searchWithEdit)
      OnSearchUpdate();
    else
      CGUIDialogKeyboard::ShowAndGetFilter(m_search, true);
    return true;
  }
  return CGUIMediaWindow::OnClick(iItem);
}

bool CGUIWindowProgramNav::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  if (m_bDisplayEmptyDatabaseMessage)
    return true;
  if (strDirectory.IsEmpty())
    AddSearchFolder();

  bool bResult = CGUIMediaWindow::GetDirectory(strDirectory, items);

  // update our content in the info manager
  if (strDirectory.Equals("plugin://music/"))
    items.SetContent("plugins");

  // clear the filter
  m_filter.Empty();
  return bResult;
}

void CGUIWindowProgramNav::UpdateButtons()
{
  CGUIMediaWindow::UpdateButtons();

  // Update object count
  int iItems = m_vecItems->Size();
  if (iItems)
  {
    // check for parent dir and "all" items
    // should always be the first two items
    for (int i = 0; i <= (iItems>=2 ? 1 : 0); i++)
    {
      CFileItemPtr pItem = m_vecItems->Get(i);
      if (pItem->IsParentFolder()) iItems--;
      if (pItem->m_strPath.Left(4).Equals("/-1/")) iItems--;
    }
    // or the last item
    if (m_vecItems->Size() > 2 &&
      m_vecItems->Get(m_vecItems->Size()-1)->m_strPath.Left(4).Equals("/-1/"))
      iItems--;
  }
  CStdString items;
  items.Format("%i %s", iItems, g_localizeStrings.Get(127).c_str());
  SET_CONTROL_LABEL(CONTROL_LABELFILES, items);

  SET_CONTROL_SELECTED(GetID(),CONTROL_BTN_FILTER, !m_filter.IsEmpty());
  SET_CONTROL_LABEL2(CONTROL_BTN_FILTER, m_filter);

  if (m_searchWithEdit)
  {
    SendMessage(GUI_MSG_SET_TYPE, CONTROL_SEARCH, CGUIEditControl::INPUT_TYPE_SEARCH);
    SET_CONTROL_LABEL2(CONTROL_SEARCH, m_search);
  }
}

void CGUIWindowProgramNav::PlayItem(int iItem)
{
  // root is not allowed
  if (m_vecItems->IsVirtualDirectoryRoot())
    return;
}

void CGUIWindowProgramNav::OnWindowLoaded()
{
  const CGUIControl *control = GetControl(CONTROL_SEARCH);
  m_searchWithEdit = (control && control->GetControlType() == CGUIControl::GUICONTROL_EDIT);

  SendMessage(GUI_MSG_SET_TYPE, CONTROL_BTN_FILTER, CGUIEditControl::INPUT_TYPE_FILTER);
  CGUIMediaWindow::OnWindowLoaded();
}

void CGUIWindowProgramNav::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
//  CGUIMediaWindow::GetNonContextButtons(buttons);
}

bool CGUIWindowProgramNav::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);

  switch (button)
  {
    default:
      break;
  }

  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

void CGUIWindowProgramNav::DisplayEmptyDatabaseMessage(bool bDisplay)
{
  m_bDisplayEmptyDatabaseMessage = bDisplay;
}

void CGUIWindowProgramNav::OnSearchUpdate()
{
  CStdString search(m_search);
  CUtil::URLEncode(search);
  if (!search.IsEmpty())
  {
    CStdString path = "programsearch://" + search + "/";
    m_history.ClearPathHistory();
    Update(path);
  }
  else if (m_vecItems->IsVirtualDirectoryRoot())
  {
    Update("");
  }
}

void CGUIWindowProgramNav::Render()
{
  static const int search_timeout = 2000;
  // update our searching
  if (m_searchTimer.IsRunning() && m_searchTimer.GetElapsedMilliseconds() > search_timeout)
  {
    OnSearchUpdate();
    m_searchTimer.Stop();
  }
  if (m_bDisplayEmptyDatabaseMessage)
    SET_CONTROL_LABEL(CONTROL_LABELEMPTY,g_localizeStrings.Get(745)+'\n'+g_localizeStrings.Get(746));
  else
    SET_CONTROL_LABEL(CONTROL_LABELEMPTY,"");
  CGUIMediaWindow::Render();
}

void CGUIWindowProgramNav::ClearFileItems()
{
  m_viewControl.Clear();
  m_vecItems->Clear();
  m_unfilteredItems->Clear();
}

void CGUIWindowProgramNav::OnFilterItems()
{
  CStdString currentItem;
  int item = m_viewControl.GetSelectedItem();
  if (item >= 0)
    currentItem = m_vecItems->Get(item)->m_strPath;

  m_viewControl.Clear();

  FilterItems(*m_vecItems);

  // and update our view control + buttons
  m_viewControl.SetItems(*m_vecItems);
  m_viewControl.SetSelectedItem(currentItem);
  UpdateButtons();
}

void CGUIWindowProgramNav::FilterItems(CFileItemList &items)
{
  if (m_vecItems->IsVirtualDirectoryRoot())
    return;

  items.ClearItems();

  CStdString filter(m_filter);
  filter.TrimLeft().ToLower();
  bool numericMatch = StringUtils::IsNaturalNumber(filter);

  for (int i = 0; i < m_unfilteredItems->Size(); i++)
  {
    CFileItemPtr item = m_unfilteredItems->Get(i);
    if (item->IsParentFolder() || filter.IsEmpty()) //||
//        CMusicDatabaseDirectory::IsAllItem(item->m_strPath))
    {
      items.Add(item);
      continue;
    }
    // TODO: Need to update this to get all labels, ideally out of the displayed info (ie from m_layout and m_focusedLayout)
    // though that isn't practical.  Perhaps a better idea would be to just grab the info that we should filter on based on
    // where we are in the library tree.
    // Another idea is tying the filter string to the current level of the tree, so that going deeper disables the filter,
    // but it's re-enabled on the way back out.
    CStdString match;
/*    if (item->GetFocusedLayout())
      match = item->GetFocusedLayout()->GetAllText();
    else if (item->GetLayout())
      match = item->GetLayout()->GetAllText();
    else*/
    match = item->GetLabel();

    if (numericMatch)
      StringUtils::WordToDigits(match);
    size_t pos = StringUtils::FindWords(match.c_str(), filter.c_str());

    if (pos != CStdString::npos)
      items.Add(item);
  }
}

void CGUIWindowProgramNav::OnFinalizeFileItems(CFileItemList &items)
{
  CGUIMediaWindow::OnFinalizeFileItems(items);
//  m_unfilteredItems->Append(items);
//   now filter as necessary
//  if (!m_filter.IsEmpty())
//    FilterItems(items);
}

void CGUIWindowProgramNav::AddSearchFolder()
{
  if (m_guiState.get())
  {
    // add our remove the musicsearch source
    VECSOURCES &sources = m_guiState->GetSources();
    bool haveSearchSource = false;
    bool needSearchSource = !m_search.IsEmpty() || !m_searchWithEdit; // we always need it if we don't have the edit control
    for (IVECSOURCES it = sources.begin(); it != sources.end(); ++it)
    {
      CMediaSource& share = *it;
      if (share.strPath == "programsearch://")
      {
        haveSearchSource = true;
        if (!needSearchSource)
        { // remove it
          sources.erase(it);
          break;
        }
      }
    }
    if (!haveSearchSource && needSearchSource)
    {
      // add earch share
      CMediaSource share;
      share.strName=g_localizeStrings.Get(137); // Search
      share.strPath = "programsearch://";
      share.m_strThumbnailImage="defaultFolderBig.png";
      share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
      sources.push_back(share);
    }
    m_rootDir.SetSources(sources);
  }
}
