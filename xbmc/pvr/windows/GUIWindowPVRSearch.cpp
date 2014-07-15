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

#include "GUIWindowPVRSearch.h"

#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogProgress.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/Key.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/dialogs/GUIDialogPVRGuideSearch.h"
#include "epg/EpgContainer.h"
#include "pvr/recordings/PVRRecordings.h"
#include "utils/log.h"
#include "pvr/addons/PVRClients.h"

using namespace PVR;
using namespace EPG;

CGUIWindowPVRSearch::CGUIWindowPVRSearch(bool bRadio) :
  CGUIWindowPVRBase(bRadio, bRadio ? WINDOW_RADIO_SEARCH : WINDOW_TV_SEARCH, "MyPVRSearch.xml"),
  m_bSearchStarted(false),
  m_bSearchConfirmed(false)
{
}

void CGUIWindowPVRSearch::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return;
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);

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
    buttons.Add(CONTEXT_BUTTON_CLEAR, 19232);             /* Clear search results */
    if (pItem->GetEPGInfoTag()->HasPVRChannel() &&
        g_PVRClients->HasMenuHooks(pItem->GetEPGInfoTag()->ChannelTag()->ClientID(), PVR_MENUHOOK_EPG))
      buttons.Add(CONTEXT_BUTTON_MENU_HOOKS, 19195);      /* PVR client specific action */
  }
}

bool CGUIWindowPVRSearch::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return false;
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);

  return OnContextButtonClear(pItem.get(), button) ||
      OnContextButtonInfo(pItem.get(), button) ||
      OnContextButtonStopRecord(pItem.get(), button) ||
      OnContextButtonStartRecord(pItem.get(), button) ||
      CGUIWindowPVRBase::OnContextButton(itemNumber, button);
}

bool CGUIWindowPVRSearch::Update(const std::string &strDirectory, bool updateFilterPath /* = true */)
{
  CGUIWindowPVRBase::Update(strDirectory);

  /* perform search */
  Search();

  return true;
}

void CGUIWindowPVRSearch::Search(void)
{
  m_vecItems->Clear();

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
    g_EpgContainer.GetEPGSearch(*m_vecItems, m_searchfilter);
    if (dlgProgress)
      dlgProgress->Close();

    if (m_vecItems->IsEmpty())
    {
      CGUIDialogOK::ShowAndGetInput(194, 284, 0, 0);
      m_bSearchConfirmed = false;
    }
  }

  if (m_vecItems->IsEmpty())
  {
    CFileItemPtr item(new CFileItem("pvr://guide/searchresults/empty.epg", false));
    item->SetLabel(g_localizeStrings.Get(19027));
    item->SetLabelPreformated(true);
    m_vecItems->Add(item);
  }

  m_viewControl.SetItems(*m_vecItems);
}

bool CGUIWindowPVRSearch::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    if (message.GetSenderId() == m_viewControl.GetCurrentControl())
    {
      int iItem = m_viewControl.GetSelectedItem();
      if (iItem > 0 || iItem < m_vecItems->Size())
      {
        CFileItemPtr pItem = m_vecItems->Get(iItem);

        /* process actions */
        switch (message.GetParam1())
        {
          case ACTION_SHOW_INFO:
          case ACTION_SELECT_ITEM:
          case ACTION_MOUSE_LEFT_CLICK:
            ActionShowSearch(pItem.get());
            return true;

          case ACTION_CONTEXT_MENU:
          case ACTION_MOUSE_RIGHT_CLICK:
            OnPopupMenu(iItem);
            return true;

          case ACTION_RECORD:
            ActionRecord(pItem.get());
            return true;
        }
      }
    }
  }

  return CGUIWindowPVRBase::OnMessage(message);
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

    Refresh(true);
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

    StartRecordFile(*item);
  }

  return bReturn;
}

bool CGUIWindowPVRSearch::OnContextButtonStopRecord(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_STOP_RECORD)
  {
    bReturn = true;

    StopRecordFile(*item);
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
  /* Load search dialog */
  CGUIDialogPVRGuideSearch* pDlgInfo = (CGUIDialogPVRGuideSearch*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_GUIDE_SEARCH);

  if (!pDlgInfo)
    return;

  if (!m_bSearchStarted)
  {
    m_bSearchStarted = true;
    m_searchfilter.Reset();
  }

  pDlgInfo->SetFilterData(&m_searchfilter);

  /* Set channel type filter */
  m_searchfilter.m_bIsRadio = m_bRadio;

  /* Open dialog window */
  pDlgInfo->DoModal();

  if (pDlgInfo->IsConfirmed())
  {
    m_bSearchConfirmed = true;
    Refresh(true);
  }
}
