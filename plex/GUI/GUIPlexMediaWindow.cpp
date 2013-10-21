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

#include "LocalizeStrings.h"

#define DEFAULT_PAGE_SIZE 50
#define XMIN(a,b) ((a)<(b)?(a):(b))

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::OnMessage(CGUIMessage &message)
{
  bool ret = CGUIMediaWindow::OnMessage(message);

  switch(message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      bool update = false;
      int ctrlId = message.GetSenderId();
      if (ctrlId < 0 && ctrlId >= FILTER_BUTTONS_START)
        update = m_filterHelper.ApplyFilter(ctrlId);
      else if (ctrlId < FILTER_BUTTONS_START && ctrlId >= SORT_BUTTONS_START)
        update = m_filterHelper.ApplySort(ctrlId);

      if (update)
        Update(m_filterHelper.GetSectionUrl().Get(), true, false);
    }
      break;

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
        Update(m_filterHelper.GetSectionUrl().Get(), true);
      else
        BuildFilter(m_filterHelper.GetSectionUrl());
      m_returningFromSkinLoad = false;

      CGUILabelControl *lbl = (CGUILabelControl*)GetControl(FILTER_LABEL);
      if (lbl)
        lbl->SetLabel(g_localizeStrings.Get(44030));

      lbl = (CGUILabelControl*)GetControl(SORT_LABEL);
      if (lbl)
        lbl->SetLabel(g_localizeStrings.Get(44031));

      break;
    }

    case GUI_MSG_UPDATE_FILTERS:
    {
      Update(m_filterHelper.GetSectionUrl().Get(), true, false);
      break;
    }

    case GUI_MSG_ITEM_SELECT:
    {
      int currentIdx = m_viewControl.GetSelectedItem();
      if (currentIdx > m_pagingOffset && m_currentJobId == -1)
      {
        /* the user selected something in the middle of where we loaded, let's just cheat and fill in everything */
        LoadPage(m_pagingOffset, currentIdx + DEFAULT_PAGE_SIZE);
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
    }
      
  }

  return ret;
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
      LoadPage(m_pagingOffset, m_viewControl.GetSelectedItem() + DEFAULT_PAGE_SIZE);
    else if (m_viewControl.GetSelectedItem() >= (m_pagingOffset - (DEFAULT_PAGE_SIZE/2)))
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

  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  CURL u(strDirectory);
  u.SetProtocolOption("containerStart", "0");
  u.SetProtocolOption("containerSize", boost::lexical_cast<std::string>(DEFAULT_PAGE_SIZE));
  m_pagingOffset = DEFAULT_PAGE_SIZE - 1;

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
  
  if (items.HasProperty("totalSize"))
  {
    if (items.GetProperty("totalSize").asInteger() > DEFAULT_PAGE_SIZE)
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
      
      for (int i=0; i < (items.GetProperty("totalSize").asInteger()) - DEFAULT_PAGE_SIZE; i++)
      {
        CFileItemPtr item = CFileItemPtr(new CFileItem);
        item->SetPath(boost::lexical_cast<std::string>(i));
        if (charMap.find(DEFAULT_PAGE_SIZE + i) != charMap.end())
          item->SetSortLabel(CStdString(charMap[DEFAULT_PAGE_SIZE + i]));
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
      LoadPage(m_pagingOffset, DEFAULT_PAGE_SIZE);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexMediaWindow::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  if (success)
  {
    CPlexDirectoryFetchJob *fjob = static_cast<CPlexDirectoryFetchJob*>(job);
    
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

  if (!item->m_bIsFolder && PlexUtils::CurrentSkinHasPreplay() &&
      item->IsPlexMediaServer() &&
      (item->GetPlexDirectoryType() == PLEX_DIR_TYPE_MOVIE ||
       item->GetPlexDirectoryType() == PLEX_DIR_TYPE_EPISODE))
  {
    CBuiltins::Execute("ActivateWindow(PlexPreplayVideo, " + item->GetProperty("key").asString() + ", return)");
    return true;
  }

  if (item->m_bIsFolder)
  {
    CStdString newPath;
    if (item->GetProperty("search").asBoolean())
      newPath = ShowPluginSearch(item);
    else if (item->GetProperty("settings").asBoolean())
      newPath = ShowPluginSettings(item);
    else
      newPath = item->GetPath();

    if (!newPath.empty())
      if (!Update(newPath, true))
        ShowShareErrorMessage(item.get());
    return true;
  }

  return OnPlayMedia(iItem);
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

    case CONTEXT_BUTTON_MARK_UNWATCHED:
      item->MarkAsUnWatched(true);
      break;

    case CONTEXT_BUTTON_MARK_WATCHED:
      item->MarkAsWatched(true);
      break;

    case CONTEXT_BUTTON_DELETE:
      g_plexApplication.mediaServerClient->deleteItem(item);
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
void CGUIPlexMediaWindow::BuildFilter(const CURL& strDirectory)
{
  if (strDirectory.Get().empty())
    return;

  m_filterHelper.BuildFilters(strDirectory, m_vecItems->GetPlexDirectoryType());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::Update(const CStdString &strDirectory, bool updateFilterPath)
{
  return Update(strDirectory, updateFilterPath, true);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::Update(const CStdString &strDirectory, bool updateFilterPath, bool updateFilters)
{
  bool isSecondary = false;
  
  CURL newUrl = m_filterHelper.GetRealDirectoryUrl(strDirectory, isSecondary);
  if (newUrl.Get().empty())
    return false;
  
  CURL oldUrl = CURL(m_vecItems->GetPath());

  if (strDirectory == m_startDirectory)
    m_startDirectory = newUrl.GetUrlWithoutOptions();

  CLog::Log(LOGDEBUG, "CGUIPlexMediaWindow::Update(%s)->%s", strDirectory.c_str(), newUrl.Get().c_str());

  bool ret = CGUIMediaWindow::Update(newUrl.Get(), updateFilterPath);

  if (isSecondary && updateFilters)
    BuildFilter(m_filterHelper.GetSectionUrl());

  g_plexApplication.themeMusicPlayer->playForItem(*m_vecItems);

  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::IsVideoContainer(CFileItemPtr item) const
{
  EPlexDirectoryType dirType = m_vecItems->GetPlexDirectoryType();

  if (dirType == PLEX_DIR_TYPE_DIRECTORY && item)
    dirType = item->GetPlexDirectoryType();

  return (dirType == PLEX_DIR_TYPE_MOVIE ||
          dirType == PLEX_DIR_TYPE_SHOW ||
          dirType == PLEX_DIR_TYPE_SEASON ||
          dirType == PLEX_DIR_TYPE_PLAYLIST ||
          dirType == PLEX_DIR_TYPE_EPISODE ||
          dirType == PLEX_DIR_TYPE_VIDEO ||
          dirType == PLEX_DIR_TYPE_CLIP);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::IsMusicContainer() const
{
  EPlexDirectoryType dirType = m_vecItems->GetPlexDirectoryType();
  return (dirType == PLEX_DIR_TYPE_ALBUM || dirType == PLEX_DIR_TYPE_ARTIST || dirType == PLEX_DIR_TYPE_TRACK);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexMediaWindow::IsPhotoContainer() const
{
  EPlexDirectoryType dirType = m_vecItems->GetPlexDirectoryType();
  return (dirType == PLEX_DIR_TYPE_PHOTOALBUM | dirType == PLEX_DIR_TYPE_PHOTO);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CStdString CGUIPlexMediaWindow::ShowPluginSearch(CFileItemPtr item)
{
  CStdString strSearchTerm = "";
  if (CGUIKeyboardFactory::ShowAndGetInput(strSearchTerm, item->GetProperty("prompt").asString(), false))
  {
    // Encode the query.
    CURL::Encode(strSearchTerm);

    // Find the ? if there is one.
    CURL u(item->GetPath());
    u.SetOption("query", strSearchTerm);
    return u.Get();
  }
  return CStdString();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CStdString CGUIPlexMediaWindow::ShowPluginSettings(CFileItemPtr item)
{
  CFileItemList fileItems;
  std::vector<CStdString> items;
  XFILE::CPlexDirectory plexDir;

  plexDir.GetDirectory(item->GetPath(), fileItems);
  CGUIDialogPlexPluginSettings::ShowAndGetInput(item->GetPath(), plexDir.GetData());
  return m_vecItems->GetPath();
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
    m_filterHelper.ClearFilters();
    g_windowManager.PreviousWindow();
    return true;
  }

  Update(parent.Get(), true);

  return true;
}
