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

#include "GUIWindowPVRTimers.h"

#include "ContextMenuManager.h"
#include "GUIInfoManager.h"
#include "guilib/GUIKeyboardFactory.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "pvr/PVRManager.h"
#include "pvr/dialogs/GUIDialogPVRTimerSettings.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/addons/PVRClients.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"

using namespace PVR;

CGUIWindowPVRTimers::CGUIWindowPVRTimers(bool bRadio) :
  CGUIWindowPVRBase(bRadio, bRadio ? WINDOW_RADIO_TIMERS : WINDOW_TV_TIMERS, "MyPVRTimers.xml")
{
}

void CGUIWindowPVRTimers::UnregisterObservers(void)
{
  CSingleLock lock(m_critSection);
  if (g_PVRTimers)
    g_PVRTimers->UnregisterObserver(this);
  g_infoManager.UnregisterObserver(this);
}

void CGUIWindowPVRTimers::ResetObservers(void)
{
  CSingleLock lock(m_critSection);
  UnregisterObservers();
  g_PVRTimers->RegisterObserver(this);
  g_infoManager.RegisterObserver(this);
}

std::string CGUIWindowPVRTimers::GetDirectoryPath(void)
{
  const std::string basePath(
    CPVRTimersPath(m_bRadio, CSettings::Get().GetBool("pvrtimers.timertypefilter")).GetPath());
  return StringUtils::StartsWith(m_vecItems->GetPath(), basePath) ? m_vecItems->GetPath() : basePath;
}

void CGUIWindowPVRTimers::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return;
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);

  /* Check for a empty file item list, means only a
     file item with the name "Add timer..." is present */
  if (URIUtils::PathEquals(pItem->GetPath(), CPVRTimersPath::PATH_ADDTIMER))
  {
    buttons.Add(CONTEXT_BUTTON_ADD, 19056);             /* new timer */
  }
  else
  {
    if (pItem->GetPVRTimerInfoTag()->GetEpgInfoTag())
      buttons.Add(CONTEXT_BUTTON_INFO, 19047);          /* epg info */

    buttons.Add(CONTEXT_BUTTON_FIND, 19003);            /* Find similar program */

    if (pItem->GetPVRTimerInfoTag()->HasTimerType() &&
        !pItem->GetPVRTimerInfoTag()->GetTimerType()->IsReadOnly())
    {
      if (pItem->GetPVRTimerInfoTag()->GetTimerType()->SupportsEnableDisable())
      {
        if (pItem->GetPVRTimerInfoTag()->m_state == PVR_TIMER_STATE_DISABLED)
          buttons.Add(CONTEXT_BUTTON_ACTIVATE, 843);    /* activate timer */
        else
          buttons.Add(CONTEXT_BUTTON_ACTIVATE, 844);    /* deactivate timer */
      }

      buttons.Add(CONTEXT_BUTTON_DELETE, 117);          /* delete */
      buttons.Add(CONTEXT_BUTTON_EDIT, 19057);          /* edit timer */

      // As epg-based timers will get it's title from the epg tag, they should not be renamable.
      if (pItem->GetPVRTimerInfoTag()->IsManual())
        buttons.Add(CONTEXT_BUTTON_RENAME, 118);        /* rename */
    }

    buttons.Add(CONTEXT_BUTTON_ADD, 19056);             /* new timer */

    if (g_PVRClients->HasMenuHooks(pItem->GetPVRTimerInfoTag()->m_iClientId, PVR_MENUHOOK_TIMER))
      buttons.Add(CONTEXT_BUTTON_MENU_HOOKS, 19195);    /* PVR client specific action */
  }

  CGUIWindowPVRBase::GetContextButtons(itemNumber, buttons);
  CContextMenuManager::Get().AddVisibleItems(pItem, buttons);
}

bool CGUIWindowPVRTimers::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_PARENT_DIR ||
      action.GetID() == ACTION_NAV_BACK)
  {
    CPVRTimersPath path(m_vecItems->GetPath());
    if (path.IsValid() && path.IsTimerSchedule())
    {
      m_currentFileItem.reset();
      GoParentFolder();
      return true;
    }
  }
  return CGUIWindowPVRBase::OnAction(action);
}

