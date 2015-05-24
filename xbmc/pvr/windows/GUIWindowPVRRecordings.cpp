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

#include "ContextMenuManager.h"
#include "guilib/GUIKeyboardFactory.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "guilib/LocalizeStrings.h"
#include "GUIInfoManager.h"
#include "pvr/PVRManager.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"
#include "utils/StringUtils.h"
#include "threads/SingleLock.h"
#include "pvr/addons/PVRClients.h"
#include "video/windows/GUIWindowVideoNav.h"

using namespace PVR;

CGUIWindowPVRRecordings::CGUIWindowPVRRecordings(bool bRadio) :
  CGUIWindowPVRBase(bRadio, bRadio ? WINDOW_RADIO_RECORDINGS : WINDOW_TV_RECORDINGS, "MyPVRRecordings.xml") ,
  m_bShowDeletedRecordings(false)
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

void CGUIWindowPVRRecordings::OnWindowLoaded()
{
  CONTROL_SELECT(CONTROL_BTNGROUPITEMS);
}

std::string CGUIWindowPVRRecordings::GetDirectoryPath(void)
{
  std::string basePath = StringUtils::Format("pvr://recordings/%s/", m_bShowDeletedRecordings ? "deleted" : "active");

  if (StringUtils::StartsWith(m_vecItems->GetPath(), basePath))
    return m_vecItems->GetPath();

  return basePath;
}

std::string CGUIWindowPVRRecordings::GetResumeString(const CFileItem& item)
{
  std::string resumeString;
  if (item.IsUsablePVRRecording())
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
        std::string itemPath(item.GetPVRRecordingInfoTag()->m_strFileNameAndPath);
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

  bool isDeletedRecording = false;

  if (pItem->HasPVRRecordingInfoTag())
  {
    isDeletedRecording = pItem->GetPVRRecordingInfoTag()->IsDeleted();

    buttons.Add(CONTEXT_BUTTON_INFO, 19053);      /* Get Information of this recording */
    if (!isDeletedRecording)
    {
      buttons.Add(CONTEXT_BUTTON_FIND, 19003);      /* Find similar program */
      buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 12021); /* Play this recording */
      std::string resumeString = GetResumeString(*pItem);
      if (!resumeString.empty())
      {
        buttons.Add(CONTEXT_BUTTON_RESUME_ITEM, resumeString);
      }
    }
    else
    {
      buttons.Add(CONTEXT_BUTTON_UNDELETE, 19290);    /* Undelete this recording */
      buttons.Add(CONTEXT_BUTTON_DELETE, 19291);      /* Delete this permanently */
      if (m_vecItems->GetObjectCount() > 1)
        buttons.Add(CONTEXT_BUTTON_DELETE_ALL, 19292);  /* Delete all permanently */
    }
  }
  if (!isDeletedRecording)
  {
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

    buttons.Add(CONTEXT_BUTTON_DELETE, 117);
  }

  if (pItem->HasPVRRecordingInfoTag())
  {
    if (!isDeletedRecording && g_PVRClients->HasMenuHooks(pItem->GetPVRRecordingInfoTag()->m_iClientId, PVR_MENUHOOK_RECORDING))
      buttons.Add(CONTEXT_BUTTON_MENU_HOOKS, 19195);      /* PVR client specific action */
    else if (isDeletedRecording && g_PVRClients->HasMenuHooks(pItem->GetPVRRecordingInfoTag()->m_iClientId, PVR_MENUHOOK_DELETED_RECORDING))
      buttons.Add(CONTEXT_BUTTON_MENU_HOOKS, 19195);      /* PVR client specific action */
  }

  if (!isDeletedRecording)
    CGUIWindowPVRBase::GetContextButtons(itemNumber, buttons);

  CContextMenuManager::Get().AddVisibleItems(pItem, buttons);
}

