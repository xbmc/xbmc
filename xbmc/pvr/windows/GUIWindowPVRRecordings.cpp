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

#include "GUIWindowPVRRecordings.h"

#include "dialogs/GUIDialogKeyboard.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRManager.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/windows/GUIWindowPVR.h"
#include "utils/log.h"
#include "threads/SingleLock.h"

using namespace PVR;

CGUIWindowPVRRecordings::CGUIWindowPVRRecordings(CGUIWindowPVR *parent) :
  CGUIWindowPVRCommon(parent, PVR_WINDOW_RECORDINGS, CONTROL_BTNRECORDINGS, CONTROL_LIST_RECORDINGS)
{
  m_strSelectedPath = "pvr://recordings/";
}

void CGUIWindowPVRRecordings::GetContextButtons(int itemNumber, CContextButtons &buttons) const
{
  if (itemNumber < 0 || itemNumber >= m_parent->m_vecItems->Size())
    return;
  CFileItemPtr pItem = m_parent->m_vecItems->Get(itemNumber);

  buttons.Add(CONTEXT_BUTTON_INFO, 19053);      /* Get Information of this recording */
  buttons.Add(CONTEXT_BUTTON_FIND, 19003);      /* Find similar program */
  buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 12021); /* Play this recording */
//buttons.Add(CONTEXT_BUTTON_RESUME_ITEM, 12022);
  buttons.Add(CONTEXT_BUTTON_RENAME, 118);      /* Rename this recording */
  buttons.Add(CONTEXT_BUTTON_DELETE, 117);      /* Delete this recording */

  // Update sort by button
//if (m_guiState->GetSortMethod()!=SORT_METHOD_NONE)
//{
//  CStdString sortLabel;
//  sortLabel.Format(g_localizeStrings.Get(550).c_str(), g_localizeStrings.Get(m_guiState->GetSortMethodLabel()).c_str());
//  buttons.Add(CONTEXT_BUTTON_SORTBY, sortLabel);   /* Sort method */
//
//  if (m_guiState->GetDisplaySortOrder()==SORT_ORDER_ASC)
//    buttons.Add(CONTEXT_BUTTON_SORTASC, 584);        /* Sort up or down */
//  else
//    buttons.Add(CONTEXT_BUTTON_SORTASC, 585);        /* Sort up or down */
//}
}

bool CGUIWindowPVRRecordings::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_PARENT_DIR)
  {
    if (m_parent->m_vecItems->m_strPath != "pvr://recordings/")
      m_parent->GoParentFolder();
    else
      g_windowManager.PreviousWindow();

    return true;
  }

  return CGUIWindowPVRCommon::OnAction(action);
}

bool CGUIWindowPVRRecordings::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 || itemNumber >= m_parent->m_vecItems->Size())
    return false;
  CFileItemPtr pItem = m_parent->m_vecItems->Get(itemNumber);

  return OnContextButtonPlay(pItem.get(), button) ||
      OnContextButtonRename(pItem.get(), button) ||
      OnContextButtonDelete(pItem.get(), button) ||
      OnContextButtonInfo(pItem.get(), button) ||
      CGUIWindowPVRCommon::OnContextButton(itemNumber, button);
}

void CGUIWindowPVRRecordings::OnWindowUnload(void)
{
  m_strSelectedPath = m_parent->m_vecItems->m_strPath;
  CGUIWindowPVRCommon::OnWindowUnload();
}

void CGUIWindowPVRRecordings::UpdateData(void)
{
  CSingleLock lock(m_critSection);
  if (m_bIsFocusing)
    return;

  CLog::Log(LOGDEBUG, "CGUIWindowPVRRecordings - %s - update window '%s'. set view to %d", __FUNCTION__, GetName(), m_iControlList);
  m_bIsFocusing = true;
  m_bUpdateRequired = false;

  /* lock the graphics context while updating */
  CSingleLock graphicsLock(g_graphicsContext);

  m_iSelected = m_parent->m_viewControl.GetSelectedItem();
  m_parent->m_viewControl.Clear();
  m_parent->m_vecItems->Clear();
  m_parent->m_viewControl.SetCurrentView(m_iControlList);
  m_parent->m_vecItems->m_strPath = "pvr://recordings/";
  m_parent->Update(m_strSelectedPath);
  m_parent->m_viewControl.SetItems(*m_parent->m_vecItems);
  m_parent->m_viewControl.SetSelectedItem(m_iSelected);

  m_parent->SetLabel(CONTROL_LABELHEADER, g_localizeStrings.Get(19017));
  m_parent->SetLabel(CONTROL_LABELGROUP, "");
  m_bIsFocusing = false;
}

bool CGUIWindowPVRRecordings::OnClickButton(CGUIMessage &message)
{
  bool bReturn = false;

  if (IsSelectedButton(message))
  {
    bReturn = true;
    g_PVRManager.TriggerRecordingsUpdate();
  }

  return bReturn;
}

bool CGUIWindowPVRRecordings::OnClickList(CGUIMessage &message)
{
  bool bReturn = false;

  if (IsSelectedList(message))
  {
    bReturn = true;
    int iAction = message.GetParam1();
    int iItem = m_parent->m_viewControl.GetSelectedItem();

    /* get the fileitem pointer */
    if (iItem < 0 || iItem >= (int) m_parent->m_vecItems->Size())
      return bReturn;
    CFileItemPtr pItem = m_parent->m_vecItems->Get(iItem);

    /* process actions */
    if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK || iAction == ACTION_PLAY)
      bReturn = PlayFile(pItem.get(), false);
    else if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
      m_parent->OnPopupMenu(iItem);
    else if (iAction == ACTION_SHOW_INFO)
      ShowRecordingInfo(pItem.get());
    else if (iAction == ACTION_DELETE_ITEM)
      bReturn = ActionDeleteRecording(pItem.get());
    else
      bReturn = false;
  }

  return bReturn;
}

bool CGUIWindowPVRRecordings::OnContextButtonDelete(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_DELETE)
  {
    bReturn = false;

    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialog)
      return bReturn;
    pDialog->SetHeading(122);
    pDialog->SetLine(0, 19043);
    pDialog->SetLine(1, "");
    pDialog->SetLine(2, item->GetPVRRecordingInfoTag()->m_strTitle);
    pDialog->DoModal();

    if (!pDialog->IsConfirmed())
      return bReturn;

    bReturn = g_PVRRecordings->DeleteRecording(*item);
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

  if (button == CONTEXT_BUTTON_PLAY_ITEM)
  {
    bReturn = true;
    PlayFile(item, false); /* play recording */
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
    if (CGUIDialogKeyboard::ShowAndGetInput(strNewName, g_localizeStrings.Get(19041), false))
    {
      if (g_PVRRecordings->RenameRecording(*item, strNewName))
        UpdateData();
    }
  }

  return bReturn;
}
