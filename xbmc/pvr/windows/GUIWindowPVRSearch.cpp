/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowPVRSearch.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "pvr/PVRItem.h"
#include "pvr/PVRManager.h"
#include "pvr/dialogs/GUIDialogPVRGuideSearch.h"
#include "pvr/epg/Epg.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/epg/EpgSearchFilter.h"
#include "pvr/epg/EpgSearchPath.h"
#include "pvr/guilib/PVRGUIActionsEPG.h"
#include "pvr/guilib/PVRGUIActionsTimers.h"
#include "pvr/recordings/PVRRecording.h"
#include "threads/IRunnable.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

#include <algorithm>
#include <memory>
#include <vector>

using namespace PVR;
using namespace KODI::MESSAGING;

namespace
{
class AsyncSearchAction : private IRunnable
{
public:
  AsyncSearchAction() = delete;
  AsyncSearchAction(CFileItemList* items, CPVREpgSearchFilter* filter)
    : m_items(items), m_filter(filter)
  {
  }
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
  std::vector<std::shared_ptr<CPVREpgInfoTag>> results =
      CServiceBroker::GetPVRManager().EpgContainer().GetTags(m_filter->GetEpgSearchData());
  m_filter->SetEpgSearchDataFiltered();

  // Tags can still contain false positives, for search criteria that cannot be handled via
  // database. So, run extended search filters on what we got from the database.
  for (auto it = results.begin(); it != results.end();)
  {
    it = results.erase(std::remove_if(results.begin(), results.end(),
                                      [this](const std::shared_ptr<CPVREpgInfoTag>& entry) {
                                        return !m_filter->FilterEntry(entry);
                                      }),
                       results.end());
  }

  if (m_filter->ShouldRemoveDuplicates())
    m_filter->RemoveDuplicates(results);

  m_filter->SetLastExecutedDateTime(CDateTime::GetUTCDateTime());

  for (const auto& tag : results)
  {
    m_items->Add(std::make_shared<CFileItem>(tag));
  }
}
} // unnamed namespace

CGUIWindowPVRSearchBase::CGUIWindowPVRSearchBase(bool bRadio, int id, const std::string& xmlFile)
  : CGUIWindowPVRBase(bRadio, id, xmlFile)
{
}

CGUIWindowPVRSearchBase::~CGUIWindowPVRSearchBase()
{
}

void CGUIWindowPVRSearchBase::GetContextButtons(int itemNumber, CContextButtons& buttons)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return;

  const CPVREpgSearchPath path(m_vecItems->GetPath());
  const bool bIsSavedSearchesRoot = (path.IsValid() && path.IsSavedSearchesRoot());
  if (!bIsSavedSearchesRoot)
    buttons.Add(CONTEXT_BUTTON_CLEAR, 19232); // "Clear search results"

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

void CGUIWindowPVRSearchBase::SetItemToSearch(const CFileItem& item)
{
  if (item.HasEPGSearchFilter())
  {
    SetSearchFilter(item.GetEPGSearchFilter());
  }
  else if (item.IsUsablePVRRecording())
  {
    SetSearchFilter(std::make_shared<CPVREpgSearchFilter>(m_bRadio));
    m_searchfilter->SetSearchPhrase(item.GetPVRRecordingInfoTag()->m_strTitle);
  }
  else
  {
    SetSearchFilter(std::make_shared<CPVREpgSearchFilter>(m_bRadio));

    const std::shared_ptr<const CPVREpgInfoTag> epgTag(CPVRItem(item).GetEpgInfoTag());
    if (epgTag && !CServiceBroker::GetPVRManager().IsParentalLocked(epgTag))
      m_searchfilter->SetSearchPhrase(epgTag->Title());
  }

  ExecuteSearch();
}

