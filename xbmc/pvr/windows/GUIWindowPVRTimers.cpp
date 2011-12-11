/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "GUIWindowPVRTimers.h"

#include "dialogs/GUIDialogKeyboard.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIWindowManager.h"
#include "pvr/PVRManager.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/addons/PVRClients.h"
#include "GUIWindowPVR.h"
#include "threads/SingleLock.h"

using namespace PVR;

CGUIWindowPVRTimers::CGUIWindowPVRTimers(CGUIWindowPVR *parent) :
  CGUIWindowPVRCommon(parent, PVR_WINDOW_TIMERS, CONTROL_BTNTIMERS, CONTROL_LIST_TIMERS)
{
}

void CGUIWindowPVRTimers::UnregisterObservers(void)
{
  CSingleLock lock(m_critSection);
  if (g_PVRTimers)
    g_PVRTimers->UnregisterObserver(this);
}

void CGUIWindowPVRTimers::ResetObservers(void)
{
  CSingleLock lock(m_critSection);
  g_PVRTimers->RegisterObserver(this);
}

void CGUIWindowPVRTimers::GetContextButtons(int itemNumber, CContextButtons &buttons) const
{
  if (itemNumber < 0 || itemNumber >= m_parent->m_vecItems->Size())
    return;
  CFileItemPtr pItem = m_parent->m_vecItems->Get(itemNumber);

  /* Check for a empty file item list, means only a
     file item with the name "Add timer..." is present */
  if (pItem->GetPath() == "pvr://timers/add.timer")
  {
    buttons.Add(CONTEXT_BUTTON_ADD, 19056);             /* new timer */
    if (m_parent->m_vecItems->Size() > 1)
    {
      buttons.Add(CONTEXT_BUTTON_SORTBY_NAME, 103);     /* sort by name */
      buttons.Add(CONTEXT_BUTTON_SORTBY_DATE, 104);     /* sort by date */
    }
  }
  else
  {
    buttons.Add(CONTEXT_BUTTON_EDIT, 19057);            /* edit timer */
    buttons.Add(CONTEXT_BUTTON_ADD, 19056);             /* new timer */
    buttons.Add(CONTEXT_BUTTON_ACTIVATE, 19058);        /* activate/deactivate */
    buttons.Add(CONTEXT_BUTTON_RENAME, 118);            /* rename timer */
    buttons.Add(CONTEXT_BUTTON_DELETE, 117);            /* delete timer */
    buttons.Add(CONTEXT_BUTTON_SORTBY_NAME, 103);       /* sort by name */
    buttons.Add(CONTEXT_BUTTON_SORTBY_DATE, 104);       /* sort by date */
    if (g_PVRClients->HasMenuHooks(pItem->GetPVRTimerInfoTag()->m_iClientId))
      buttons.Add(CONTEXT_BUTTON_MENU_HOOKS, 19195);    /* PVR client specific action */
  }
}

bool CGUIWindowPVRTimers::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 || itemNumber >= m_parent->m_vecItems->Size())
    return false;
  CFileItemPtr pItem = m_parent->m_vecItems->Get(itemNumber);

  return OnContextButtonActivate(pItem.get(), button) ||
      OnContextButtonAdd(pItem.get(), button) ||
      OnContextButtonDelete(pItem.get(), button) ||
      OnContextButtonEdit(pItem.get(), button) ||
      OnContextButtonRename(pItem.get(), button) ||
      CGUIWindowPVRCommon::OnContextButton(itemNumber, button);
}

void CGUIWindowPVRTimers::UpdateData(void)
{
  CSingleLock lock(m_critSection);
  CLog::Log(LOGDEBUG, "CGUIWindowPVRTimers - %s - update window '%s'. set view to %d", __FUNCTION__, GetName(), m_iControlList);
  m_bUpdateRequired = false;

  g_PVRTimers->RegisterObserver(this);

  /* lock the graphics context while updating */
  CSingleLock graphicsLock(g_graphicsContext);

  m_iSelected = m_parent->m_viewControl.GetSelectedItem();
  m_parent->m_viewControl.Clear();
  m_parent->m_vecItems->Clear();
  m_parent->m_viewControl.SetCurrentView(m_iControlList);
  m_parent->m_vecItems->SetPath("pvr://timers/");
  m_parent->Update(m_parent->m_vecItems->GetPath());
  m_parent->m_vecItems->Sort(m_iSortMethod, m_iSortOrder);
  m_parent->m_viewControl.SetItems(*m_parent->m_vecItems);
  m_parent->m_viewControl.SetSelectedItem(m_iSelected);

  m_parent->SetLabel(CONTROL_LABELHEADER, g_localizeStrings.Get(19025));
  m_parent->SetLabel(CONTROL_LABELGROUP, "");
}

bool CGUIWindowPVRTimers::OnClickButton(CGUIMessage &message)
{
  bool bReturn = false;

  if (IsSelectedButton(message))
  {
    bReturn = true;
    g_PVRManager.TriggerTimersUpdate();
  }

  return bReturn;
}

bool CGUIWindowPVRTimers::OnClickList(CGUIMessage &message)
{
  bool bReturn = false;

  if (IsSelectedList(message))
  {
    bReturn = true;
    int iAction = message.GetParam1();
    int iItem = m_parent->m_viewControl.GetSelectedItem();

    /* get the fileitem pointer */
    if (iItem < 0 || iItem >= m_parent->m_vecItems->Size())
      return bReturn;
    CFileItemPtr pItem = m_parent->m_vecItems->Get(iItem);

    /* process actions */
    if (iAction == ACTION_SHOW_INFO || iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
      ActionShowTimer(pItem.get());
    else if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
      m_parent->OnPopupMenu(iItem);
    else if (iAction == ACTION_DELETE_ITEM)
      ActionDeleteTimer(pItem.get());
  }

  return bReturn;
}

bool CGUIWindowPVRTimers::OnContextButtonActivate(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_ACTIVATE)
  {
    bReturn = true;
    if (!item->HasPVRTimerInfoTag())
      return bReturn;

    CPVRTimerInfoTag *timer = item->GetPVRTimerInfoTag();
    int iLabelId;
    if (timer->IsActive())
    {
      timer->m_state = PVR_TIMER_STATE_CANCELLED;
      iLabelId = 13106;
    }
    else
    {
      timer->m_state = PVR_TIMER_STATE_SCHEDULED;
      iLabelId = 305;
    }

    CGUIDialogOK::ShowAndGetInput(19033, 19040, 0, iLabelId);
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

    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialog)
      return bReturn;
    pDialog->SetHeading(122);
    pDialog->SetLine(0, 19040);
    pDialog->SetLine(1, "");
    pDialog->SetLine(2, item->GetPVRTimerInfoTag()->m_strTitle);
    pDialog->DoModal();

    if (!pDialog->IsConfirmed())
      return bReturn;

    g_PVRTimers->DeleteTimer(*item);
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
    CPVRTimerInfoTag *timer = item->GetPVRTimerInfoTag();

    CStdString strNewName(timer->m_strTitle);
    if (CGUIDialogKeyboard::ShowAndGetInput(strNewName, g_localizeStrings.Get(19042), false))
      g_PVRTimers->RenameTimer(*item, strNewName);
  }

  return bReturn;
}

void CGUIWindowPVRTimers::Notify(const Observable &obs, const CStdString& msg)
{
  if (msg.Equals("timers"))
  {
    if (IsVisible())
      SetInvalid();
    else
      m_bUpdateRequired = true;
  }
  else if (msg.Equals("timers-reset"))
  {
    if (IsVisible())
      UpdateData();
    else
      m_bUpdateRequired = true;
  }
}