bool CGUIWindowPVRTimers::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return false;
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);

  return OnContextButtonActivate(pItem.get(), button) ||
      OnContextButtonAdd(pItem.get(), button) ||
      OnContextButtonDelete(pItem.get(), button) ||
      OnContextButtonEdit(pItem.get(), button) ||
      OnContextButtonRename(pItem.get(), button) ||
      OnContextButtonInfo(pItem.get(), button) ||
      CGUIWindowPVRBase::OnContextButton(itemNumber, button);
}

bool CGUIWindowPVRTimers::Update(const std::string &strDirectory, bool updateFilterPath /* = true */)
{
  return CGUIWindowPVRBase::Update(strDirectory);
}

void CGUIWindowPVRTimers::UpdateButtons(void)
{
  SET_CONTROL_SELECTED(GetID(), CONTROL_BTNTIMERTYPEFILTER, CSettings::Get().GetBool("pvrtimers.timertypefilter"));

  CGUIWindowPVRBase::UpdateButtons();

  std::string strHeaderTitle;
  if (m_currentFileItem && m_currentFileItem->HasPVRTimerInfoTag())
  {
    CPVRTimerInfoTagPtr timer = m_currentFileItem->GetPVRTimerInfoTag();
    strHeaderTitle = timer->Title();
  }

  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER1, strHeaderTitle);
}

bool CGUIWindowPVRTimers::OnMessage(CGUIMessage &message)
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
          bReturn = true;
          switch (message.GetParam1())
          {
            case ACTION_SHOW_INFO:
            case ACTION_SELECT_ITEM:
            case ACTION_MOUSE_LEFT_CLICK:
            {
              CFileItemPtr item(m_vecItems->Get(iItem));
              if (item->m_bIsFolder && (message.GetParam1() != ACTION_SHOW_INFO))
              {
                m_currentFileItem = item;
                bReturn = false; // folders are handled by base class
              }
              else
              {
                m_currentFileItem.reset();
                ActionShowTimer(item.get());
              }
              break;
            }
            case ACTION_CONTEXT_MENU:
            case ACTION_MOUSE_RIGHT_CLICK:
              OnPopupMenu(iItem);
              break;
            case ACTION_DELETE_ITEM:
              ActionDeleteTimer(m_vecItems->Get(iItem).get());
              break;
            default:
              bReturn = false;
              break;
          }
        }
      }
      else if (message.GetSenderId() == CONTROL_BTNTIMERTYPEFILTER)
      {
        CSettings::Get().ToggleBool("pvrtimers.timertypefilter");
        CSettings::Get().Save();
        Update(GetDirectoryPath());
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
        case ObservableMessageTimersReset:
        {
          if (IsActive())
            Refresh(true);
          bReturn = true;
          break;
        }
      }
  }

  return bReturn || CGUIWindowPVRBase::OnMessage(message);
}

bool CGUIWindowPVRTimers::OnContextButtonActivate(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_ACTIVATE)
  {
    bReturn = true;
    if (!item->HasPVRTimerInfoTag())
      return bReturn;

    CPVRTimerInfoTagPtr timer = item->GetPVRTimerInfoTag();
    if (timer->m_state == PVR_TIMER_STATE_DISABLED)
      timer->m_state = PVR_TIMER_STATE_SCHEDULED;
    else
      timer->m_state = PVR_TIMER_STATE_DISABLED;

    g_PVRTimers->UpdateTimer(*item);
  }

  return bReturn;
}

bool CGUIWindowPVRTimers::OnContextButtonAdd(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_ADD)
    bReturn = ShowNewTimerDialog();

  return bReturn;
}

bool CGUIWindowPVRTimers::OnContextButtonDelete(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_DELETE)
  {
    bReturn = true;

    if (!item->HasPVRTimerInfoTag())
      return bReturn;

    bool bDeleteSchedule(false);
    if (ConfirmDeleteTimer(item, bDeleteSchedule))
      g_PVRTimers->DeleteTimer(*item, false, bDeleteSchedule);
  }

  return bReturn;
}