bool CGUIWindowPVRRecordings::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_PARENT_DIR ||
      action.GetID() == ACTION_NAV_BACK)
  {
    if (m_vecItems->GetPath() != "pvr://recordings/active/" && m_vecItems->GetPath() != "pvr://recordings/deleted/")
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
      OnContextButtonUndelete(pItem.get(), button) ||
      OnContextButtonDeleteAll(pItem.get(), button) ||
      OnContextButtonInfo(pItem.get(), button) ||
      OnContextButtonMarkWatched(pItem, button) ||
      CGUIWindowPVRBase::OnContextButton(itemNumber, button);
}

bool CGUIWindowPVRRecordings::Update(const std::string &strDirectory, bool updateFilterPath /* = true */)
{
  m_thumbLoader.StopThread();

  bool bReturn = CGUIWindowPVRBase::Update(strDirectory);

  /* empty list for deleted recordings */
  if (m_vecItems->GetObjectCount() == 0 && m_bShowDeletedRecordings)
  {
    /* show the normal recordings instead */
    m_bShowDeletedRecordings = false;
    Update(GetDirectoryPath());
  }

  return bReturn;
}

void CGUIWindowPVRRecordings::UpdateButtons(void)
{
  CGUIRadioButtonControl *btnShowDeleted = (CGUIRadioButtonControl*) GetControl(CONTROL_BTNSHOWDELETED);
  if (btnShowDeleted)
  {
    btnShowDeleted->SetVisible(g_PVRRecordings->HasDeletedRecordings());
    btnShowDeleted->SetSelected(m_bShowDeletedRecordings);
  }

  CGUIWindowPVRBase::UpdateButtons();
  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER1, m_bShowDeletedRecordings ? g_localizeStrings.Get(19179) : ""); /* Deleted recordings trash */
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
        if (iItem >= 0 && iItem < m_vecItems->Size())
        {
          switch (message.GetParam1())
          {
            case ACTION_SELECT_ITEM:
            case ACTION_MOUSE_LEFT_CLICK:
            case ACTION_PLAY:
            {
              bReturn = PlayFile(m_vecItems->Get(iItem).get());
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
      else if (message.GetSenderId() == CONTROL_BTNGROUPITEMS)
      {
        CGUIRadioButtonControl *radioButton = (CGUIRadioButtonControl*) GetControl(CONTROL_BTNGROUPITEMS);
        g_PVRRecordings->SetGroupItems(radioButton->IsSelected());
        Refresh(true);
      }
      else if (message.GetSenderId() == CONTROL_BTNSHOWDELETED)
      {
        CGUIRadioButtonControl *radioButton = (CGUIRadioButtonControl*) GetControl(CONTROL_BTNSHOWDELETED);
        if (radioButton)
        {
          m_bShowDeletedRecordings = radioButton->IsSelected();
          Update(GetDirectoryPath());
        }
        bReturn = true;
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

bool CGUIWindowPVRRecordings::ActionDeleteRecording(CFileItem *item)
{
  bool bReturn = false;

  if ((!item->IsPVRRecording() && !item->m_bIsFolder) || item->IsParentFolder())
    return bReturn;

  /* show a confirmation dialog */
  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (!pDialog)
    return bReturn;

  pDialog->SetHeading(122); // Confirm delete
  pDialog->SetLine(0, item->m_bIsFolder ? 19113 : item->GetPVRRecordingInfoTag()->IsDeleted() ? 19294 : 19112); // Delete all recordings in this folder? / Delete this recording permanently? / Delete this recording?
  pDialog->SetLine(1, "");
  pDialog->SetLine(2, item->GetLabel());
  pDialog->SetChoice(1, 117); // Delete

  /* prompt for the user's confirmation */
  pDialog->DoModal();
  if (!pDialog->IsConfirmed())
    return bReturn;

  /* delete the recording */
  if (g_PVRRecordings->Delete(*item))
  {
    g_PVRManager.TriggerRecordingsUpdate();
    bReturn = true;

    /* remove the item from the list immediately, otherwise the
    item count further down may be wrong */
    m_vecItems->Remove(item);

    /* go to the parent folder if we're in a subdirectory and just deleted the last item */
    if (m_vecItems->GetPath() != "pvr://recordings/active/" &&
        m_vecItems->GetPath() != "pvr://recordings/deleted/" && m_vecItems->GetObjectCount() == 0)
      GoParentFolder();
  }

  return bReturn;
}

bool CGUIWindowPVRRecordings::OnContextButtonDelete(CFileItem *item, CONTEXT_BUTTON button)
{
  return button == CONTEXT_BUTTON_DELETE ? ActionDeleteRecording(item) : false;
}

bool CGUIWindowPVRRecordings::OnContextButtonUndelete(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button != CONTEXT_BUTTON_UNDELETE || !item->IsDeletedPVRRecording())
    return bReturn;

  /* undelete the recording */
  if (g_PVRRecordings->Undelete(*item))
  {
    g_PVRManager.TriggerRecordingsUpdate();
    bReturn = true;

    /* remove the item from the list immediately, otherwise the
    item count further down may be wrong */
    m_vecItems->Remove(item);

    /* go to the parent folder if we're in a subdirectory and just deleted the last item */
    if (m_vecItems->GetPath() != "pvr://recordings/deleted/" && m_vecItems->GetObjectCount() == 0)
      GoParentFolder();
  }

  return bReturn;
}

bool CGUIWindowPVRRecordings::OnContextButtonDeleteAll(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button != CONTEXT_BUTTON_DELETE_ALL || !item->IsDeletedPVRRecording())
    return bReturn;

  /* show a confirmation dialog */
  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (!pDialog)
    return bReturn;


  pDialog->SetHeading(19292); // Delete all permanently
  pDialog->SetLine(0, 19293); // Delete all recordings permanently?
  pDialog->SetLine(1, "");
  pDialog->SetLine(2, "");
  pDialog->SetChoice(1, 117); // Delete

  /* prompt for the user's confirmation */
  pDialog->DoModal();
  if (!pDialog->IsConfirmed())
    return bReturn;

  /* undelete the recording */
  if (g_PVRRecordings->DeleteAllRecordingsFromTrash())
  {
    g_PVRManager.TriggerRecordingsUpdate();
    bReturn = true;

    /* remove the item from the list immediately, otherwise the
    item count further down may be wrong */
    m_vecItems->Clear();

    /* go to the parent folder if we're in a subdirectory and just deleted the last item */
    if (m_vecItems->GetPath() != "pvr://recordings/deleted/" && m_vecItems->GetObjectCount() == 0)
      GoParentFolder();
  }
  return bReturn;
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
    CPVRRecordingPtr recording = item->GetPVRRecordingInfoTag();
    if (recording)
    {
      bReturn = true;

      std::string strNewName = recording->m_strTitle;
      if (CGUIKeyboardFactory::ShowAndGetInput(strNewName, g_localizeStrings.Get(19041), false))
      {
        if (g_PVRRecordings->RenameRecording(*item, strNewName))
          Refresh(true);
      }
    }
  }

  return bReturn;
}

bool CGUIWindowPVRRecordings::OnContextButtonMarkWatched(const CFileItemPtr &item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_MARK_WATCHED || button == CONTEXT_BUTTON_MARK_UNWATCHED)
  {
    int playCount = button == CONTEXT_BUTTON_MARK_WATCHED ? 1 : 0;

    if (g_PVRRecordings->SetRecordingsPlayCount(item, playCount))
    {
      // Advance the selected item one notch
      m_viewControl.SetSelectedItem(m_viewControl.GetSelectedItem() + 1);
      Refresh(true);
      bReturn = true;
    }
  }

  return bReturn;
}

void CGUIWindowPVRRecordings::OnPrepareFileItems(CFileItemList& items)
{
  if (items.IsEmpty())
    return;

  CFileItemList files;
  VECFILEITEMS vecItems = items.GetList();
  for (VECFILEITEMS::const_iterator it = vecItems.begin(); it != vecItems.end(); ++it)
  {
    if (!(*it)->m_bIsFolder)
      files.Add((*it));
  }

  if (!files.IsEmpty())
  {
    if (m_database.Open())
    {
      CGUIWindowVideoNav::LoadVideoInfo(files, m_database, false);
      m_database.Close();
    }
    m_thumbLoader.Load(files);
  }

  CGUIWindowPVRBase::OnPrepareFileItems(items);
}
