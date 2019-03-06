/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowPVRSearch.h"

#include "ServiceBroker.h"
#include "dialogs/GUIDialogBusy.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/Key.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "threads/IRunnable.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

#include "pvr/PVRGUIActions.h"
#include "pvr/PVRItem.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/dialogs/GUIDialogPVRGuideSearch.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/epg/EpgSearchFilter.h"

using namespace PVR;
using namespace KODI::MESSAGING;

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
    std::vector<std::shared_ptr<CPVREpgInfoTag>> results = CServiceBroker::GetPVRManager().EpgContainer().GetAllTags();
    for (auto it = results.begin(); it != results.end();)
    {
      it = results.erase(std::remove_if(results.begin(),
                                        results.end(),
                                        [this](const std::shared_ptr<CPVREpgInfoTag>& entry)
                                        {
                                          return !m_filter->FilterEntry(entry);
                                        }),
                         results.end());
    }

    if (m_filter->ShouldRemoveDuplicates())
      m_filter->RemoveDuplicates(results);

    for (const auto& tag : results)
    {
      m_items->Add(std::make_shared<CFileItem>(tag));
    }
  }
} // unnamed namespace

CGUIWindowPVRSearchBase::CGUIWindowPVRSearchBase(bool bRadio, int id, const std::string &xmlFile) :
  CGUIWindowPVRBase(bRadio, id, xmlFile),
  m_bSearchConfirmed(false)
{
}

CGUIWindowPVRSearchBase::~CGUIWindowPVRSearchBase()
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
  m_searchfilter.reset(new CPVREpgSearchFilter(m_bRadio));

  if (item->IsUsablePVRRecording())
  {
    m_searchfilter->SetSearchPhrase(item->GetPVRRecordingInfoTag()->m_strTitle);
  }
  else
  {
    const CPVREpgInfoTagPtr epgTag(CPVRItem(item).GetEpgInfoTag());
    if (epgTag && !CServiceBroker::GetPVRManager().IsParentalLocked(epgTag))
      m_searchfilter->SetSearchPhrase(epgTag->Title());
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
    AsyncSearchAction(&items, m_searchfilter.get()).Execute();

    if (items.IsEmpty())
      HELPERS::ShowOKDialogText(CVariant{284}, // "No results found"
                                m_searchfilter->GetSearchTerm());
  }

  if (bAddSpecialSearchItem)
  {
    CFileItemPtr item(new CFileItem("pvr://guide/searchresults/search/", false));
    item->SetLabel(g_localizeStrings.Get(19140)); // "Search..."
    item->SetLabelPreformatted(true);
    item->SetSpecialSort(SortSpecialOnTop);
    item->SetIconImage("DefaultTVShows.png");
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
    m_searchfilter.reset();

    Refresh(true);
  }

  return bReturn;
}

void CGUIWindowPVRSearchBase::OpenDialogSearch()
{
  CGUIDialogPVRGuideSearch* dlgSearch = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogPVRGuideSearch>(WINDOW_DIALOG_PVR_GUIDE_SEARCH);

  if (!dlgSearch)
    return;

  if (!m_searchfilter)
    m_searchfilter.reset(new CPVREpgSearchFilter(m_bRadio));

  dlgSearch->SetFilterData(m_searchfilter.get());

  /* Open dialog window */
  dlgSearch->Open();

  if (dlgSearch->IsConfirmed())
  {
    m_bSearchConfirmed = true;
    Refresh(true);
  }
}
