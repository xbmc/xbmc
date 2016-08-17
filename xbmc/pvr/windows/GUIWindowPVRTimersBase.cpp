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

#include "ContextMenuManager.h"
#include "GUIInfoManager.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

#include "pvr/PVRManager.h"
#include "pvr/dialogs/GUIDialogPVRTimerSettings.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/addons/PVRClients.h"

#include "GUIWindowPVRTimers.h"

using namespace PVR;

CGUIWindowPVRTimersBase::CGUIWindowPVRTimersBase(bool bRadio, int id, const std::string &xmlFile) :
  CGUIWindowPVRBase(bRadio, id, xmlFile)
{
}

void CGUIWindowPVRTimersBase::RegisterObservers(void)
{
  CSingleLock lock(m_critSection);
  g_PVRManager.RegisterObserver(this);
  g_infoManager.RegisterObserver(this);
  CGUIWindowPVRBase::RegisterObservers();
}

void CGUIWindowPVRTimersBase::UnregisterObservers(void)
{
  CSingleLock lock(m_critSection);
  CGUIWindowPVRBase::UnregisterObservers();
  g_infoManager.UnregisterObserver(this);
  g_PVRManager.UnregisterObserver(this);
}

void CGUIWindowPVRTimersBase::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return;
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);

  if (!URIUtils::PathEquals(pItem->GetPath(), CPVRTimersPath::PATH_ADDTIMER))
  {
    CPVRTimerInfoTagPtr timer(pItem->GetPVRTimerInfoTag());
    if (timer)
    {
      if (timer->GetEpgInfoTag())
        buttons.Add(CONTEXT_BUTTON_INFO, 19047);          /* Programme information */

      CPVRTimerTypePtr timerType(timer->GetTimerType());
      if (timerType)
      {
        if (timerType->SupportsEnableDisable())
        {
          if (timer->m_state == PVR_TIMER_STATE_DISABLED)
            buttons.Add(CONTEXT_BUTTON_ACTIVATE, 843);    /* Activate */
          else
            buttons.Add(CONTEXT_BUTTON_ACTIVATE, 844);    /* Deactivate */
        }

        if (timer->GetTimerRuleId() != PVR_TIMER_NO_PARENT)
        {
          buttons.Add(CONTEXT_BUTTON_EDIT_TIMER_RULE, 19243); /* Edit timer rule */
          buttons.Add(CONTEXT_BUTTON_DELETE_TIMER_RULE, 19295); /* Delete timer rule */
        }

        if (timerType && !timerType->IsReadOnly() && timer->GetTimerRuleId() == PVR_TIMER_NO_PARENT)
          buttons.Add(CONTEXT_BUTTON_EDIT_TIMER, 21450);  /* Edit */
        else
          buttons.Add(CONTEXT_BUTTON_EDIT_TIMER, 19241);  /* View timer information */

        // As epg-based timers will get it's title from the epg tag, they should not be renamable.
        if (timer->IsManual() && !timerType->IsReadOnly())
          buttons.Add(CONTEXT_BUTTON_RENAME, 118);        /* Rename */

        if (timer->IsRecording())
          buttons.Add(CONTEXT_BUTTON_STOP_RECORD, 19059); /* Stop recording */
        else if (timerType && !timerType->IsReadOnly())
            buttons.Add(CONTEXT_BUTTON_DELETE, 117);      /* Delete */
      }

      if (g_PVRClients->HasMenuHooks(timer->m_iClientId, PVR_MENUHOOK_TIMER))
        buttons.Add(CONTEXT_BUTTON_MENU_HOOKS, 19195);    /* PVR client specific action */
    }
  }

  CGUIWindowPVRBase::GetContextButtons(itemNumber, buttons);
}

bool CGUIWindowPVRTimersBase::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_PARENT_DIR ||
      action.GetID() == ACTION_NAV_BACK)
  {
    CPVRTimersPath path(m_vecItems->GetPath());
    if (path.IsValid() && path.IsTimerRule())
    {
      m_currentFileItem.reset();
      GoParentFolder();
      return true;
    }
  }
  return CGUIWindowPVRBase::OnAction(action);
}

