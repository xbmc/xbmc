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
#include "PlexContentPlayerMixin.h"
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
#include "PlexContentPlayerMixin.h"
#include "PlexFilterManager.h"
#include "Filters/GUIPlexFilterFactory.h"
#include "dialogs/GUIDialogBusy.h"
#include "Client/PlexTimelineManager.h"
#include "client/PlexServerDataLoader.h"

#include "LocalizeStrings.h"
#include "DirectoryCache.h"

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

  bool ret = CGUIMediaWindow::OnMessage(message);

  switch(message.GetMessage())
  {
    case GUI_MSG_UPDATE:
    {
      Update(m_sectionRoot.Get(), false, false);
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
      g_plexApplication.timelineManager->UpdateLocation();
      break;
    }

    case GUI_MSG_ITEM_SELECT:
    {
      int currentIdx = m_viewControl.GetSelectedItem();
      if (currentIdx > m_pagingOffset && m_currentJobId == -1)
      {
        /* the user selected something in the middle of where we loaded, let's just cheat and fill in everything */
        LoadPage(m_pagingOffset, currentIdx + PLEX_DEFAULT_PAGE_SIZE);
      }
      break;
    }

    case GUI_MSG_PLEX_SERVER_DATA_UNLOADED:
    {
      if (message.GetStringParam() == m_vecItems->GetProperty("plexserver").asString())
      {
        CLog::Log(LOGDEBUG, "CGUIPlexMediaWindow::OnMessage got a notice that server that we are browsing is going away, returning home");
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(52300), g_localizeStrings.Get(52301));
        g_windowManager.ActivateWindow(WINDOW_HOME);
      }
      break;
    }

    case GUI_MSG_FILTER_LOADED:
    {
      if (message.GetStringParam() == m_sectionRoot.Get())
      {
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
      
  }

  return ret;
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
  CPlexSectionFilterPtr filter = g_plexApplication.filterManager->getFilterForSection(m_sectionRoot.Get());
  if (!filter)
  {
    CLog::Log(LOGWARNING, "CGUIPlexMediaWindow::OnFilterButton failed to get filters for %s", m_sectionRoot.Get().c_str());
    return;
  }

  if (filterButtonId >= FILTER_PRIMARY_BUTTONS_START && filterButtonId < FILTER_SECONDARY_BUTTONS_START)
  {
    PlexStringPairVector filterButtons = filter->getPrimaryFilters();

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

    filter->setPrimaryFilter(selectedFilter.first);

    bool clear = false;
    if (!filter->secondaryFiltersActivated())
    {
      filter->clearFilters();
      clear = true;
    }

    updateFilterButtons(filter, clear, !filter->secondaryFiltersActivated());

  }
  else if (filterButtonId >= FILTER_SECONDARY_BUTTONS_START && filterButtonId < SORT_BUTTONS_START)
  {
    std::vector<CPlexSecondaryFilterPtr> secondaryFilters = filter->getSecondaryFilters();
    CPlexSecondaryFilterPtr currentFilter;
    try { currentFilter = secondaryFilters.at(filterButtonId - FILTER_SECONDARY_BUTTONS_START); }
    catch(...) { return; }

    if (currentFilter->getFilterType() == CPlexSecondaryFilter::FILTER_TYPE_BOOLEAN)
    {
      currentFilter->setSelected(!currentFilter->isSelected());
      filter->addSecondaryFilter(currentFilter);
      m_clearFilterButton->SetVisible(filter->hasActiveSecondaryFilters());
    }
    else
    {

      CGUIRadioButtonControl *radio = (CGUIRadioButtonControl*)GetControl(filterButtonId);
      if (radio)
        radio->SetSelected(currentFilter->isSelected());

      m_filterValuesEvent.Reset();
      m_waitingForFilter = currentFilter->getFilterKey();

      filter->loadFilterValues(currentFilter);
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
    PlexStringPairVector sortOrders = filter->getSortOrders();
    PlexStringPair currentOrder;

    try { currentOrder = sortOrders.at(filterButtonId - SORT_BUTTONS_START); }
    catch (...) { return; }

    int id = SORT_BUTTONS_START;
    CGUIFilterOrderButtonControl::FilterOrderButtonState state;

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

    filter->setSortOrder(currentOrder.first);
    filter->setSortOrderAscending(state == CGUIFilterOrderButtonControl::ASCENDING);
  }

  Update(m_sectionRoot.Get(), false, true);
  g_plexApplication.filterManager->saveFiltersToDisk();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexMediaWindow::OnFilterSelected(const std::string &filterKey, int filterButtonId)
{
  CPlexSectionFilterPtr sectionFilter = g_plexApplication.filterManager->getFilterForSection(m_sectionRoot.Get());
  if (!sectionFilter)
    return;

  CPlexSecondaryFilterPtr filter = sectionFilter->addSecondaryFilter(filterKey);
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

  m_clearFilterButton->SetVisible(sectionFilter->hasActiveSecondaryFilters());

  g_plexApplication.filterManager->saveFiltersToDisk();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::OnAction(const CAction &action)
{
  bool ret = CGUIMediaWindow::OnAction(action);

  if ((action.GetID() > ACTION_NONE &&
      action.GetID() <= ACTION_PAGE_DOWN) ||
      action.GetID() >= KEY_ASCII) // KEY_ASCII means that we letterjumped.
  {
    if (m_viewControl.GetSelectedItem() >= m_pagingOffset)
      LoadPage(m_pagingOffset, m_viewControl.GetSelectedItem() + PLEX_DEFAULT_PAGE_SIZE);
    else if (m_viewControl.GetSelectedItem() >= (m_pagingOffset - (PLEX_DEFAULT_PAGE_SIZE/2)))
      LoadNextPage();
  }
  else if (action.GetID() == ACTION_TOGGLE_WATCHED)
  {
    if (m_viewControl.GetSelectedItem() != -1)
    {
      CFileItemPtr pItem = m_vecItems->Get(m_viewControl.GetSelectedItem());
      if (pItem && pItem->GetVideoInfoTag()->m_playCount == 0)
        return OnContextButton(m_viewControl.GetSelectedItem(),CONTEXT_BUTTON_MARK_WATCHED);
      if (pItem && pItem->GetVideoInfoTag()->m_playCount > 0)
        return OnContextButton(m_viewControl.GetSelectedItem(),CONTEXT_BUTTON_MARK_UNWATCHED);
    }
  }
  else if (action.GetID() == ACTION_PLAYER_PLAY)
  {
    CGUIControl *pControl = (CGUIControl*)GetControl(m_viewControl.GetCurrentControl());
    if (pControl)
      PlayFileFromContainer(pControl);
  }
  else if (action.GetID() == ACTION_CLEAR_FILTERS)
  {
    CPlexSectionFilterPtr sectionFilter = g_plexApplication.filterManager->getFilterForSection(m_sectionRoot.Get());
    if (sectionFilter)
    {
      sectionFilter->clearFilters();
      updateFilterButtons(sectionFilter, true, !sectionFilter->secondaryFiltersActivated());

      /* set focus to the next filter */
      CGUIControl* ctrl = (CGUIControl*)GetControl(FILTER_SECONDARY_BUTTONS_START);
      ctrl->SetFocus(true);

      m_clearFilterButton->SetFocus(false);
      m_clearFilterButton->SetVisible(false);

      g_plexApplication.filterManager->saveFiltersToDisk();
      Update(m_sectionRoot.Get(), false, true);
    }
  }

  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  CURL u(strDirectory);
  u.SetProtocolOption("containerStart", "0");
  u.SetProtocolOption("containerSize", boost::lexical_cast<std::string>(PLEX_DEFAULT_PAGE_SIZE));
  m_pagingOffset = PLEX_DEFAULT_PAGE_SIZE - 1;

  if (u.GetProtocol() == "plexserver" &&
      (u.GetHostName() != "channels" && u.GetHostName() != "shared" && u.GetHostName() != "channeldirectory"))
  {
    if (!XFILE::CPlexFile::CanBeTranslated(u))
    {
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(52300), g_localizeStrings.Get(52301));
      g_windowManager.ActivateWindow(WINDOW_HOME);
      return false;
    }
  }
  
  bool ret = CGUIMediaWindow::GetDirectory(u.Get(), items);

  CPlexServerPtr server = g_plexApplication.serverManager->FindByUUID(u.GetHostName());
  if (server && server->GetActiveConnection() && server->GetActiveConnection()->IsLocal())
    g_directoryCache.ClearDirectory(u.Get());
  
  if (items.HasProperty("totalSize"))
  {
    if (items.GetProperty("totalSize").asInteger() > PLEX_DEFAULT_PAGE_SIZE)
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
      
      for (int i=0; i < (items.GetProperty("totalSize").asInteger()) - PLEX_DEFAULT_PAGE_SIZE; i++)
      {
        CFileItemPtr item = CFileItemPtr(new CFileItem);
        item->SetPath(boost::lexical_cast<std::string>(i));
        if (charMap.find(PLEX_DEFAULT_PAGE_SIZE + i) != charMap.end())
          item->SetSortLabel(CStdString(charMap[PLEX_DEFAULT_PAGE_SIZE + i]));
        items.Add(item);
      }
    }
  }
  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexMediaWindow::LoadPage(int start, int numberOfItems)
{
  if (start >= m_vecItems->GetProperty("totalSize").asInteger())
    return;
  if (m_currentJobId != -1)
  {
    CJobManager::GetInstance().CancelJob(m_currentJobId);
    m_currentJobId = -1;
  }
  
  CURL u(m_vecItems->GetPath());
  
  int pageSize = XMIN(numberOfItems, m_vecItems->GetProperty("totalSize").asInteger() - start);
  
  u.SetProtocolOption("containerStart", boost::lexical_cast<std::string>(start));
  u.SetProtocolOption("containerSize", boost::lexical_cast<std::string>(pageSize));
  
  CLog::Log(LOGDEBUG, "CGUIPlexMediaWindow::LoadPage loading %d to %d", start, start+pageSize);
  
  m_currentJobId = CJobManager::GetInstance().AddJob(new CPlexDirectoryFetchJob(u), this);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexMediaWindow::LoadNextPage()
{
  if (m_vecItems->HasProperty("totalSize"))
  {
    if (m_vecItems->GetProperty("totalSize").asInteger() > m_pagingOffset)
    {
      LoadPage(m_pagingOffset, PLEX_DEFAULT_PAGE_SIZE);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexMediaWindow::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CPlexDirectoryFetchJob *fjob = static_cast<CPlexDirectoryFetchJob*>(job);
  if (!fjob)
    return;

  if (success)
  {    
    int nItem = m_viewControl.GetSelectedItem();
    CStdString strSelected;
    if (nItem >= 0)
      strSelected = m_vecItems->Get(nItem)->GetPath();
    
    int itemsToRemove = fjob->m_items.Size();
    for (int i = 0; i < itemsToRemove; i ++)
      m_vecItems->Remove(m_pagingOffset);
    
    for (int i = 0; i < fjob->m_items.Size(); i ++)
      m_vecItems->Insert(m_pagingOffset + i, fjob->m_items.Get(i));
    
    m_pagingOffset += fjob->m_items.Size();
    m_viewControl.SetItems(*m_vecItems);
    m_viewControl.SetSelectedItem(strSelected);
  }
  
  m_currentJobId = -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::OnSelect(int iItem)
{
  CFileItemPtr item = m_vecItems->Get(iItem);
  if (!item)
    return false;

  if (!item->m_bIsFolder)
  {
    if (!PlexUtils::CurrentSkinHasPreplay() || item->GetPlexDirectoryType() == PLEX_DIR_TYPE_TRACK || item->GetPlexDirectoryType() == PLEX_DIR_TYPE_PHOTO)
      return OnPlayMedia(iItem);
  }

  CStdString newUrl = m_navHelper.navigateToItem(item, m_vecItems->GetPath(), GetID());

  if (item->m_bIsFolder && !newUrl.empty())
  {
    CURL u(m_vecItems->GetPath());
    m_lastSelectedIndex[u.GetUrlWithoutOptions()] = iItem;

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

  if (IsMusicContainer())
    QueueItems(*m_vecItems, item);
  else if (IsPhotoContainer())
    CApplicationMessenger::Get().PictureSlideShow(m_vecItems->GetPath(), false, item->GetPath());
  else
    PlexContentPlayerMixin::PlayPlexItem(item);

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexMediaWindow::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (!item)
    return;

  int currentPlaylist = ContainerPlaylistType();

  if (currentPlaylist != PLAYLIST_NONE)
  {
    if (g_playlistPlayer.GetPlaylist(currentPlaylist).size() > 0)
    {
      buttons.Add(CONTEXT_BUTTON_NOW_PLAYING, 13350);
    }
  }

  if (item->CanQueue())
  {
    buttons.Add(CONTEXT_BUTTON_SHUFFLE, 191);
    buttons.Add(CONTEXT_BUTTON_QUEUE_ITEM, 13347);
  }

  if (IsVideoContainer() && item->IsPlexMediaServerLibrary())
  {
    CStdString viewOffset = item->GetProperty("viewOffset").asString();

    if (item->GetVideoInfoTag()->m_playCount > 0 || viewOffset.size() > 0)
      buttons.Add(CONTEXT_BUTTON_MARK_UNWATCHED, 16104);
    if (item->GetVideoInfoTag()->m_playCount == 0 || viewOffset.size() > 0)
      buttons.Add(CONTEXT_BUTTON_MARK_WATCHED, 16103);
  }

  EPlexDirectoryType dirType = item->GetPlexDirectoryType();

  if (item->IsPlexMediaServerLibrary() &&
      (item->IsRemoteSharedPlexMediaServerLibrary() == false) &&
      (dirType == PLEX_DIR_TYPE_EPISODE || dirType == PLEX_DIR_TYPE_MOVIE ||
       dirType == PLEX_DIR_TYPE_VIDEO || dirType == PLEX_DIR_TYPE_TRACK))
  {
    CPlexServerPtr server = g_plexApplication.serverManager->FindByUUID(item->GetProperty("plexserver").asString());
    if (server && server->SupportsDeletion())
      buttons.Add(CONTEXT_BUTTON_DELETE, 15015);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (!item)
    return false;

  switch(button)
  {
    case CONTEXT_BUTTON_NOW_PLAYING:
    {
      if (IsVideoContainer() && g_application.IsPlayingVideo())
        g_windowManager.ActivateWindow(WINDOW_FULLSCREEN_VIDEO);
      else if (IsVideoContainer() && g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO).size() > 0)
        CApplicationMessenger::Get().MediaPlay(PLAYLIST_VIDEO);
      else if (IsMusicContainer() && g_application.IsPlayingAudio())
        g_windowManager.ActivateWindow(WINDOW_NOW_PLAYING);
       break;
    }
    case CONTEXT_BUTTON_SHUFFLE:
      ShuffleItem(item);
      break;

    case CONTEXT_BUTTON_QUEUE_ITEM:
      QueueItem(item);
      break;

    case CONTEXT_BUTTON_MARK_WATCHED:
    case CONTEXT_BUTTON_MARK_UNWATCHED:
    {
      CPlexSectionFilterPtr filter = g_plexApplication.filterManager->getFilterForSection(m_sectionRoot.Get());
      bool reload = filter->needRefreshOnStateChange();

      if (button == CONTEXT_BUTTON_MARK_WATCHED)
        item->MarkAsWatched(reload);
      else
        item->MarkAsUnWatched(reload);

      g_directoryCache.ClearSubPaths(m_vecItems->GetPath());
      break;
    }

    case CONTEXT_BUTTON_DELETE:
      g_plexApplication.mediaServerClient->deleteItem(item);
      g_directoryCache.ClearSubPaths(m_vecItems->GetPath());
      break;

    default:
      break;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexMediaWindow::ShuffleItem(CFileItemPtr item)
{
  int currentPlaylist = ContainerPlaylistType();
  if (currentPlaylist == PLAYLIST_NONE)
    return;

  CApplicationMessenger &appMsg = CApplicationMessenger::Get();

  appMsg.MediaStop();
  appMsg.PlayListPlayerClear(currentPlaylist);

  if (!item->m_bIsFolder)
    appMsg.PlayListPlayerAdd(currentPlaylist, *m_vecItems);
  else
  {
    XFILE::CPlexDirectory dir;
    CFileItemList list;
    dir.GetDirectory(item->GetPath(), list);
    appMsg.PlayListPlayerAdd(currentPlaylist, list);
  }

  appMsg.PlayListPlayerShuffle(currentPlaylist, true);
  appMsg.MediaPlay(currentPlaylist);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexMediaWindow::QueueItem(CFileItemPtr item)
{
  int currentPlaylist = ContainerPlaylistType();
  if (currentPlaylist == PLAYLIST_NONE)
    return;

  CApplicationMessenger &appMsg = CApplicationMessenger::Get();

  if ((IsVideoContainer() && !g_application.IsPlayingVideo()) ||
      (IsMusicContainer() && !g_application.IsPlayingAudio()))
    appMsg.PlayListPlayerClear(currentPlaylist);

  if(item->m_bIsFolder)
  {
    XFILE::CPlexDirectory dir;
    CFileItemList list;
    dir.GetDirectory(item->GetPath(), list);
    appMsg.PlayListPlayerAdd(currentPlaylist, list);
  }
  else
  {
    appMsg.PlayListPlayerAdd(currentPlaylist, *item.get());
  }

  if ((IsVideoContainer() && !g_application.IsPlayingVideo()) ||
      (IsMusicContainer() && !g_application.IsPlayingAudio()))
  {
    appMsg.MediaStop(true);
    appMsg.MediaPlay(currentPlaylist);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexMediaWindow::QueueItems(const CFileItemList &list, CFileItemPtr startItem)
{
  int currentPlaylist = ContainerPlaylistType();
  if (currentPlaylist == PLAYLIST_NONE)
    return;

  CApplicationMessenger &appMsg = CApplicationMessenger::Get();
  appMsg.PlayListPlayerClear(currentPlaylist);
  appMsg.PlayListPlayerAdd(currentPlaylist, list);
  appMsg.MediaStop(true);

  bool found = false;
  int idx = 0;
  if (startItem)
  {
    for (; idx < list.Size(); idx++)
    {
      if (list.Get(idx)->GetPath() == startItem->GetPath())
      {
        found = true;
        break;
      }
    }
  }
  appMsg.MediaPlay(currentPlaylist, found ? idx : 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::Update(const CStdString &strDirectory, bool updateFilterPath)
{
  return Update(strDirectory, updateFilterPath, false);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::Update(const CStdString &strDirectory, bool updateFilterPath, bool updateFromFilter)
{
  CURL newUrl = GetRealDirectoryUrl(strDirectory);
  if (newUrl.Get().empty())
    return false;

  if (strDirectory == m_startDirectory)
  {
    m_sectionRoot = strDirectory;
    g_plexApplication.filterManager->loadFilterForSection(m_sectionRoot.Get());
  }

  CPlexSectionFilterPtr filter = g_plexApplication.filterManager->getFilterForSection(m_sectionRoot.Get());
  if (filter && boost::ends_with(newUrl.GetFileName(), filter->currentPrimaryFilter()))
    m_startDirectory = newUrl.GetUrlWithoutOptions();

  if (updateFromFilter)
    m_history.RemoveParentPath();

  bool ret = CGUIMediaWindow::Update(newUrl.Get(), updateFilterPath);
  if (!updateFromFilter)
    g_plexApplication.themeMusicPlayer->playForItem(*m_vecItems);

  UpdateSectionTitle();

  /* try to restore section a bit better */
  int idx = 0;
  CURL u(m_vecItems->GetPath());
  if (m_lastSelectedIndex.find(u.GetUrlWithoutOptions()) != m_lastSelectedIndex.end())
    idx = m_lastSelectedIndex[u.GetUrlWithoutOptions()];

  if (m_viewControl.GetSelectedItem() == 0 && idx != 0)
    m_viewControl.SetSelectedItem(idx);

  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexMediaWindow::CheckPlexFilters(CFileItemList &list)
{
  CPlexSectionFilterPtr filter = g_plexApplication.filterManager->getFilterForSection(m_sectionRoot.Get());

  list.SetProperty("hasAdvancedFilters", (filter && filter->hasAdvancedFilters()) ? "yes" : "");
  list.SetProperty("primaryFilterActivated", (filter && filter->secondaryFiltersActivated()) ? "" : "yes");

  CFileItemPtr section = g_plexApplication.dataLoader->GetSection(m_sectionRoot);
  if (section && section->GetPlexDirectoryType() == PLEX_DIR_TYPE_HOME_MOVIES)
    list.SetContent("homemovies");

  if (filter->currentPrimaryFilter() == "folder")
    list.SetContent("folders");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::IsVideoContainer(CFileItemPtr item) const
{
  EPlexDirectoryType dirType = m_vecItems->GetPlexDirectoryType();

  if (dirType == PLEX_DIR_TYPE_CHANNEL)
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
  if (dirType == PLEX_DIR_TYPE_CHANNEL)
    dirType = m_vecItems->Get(0)->GetPlexDirectoryType();
  return (dirType == PLEX_DIR_TYPE_ALBUM || dirType == PLEX_DIR_TYPE_ARTIST || dirType == PLEX_DIR_TYPE_TRACK);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::IsPhotoContainer() const
{
  EPlexDirectoryType dirType = m_vecItems->GetPlexDirectoryType();

  if (dirType == PLEX_DIR_TYPE_CHANNEL)
    dirType = m_vecItems->Get(0)->GetPlexDirectoryType();

  return (dirType == PLEX_DIR_TYPE_PHOTOALBUM | dirType == PLEX_DIR_TYPE_PHOTO);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::OnBack(int actionID)
{
  CURL currPath(m_vecItems->GetPath());

  CURL parent(m_history.GetParentPath());
  if (!parent.Get().empty())
  {
    m_history.RemoveParentPath();

    while (parent.GetUrlWithoutOptions() == currPath.GetUrlWithoutOptions())
    {
      parent = CURL(m_history.GetParentPath());
      m_history.RemoveParentPath();
    }
  }

  if (currPath.GetUrlWithoutOptions() == m_startDirectory || parent.Get().empty())
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
  CFileItemList tmpItems;
  XFILE::CPlexDirectory dir;
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

    CPlexSectionFilterPtr filter = g_plexApplication.filterManager->getFilterForSection(url.Get());
    if (filter)
      url = filter->addFiltersToUrl(url);
    else
      PlexUtils::AppendPathToURL(url, "all");

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

  CPlexSectionFilterPtr sectionFilter = g_plexApplication.filterManager->getFilterForSection(m_sectionRoot.Get());
  CGUIPlexFilterFactory factory(this);

  if (!sectionFilter)
    return;

  m_hasAdvancedFilters = sectionFilter->hasAdvancedFilters();
  m_vecItems->SetProperty("hasAdvancedFilters", m_hasAdvancedFilters ? "yes" : "");

  CGUIControlGroupList *primaryFilters = (CGUIControlGroupList*)GetControl(FILTER_PRIMARY_CONTAINER);
  if (primaryFilters)
  {
    primaryFilters->ClearAll();

    PlexStringPairVector pfilterLabel = sectionFilter->getPrimaryFilters();
    int id = FILTER_PRIMARY_BUTTONS_START;
    BOOST_FOREACH(PlexStringPair p, pfilterLabel)
    {
      CGUIButtonControl *button = factory.getPrimaryFilterButton(p.second);
      if (button)
      {
        button->SetID(id ++);

        primaryFilters->AddControl(button);
        if (p.first == sectionFilter->currentPrimaryFilter())
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
      BOOST_FOREACH(CPlexSecondaryFilterPtr filter, sectionFilter->getSecondaryFilters())
      {
        CGUIButtonControl *button = factory.getSecondaryFilterButton(filter);
        if (button)
        {
          button->SetID(id ++);

          if (!sectionFilter->secondaryFiltersActivated())
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

      PlexStringPairVector sorts = sectionFilter->getSortOrders();

      if (sorts.size() > 0)
      {
        SET_CONTROL_VISIBLE(SORT_LIST);
        SET_CONTROL_VISIBLE(SORT_LABEL);

        int id = SORT_BUTTONS_START;
        BOOST_FOREACH(PlexStringPair p, sorts)
        {
          CGUIFilterOrderButtonControl::FilterOrderButtonState state = CGUIFilterOrderButtonControl::OFF;

          if (p.first == sectionFilter->currentSortOrder())
            state = sectionFilter->currentSortOrderAscending() ? CGUIFilterOrderButtonControl::ASCENDING : CGUIFilterOrderButtonControl::DESCENDING;

          CGUIFilterOrderButtonControl* button = factory.getSortButton(p.second, state);
          if (button)
          {
            button->SetID(id ++);

            if (!sectionFilter->secondaryFiltersActivated())
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
}
