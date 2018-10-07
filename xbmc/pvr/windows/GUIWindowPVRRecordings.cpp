/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowPVRRecordings.h"

#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/Key.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SingleLock.h"
#include "utils/URIUtils.h"
#include "video/windows/GUIWindowVideoNav.h"

#include "pvr/PVRGUIActions.h"
#include "pvr/PVRManager.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/recordings/PVRRecordingsPath.h"

using namespace PVR;

CGUIWindowPVRRecordingsBase::CGUIWindowPVRRecordingsBase(bool bRadio, int id, const std::string &xmlFile) :
  CGUIWindowPVRBase(bRadio, id, xmlFile),
  m_bShowDeletedRecordings(false),
  m_settings({
    CSettings::SETTING_PVRRECORD_GROUPRECORDINGS,
    CSettings::SETTING_MYVIDEOS_SELECTACTION
  })
{
  CServiceBroker::GetGUI()->GetInfoManager().RegisterObserver(this);
}

CGUIWindowPVRRecordingsBase::~CGUIWindowPVRRecordingsBase()
{
  CServiceBroker::GetGUI()->GetInfoManager().UnregisterObserver(this);
}

void CGUIWindowPVRRecordingsBase::OnWindowLoaded()
{
  CONTROL_SELECT(CONTROL_BTNGROUPITEMS);
}

std::string CGUIWindowPVRRecordingsBase::GetDirectoryPath()
{
  const std::string basePath = CPVRRecordingsPath(m_bShowDeletedRecordings, m_bRadio);
  return URIUtils::PathHasParent(m_vecItems->GetPath(), basePath) ? m_vecItems->GetPath() : basePath;
}

void CGUIWindowPVRRecordingsBase::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return;
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);

  if (pItem->IsParentFolder())
  {
    // No context menu for ".." items
    return;
  }

  bool isDeletedRecording = false;

  CPVRRecordingPtr recording(pItem->GetPVRRecordingInfoTag());
  if (recording)
  {
    isDeletedRecording = recording->IsDeleted();

    if (isDeletedRecording)
    {
      if (m_vecItems->GetObjectCount() > 1)
        buttons.Add(CONTEXT_BUTTON_DELETE_ALL, 19292); /* Delete all permanently */
    }
  }

  if (!isDeletedRecording)
    CGUIWindowPVRBase::GetContextButtons(itemNumber, buttons);
}

bool CGUIWindowPVRRecordingsBase::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_PARENT_DIR ||
      action.GetID() == ACTION_NAV_BACK)
  {
    CPVRRecordingsPath path(m_vecItems->GetPath());
    if (path.IsValid() && !path.IsRecordingsRoot())
    {
      GoParentFolder();
      return true;
    }
  }
  return CGUIWindowPVRBase::OnAction(action);
}

bool CGUIWindowPVRRecordingsBase::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return false;
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);

  return OnContextButtonDeleteAll(pItem.get(), button) ||
      CGUIMediaWindow::OnContextButton(itemNumber, button);
}

bool CGUIWindowPVRRecordingsBase::Update(const std::string &strDirectory, bool updateFilterPath /* = true */)
{
  m_thumbLoader.StopThread();

  int iOldCount = m_vecItems->GetObjectCount();
  const std::string oldPath = m_vecItems->GetPath();

  bool bReturn = CGUIWindowPVRBase::Update(strDirectory);

  if (bReturn)
  {
    // TODO: does it make sense to show the non-deleted recordings, although user wants
    //       to see the deleted recordings? Or is this just another hack to avoid misbehavior
    //       of CGUIMediaWindow if it has no content?

    CSingleLock lock(m_critSection);

    /* empty list for deleted recordings */
    if (m_vecItems->GetObjectCount() == 0 && m_bShowDeletedRecordings)
    {
      /* show the normal recordings instead */
      m_bShowDeletedRecordings = false;
      lock.Leave();
      Update(GetDirectoryPath());
      return bReturn;
    }
  }

  if (bReturn && iOldCount > 0 && m_vecItems->GetObjectCount() == 0 && oldPath == m_vecItems->GetPath())
  {
    /* go to the parent folder if we're in a subdirectory and for instance just deleted the last item */
    const CPVRRecordingsPath path(m_vecItems->GetPath());
    if (path.IsValid() && !path.IsRecordingsRoot())
      GoParentFolder();
  }
  return bReturn;
}

