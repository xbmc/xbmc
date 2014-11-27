//
//  GUIWindowMediaFilterView.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-11-19.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#include "GUIPlexMediaWindow.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUISpinControlEx.h"
#include "plex/PlexUtils.h"
#include "plex/FileSystem/PlexDirectory.h"
#include "GUIUserMessages.h"
#include "AdvancedSettings.h"
#include "guilib/GUILabelControl.h"
#include "GUI/GUIDialogFilterSort.h"
#include "GUIWindowManager.h"
#include "ApplicationMessenger.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "PlexUtils.h"
#include "interfaces/Builtins.h"
#include "PlayList.h"
#include "PlexApplication.h"
#include "Client/PlexServerManager.h"
#include "GUIKeyboardFactory.h"
#include "utils/URIUtils.h"
#include "plex/GUI/GUIDialogPlexPluginSettings.h"
#include "PlexThemeMusicPlayer.h"
#include "PlexFilterManager.h"
#include "Filters/GUIPlexFilterFactory.h"
#include "dialogs/GUIDialogBusy.h"
#include "Client/PlexTimelineManager.h"
#include "Client/PlexServerDataLoader.h"
#include "dialogs/GUIDialogYesNo.h"
#include "Client/PlexExtraInfoLoader.h"
#include "ViewDatabase.h"
#include "ViewState.h"
#include "PlexPlayQueueManager.h"
#include "Client/PlexServerVersion.h"
#include "settings/GUISettings.h"
#include "Application.h"

#include "LocalizeStrings.h"
#include "DirectoryCache.h"
#include "music/tags/MusicInfoTag.h"
#include "plex/FileSystem/PlexExtraDataLoader.h"
#include "GUIPlexDefaultActionHandler.h"

