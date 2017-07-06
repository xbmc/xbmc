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

#include "ServiceBroker.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogBusy.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/Key.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

#include "pvr/PVRGUIActions.h"
#include "pvr/PVRItem.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/dialogs/GUIDialogPVRGuideSearch.h"
#include "pvr/epg/EpgContainer.h"

using namespace PVR;

namespace
{
  class AsyncSearchAction : private IRunnable
  {
  public:
    AsyncSearchAction() = delete;
    AsyncSearchAction(CFileItemList* items, CPVREpgSearchFilter* filter) : m_items(items), m_filter(filter) {}
    bool Execute();

  private:
    // IRunnable implementation
    void Run() override;

    CFileItemList* m_items;
    CPVREpgSearchFilter* m_filter;
  };

  bool AsyncSearchAction::Execute()
  {
    CGUIDialogBusy::Wait(this, 100, false);
    return true;
  }

  void AsyncSearchAction::Run()
  {
    CServiceBroker::GetPVRManager().EpgContainer().GetEPGSearch(*m_items, *m_filter);
  }
} // unnamed namespace

CGUIWindowPVRSearchBase::CGUIWindowPVRSearchBase(bool bRadio, int id, const std::string &xmlFile) :
  CGUIWindowPVRBase(bRadio, id, xmlFile),
  m_bSearchConfirmed(false)
{
}

void CGUIWindowPVRSearchBase::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return;

  buttons.Add(CONTEXT_BUTTON_CLEAR, 19232); /* Clear search results */

  CGUIWindowPVRBase::GetContextButtons(itemNumber, buttons);
}

bool CGUIWindowPVRSearchBase::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return false;
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);

  return OnContextButtonClear(pItem.get(), button) ||
      CGUIMediaWindow::OnContextButton(itemNumber, button);
}

void CGUIWindowPVRSearchBase::SetItemToSearch(const CFileItemPtr &item)
{
  m_searchfilter.Reset();

  if (item->IsUsablePVRRecording())
  {
    m_searchfilter.SetSearchPhrase(item->GetPVRRecordingInfoTag()->m_strTitle);
  }
  else
  {
    const CPVREpgInfoTagPtr epgTag(CPVRItem(item).GetEpgInfoTag());
    if (epgTag)
      m_searchfilter.SetSearchPhrase(epgTag->Title());
  }

  m_bSearchConfirmed = true;

  if (IsActive())
    Refresh(true);
}

void CGUIWindowPVRSearchBase::OnPrepareFileItems(CFileItemList &items)
{
  bool bAddSpecialSearchItem = items.IsEmpty();

  if (m_bSearchConfirmed)
  {
    bAddSpecialSearchItem = true;

    items.Clear();
    AsyncSearchAction(&items, &m_searchfilter).Execute();

    if (items.IsEmpty())
      CGUIDialogOK::ShowAndGetInput(CVariant{284}, // "No results found"
                                    m_searchfilter.GetSearchTerm());
  }

  if (bAddSpecialSearchItem)
  {
    CFileItemPtr item(new CFileItem("pvr://guide/searchresults/search/", true));
    item->SetLabel(g_localizeStrings.Get(19140)); // "Search..."
    item->SetLabelPreformatted(true);
    item->SetSpecialSort(SortSpecialOnTop);
    items.Add(item);
  }
}

bool CGUIWindowPVRSearchBase::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    if (message.GetSenderId() == m_viewControl.GetCurrentControl())
    {
      int iItem = m_viewControl.GetSelectedItem();
      if (iItem >= 0 && iItem < m_vecItems->Size())
      {
        CFileItemPtr pItem = m_vecItems->Get(iItem);

        /* process actions */
        switch (message.GetParam1())
        {
          case ACTION_SHOW_INFO:
          case ACTION_SELECT_ITEM:
          case ACTION_MOUSE_LEFT_CLICK:
          {
            if (URIUtils::PathEquals(pItem->GetPath(), "pvr://guide/searchresults/search/"))
              OpenDialogSearch();
            else
               CServiceBroker::GetPVRManager().GUIActions()->ShowEPGInfo(pItem);
            return true;
          }

          case ACTION_CONTEXT_MENU:
          case ACTION_MOUSE_RIGHT_CLICK:
            OnPopupMenu(iItem);
            return true;

          case ACTION_RECORD:
            CServiceBroker::GetPVRManager().GUIActions()->ToggleTimer(pItem);
            return true;
        }
      }
    }
  }

  return CGUIWindowPVRBase::OnMessage(message);
}

bool CGUIWindowPVRSearchBase::OnContextButtonClear(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_CLEAR)
  {
    bReturn = true;

    m_bSearchConfirmed = false;
    m_searchfilter.Reset();

    Refresh(true);
  }

  return bReturn;
}

void CGUIWindowPVRSearchBase::OpenDialogSearch()
{
  CGUIDialogPVRGuideSearch* dlgSearch = g_windowManager.GetWindow<CGUIDialogPVRGuideSearch>(WINDOW_DIALOG_PVR_GUIDE_SEARCH);

  if (!dlgSearch)
    return;

  dlgSearch->SetFilterData(&m_searchfilter);

  /* Set channel type filter */
  m_searchfilter.SetIsRadio(m_bRadio);

  /* Open dialog window */
  dlgSearch->Open();

  if (dlgSearch->IsConfirmed())
  {
    m_bSearchConfirmed = true;
    Refresh(true);
  }
}