void CGUIWindowPVRRecordingsBase::UpdateButtons()
{
  int iWatchMode = CMediaSettings::GetInstance().GetWatchedMode("recordings");
  int iStringId = 257; // "Error"

  if (iWatchMode == WatchedModeAll)
    iStringId = 22015; // "All recordings"
  else if (iWatchMode == WatchedModeUnwatched)
    iStringId = 16101; // "Unwatched"
  else if (iWatchMode == WatchedModeWatched)
    iStringId = 16102; // "Watched"

  SET_CONTROL_LABEL(CONTROL_BTNSHOWMODE, g_localizeStrings.Get(iStringId));

  bool bGroupRecordings = m_settings.GetBoolValue(CSettings::SETTING_PVRRECORD_GROUPRECORDINGS);
  SET_CONTROL_SELECTED(GetID(), CONTROL_BTNGROUPITEMS, bGroupRecordings);

  CGUIRadioButtonControl *btnShowDeleted = static_cast<CGUIRadioButtonControl*>(GetControl(CONTROL_BTNSHOWDELETED));
  if (btnShowDeleted)
  {
    btnShowDeleted->SetVisible(m_bRadio ? CServiceBroker::GetPVRManager().Recordings()->HasDeletedRadioRecordings() : CServiceBroker::GetPVRManager().Recordings()->HasDeletedTVRecordings());
    btnShowDeleted->SetSelected(m_bShowDeletedRecordings);
  }

  CGUIWindowPVRBase::UpdateButtons();
  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER1, m_bShowDeletedRecordings ? g_localizeStrings.Get(19179) : ""); /* Deleted recordings trash */

  const CPVRRecordingsPath path(m_vecItems->GetPath());
  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER2, bGroupRecordings && path.IsValid() ? path.GetUnescapedDirectoryPath() : "");
}

