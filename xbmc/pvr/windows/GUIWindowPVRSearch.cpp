/*
 *      Copyright (C) 2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIWindowPVRSearch.h"

#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogProgress.h"
#include "guilib/GUIWindowManager.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/dialogs/GUIDialogPVRGuideSearch.h"
#include "epg/EpgContainer.h"
#include "pvr/recordings/PVRRecordings.h"
#include "GUIWindowPVR.h"
#include "utils/log.h"
#include "pvr/addons/PVRClients.h"

using namespace PVR;
using namespace EPG;

CGUIWindowPVRSearch::CGUIWindowPVRSearch(CGUIWindowPVR *parent) :
  CGUIWindowPVRCommon(parent, PVR_WINDOW_SEARCH, CONTROL_BTNSEARCH, CONTROL_LIST_SEARCH),
  m_bSearchStarted(false),
  m_bSearchConfirmed(false)
{
}

void CGUIWindowPVRSearch::GetContextButtons(int itemNumber, CContextButtons &buttons) const
{
  if (itemNumber < 0 || itemNumber >= m_parent->m_vecItems->Size())
    return;
  CFileItemPtr pItem = m_parent->m_vecItems->Get(itemNumber);

  if (pItem->GetLabel() != g_localizeStrings.Get(19027))
  {
    if (pItem->GetEPGInfoTag()->EndAsLocalTime() > CDateTime::GetCurrentDateTime())
    {
      if (!pItem->GetEPGInfoTag()->HasTimer())
      {
        if (pItem->GetEPGInfoTag()->StartAsLocalTime() < CDateTime::GetCurrentDateTime())
          buttons.Add(CONTEXT_BUTTON_START_RECORD, 264);   /* RECORD programme */
        else
          buttons.Add(CONTEXT_BUTTON_START_RECORD, 19061); /* Create a Timer */
      }
      else
      {
        if (pItem->GetEPGInfoTag()->StartAsLocalTime() < CDateTime::GetCurrentDateTime())
          buttons.Add(CONTEXT_BUTTON_STOP_RECORD, 19059); /* Stop recording */
        else
          buttons.Add(CONTEXT_BUTTON_STOP_RECORD, 19060); /* Delete Timer */
      }
    }

    buttons.Add(CONTEXT_BUTTON_INFO, 19047);              /* Epg info button */
    buttons.Add(CONTEXT_BUTTON_SORTBY_CHANNEL, 19062);    /* Sort by channel */
    buttons.Add(CONTEXT_BUTTON_SORTBY_NAME, 103);         /* Sort by Name */
    buttons.Add(CONTEXT_BUTTON_SORTBY_DATE, 104);         /* Sort by Date */
    buttons.Add(CONTEXT_BUTTON_CLEAR, 19232);             /* Clear search results */
    if (pItem->GetEPGInfoTag()->HasPVRChannel() &&
        g_PVRClients->HasMenuHooks(pItem->GetEPGInfoTag()->ChannelTag()->ClientID()))
      buttons.Add(CONTEXT_BUTTON_MENU_HOOKS, 19195);      /* PVR client specific action */
  }
}

bool CGUIWindowPVRSearch::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 || itemNumber >= m_parent->m_vecItems->Size())
    return false;
  CFileItemPtr pItem = m_parent->m_vecItems->Get(itemNumber);

  return OnContextButtonClear(pItem.get(), button) ||
      OnContextButtonInfo(pItem.get(), button) ||
      OnContextButtonStopRecord(pItem.get(), button) ||
      OnContextButtonStartRecord(pItem.get(), button) ||
      CGUIWindowPVRCommon::OnContextButton(itemNumber, button);
}