bool CGUIWindowPVRTimers::OnContextButtonEdit(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_EDIT)
  {
    bReturn = true;
    if (!item->HasPVRTimerInfoTag())
      return bReturn;

    if (ShowTimerSettings(item))
      g_PVRTimers->UpdateTimer(*item);
  }

  return bReturn;
}

bool CGUIWindowPVRTimers::OnContextButtonRename(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_RENAME)
  {
    bReturn = true;
    if (!item->HasPVRTimerInfoTag())
      return bReturn;
    CPVRTimerInfoTagPtr timer = item->GetPVRTimerInfoTag();

    std::string strNewName(timer->m_strTitle);
    if (CGUIKeyboardFactory::ShowAndGetInput(strNewName, g_localizeStrings.Get(19042), false))
      g_PVRTimers->RenameTimer(*item, strNewName);
  }

  return bReturn;
}

bool CGUIWindowPVRTimers::OnContextButtonInfo(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_INFO)
  {
    ShowEPGInfo(item);
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRTimers::ActionDeleteTimer(CFileItem *item)
{
  /* check if the timer tag is valid */
  CPVRTimerInfoTagPtr timerTag = item->GetPVRTimerInfoTag();
  if (!timerTag || timerTag->m_iClientIndex < 0)
    return false;

  bool bDeleteSchedule(false);
  if (!ConfirmDeleteTimer(item, bDeleteSchedule))
    return false;

  /* delete the timer */
  bool bReturn = g_PVRTimers->DeleteTimer(*item, false, bDeleteSchedule);

  if (bReturn && (m_vecItems->GetObjectCount() == 0))
  {
    /* go to the parent folder if we're in a subdirectory and just deleted the last item */
    CPVRTimersPath path(m_vecItems->GetPath());
    if (path.IsValid() && path.IsTimerSchedule())
    {
      m_currentFileItem.reset();
      GoParentFolder();
    }
  }
  return bReturn;
}

bool CGUIWindowPVRTimers::ActionShowTimer(CFileItem *item)
{
  bool bReturn = false;

  /* Check if "Add timer..." entry is pressed by OK, if yes
     create a new timer and open settings dialog, otherwise
     open settings for selected timer entry */
  if (URIUtils::PathEquals(item->GetPath(), CPVRTimersPath::PATH_ADDTIMER))
  {
    bReturn = ShowNewTimerDialog();
  }
  else
  {
    if (ShowTimerSettings(item))
    {
      /* Update timer on pvr backend */
      bReturn = g_PVRTimers->UpdateTimer(*item);
    }
  }

  return bReturn;
}

bool CGUIWindowPVRTimers::ShowNewTimerDialog(void)
{
  bool bReturn(false);

  CPVRTimerInfoTagPtr newTimer(new CPVRTimerInfoTag(m_bRadio));
  CFileItem *newItem = new CFileItem(newTimer);

  if (ShowTimerSettings(newItem))
  {
    /* Add timer to backend */
    bReturn = g_PVRTimers->AddTimer(newItem->GetPVRTimerInfoTag());
  }

  delete newItem;

  return bReturn;
}

bool CGUIWindowPVRTimers::ShowTimerSettings(CFileItem *item)
{
  /* Check item is TV timer information tag */
  if (!item->IsPVRTimer())
  {
    CLog::Log(LOGERROR, "CGUIWindowPVRTimers: Can't open timer settings dialog, no timer info tag!");
    return false;
  }

  /* Load timer settings dialog */
  CGUIDialogPVRTimerSettings* pDlgInfo = (CGUIDialogPVRTimerSettings*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_TIMER_SETTING);

  if (!pDlgInfo)
    return false;

  /* inform dialog about the file item */
  pDlgInfo->SetTimer(item);

  /* Open dialog window */
  pDlgInfo->DoModal();

  /* Get modify flag from window and return it to caller */
  return pDlgInfo->IsConfirmed();
}
