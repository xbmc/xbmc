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

#include "GUIWindowPVRRecordings.h"

#include "guilib/GUIKeyboardFactory.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/Key.h"
#include "guilib/LocalizeStrings.h"
#include "GUIInfoManager.h"
#include "pvr/PVRManager.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "threads/SingleLock.h"
#include "pvr/addons/PVRClients.h"
#include "video/windows/GUIWindowVideoNav.h"

using namespace std;
using namespace PVR;

CGUIWindowPVRRecordings::CGUIWindowPVRRecordings(bool bRadio) :
  CGUIWindowPVRBase(bRadio, bRadio ? WINDOW_RADIO_RECORDINGS : WINDOW_TV_RECORDINGS, "MyPVRRecordings.xml")
{
}

void CGUIWindowPVRRecordings::UnregisterObservers(void)
{
  CSingleLock lock(m_critSection);
  if (g_PVRRecordings)
    g_PVRRecordings->UnregisterObserver(this);
  if (g_PVRTimers)
    g_PVRTimers->UnregisterObserver(this);
  g_infoManager.UnregisterObserver(this);
}

void CGUIWindowPVRRecordings::ResetObservers(void)
{
  CSingleLock lock(m_critSection);
  UnregisterObservers();
  g_PVRRecordings->RegisterObserver(this);
  g_PVRTimers->RegisterObserver(this);
  g_infoManager.RegisterObserver(this);
}

std::string CGUIWindowPVRRecordings::GetDirectoryPath(void)
{
  if (StringUtils::StartsWith(m_vecItems->GetPath(), "pvr://recordings/"))
    return m_vecItems->GetPath();
  return "pvr://recordings/";
}

CStdString CGUIWindowPVRRecordings::GetResumeString(const CFileItem& item)
{
  CStdString resumeString;
  if (item.IsPVRRecording())
  {

    // First try to find the resume position on the back-end, if that fails use video database
    int positionInSeconds = item.GetPVRRecordingInfoTag()->GetLastPlayedPosition();
    // If the back-end does report a saved position it will be picked up by FileItem
    if (positionInSeconds < 0)
    {
      CVideoDatabase db;
      if (db.Open())
      {
        CBookmark bookmark;
        CStdString itemPath(item.GetPVRRecordingInfoTag()->m_strFileNameAndPath);
        if (db.GetResumeBookMark(itemPath, bookmark) )
          positionInSeconds = lrint(bookmark.timeInSeconds);
        db.Close();
      }
    }

    // Suppress resume from 0
    if (positionInSeconds > 0)
      resumeString = StringUtils::Format(g_localizeStrings.Get(12022).c_str(), StringUtils::SecondsToTimeString(positionInSeconds).c_str());
  }
  return resumeString;
}

void CGUIWindowPVRRecordings::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return;
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);

  if (pItem->HasPVRRecordingInfoTag())
  {
    buttons.Add(CONTEXT_BUTTON_INFO, 19053);      /* Get Information of this recording */
    buttons.Add(CONTEXT_BUTTON_FIND, 19003);      /* Find similar program */
    buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 12021); /* Play this recording */
    CStdString resumeString = GetResumeString(*pItem);
    if (!resumeString.empty())
    {
      buttons.Add(CONTEXT_BUTTON_RESUME_ITEM, resumeString);
    }
  }
  if (pItem->m_bIsFolder)
  {
    // Have both options for folders since we don't know whether all childs are watched/unwatched
    buttons.Add(CONTEXT_BUTTON_MARK_UNWATCHED, 16104); /* Mark as UnWatched */
    buttons.Add(CONTEXT_BUTTON_MARK_WATCHED, 16103);   /* Mark as Watched */
  }
  if (pItem->HasPVRRecordingInfoTag())
  {
    if (pItem->GetPVRRecordingInfoTag()->m_playCount > 0)
      buttons.Add(CONTEXT_BUTTON_MARK_UNWATCHED, 16104); /* Mark as UnWatched */
    else
      buttons.Add(CONTEXT_BUTTON_MARK_WATCHED, 16103);   /* Mark as Watched */

    buttons.Add(CONTEXT_BUTTON_RENAME, 118);      /* Rename this recording */
  }
  
  // Add delete button for all items except the All recordings directory
  if (!g_PVRRecordings->IsAllRecordingsDirectory(*pItem.get()))
    buttons.Add(CONTEXT_BUTTON_DELETE, 117);

  if (pItem->HasPVRRecordingInfoTag() &&
      g_PVRClients->HasMenuHooks(pItem->GetPVRRecordingInfoTag()->m_iClientId, PVR_MENUHOOK_RECORDING))
    buttons.Add(CONTEXT_BUTTON_MENU_HOOKS, 19195);      /* PVR client specific action */
}