void CGUIWindowPVRSearch::UpdateData(bool bUpdateSelectedFile /* = true */)
{
  CLog::Log(LOGDEBUG, "CGUIWindowPVRSearch - %s - update window '%s'. set view to %d", __FUNCTION__, GetName(), m_iControlList);
  m_bUpdateRequired = false;

  /* lock the graphics context while updating */
  CSingleLock graphicsLock(g_graphicsContext);

  m_iSelected = m_parent->m_viewControl.GetSelectedItem();
  m_parent->m_viewControl.Clear();
  m_parent->m_vecItems->Clear();
  m_parent->m_viewControl.SetCurrentView(m_iControlList);

  if (m_bSearchConfirmed)
  {
    CGUIDialogProgress* dlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (dlgProgress)
    {
      dlgProgress->SetHeading(194);
      dlgProgress->SetLine(0, m_searchfilter.m_strSearchTerm);
      dlgProgress->SetLine(1, "");
      dlgProgress->SetLine(2, "");
      dlgProgress->StartModal();
      dlgProgress->Progress();
    }

    // TODO get this from the selected channel group
    g_EpgContainer.GetEPGSearch(*m_parent->m_vecItems, m_searchfilter);
    if (dlgProgress)
      dlgProgress->Close();

    if (m_parent->m_vecItems->Size() == 0)
    {
      CGUIDialogOK::ShowAndGetInput(194, 284, 0, 0);
      m_bSearchConfirmed = false;
    }
  }

  if (m_parent->m_vecItems->Size() == 0)
  {
    CFileItemPtr item;
    item.reset(new CFileItem("pvr://guide/searchresults/empty.epg", false));
    item->SetLabel(g_localizeStrings.Get(19027));
    item->SetLabelPreformated(true);
    m_parent->m_vecItems->Add(item);
  }
  else
  {
    m_parent->m_vecItems->Sort(m_iSortMethod, m_iSortOrder);
  }

  m_parent->m_viewControl.SetItems(*m_parent->m_vecItems);

  if (bUpdateSelectedFile)
    m_parent->m_viewControl.SetSelectedItem(m_iSelected);

  m_parent->SetLabel(CONTROL_LABELHEADER, g_localizeStrings.Get(283));
  m_parent->SetLabel(CONTROL_LABELGROUP, "");
}

bool CGUIWindowPVRSearch::OnClickButton(CGUIMessage &message)
{
  bool bReturn = false;

  if (IsSelectedButton(message))
  {
    bReturn = true;
    ShowSearchResults();
  }

  return bReturn;
}

bool CGUIWindowPVRSearch::OnClickList(CGUIMessage &message)
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
      ActionShowSearch(pItem.get());
    else if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
      m_parent->OnPopupMenu(iItem);
    else if (iAction == ACTION_RECORD)
      ActionRecord(pItem.get());
  }

  return bReturn;
}

bool CGUIWindowPVRSearch::OnContextButtonClear(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_CLEAR)
  {
    bReturn = true;

    m_bSearchStarted = false;
    m_bSearchConfirmed = false;
    m_searchfilter.Reset();

    UpdateData();
  }

  return bReturn;
}

bool CGUIWindowPVRSearch::OnContextButtonInfo(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_INFO)
  {
    bReturn = true;

    ShowEPGInfo(item);
  }

  return bReturn;
}

bool CGUIWindowPVRSearch::OnContextButtonStartRecord(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_START_RECORD)
  {
    bReturn = true;

    StartRecordFile(item);
  }

  return bReturn;
}

bool CGUIWindowPVRSearch::OnContextButtonStopRecord(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_STOP_RECORD)
  {
    bReturn = true;

    StopRecordFile(item);
  }

  return bReturn;
}

bool CGUIWindowPVRSearch::ActionShowSearch(CFileItem *item)
{
  if (item->GetPath() == "pvr://guide/searchresults/empty.epg")
    ShowSearchResults();
  else
    ShowEPGInfo(item);

  return true;
}

void CGUIWindowPVRSearch::ShowSearchResults()
{
  /* Load timer settings dialog */
  CGUIDialogPVRGuideSearch* pDlgInfo = (CGUIDialogPVRGuideSearch*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_GUIDE_SEARCH);

  if (!pDlgInfo)
    return;

  if (!m_bSearchStarted)
  {
    m_bSearchStarted = true;
    m_searchfilter.Reset();
  }

  pDlgInfo->SetFilterData(&m_searchfilter);

  /* Open dialog window */
  pDlgInfo->DoModal();

  if (pDlgInfo->IsConfirmed())
  {
    m_bSearchConfirmed = true;
    UpdateData();
  }
}