void CGUIWindowPVRSearchBase::OnPrepareFileItems(CFileItemList& items)
{
  if (m_bSearchConfirmed)
  {
    items.Clear();
  }

  if (items.IsEmpty())
  {
    auto item = std::make_shared<CFileItem>(CPVREpgSearchPath::PATH_SEARCH_DIALOG, false);
    item->SetLabel(g_localizeStrings.Get(
        m_searchfilter == nullptr ? 19335 : 19336)); // "New search..." / "Edit search..."
    item->SetLabelPreformatted(true);
    item->SetSpecialSort(SortSpecialOnTop);
    item->SetArt("icon", "DefaultPVRSearch.png");
    items.Add(item);

    item = std::make_shared<CFileItem>(m_bRadio ? CPVREpgSearchPath::PATH_RADIO_SAVEDSEARCHES
                                                : CPVREpgSearchPath::PATH_TV_SAVEDSEARCHES,
                                       true);
    item->SetLabel(g_localizeStrings.Get(19337)); // "Saved searches"
    item->SetLabelPreformatted(true);
    item->SetSpecialSort(SortSpecialOnTop);
    item->SetArt("icon", "DefaultFolder.png");
    items.Add(item);
  }

  if (m_bSearchConfirmed)
  {
    const int itemCount = items.GetObjectCount();

    AsyncSearchAction(&items, m_searchfilter.get()).Execute();

    if (items.GetObjectCount() == itemCount)
    {
      HELPERS::ShowOKDialogText(CVariant{284}, // "No results found"
                                m_searchfilter->GetSearchTerm());
    }
  }
}

bool CGUIWindowPVRSearchBase::OnAction(const CAction& action)
{
  if (action.GetID() == ACTION_PARENT_DIR || action.GetID() == ACTION_NAV_BACK)
  {
    const CPVREpgSearchPath path(m_vecItems->GetPath());
    if (path.IsValid() && path.IsSavedSearchesRoot())
    {
      // Go to root dir and show previous search results if any
      m_bSearchConfirmed = (m_searchfilter != nullptr);
      GoParentFolder();
      return true;
    }
  }
  return CGUIWindowPVRBase::OnAction(action);
}

bool CGUIWindowPVRSearchBase::OnMessage(CGUIMessage& message)
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
            const CPVREpgSearchPath path(pItem->GetPath());
            const bool bIsSavedSearch = (path.IsValid() && path.IsSavedSearch());
            const bool bIsSavedSearchesRoot = (path.IsValid() && path.IsSavedSearchesRoot());

            if (message.GetParam1() != ACTION_SHOW_INFO)
            {
              if (pItem->IsParentFolder())
              {
                // Go to root dir and show previous search results if any
                m_bSearchConfirmed = (m_searchfilter != nullptr);
                break; // handled by base class
              }

              if (bIsSavedSearchesRoot)
              {
                // List saved searches
                m_bSearchConfirmed = false;
                break; // handled by base class
              }

              if (bIsSavedSearch)
              {
                // Execute selected saved search
                SetSearchFilter(pItem->GetEPGSearchFilter());
                ExecuteSearch();
                return true;
              }
            }

            if (bIsSavedSearch)
            {
              OpenDialogSearch(*pItem);
            }
            else if (pItem->GetPath() == CPVREpgSearchPath::PATH_SEARCH_DIALOG)
            {
              OpenDialogSearch(m_searchfilter);
            }
            else
            {
              CServiceBroker::GetPVRManager().Get<PVR::GUI::EPG>().ShowEPGInfo(*pItem);
            }
            return true;
          }

          case ACTION_CONTEXT_MENU:
          case ACTION_MOUSE_RIGHT_CLICK:
            OnPopupMenu(iItem);
            return true;

          case ACTION_RECORD:
            CServiceBroker::GetPVRManager().Get<PVR::GUI::Timers>().ToggleTimer(*pItem);
            return true;
        }
      }
    }
  }
  else if (message.GetMessage() == GUI_MSG_REFRESH_LIST)
  {
    if (static_cast<PVREvent>(message.GetParam1()) == PVREvent::SavedSearchesInvalidated)
    {
      Refresh(true);

      // Refresh triggered by deleted saved search?
      if (m_searchfilter)
      {
        const CPVREpgSearchPath path(m_vecItems->GetPath());
        const bool bIsSavedSearchesRoot = (path.IsValid() && path.IsSavedSearchesRoot());
        if (bIsSavedSearchesRoot)
        {
          const std::string filterPath = m_searchfilter->GetPath();
          bool bFound = false;
          for (const auto& item : *m_vecItems)
          {
            const auto filter = item->GetEPGSearchFilter();
            if (filter && filter->GetPath() == filterPath)
            {
              bFound = true;
              break;
            }
          }
          if (!bFound)
            SetSearchFilter(nullptr);
        }
      }
    }
  }
  else if (message.GetMessage() == GUI_MSG_WINDOW_INIT)
  {
    const CPVREpgSearchPath path(message.GetStringParam(0));
    if (path.IsValid() && path.IsSavedSearch())
    {
      const std::shared_ptr<CPVREpgSearchFilter> filter =
          CServiceBroker::GetPVRManager().EpgContainer().GetSavedSearchById(path.IsRadio(),
                                                                            path.GetId());
      if (filter)
      {
        SetSearchFilter(filter);
        m_bSearchConfirmed = true;
      }
    }
  }

  return CGUIWindowPVRBase::OnMessage(message);
}