bool CGUIWindowPVRRecordings::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_PARENT_DIR ||
      action.GetID() == ACTION_NAV_BACK)
  {
    if (m_vecItems->GetPath() != "pvr://recordings/")
    {
      GoParentFolder();
      return true;
    }
  }
  return CGUIWindowPVRBase::OnAction(action);
}

bool CGUIWindowPVRRecordings::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return false;
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);

  return OnContextButtonPlay(pItem.get(), button) ||
      OnContextButtonRename(pItem.get(), button) ||
      OnContextButtonDelete(pItem.get(), button) ||
      OnContextButtonInfo(pItem.get(), button) ||
      OnContextButtonMarkWatched(pItem, button) ||
      CGUIWindowPVRBase::OnContextButton(itemNumber, button);
}

bool CGUIWindowPVRRecordings::Update(const std::string &strDirectory, bool updateFilterPath /* = true */)
{
  m_thumbLoader.StopThread();

  bool bReturn = CGUIWindowPVRBase::Update(strDirectory);

  AfterUpdate(*m_unfilteredItems);

  return bReturn;
}

bool CGUIWindowPVRRecordings::OnMessage(CGUIMessage &message)
{
  bool bReturn = false;
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
      if (message.GetSenderId() == m_viewControl.GetCurrentControl())
      {
        int iItem = m_viewControl.GetSelectedItem();
        if (iItem > 0 || iItem < (int) m_vecItems->Size())
        {
          switch (message.GetParam1())
          {
            case ACTION_SELECT_ITEM:
            case ACTION_MOUSE_LEFT_CLICK:
            case ACTION_PLAY:
            {
              CFileItemPtr pItem = m_vecItems->Get(iItem);
              string resumeString = GetResumeString(*pItem);
              if (!resumeString.empty())
              {
                CContextButtons choices;
                choices.Add(CONTEXT_BUTTON_RESUME_ITEM, resumeString);
                choices.Add(CONTEXT_BUTTON_PLAY_ITEM, 12021);
                int choice = CGUIDialogContextMenu::ShowAndGetChoice(choices);
                if (choice > 0)
                  OnContextButtonPlay(pItem.get(), (CONTEXT_BUTTON)choice);
                bReturn = true;
              }
              break;
            }
            case ACTION_CONTEXT_MENU:
            case ACTION_MOUSE_RIGHT_CLICK:
              OnPopupMenu(iItem);
              bReturn = true;
              break;
            case ACTION_SHOW_INFO:
              ShowRecordingInfo(m_vecItems->Get(iItem).get());
              bReturn = true;
              break;
            case ACTION_DELETE_ITEM:
              ActionDeleteRecording(m_vecItems->Get(iItem).get());
              bReturn = true;
              break;
            default:
              bReturn = false;
              break;
          }
        }
      }
      break;
    case GUI_MSG_REFRESH_LIST:
      switch(message.GetParam1())
      {
        case ObservableMessageTimers:
        case ObservableMessageCurrentItem:
        {
          if (IsActive())
            SetInvalid();
          bReturn = true;
          break;
        }
        case ObservableMessageRecordings:
        case ObservableMessageTimersReset:
        {
          if (IsActive())
            Refresh(true);
          bReturn = true;
          break;
        }
      }
      break;
  }

  return bReturn || CGUIWindowPVRBase::OnMessage(message);
}