bool CGUIWindowPVRRecordingsBase::OnMessage(CGUIMessage &message)
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
          const CFileItemPtr item(m_vecItems->Get(iItem));
          switch (message.GetParam1())
          {
            case ACTION_SELECT_ITEM:
            case ACTION_MOUSE_LEFT_CLICK:
            case ACTION_PLAYER_PLAY:
            {
              const CPVRRecordingsPath path(m_vecItems->GetPath());
              if (path.IsValid() && path.IsRecordingsRoot() && item->IsParentFolder())
              {
                // handle special 'go home' item.
                CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_HOME);
                bReturn = true;
                break;
              }

              if (item->m_bIsFolder)
              {
                // recording folders and ".." folders in subfolders are handled by base class.
                bReturn = false;
                break;
              }

              if (message.GetParam1() == ACTION_PLAYER_PLAY)
              {
                CServiceBroker::GetPVRManager().GUIActions()->PlayRecording(item, true /* check resume */);
                bReturn = true;
              }
              else
              {
                switch (m_settings.GetIntValue(CSettings::SETTING_MYVIDEOS_SELECTACTION))
                {
                  case SELECT_ACTION_CHOOSE:
                    OnPopupMenu(iItem);
                    bReturn = true;
                    break;
                  case SELECT_ACTION_PLAY_OR_RESUME:
                    CServiceBroker::GetPVRManager().GUIActions()->PlayRecording(item, true /* check resume */);
                    bReturn = true;
                    break;
                  case SELECT_ACTION_RESUME:
                    CServiceBroker::GetPVRManager().GUIActions()->ResumePlayRecording(item, true /* fall back to play if no resume possible */);
                    bReturn = true;
                    break;
                  case SELECT_ACTION_INFO:
                    CServiceBroker::GetPVRManager().GUIActions()->ShowRecordingInfo(item);
                    bReturn = true;
                    break;
                  default:
                    bReturn = false;
                    break;
                }
              }
              break;
            }
            case ACTION_CONTEXT_MENU:
            case ACTION_MOUSE_RIGHT_CLICK:
              OnPopupMenu(iItem);
              bReturn = true;
              break;
            case ACTION_SHOW_INFO:
              CServiceBroker::GetPVRManager().GUIActions()->ShowRecordingInfo(item);
              bReturn = true;
              break;
            case ACTION_DELETE_ITEM:
              CServiceBroker::GetPVRManager().GUIActions()->DeleteRecording(item);
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
        const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
        settings->ToggleBool(CSettings::SETTING_PVRRECORD_GROUPRECORDINGS);
        settings->Save();
        Refresh(true);
      }
      else if (message.GetSenderId() == CONTROL_BTNSHOWDELETED)
      {
        CGUIRadioButtonControl *radioButton = static_cast<CGUIRadioButtonControl*>(GetControl(CONTROL_BTNSHOWDELETED));
        if (radioButton)
        {
          m_bShowDeletedRecordings = radioButton->IsSelected();
          Update(GetDirectoryPath());
        }
        bReturn = true;
      }
      else if (message.GetSenderId() == CONTROL_BTNSHOWMODE)
      {
        CMediaSettings::GetInstance().CycleWatchedMode("recordings");
        CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
        OnFilterItems(GetProperty("filter").asString());
        UpdateButtons();
        return true;
      }
      break;
    case GUI_MSG_REFRESH_LIST:
      switch(message.GetParam1())
      {
        case ObservableMessageTimers:
        case ObservableMessageEpg:
        case ObservableMessageEpgContainer:
        case ObservableMessageEpgActiveItem:
        case ObservableMessageCurrentItem:
        {
          SetInvalid();
          break;
        }
        case ObservableMessageRecordings:
        case ObservableMessageTimersReset:
        {
          Refresh(true);
          break;
        }
      }
      break;
  }

  return bReturn || CGUIWindowPVRBase::OnMessage(message);
}

bool CGUIWindowPVRRecordingsBase::OnContextButtonDeleteAll(CFileItem *item, CONTEXT_BUTTON button)
{
  if (button == CONTEXT_BUTTON_DELETE_ALL)
  {
    CServiceBroker::GetPVRManager().GUIActions()->DeleteAllRecordingsFromTrash();
    return true;
  }

  return false;
}

void CGUIWindowPVRRecordingsBase::OnPrepareFileItems(CFileItemList& items)
{
  if (items.IsEmpty())
    return;

  CFileItemList files;
  for (const auto& item : items)
  {
    if (!item->m_bIsFolder)
      files.Add(item);
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

bool CGUIWindowPVRRecordingsBase::GetFilteredItems(const std::string &filter, CFileItemList &items)
{
  bool listchanged = CGUIWindowPVRBase::GetFilteredItems(filter, items);

  int watchMode = CMediaSettings::GetInstance().GetWatchedMode("recordings");

  CFileItemPtr item;
  for (int i = 0; i < items.Size(); i++)
  {
    item = items.Get(i);

    if (item->IsParentFolder()) // Don't delete the go to parent folder
      continue;

    if (!item->HasPVRRecordingInfoTag())
      continue;

    int iPlayCount = item->GetPVRRecordingInfoTag()->GetPlayCount();
    if ((watchMode == WatchedModeWatched && iPlayCount == 0) ||
        (watchMode == WatchedModeUnwatched && iPlayCount > 0))
    {
      items.Remove(i);
      i--;
      listchanged = true;
    }
  }

  // Remove the parent folder item, if it's the only item in the folder.
  if (items.GetObjectCount() == 0 && items.GetFileCount() > 0 && items.Get(0)->IsParentFolder())
    items.Remove(0);

  return listchanged;
}