bool CGUIWindowPVRSearchBase::Update(const std::string& strDirectory,
                                     bool updateFilterPath /* = true */)
{
  if (m_vecItems->GetObjectCount() > 0)
  {
    const CPVREpgSearchPath path(m_vecItems->GetPath());
    if (path.IsValid() && path.IsSavedSearchesRoot())
    {
      const std::string oldPath = m_vecItems->GetPath();

      const bool bReturn = CGUIWindowPVRBase::Update(strDirectory);

      if (bReturn && oldPath == m_vecItems->GetPath() && m_vecItems->GetObjectCount() == 0)
      {
        // Go to parent folder if we're in a subdir and for instance just deleted the last item
        GoParentFolder();
      }
      return bReturn;
    }
  }
  return CGUIWindowPVRBase::Update(strDirectory);
}

void CGUIWindowPVRSearchBase::UpdateButtons()
{
  CGUIWindowPVRBase::UpdateButtons();

  bool bSavedSearchesRoot = false;
  std::string header;
  const CPVREpgSearchPath path(m_vecItems->GetPath());
  if (path.IsValid() && path.IsSavedSearchesRoot())
  {
    bSavedSearchesRoot = true;
    header = g_localizeStrings.Get(19337); // "Saved searches"
  }

  if (header.empty() && m_searchfilter)
  {
    header = m_searchfilter->GetTitle();
    if (header.empty())
    {
      header = m_searchfilter->GetSearchTerm();
      StringUtils::Trim(header, "\"");
    }
  }
  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER1, header);

  if (!bSavedSearchesRoot && m_searchfilter && m_searchfilter->IsChanged())
    SET_CONTROL_LABEL(CONTROL_LABEL_HEADER2, g_localizeStrings.Get(19342)); // "[not saved]"
  else if (!bSavedSearchesRoot && m_searchfilter)
    SET_CONTROL_LABEL(CONTROL_LABEL_HEADER2, g_localizeStrings.Get(19343)); // "[saved]"
  else
    SET_CONTROL_LABEL(CONTROL_LABEL_HEADER2, "");
}

bool CGUIWindowPVRSearchBase::OnContextButtonClear(CFileItem* item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_CLEAR)
  {
    bReturn = true;

    m_bSearchConfirmed = false;
    SetSearchFilter(nullptr);

    Refresh(true);
  }

  return bReturn;
}

CGUIDialogPVRGuideSearch::Result CGUIWindowPVRSearchBase::OpenDialogSearch(const CFileItem& item)
{
  const auto searchFilter = item.GetEPGSearchFilter();
  if (!searchFilter)
    return CGUIDialogPVRGuideSearch::Result::CANCEL;

  return OpenDialogSearch(searchFilter);
}