bool CGUIWindowPVRTimersBase::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return false;
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);

  return OnContextButtonActivate(pItem.get(), button) ||
      OnContextButtonAdd(pItem.get(), button) ||
      OnContextButtonDelete(pItem.get(), button) ||
      OnContextButtonStopRecord(pItem.get(), button) ||
      OnContextButtonEditTimer(pItem.get(), button) ||
      OnContextButtonEditTimerRule(pItem.get(), button) ||
      OnContextButtonDeleteTimerRule(pItem.get(), button) ||
      OnContextButtonRename(pItem.get(), button) ||
      OnContextButtonInfo(pItem.get(), button) ||
      CGUIWindowPVRBase::OnContextButton(itemNumber, button);
}

void CGUIWindowPVRTimersBase::UpdateButtons(void)
{
  SET_CONTROL_SELECTED(GetID(), CONTROL_BTNHIDEDISABLEDTIMERS, CSettings::GetInstance().GetBool(CSettings::SETTING_PVRTIMERS_HIDEDISABLEDTIMERS));

  CGUIWindowPVRBase::UpdateButtons();

  std::string strHeaderTitle;
  if (m_currentFileItem && m_currentFileItem->HasPVRTimerInfoTag())
  {
    CPVRTimerInfoTagPtr timer = m_currentFileItem->GetPVRTimerInfoTag();
    strHeaderTitle = timer->Title();
  }

  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER1, strHeaderTitle);
}

bool CGUIWindowPVRTimersBase::OnMessage(CGUIMessage &message)
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
      else if (message.GetSenderId() == CONTROL_BTNHIDEDISABLEDTIMERS)
      {
        CSettings::GetInstance().ToggleBool(CSettings::SETTING_PVRTIMERS_HIDEDISABLEDTIMERS);
        CSettings::GetInstance().Save();
        Update(GetDirectoryPath());
        bReturn = true;
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
        case ObservableMessageTimersReset:
        {
          Refresh(true);
          break;
        }
      }
  }

  return bReturn || CGUIWindowPVRBase::OnMessage(message);
}

bool CGUIWindowPVRTimersBase::OnContextButtonActivate(CFileItem *item, CONTEXT_BUTTON button)
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

    g_PVRTimers->UpdateTimer(timer);
  }

  return bReturn;
}

bool CGUIWindowPVRTimersBase::OnContextButtonAdd(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_ADD)
    bReturn = ShowNewTimerDialog();

  return bReturn;
}

bool CGUIWindowPVRTimersBase::OnContextButtonDelete(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_DELETE)
  {
    DeleteTimer(item);
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRTimersBase::OnContextButtonStopRecord(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_STOP_RECORD)
  {
    StopRecordFile(item);
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRTimersBase::OnContextButtonRename(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_RENAME)
  {
    bReturn = true;
    if (!item->HasPVRTimerInfoTag())
      return bReturn;
    CPVRTimerInfoTagPtr timer = item->GetPVRTimerInfoTag();

    std::string strNewName(timer->m_strTitle);
    if (CGUIKeyboardFactory::ShowAndGetInput(strNewName, CVariant{g_localizeStrings.Get(19042)}, false))
      g_PVRTimers->RenameTimer(*item, strNewName);
  }

  return bReturn;
}

bool CGUIWindowPVRTimersBase::OnContextButtonInfo(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_INFO)
  {
    ShowEPGInfo(item);
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRTimersBase::ActionDeleteTimer(CFileItem *item)
{
  bool bReturn = DeleteTimer(item);

  if (bReturn && (m_vecItems->GetObjectCount() == 0))
  {
    /* go to the parent folder if we're in a subdirectory and just deleted the last item */
    CPVRTimersPath path(m_vecItems->GetPath());
    if (path.IsValid() && path.IsTimerRule())
    {
      m_currentFileItem.reset();
      GoParentFolder();
    }
  }
  return bReturn;
}

bool CGUIWindowPVRTimersBase::ActionShowTimer(CFileItem *item)
{
  if (!g_PVRClients->SupportsTimers())
  {
    CGUIDialogOK::ShowAndGetInput(CVariant{19033}, CVariant{19215}); // "Information", "The PVR backend does not support timers."
    return false;
  }

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
    bReturn = EditTimer(item);
  }

  return bReturn;
}

bool CGUIWindowPVRTimersBase::ShowNewTimerDialog(void)
{
  bool bReturn(false);

  CPVRTimerInfoTagPtr newTimer(new CPVRTimerInfoTag(m_bRadio));

  if (ShowTimerSettings(newTimer))
  {
    /* Add timer to backend */
    bReturn = g_PVRTimers->AddTimer(newTimer);
  }

  return bReturn;
}