#define XMIN(a,b) ((a)<(b)?(a):(b))

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::OnMessage(CGUIMessage &message)
{

  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    if (message.GetSenderId() >= FILTER_BUTTONS_START && message.GetSenderId() < FILTER_BUTTONS_STOP)
    {
      OnFilterButton(message.GetSenderId());
      return true;
    }
    else if (message.GetSenderId() == FILTER_CLEAR_FILTER_BUTTON)
      OnAction(CAction(ACTION_CLEAR_FILTERS));
  }
  else if (message.GetMessage() == GUI_MSG_WINDOW_DEINIT)
  {
    CGUIDialog *dialog = (CGUIDialog*) g_windowManager.GetWindow(WINDOW_DIALOG_FILTER_SORT);
    if (dialog && dialog->IsActive())
      dialog->Close();

    SaveSelection();
  }
  else if (message.GetMessage() == GUI_MSG_WINDOW_DEINIT)
    m_sectionFilter.reset();

  bool ret = CGUIMediaWindow::OnMessage(message);

  switch(message.GetMessage())
  {
    case GUI_MSG_UPDATE:
    {
      Update(m_vecItems->GetPath(), false, false);
      break;
    }
    case GUI_MSG_LOAD_SKIN:
    {
      /* This is called BEFORE the skin is reloaded, so let's save this event to be handled
       * in WINDOW_INIT instead */
      if (IsActive())
        m_returningFromSkinLoad = true;
    }
      break;

    case GUI_MSG_WINDOW_INIT:
    {
      /* If this is a reload event we must make sure to get the filters back */
      if (m_returningFromSkinLoad)
        AddFilters();
      m_returningFromSkinLoad = false;
      g_plexApplication.timelineManager->RefreshSubscribers();
      m_fetchedPages.clear();

      RestoreSelection();
      break;
    }

    case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_PLEX_SERVER_DATA_UNLOADED)
      {
        if (!message.GetStringParam().empty() && message.GetStringParam() == m_vecItems->GetProperty("plexserver").asString() &&
            g_windowManager.GetActiveWindow() == GetID())
        {
          CLog::Log(LOGDEBUG, "CGUIPlexMediaWindow::OnMessage got a notice that server that we are browsing is going away, returning home");
          CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(52300), g_localizeStrings.Get(52301));
          g_windowManager.ActivateWindow(WINDOW_HOME);
        }
      }
      break;
    }

    case GUI_MSG_FILTER_LOADED:
    {
      if (message.GetStringParam() == m_sectionRoot.Get())
      {
        m_sectionFilter = g_plexApplication.filterManager->getFilterForSection(m_sectionRoot.Get());

        CLog::Log(LOGDEBUG, "CGUIPlexMediaWindow::OnMessage filter is loaded for %s", m_sectionRoot.Get().c_str());
        AddFilters();

        g_plexApplication.filterManager->saveFiltersToDisk();
      }
      break;
    }

    case GUI_MSG_FILTER_SELECTED:
    {
      if (!message.GetStringParam().empty())
      {
        OnFilterSelected(message.GetStringParam(), message.GetParam1());
      }
      break;
    }

    case GUI_MSG_FILTER_VALUES_LOADED:
    {
      CSingleLock lk(m_filterValuesSection);
      if (m_waitingForFilter == message.GetStringParam())
      {
        m_filterValuesEvent.Set();
        m_waitingForFilter.clear();
      }
      break;
    }

    case GUI_MSG_PLEX_PAGE_LOADED:
    {
      InsertPage((CFileItemList*)message.GetPointer(), message.GetParam2());
      break;
    }

    case GUI_MSG_CHANGE_VIEW_MODE:
    {
      int viewMode = 0;
      if (message.GetParam1())  // we have an id
        viewMode = m_viewControl.GetViewModeByID(message.GetParam1());
      else if (message.GetParam2())
        viewMode = m_viewControl.GetNextViewMode((int)message.GetParam2());

      g_plexApplication.mediaServerClient->SetViewMode(CFileItemPtr(new CFileItem(*m_vecItems)), viewMode);
      CLog::Log(LOGDEBUG, "CGUIMediaWindow::OnMessage updating viewMode to %d", viewMode);
      m_vecItems->SetProperty("viewMode", viewMode);
      g_directoryCache.ClearDirectory(m_vecItems->GetPath());

      CViewDatabase db;
      if (db.Open())
      {
        CViewState state;
        state.m_viewMode = viewMode;

        db.SetViewState(GetLevelURL(), GetID(), state, g_guiSettings.GetString("lookandfeel.skin"));
        db.Close();
      }

      UpdateButtons();
      break;
    }
  }

  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexMediaWindow::SaveSelection()
{
  // Store the current selected item
  if (m_vecItems)
  {
    int offset = m_vecItems->GetProperty("offset").asInteger();
    CURL u(m_vecItems->GetPath());
    std::string key = u.GetUrlWithoutOptions();
    int idx = m_viewControl.GetSelectedItem();
    if (idx >= 0)
    {
      m_lastSelectedIndex[key] = m_vecItems->Get(idx)->GetProperty("index").asInteger();;
      CLog::Log(LOGDEBUG, "SaveSelection index for %s is %d", key.c_str(), m_lastSelectedIndex[key]);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::RestoreSelection()
{
  // Restore selected item for the section
  int idx = 0;
  CURL u(m_vecItems->GetPath());
  std::string key = u.GetUrlWithoutOptions();
  int currentSelection = m_viewControl.GetSelectedItem();

  if (m_lastSelectedIndex.find(key) != m_lastSelectedIndex.end())
  {
    idx = m_lastSelectedIndex[key];

    for (int i=0; i < m_vecItems->Size(); i++)
    {
      if (m_vecItems->Get(i)->GetProperty("index").asInteger() == idx)
      {
        m_viewControl.SetSelectedItem(i);
        CLog::Log(LOGDEBUG, "RestoreSelection index for %s is %d", key.c_str(), m_lastSelectedIndex[key]);
        return (currentSelection != i);
      }
    }
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexMediaWindow::InsertPage(CFileItemList* items, int Where)
{
#ifdef USE_PAGING
  int nItem = m_viewControl.GetSelectedItem();
  CStdString strSelected;
  if (nItem >= 0)
    strSelected = m_vecItems->Get(nItem)->GetPath();

  int itemsToRemove = items->Size();
  for (int i = 0; i < itemsToRemove; i ++)
    m_vecItems->Remove(Where);

  for (int i = 0; i < items->Size(); i ++)
    m_vecItems->Insert(Where + i, items->Get(i));

  m_viewControl.SetItems(*m_vecItems);
  m_viewControl.SetSelectedItem(strSelected);

  delete items;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexMediaWindow::updateFilterButtons(CPlexSectionFilterPtr filter, bool clear, bool disable)
{
  for (int i = FILTER_SECONDARY_BUTTONS_START; i < SORT_BUTTONS_START; i ++)
  {
    CGUIRadioButtonControl* button = (CGUIRadioButtonControl*)GetControl(i);
    if (!button)
      break;

    if (button->IsSelected() && clear)
    {
      std::vector<CPlexSecondaryFilterPtr> filters = filter->getSecondaryFilters();
      CPlexSecondaryFilterPtr currentSecFilter;

      try { currentSecFilter = filters.at(i - FILTER_SECONDARY_BUTTONS_START); }
      catch (...) { break; }

      button->SetSelected(false);
      button->SetLabel(currentSecFilter->getFilterTitle());
    }

    button->SetEnabled(!disable);
  }

  for (int i = SORT_BUTTONS_START; i < SORT_BUTTONS_START + 30; i++)
  {
    CGUIFilterOrderButtonControl* button = (CGUIFilterOrderButtonControl*)GetControl(i);
    if (!button)
      break;
    if (clear)
      button->SetTristate(CGUIFilterOrderButtonControl::OFF);
    else
    {
      PlexStringPairVector sortOrders = filter->getSortOrders();
      PlexStringPair p;

      try { p = sortOrders.at(i - SORT_BUTTONS_START); }
      catch (...) { break; }

      if (p.first == filter->currentSortOrder())
        button->SetTristate(filter->currentSortOrderAscending() ? CGUIFilterOrderButtonControl::ASCENDING : CGUIFilterOrderButtonControl::DESCENDING);
    }
    button->SetEnabled(!disable);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexMediaWindow::OnFilterButton(int filterButtonId)
{
  if (!m_sectionFilter)
  {
    CLog::Log(LOGWARNING, "CGUIPlexMediaWindow::OnFilterButton failed to get filters for %s", m_sectionRoot.Get().c_str());
    return;
  }

  if (filterButtonId >= FILTER_PRIMARY_BUTTONS_START && filterButtonId < FILTER_SECONDARY_BUTTONS_START)
  {
    PlexStringPairVector filterButtons = m_sectionFilter->getPrimaryFilters();

    PlexStringPair selectedFilter;
    try { selectedFilter = filterButtons.at(filterButtonId - FILTER_PRIMARY_BUTTONS_START); }
    catch(...) { return; }

    CLog::Log(LOGDEBUG, "CGUIPlexMediaWindow::OnFilterButton button %s was pressed", selectedFilter.second.c_str());

    int id = FILTER_PRIMARY_BUTTONS_START;
    BOOST_FOREACH(PlexStringPair fp, filterButtons)
    {
      CGUIRadioButtonControl* ctrl = (CGUIRadioButtonControl*)GetControl(id);
      if(ctrl)
        ctrl->SetSelected(id == filterButtonId);

      id ++;
    }

    m_sectionFilter->setPrimaryFilter(selectedFilter.first);

    bool clear = false;
    if (!m_sectionFilter->secondaryFiltersActivated())
    {
      m_sectionFilter->clearFilters();
      clear = true;
    }

    updateFilterButtons(m_sectionFilter, clear, !m_sectionFilter->secondaryFiltersActivated());

  }
  else if (filterButtonId >= FILTER_SECONDARY_BUTTONS_START && filterButtonId < SORT_BUTTONS_START)
  {
    std::vector<CPlexSecondaryFilterPtr> secondaryFilters = m_sectionFilter->getSecondaryFilters();
    CPlexSecondaryFilterPtr currentFilter;
    try { currentFilter = secondaryFilters.at(filterButtonId - FILTER_SECONDARY_BUTTONS_START); }
    catch(...) { return; }

    if (currentFilter->getFilterType() == CPlexSecondaryFilter::FILTER_TYPE_BOOLEAN)
    {
      currentFilter->setSelected(!currentFilter->isSelected());
      m_sectionFilter->addSecondaryFilter(currentFilter);
      m_clearFilterButton->SetVisible(m_sectionFilter->hasActiveSecondaryFilters());
    }
    else
    {

      CGUIRadioButtonControl *radio = (CGUIRadioButtonControl*)GetControl(filterButtonId);
      if (radio)
        radio->SetSelected(currentFilter->isSelected());

      m_filterValuesEvent.Reset();
      m_waitingForFilter = currentFilter->getFilterKey();

      m_sectionFilter->loadFilterValues(currentFilter);
      CGUIDialogBusy *busy = (CGUIDialogBusy*)g_windowManager.GetWindow(WINDOW_DIALOG_BUSY);
      if (busy)
      {
        busy->Show();
        while(!m_filterValuesEvent.WaitMSec(10))
        {
          g_windowManager.ProcessRenderLoop(false);
          if (busy->IsCanceled())
          {
            CSingleLock lk(m_filterValuesSection);
            m_filterValuesEvent.Set();
            m_waitingForFilter.clear();
            break;
          }
        }
        busy->Close();
      }

      CGUIDialogFilterSort* dialog = (CGUIDialogFilterSort*)g_windowManager.GetWindow(WINDOW_DIALOG_FILTER_SORT);
      if (dialog)
      {
        dialog->SetFilter(currentFilter, filterButtonId);
        dialog->DoModal();

      }

      if (radio)
        radio->SetSelected(currentFilter->isSelected());
    }
  }
  else
  {
    /* sort buttons */
    PlexStringPairVector sortOrders = m_sectionFilter->getSortOrders();
    PlexStringPair currentOrder;

    try { currentOrder = sortOrders.at(filterButtonId - SORT_BUTTONS_START); }
    catch (...) { return; }

    int id = SORT_BUTTONS_START;
    CGUIFilterOrderButtonControl::FilterOrderButtonState state = CGUIFilterOrderButtonControl::ASCENDING;

    BOOST_FOREACH(PlexStringPair p, sortOrders)
    {
      CGUIFilterOrderButtonControl* button = (CGUIFilterOrderButtonControl*)GetControl(id);
      if (button)
      {
        if (p.first == currentOrder.first)
          state = button->GetTristate();
        else
          button->SetTristate(CGUIFilterOrderButtonControl::OFF);
      }
      id ++;

    }

    m_sectionFilter->setSortOrder(currentOrder.first);
    m_sectionFilter->setSortOrderAscending(state == CGUIFilterOrderButtonControl::ASCENDING);
  }

  Update(m_sectionRoot.Get(), false, true);
  g_plexApplication.filterManager->saveFiltersToDisk();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexMediaWindow::OnFilterSelected(const std::string &filterKey, int filterButtonId)
{
  if (!m_sectionFilter)
    return;

  CPlexSecondaryFilterPtr filter = m_sectionFilter->addSecondaryFilter(filterKey);
  if (!filter)
    return;

  Update(m_sectionRoot.Get(), false, true);

  CGUIButtonControl* button = (CGUIButtonControl*)GetControl(filterButtonId);
  if (button)
  {
    button->SetSelected(filter->isSelected());
    if (filter->isSelected())
      button->SetLabel(filter->getFilterTitle() + ": " + filter->getCurrentValueLabel());
    else
      button->SetLabel(filter->getFilterTitle());
  }

  m_clearFilterButton->SetVisible(m_sectionFilter->hasActiveSecondaryFilters());

  g_plexApplication.filterManager->saveFiltersToDisk();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_CLEAR_FILTERS ||
           action.GetID() == ACTION_PLEX_TOGGLE_UNWATCHED_FILTER ||
           action.GetID() == ACTION_PLEX_CYCLE_PRIMARY_FILTER)
  {
    if (m_sectionFilter)
    {
      if (action.GetID() == ACTION_CLEAR_FILTERS)
      {
        m_sectionFilter->clearFilters();
        updateFilterButtons(m_sectionFilter, true, !m_sectionFilter->secondaryFiltersActivated());

        /* set focus to the next filter */
        CGUIControl* ctrl = (CGUIControl*)GetControl(FILTER_SECONDARY_BUTTONS_START);
        if (ctrl)
          ctrl->SetFocus(true);

        if (m_clearFilterButton)
        {
          m_clearFilterButton->SetFocus(false);
          m_clearFilterButton->SetVisible(false);
        }

        g_plexApplication.filterManager->saveFiltersToDisk();
        Update(m_sectionRoot.Get(), false, true);
        return true;
      }
      else if (action.GetID() == ACTION_PLEX_CYCLE_PRIMARY_FILTER)
      {
        PlexStringPairVector vec = m_sectionFilter->getPrimaryFilters();
        CStdString curr = m_sectionFilter->currentPrimaryFilter();
        int idx = 0;

        BOOST_FOREACH(PlexStringPair p, vec)
        {
          if (p.first == curr)
          {
            if (idx + 1 < vec.size())
              idx ++;
            else
              idx = 0;
            break;
          }
          idx ++;
        }

        OnFilterButton(FILTER_PRIMARY_BUTTONS_START + idx);
        std::string filterName = vec.at(idx).second;
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, "Switched primary filter to: ", filterName, 3000, false);
      }
      else if (action.GetID() == ACTION_PLEX_TOGGLE_UNWATCHED_FILTER)
      {
        if (m_sectionFilter->secondaryFiltersActivated())
        {
          std::vector<CPlexSecondaryFilterPtr> secFilters = m_sectionFilter->getSecondaryFilters();

          int i = 0;
          bool found = false;
          bool enabled;

          BOOST_FOREACH(CPlexSecondaryFilterPtr p, secFilters)
          {
            if (p->getFilterName() == "unwatched" || p->getFilterName() == "unwatchedLeaves")
            {
              found = true;
              enabled = !p->isSelected();
              break;
            }
            i ++;
          }

          if (found)
          {
            OnFilterButton(FILTER_SECONDARY_BUTTONS_START + i);
            CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, "Filter unwatched", enabled ? "Enabled" : "Disabled", 3000, false);
          }
        }
      }
    }
  }

  if (g_plexApplication.defaultActionHandler->OnAction(WINDOW_VIDEO_NAV, action, m_vecItems->Get(m_viewControl.GetSelectedItem()), m_vecItems))
    return true;
  
  bool ret = CGUIMediaWindow::OnAction(action);

#ifdef USE_PAGING
  if ((action.GetID() > ACTION_NONE &&
      action.GetID() <= ACTION_PAGE_DOWN) ||
      action.GetID() >= KEY_ASCII) // KEY_ASCII means that we letterjumped.
  {
    FetchItemPage(m_viewControl.GetSelectedItem());
  }
#endif

  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  CURL u(strDirectory);
#ifdef USE_PAGING
  // find the item range we need
  int Index = m_viewControl.GetSelectedItem();

  // if we have no items loaded, just center the page on last selection
  // as we will restor selection afterwards
  if (Index < 0)
  {
    if (m_lastSelectedIndex.find(u.GetUrlWithoutOptions()) != m_lastSelectedIndex.end())
      Index = m_lastSelectedIndex[u.GetUrlWithoutOptions()];
  }

  int NeededRangeStart = Index - PLEX_DEFAULT_PAGE_SIZE / 2;
  int NeededRangeEnd = Index + PLEX_DEFAULT_PAGE_SIZE / 2;

  if (NeededRangeStart <  0)
  {
    NeededRangeEnd -= NeededRangeStart;
    NeededRangeStart = 0;
  }
  
  if (!boost::algorithm::ends_with(u.GetFileName() , "url/lookup"))
  {
    u.SetOption("X-Plex-Container-Start", boost::lexical_cast<std::string>(NeededRangeStart));
    u.SetOption("X-Plex-Container-Size", boost::lexical_cast<std::string>(NeededRangeEnd - NeededRangeStart));
  }
#endif

  if (u.GetProtocol() == "plexserver" &&
      (u.GetHostName() != "channels" && u.GetHostName() != "shared" &&
       u.GetHostName() != "channeldirectory") && u.GetHostName() != "playqueue")
  {
    if (!XFILE::CPlexFile::CanBeTranslated(u))
    {
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(52300), g_localizeStrings.Get(52301));
      g_windowManager.ActivateWindow(WINDOW_HOME);
      return false;
    }
  }
  
  bool ret = CGUIMediaWindow::GetDirectory(u.Get(), items);

#ifndef TARGET_RASPBERRY_PI
  m_thumbCache.Load(items);
#endif

  CPlexServerPtr server = g_plexApplication.serverManager->FindByUUID(u.GetHostName());
  if (server && server->GetActiveConnection() && server->GetActiveConnection()->IsLocal())
    g_directoryCache.ClearDirectory(u.Get());
  
#ifdef USE_PAGING
  if (items.HasProperty("totalSize"))
  {
    if (items.GetProperty("totalSize").asInteger() > items.Size())
    {
     
      std::map<int, std::string> charMap;
      if (boost::ends_with(u.GetFileName(), "/all"))
      {
        /* we need the first characters, this is blocking this thread, which is not optimal :( */
        u.SetProtocolOptions("");
        
        /* cut off the all in the end */
        u.SetFileName(u.GetFileName().substr(0, u.GetFileName().size()-3));
        
        PlexUtils::AppendPathToURL(u, "firstCharacter");
        XFILE::CPlexDirectory dir;
        CFileItemList characters;

        if (dir.GetDirectory(u, characters))
        {
          int total = 0;
          for (int i = 0; i < characters.Size(); i++)
          {
            CFileItemPtr charDir = characters.Get(i);
            int num = charDir->GetProperty("size").asInteger();
            for (int j = 0; j < num; j ++)
              charMap[total ++] = charDir->GetProperty("title").asString();
          }
        }
      }
      
      for (int i = 0; i < NeededRangeStart; i++)
      {
        CFileItemPtr item = CFileItemPtr(new CFileItem);
        item->SetPath(boost::lexical_cast<std::string>(i));
        if (charMap.find(PLEX_DEFAULT_PAGE_SIZE + i) != charMap.end())
          item->SetSortLabel(CStdString(charMap[PLEX_DEFAULT_PAGE_SIZE + i]));
        items.AddFront(item, 0);
      }

      for (int i = NeededRangeEnd; i < items.GetProperty("totalSize").asInteger(); i++)
      {
        CFileItemPtr item = CFileItemPtr(new CFileItem);
        item->SetPath(boost::lexical_cast<std::string>(i));
        if (charMap.find(PLEX_DEFAULT_PAGE_SIZE + i) != charMap.end())
          item->SetSortLabel(CStdString(charMap[PLEX_DEFAULT_PAGE_SIZE + i]));
        items.Add(item);
      }
    }
  }
#endif
  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexMediaWindow::LoadPage(int iPage)
{
  int start = iPage * PLEX_DEFAULT_PAGE_SIZE;
  int numberOfItems = PLEX_DEFAULT_PAGE_SIZE;

  // Check if the page has been already fetched
  if (m_fetchedPages.find(iPage) != m_fetchedPages.end())
    return;

  // check if the page is already being fetched
  if (m_fetchJobs.find(iPage) != m_fetchJobs.end())
    return;

  // check if we're requesting an invalid page
  if (start >= m_vecItems->GetProperty("totalSize").asInteger())
    return;
  
  CURL u(m_vecItems->GetPath());
  
  int pageSize = XMIN(numberOfItems, m_vecItems->GetProperty("totalSize").asInteger() - start);
  
  u.SetOption("X-Plex-Container-Start", boost::lexical_cast<std::string>(start));
  u.SetOption("X-Plex-Container-Size", boost::lexical_cast<std::string>(pageSize));
  
#if TARGET_RASPBERRY_PI
  PlexUtils::PauseRendering(true, true);
#endif

  m_fetchJobs[iPage] = CJobManager::GetInstance().AddJob(new CPlexDirectoryFetchJob(u), this);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexMediaWindow::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  CPlexDirectoryFetchJob* fjob = static_cast<CPlexDirectoryFetchJob*>(job);
  if (!fjob)
    return;

#ifdef USE_PAGING
  int rangeStart = boost::lexical_cast<int>(fjob->m_url.GetOption("X-Plex-Container-Start"));
  int pageNum = GetPageFromItemIndex(rangeStart);

  if (success)
  {
    CFileItemList* list = new CFileItemList;
    list->Copy(fjob->m_items);

    // we update the fetched items list
    m_fetchedPages.insert(pageNum);

    if (list)
    {
      CGUIMessage msg(GUI_MSG_PLEX_PAGE_LOADED, 0, GetID(), 0, rangeStart, list);
      g_windowManager.SendThreadMessage(msg);
    }
  }

#if TARGET_RASPBERRY_PI
  PlexUtils::PauseRendering(false, true);
#endif
  // remove FetchJob from List
  m_fetchJobs.erase(pageNum);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::OnSelect(int iItem)
{
  CFileItemPtr item = m_vecItems->Get(iItem);
  if (!item)
    return false;

  if (!item->m_bIsFolder)
  {
    if (!PlexUtils::CurrentSkinHasPreplay() || item->GetProperty("isSynthesized").asBoolean())
      return OnPlayMedia(iItem);

      if (item->GetPlexDirectoryType() == PLEX_DIR_TYPE_TRACK ||
          item->GetPlexDirectoryType() == PLEX_DIR_TYPE_PHOTO ||
          item->GetPlexDirectoryType() == PLEX_DIR_TYPE_VIDEO)
      return OnPlayMedia(iItem);
  }

  if (item->GetPlexDirectoryType() == PLEX_DIR_TYPE_SEASON ||
      item->GetPlexDirectoryType() == PLEX_DIR_TYPE_SHOW)
  {
    if (m_sectionFilter && m_sectionFilter->secondaryFiltersActivated())
    {
      CPlexSecondaryFilterPtr unwatchedFilter = m_sectionFilter->getSecondaryFilterOfName("unwatchedLeaves");
      if (!unwatchedFilter)
        unwatchedFilter = m_sectionFilter->getSecondaryFilterOfName("unwatched");
      if (unwatchedFilter && unwatchedFilter->isSelected())
      {
        CURL u(item->GetPath());
        u.SetOption("unwatched", "1");
        item->SetPath(u.Get());
      }
    }
  }

  CStdString newUrl = m_navHelper.navigateToItem(item, m_vecItems->GetPath(), GetID());

  if (item->m_bIsFolder && !newUrl.empty())
  {
    CURL u(m_vecItems->GetPath());
    if (!Update(newUrl, true))
      ShowShareErrorMessage(item.get());
    return true;
  }

  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::OnPlayMedia(int iItem)
{
  CFileItemPtr item = m_vecItems->Get(iItem);
  if (!item)
    return false;
  g_plexApplication.defaultActionHandler->OnAction(WINDOW_VIDEO_NAV, CAction(ACTION_PLAYER_PLAY), item, m_vecItems);

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexMediaWindow::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (!item)
    return;


  
  g_plexApplication.defaultActionHandler->GetContextButtons(WINDOW_VIDEO_NAV, item, m_vecItems, buttons);



}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (!item)
    return false;

  
  if (g_plexApplication.defaultActionHandler->OnAction(WINDOW_VIDEO_NAV, button, item, m_vecItems))
    return true;

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::UnwatchedEnabled() const
{
  if (!m_sectionFilter)
    return false;

  CPlexSecondaryFilterPtr filter = m_sectionFilter->getSecondaryFilterOfName("unwatchedLeaves");
  if (filter)
    return filter->isSelected();
  return false;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::Update(const CStdString &strDirectory, bool updateFilterPath)
{
  return Update(strDirectory, updateFilterPath, false);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::Update(const CStdString &strDirectory, bool updateFilterPath, bool updateFromFilter)
{
  CSingleLock lock(m_fetchMapsSection);
  m_fetchedPages.clear();
  m_fetchJobs.clear();
  lock.Leave();

  CURL newUrl = GetRealDirectoryUrl(strDirectory);
  if (newUrl.Get().empty())
    return false;

  SaveSelection();
  
  if (strDirectory == m_startDirectory)
  {
    m_sectionRoot = strDirectory;
    g_plexApplication.filterManager->loadFilterForSection(m_sectionRoot.Get());
  }

  // since the filters might not have been loaded yet we should really make sure that we
  // use *a* primaryFilter here. We just default to all since that seems sane.
  CStdString primaryFilter = "all";
  if (m_sectionFilter)
  {
    primaryFilter = m_sectionFilter->currentPrimaryFilter();

    // A hack to handle by Albums
    if (primaryFilter == "albums")
      primaryFilter = "all";
  }

  if (boost::ends_with(newUrl.GetFileName(), primaryFilter))
  {
    CLog::Log(LOGDEBUG, "CGUIPlexMediaWindow::Update m_startDirectory=%s", newUrl.GetUrlWithoutOptions().c_str());
    m_startDirectory = newUrl.GetUrlWithoutOptions();
  }

  if (updateFromFilter)
    m_history.RemoveParentPath();

  bool ret = CGUIMediaWindow::Update(newUrl.Get(), updateFilterPath);

  m_vecItems->SetProperty("PlexContent", PlexUtils::GetPlexContent(*m_vecItems));

  m_vecItems->SetProperty("PlexFilter", "all");
  if (m_sectionFilter && !m_sectionFilter->currentPrimaryFilter().empty())
    m_vecItems->SetProperty("PlexFilter", m_sectionFilter->currentPrimaryFilter());

  g_plexApplication.extraInfo->LoadExtraInfoForItem(m_vecItems);

  if (!updateFromFilter)
    g_plexApplication.themeMusicPlayer->playForItem(*m_vecItems);

  UpdateSectionTitle();

  if (RestoreSelection())
  {
    FetchItemPage(m_viewControl.GetSelectedItem());
  }

  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexMediaWindow::CheckPlexFilters(CFileItemList &list)
{
  m_contentMatch.clear();

  if (m_sectionFilter)
  {
    list.SetProperty("hasAdvancedFilters", m_sectionFilter->hasAdvancedFilters() ? "yes" : "");
    list.SetProperty("primaryFilterActivated", m_sectionFilter->secondaryFiltersActivated() ? "" : "yes");
    list.SetProperty("secondaryFilterActivated", m_sectionFilter->hasActiveSecondaryFilters() ? "yes" : "");
  }

  CFileItemPtr section = g_plexApplication.dataLoader->GetSection(m_sectionRoot);
  if (section && section->GetPlexDirectoryType() == PLEX_DIR_TYPE_HOME_MOVIES)
  {
    list.SetContent("homemovies");
    list.SetProperty("sectionType", (int)section->GetPlexDirectoryType());
  }

  if (m_sectionFilter && m_sectionFilter->currentPrimaryFilter() == "folder")
    list.SetContent("folders");

  /* check if we have gone deeper down or not */
  CURL newPath(list.GetPath());
  if (m_startDirectory != newPath.GetUrlWithoutOptions())
  {
    if (list.Size())
      m_directoryType = list.GetPlexDirectoryType();

    if (m_directoryType == PLEX_DIR_TYPE_SEASON ||
        m_directoryType == PLEX_DIR_TYPE_EPISODE ||
        m_directoryType == PLEX_DIR_TYPE_ALBUM ||
        m_directoryType == PLEX_DIR_TYPE_TRACK ||
        m_directoryType == PLEX_DIR_TYPE_VIDEO)
    {
      CLog::Log(LOGDEBUG, "CGUIPlexMediaWindow::CheckPlexFilters setting preplay flag");
      list.SetProperty("PlexPreplay", "yes");
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexMediaWindow::UpdateButtons()
{
  CViewDatabase db;
  int viewMode = -1;

  if (db.Open())
  {
    CViewState state;
    if (db.GetViewState(GetLevelURL(), GetID(), state, g_guiSettings.GetString("lookandfeel.skin")))
    {
      CLog::Log(LOGDEBUG, "GUIPlexMediaWindow::UpdateButtons got viewMode from db: %d", state.m_viewMode);
      viewMode = state.m_viewMode;
    }
  }

  if (viewMode == -1 && CurrentDirectory().HasProperty("viewMode"))
  {
    viewMode = (int)CurrentDirectory().GetProperty("viewMode").asInteger();
    if (db.IsOpen())
    {
      CViewState state;
      state.m_viewMode = viewMode;
      db.SetViewState(GetLevelURL(), GetID(), state, g_guiSettings.GetString("lookandfeel.skin"));
      CLog::Log(LOGDEBUG, "GUIPlexMediaWindow::UpdateButtons storing viewMode to db: %d", state.m_viewMode);
    }
  }

  if (db.IsOpen())
    db.Close();

  CLog::Log(LOGDEBUG, "CGUIMediaWindow::UpdateButtons setting viewMode to %d", viewMode);
  m_viewControl.SetCurrentView(viewMode);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::IsVideoContainer(CFileItemPtr item) const
{
  EPlexDirectoryType dirType = m_vecItems->GetPlexDirectoryType();

  if (dirType == PLEX_DIR_TYPE_CHANNEL && m_vecItems->Get(0))
    dirType = m_vecItems->Get(0)->GetPlexDirectoryType();

  if (dirType == PLEX_DIR_TYPE_DIRECTORY && item)
    dirType = item->GetPlexDirectoryType();

  return (dirType == PLEX_DIR_TYPE_MOVIE    ||
          dirType == PLEX_DIR_TYPE_SHOW     ||
          dirType == PLEX_DIR_TYPE_SEASON   ||
          dirType == PLEX_DIR_TYPE_PLAYLIST ||
          dirType == PLEX_DIR_TYPE_EPISODE  ||
          dirType == PLEX_DIR_TYPE_VIDEO    ||
          dirType == PLEX_DIR_TYPE_CLIP);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::IsMusicContainer() const
{
  EPlexDirectoryType dirType = m_vecItems->GetPlexDirectoryType();
  if (dirType == PLEX_DIR_TYPE_CHANNEL && m_vecItems->Get(0))
    dirType = m_vecItems->Get(0)->GetPlexDirectoryType();
  return (dirType == PLEX_DIR_TYPE_ALBUM || dirType == PLEX_DIR_TYPE_ARTIST || dirType == PLEX_DIR_TYPE_TRACK);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
CURL CGUIPlexMediaWindow::GetUrlWithParentArgument(const CURL &originalUrl)
{
  CURL o(originalUrl.GetUrlWithoutOptions());
  if (originalUrl.HasOption("parent"))
    o.SetOption("parent", originalUrl.GetOption("parent"));
  return o;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::OnBack(int actionID)
{
  CURL currPath = GetUrlWithParentArgument(m_vecItems->GetPath());
  CURL parent = GetUrlWithParentArgument(m_history.GetParentPath());

  if (!parent.Get().empty())
  {
    m_history.RemoveParentPath();

    while (parent.Get() == currPath.Get())
    {
      parent = CURL(m_history.GetParentPath());
      m_history.RemoveParentPath();
    }
  }

  if (currPath.Get() == m_startDirectory || parent.Get().empty())
  {
    g_windowManager.PreviousWindow();
    return true;
  }

  Update(parent.Get(), true);

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CURL CGUIPlexMediaWindow::GetRealDirectoryUrl(const CStdString& url_)
{
  CURL dirUrl(url_);

  if (!PlexUtils::CurrentSkinHasFilters())
    return url_;

  if (dirUrl.GetProtocol() == "plexserver" &&
      (dirUrl.GetHostName() == "channels" || dirUrl.GetHostName() == "shared" || dirUrl.GetHostName() == "channeldirectory"))

    return url_;

  bool isSecondary = false;

  int sectionNumber = -1;
  if (dirUrl.GetProtocol() == "plexserver" &&
      (boost::starts_with(dirUrl.GetFileName(), "library/sections/") ||
       (boost::starts_with(dirUrl.GetFileName(), "sync/"))))
  {
    /* remove library/sections/ at the beginning of the string */
    CStdString sectionName = URIUtils::GetFileName(dirUrl.GetFileName());

    /* now let's check if this is a number i.e. 5 or something */
    try { sectionNumber = boost::lexical_cast<int>(sectionName); }
    catch (...) { }

    if (sectionNumber != -1)
    {
      CLog::Log(LOGDEBUG, "CPlexFilterHelper::GetRealDirectoryUrl got section %d", sectionNumber);
     isSecondary = true;
    }
  }

  if (dirUrl.GetProtocol() == "plexserver" &&
      dirUrl.GetHostName() == "myplex" &&
      dirUrl.GetFileName() == "pms/playlists")
  {
    CLog::Log(LOGDEBUG, "CPlexMediaWindow::GetRealDirectoryUrl at myPlex playlists..");
    isSecondary = true;
  }

  if (isSecondary)
  {
    CURL url(dirUrl);

    if (g_plexApplication.filterManager)
    {
      CPlexSectionFilterPtr sectionFilter = g_plexApplication.filterManager->getFilterForSection(url.Get());
      if (sectionFilter)
        url = sectionFilter->addFiltersToUrl(url);
      else
        PlexUtils::AppendPathToURL(url, "all");
    }

    return url;
  }

  return dirUrl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexMediaWindow::UpdateSectionTitle()
{
  CGUILabelControl *lblCtrl = (CGUILabelControl*)GetControl(FILTER_PRIMARY_LABEL);
  if (lblCtrl)
  {
    std::string lbl = m_vecItems->GetLabel();
    StringUtils::ToUpper(lbl);
    lblCtrl->SetLabel(lbl);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexMediaWindow::AddFilters()
{
  if (!PlexUtils::CurrentSkinHasFilters())
    return;

  CGUIPlexFilterFactory factory(this);

  if (!m_sectionFilter)
    return;

  m_hasAdvancedFilters = m_sectionFilter->hasAdvancedFilters();
  m_vecItems->SetProperty("hasAdvancedFilters", m_hasAdvancedFilters ? "yes" : "");

  CGUIControlGroupList *primaryFilters = (CGUIControlGroupList*)GetControl(FILTER_PRIMARY_CONTAINER);
  if (primaryFilters)
  {
    primaryFilters->ClearAll();

    PlexStringPairVector pfilterLabel = m_sectionFilter->getPrimaryFilters();
    int id = FILTER_PRIMARY_BUTTONS_START;
    BOOST_FOREACH(PlexStringPair p, pfilterLabel)
    {
      CGUIButtonControl *button = factory.getPrimaryFilterButton(p.second);
      if (button)
      {
        button->SetID(id ++);

        primaryFilters->AddControl(button);
        if (p.first == m_sectionFilter->currentPrimaryFilter())
          button->SetSelected(true);
        else
          button->SetSelected(false);
      }
    }

    CGUIControlGroupList *secondaryFilters = (CGUIControlGroupList*)GetControl(FILTER_SECONDARY_CONTAINER);
    if(secondaryFilters)
    {
      bool hasActiveFilters = false;
      secondaryFilters->ClearAll();

      CGUIButtonControl *origButton = (CGUIButtonControl*)GetControl(FILTER_BUTTON);
      if (origButton)
      {
        m_clearFilterButton = new CGUIButtonControl(*origButton);
        m_clearFilterButton->SetLabel(g_localizeStrings.Get(44032));
        m_clearFilterButton->AllocResources();
        m_clearFilterButton->SetID(FILTER_CLEAR_FILTER_BUTTON);
        secondaryFilters->AddControl(m_clearFilterButton);
      }

      int id = FILTER_SECONDARY_BUTTONS_START;
      BOOST_FOREACH(CPlexSecondaryFilterPtr filter, m_sectionFilter->getSecondaryFilters())
      {
        CGUIButtonControl *button = factory.getSecondaryFilterButton(filter);
        if (button)
        {
          button->SetID(id ++);

          if (!m_sectionFilter->secondaryFiltersActivated())
            button->SetEnabled(false);

          CLog::Log(LOGDEBUG, "CGUIPlexMediaWindow::AddFilters added %s with id %d", button->GetLabel().c_str(), button->GetID());
          secondaryFilters->AddControl(button);

        }
        if (filter->isSelected())
          hasActiveFilters = true;
      }

      m_clearFilterButton->SetVisible(hasActiveFilters);
    }

    CGUIControlGroupList *sortButtons = (CGUIControlGroupList*)GetControl(SORT_LIST);
    if (sortButtons)
    {
      sortButtons->ClearAll();

      PlexStringPairVector sorts = m_sectionFilter->getSortOrders();

      if (sorts.size() > 0)
      {
        SET_CONTROL_VISIBLE(SORT_LIST);
        SET_CONTROL_VISIBLE(SORT_LABEL);

        int id = SORT_BUTTONS_START;
        BOOST_FOREACH(PlexStringPair p, sorts)
        {
          CGUIFilterOrderButtonControl::FilterOrderButtonState state = CGUIFilterOrderButtonControl::OFF;

          if (p.first == m_sectionFilter->currentSortOrder())
            state = m_sectionFilter->currentSortOrderAscending() ? CGUIFilterOrderButtonControl::ASCENDING : CGUIFilterOrderButtonControl::DESCENDING;

          CGUIFilterOrderButtonControl* button = factory.getSortButton(p.second, state);
          if (button)
          {
            button->SetID(id ++);

            if (!m_sectionFilter->secondaryFiltersActivated())
            {
              button->SetEnabled(false);
              button->SetTristate(CGUIFilterOrderButtonControl::OFF);
            }

            sortButtons->AddControl(button);
          }
        }
      }
      else
      {
        SET_CONTROL_HIDDEN(SORT_LIST);
        SET_CONTROL_HIDDEN(SORT_LABEL);
      }
    }
  }

#ifdef USE_PAGING
  FetchItemPage(m_viewControl.GetSelectedItem());
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::MatchPlexContent(const CStdString &matchStr)
{
  if (m_contentMatch.find(matchStr) != m_contentMatch.end())
    return m_contentMatch[matchStr];

  std::vector<std::string> matchVec;
  matchVec = StringUtils::Split(matchStr, ";");

  std::string content = m_vecItems->GetProperty("PlexContent").asString();
  if (content.empty())
  {
    content = PlexUtils::GetPlexContent(*m_vecItems);
    m_vecItems->SetProperty("PlexContent", content);
  }

  bool ret = false;

  BOOST_FOREACH(std::string& match, matchVec)
  {
    StringUtils::Trim(match);
    StringUtils::ToLower(match);
    if(match == content)
      ret = true;
  }

  m_contentMatch[matchStr] = ret;
  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::MatchPlexFilter(const CStdString &matchStr)
{
  if (!m_sectionFilter)
    return "all";

  CStdStringArray matchVec = StringUtils::SplitString(matchStr, ";");
  CStdString filterName = m_sectionFilter->currentPrimaryFilter();

  BOOST_FOREACH(CStdString& match, matchVec)
  {
    match = StringUtils::Trim(match);
    if(match.Equals(filterName))
      return true;
  }

  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::IsFiltered()
{
  if (m_sectionFilter)
  {
    if (m_sectionFilter->secondaryFiltersActivated())
      return true;
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::CanFilterAdvanced()
{
  if (m_sectionFilter)
    return m_sectionFilter->hasAdvancedFilters();
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::MatchUniformProperty(const CStdString& property)
{
  if (!IsMusicContainer() || !m_vecItems)
    return false;

  if (property != "artist" && property != "album")
    return false;

  bool same = true;
  CStdString lastVal;
  for (int i = 0; i < m_vecItems->Size(); i ++)
  {
    CFileItemPtr item = m_vecItems->Get(i);
    CStdString value;

    if (!item)
      continue;

    if (item->HasMusicInfoTag())
    {
      if (property == "artist" && item->GetMusicInfoTag()->GetArtist().size() > 0)
        value = item->GetMusicInfoTag()->GetArtist()[0];
      else if (property == "album")
        value = item->GetMusicInfoTag()->GetAlbum();
    }

    if (!lastVal.empty() && value != lastVal)
    {
      same = false;
      break;
    }

    lastVal = value;
  }

  return same;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexMediaWindow::FetchItemPage(int Index)
{
  CSingleLock lock(m_fetchMapsSection);
  
  // find the item range we need
  int NeededRangeStart = Index - PLEX_DEFAULT_PAGE_SIZE / 2;
  int NeededRangeEnd = Index + PLEX_DEFAULT_PAGE_SIZE / 2;

  int startPage = GetPageFromItemIndex(NeededRangeStart);
  int endPage = GetPageFromItemIndex(NeededRangeEnd);

  CLog::Log(LOGDEBUG,"CGUIPlexMediaWindow::FetchItemPage for index = %d / %lld, Page (%d-%d)", Index,m_vecItems->GetProperty("totalSize").asInteger(), startPage, endPage);

  std::set<int> jobsToRemove;
  // check now if unnecessary fetching jobs should be cancelled
  BOOST_FOREACH(FetchJobPair p, m_fetchJobs)
  {
    if ((p.first != startPage) && (p.first != endPage))
    {
      jobsToRemove.insert(p.first);
      CJobManager::GetInstance().CancelJob(p.second);
    }
  }
  
  // now remove them from the list
  for (std::set<int>::iterator it = jobsToRemove.begin(); it!=jobsToRemove.end(); ++it)
    m_fetchJobs.erase(*it);

  // now we check if both pages are cached, if its not the case then we need to cache them
  if (m_fetchedPages.find(startPage) == m_fetchedPages.end())
    LoadPage(startPage);

  if (endPage != startPage)
  {
    if (m_fetchedPages.find(endPage) == m_fetchedPages.end())
      LoadPage(endPage);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CStdString CGUIPlexMediaWindow::GetLevelURL()
{
  int level = 1;
  CStdString viewGroup = m_vecItems->GetProperty("viewGroup").asString();
  
  if ((viewGroup == "episode") || (viewGroup == "track"))
    level = 3;
  else if ((viewGroup == "season") || (viewGroup == "album"))
    level = 2;
  else if ((viewGroup == "movie") || (viewGroup == "artist"))
    level = 1;
  else
    level = 0;

  CStdString userName = g_plexApplication.myPlexManager->GetCurrentUserInfo().username;
  
  CStdString levelUrl = m_sectionRoot.Get() + "/level/";
  levelUrl += boost::lexical_cast<std::string>(level);
  if (!userName.IsEmpty())
    levelUrl += "/user/" + userName;

  return  levelUrl;
}