CGUIDialogPVRGuideSearch::Result CGUIWindowPVRSearchBase::OpenDialogSearch(
    const std::shared_ptr<CPVREpgSearchFilter>& searchFilter)
{
  CGUIDialogPVRGuideSearch* dlgSearch =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogPVRGuideSearch>(
          WINDOW_DIALOG_PVR_GUIDE_SEARCH);

  if (!dlgSearch)
    return CGUIDialogPVRGuideSearch::Result::CANCEL;

  const std::shared_ptr<CPVREpgSearchFilter> tmpSearchFilter =
      searchFilter != nullptr ? std::make_shared<CPVREpgSearchFilter>(*searchFilter)
                              : std::make_shared<CPVREpgSearchFilter>(m_bRadio);

  dlgSearch->SetFilterData(tmpSearchFilter);

  /* Open dialog window */
  dlgSearch->Open();

  const CGUIDialogPVRGuideSearch::Result result = dlgSearch->GetResult();
  if (result == CGUIDialogPVRGuideSearch::Result::SEARCH)
  {
    SetSearchFilter(tmpSearchFilter);
    ExecuteSearch();
  }
  else if (result == CGUIDialogPVRGuideSearch::Result::SAVE)
  {
    CServiceBroker::GetPVRManager().EpgContainer().PersistSavedSearch(*tmpSearchFilter);
    if (searchFilter)
      searchFilter->SetDatabaseId(tmpSearchFilter->GetDatabaseId());

    const CPVREpgSearchPath path(m_vecItems->GetPath());
    if (path.IsValid() && path.IsSearchRoot())
    {
      SetSearchFilter(tmpSearchFilter);
      ExecuteSearch();
    }
  }

  return result;
}

void CGUIWindowPVRSearchBase::ExecuteSearch()
{
  m_bSearchConfirmed = true;

  const CPVREpgSearchPath path(m_vecItems->GetPath());
  if (path.IsValid() && path.IsSavedSearchesRoot())
  {
    GoParentFolder();
  }
  else if (IsActive())
  {
    Refresh(true);
  }

  // Save if not a transient search
  if (m_searchfilter->GetDatabaseId() != -1)
    CServiceBroker::GetPVRManager().EpgContainer().UpdateSavedSearchLastExecuted(*m_searchfilter);
}

void CGUIWindowPVRSearchBase::SetSearchFilter(
    const std::shared_ptr<CPVREpgSearchFilter>& searchFilter)
{
  if (m_searchfilter && m_searchfilter->IsChanged() &&
      (!searchFilter || m_searchfilter->GetPath() != searchFilter->GetPath()))
  {
    bool bCanceled = false;
    if (!CGUIDialogYesNo::ShowAndGetInput(CVariant{14117}, // "Warning"
                                          CVariant{19341}, // "Save the current search?"
                                          CVariant{},
                                          CVariant{!m_searchfilter->GetTitle().empty()
                                                       ? m_searchfilter->GetTitle()
                                                       : m_searchfilter->GetSearchTerm()},
                                          bCanceled, CVariant{107}, // "Yes"
                                          CVariant{106}, // "No"
                                          CGUIDialogYesNo::NO_TIMEOUT) &&
        !bCanceled)
    {
      std::string title = m_searchfilter->GetTitle();
      if (title.empty())
      {
        title = m_searchfilter->GetSearchTerm();
        if (title.empty())
          title = g_localizeStrings.Get(137); // "Search"
        else
          StringUtils::Trim(title, "\"");

        m_searchfilter->SetTitle(title);
      }
      CServiceBroker::GetPVRManager().EpgContainer().PersistSavedSearch(*m_searchfilter);
    }
  }
  m_searchfilter = searchFilter;
}

std::string CGUIWindowPVRTVSearch::GetRootPath() const
{
  return CPVREpgSearchPath::PATH_TV_SEARCH;
}

std::string CGUIWindowPVRTVSearch::GetStartFolder(const std::string& dir)
{
  return CPVREpgSearchPath::PATH_TV_SEARCH;
}

std::string CGUIWindowPVRTVSearch::GetDirectoryPath()
{
  return URIUtils::PathHasParent(m_vecItems->GetPath(), CPVREpgSearchPath::PATH_TV_SEARCH)
             ? m_vecItems->GetPath()
             : CPVREpgSearchPath::PATH_TV_SEARCH;
}

std::string CGUIWindowPVRRadioSearch::GetRootPath() const
{
  return CPVREpgSearchPath::PATH_RADIO_SEARCH;
}

std::string CGUIWindowPVRRadioSearch::GetStartFolder(const std::string& dir)
{
  return CPVREpgSearchPath::PATH_RADIO_SEARCH;
}

std::string CGUIWindowPVRRadioSearch::GetDirectoryPath()
{
  return URIUtils::PathHasParent(m_vecItems->GetPath(), CPVREpgSearchPath::PATH_RADIO_SEARCH)
             ? m_vecItems->GetPath()
             : CPVREpgSearchPath::PATH_RADIO_SEARCH;
}