bool CGUIWindowPVRRecordings::OnContextButtonDelete(CFileItem *item, CONTEXT_BUTTON button)
{
  return button == CONTEXT_BUTTON_DELETE ? ActionDeleteRecording(item) : false;
}

bool CGUIWindowPVRRecordings::OnContextButtonInfo(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_INFO)
  {
    bReturn = true;
    ShowRecordingInfo(item);
  }

  return bReturn;
}

bool CGUIWindowPVRRecordings::OnContextButtonPlay(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if ((button == CONTEXT_BUTTON_PLAY_ITEM) ||
      (button == CONTEXT_BUTTON_RESUME_ITEM))
  {
    item->m_lStartOffset = button == CONTEXT_BUTTON_RESUME_ITEM ? STARTOFFSET_RESUME : 0;
    bReturn = PlayFile(item, false); /* play recording */
  }

  return bReturn;
}

bool CGUIWindowPVRRecordings::OnContextButtonRename(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_RENAME)
  {
    bReturn = true;

    CPVRRecording *recording = item->GetPVRRecordingInfoTag();
    CStdString strNewName = recording->m_strTitle;
    if (CGUIKeyboardFactory::ShowAndGetInput(strNewName, g_localizeStrings.Get(19041), false))
    {
      if (g_PVRRecordings->RenameRecording(*item, strNewName))
        Refresh(true);
    }
  }

  return bReturn;
}

bool CGUIWindowPVRRecordings::OnContextButtonMarkWatched(const CFileItemPtr &item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_MARK_WATCHED)
  {
    bReturn = true;

    int newSelection = m_viewControl.GetSelectedItem();
    g_PVRRecordings->SetRecordingsPlayCount(item, 1);
    m_viewControl.SetSelectedItem(newSelection);

    Refresh(true);
  }

  if (button == CONTEXT_BUTTON_MARK_UNWATCHED)
  {
    bReturn = true;

    g_PVRRecordings->SetRecordingsPlayCount(item, 0);

    Refresh(true);
  }

  return bReturn;
}

void CGUIWindowPVRRecordings::AfterUpdate(CFileItemList& items)
{
  if (!items.IsEmpty())
  {
    CFileItemList files;
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr pItem = items[i];
      if (!pItem->m_bIsFolder)
        files.Add(pItem);
    }

    if (!files.IsEmpty())
    {
      files.SetPath(items.GetPath());
      if (m_database.Open())
      {
        if (g_PVRRecordings->HasAllRecordingsPathExtension(files.GetPath()))
        {
          // Build a map of all files belonging to common subdirectories and call
          // LoadVideoInfo for each item list
          typedef boost::shared_ptr<CFileItemList> CFileItemListPtr;
          typedef std::map<CStdString, CFileItemListPtr> DirectoryMap;

          DirectoryMap directory_map;
          for (int i = 0; i < files.Size(); i++)
          {
            CStdString strDirectory = URIUtils::GetDirectory(files[i]->GetPath());
            DirectoryMap::iterator it = directory_map.find(strDirectory);
            if (it == directory_map.end())
              it = directory_map.insert(std::make_pair(
                  strDirectory, CFileItemListPtr(new CFileItemList(strDirectory)))).first;
            it->second->Add(files[i]);
          }

          for (DirectoryMap::iterator it = directory_map.begin(); it != directory_map.end(); it++)
            CGUIWindowVideoNav::LoadVideoInfo(*it->second, m_database, false);
        }
        else
          CGUIWindowVideoNav::LoadVideoInfo(files, m_database, false);
        m_database.Close();
      }
      m_thumbLoader.Load(files);
    }
  }
}
