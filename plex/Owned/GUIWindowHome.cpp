/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <list>
#include <map>
#include <vector>

#include "filesystem/File.h"
#include "FileItem.h"
#include "GUIBaseContainer.h"
#include "GUIStaticItem.h"
#include "GUIWindowHome.h"
#include "GUI/GUIDialogTimer.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "MediaSource.h"
#include "AlarmClock.h"
#include "Key.h"

#include "MyPlexManager.h"
#include "PlexDirectory.h"
#include "PlexSourceScanner.h"
#include "PlexLibrarySectionManager.h"
#include "threads/SingleLock.h"
#include "PlexUtils.h"
#include "video/VideoInfoTag.h"

#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogVideoInfo.h"
#include "dialogs/GUIDialogOK.h"

#include "plex/PlexMediaServerQueue.h"

#include "powermanagement/PowerManager.h"

#include "ApplicationMessenger.h"

#include "AdvancedSettings.h"

#include "Job.h"
#include "JobManager.h"

#include "BackgroundMusicPlayer.h"

#include "interfaces/Builtins.h"

using namespace std;
using namespace XFILE;
using namespace boost;

#define MAIN_MENU         300 // THIS WAS 300 for Plex skin.
#define POWER_MENU        407

#define QUIT_ITEM          111
#define SLEEP_ITEM         112
#define SHUTDOWN_ITEM      113
#define SLEEP_DISPLAY_ITEM 114

#define CHANNELS_VIDEO 1
#define CHANNELS_MUSIC 2
#define CHANNELS_PHOTO 3
#define CHANNELS_APPLICATION 4

#define SLIDESHOW_MULTIIMAGE 10101


//////////////////////////////////////////////////////////////////////////////
CPlexSectionFanout::CPlexSectionFanout(const CStdString &url, int sectionType)
  : m_url(url), m_sectionType(sectionType)
{
  Refresh();
}

//////////////////////////////////////////////////////////////////////////////
CFileItemListPtr CPlexSectionFanout::GetContentList(int type)
{
  CSingleLock lk(m_critical);
  return m_fileLists[type];
}

//////////////////////////////////////////////////////////////////////////////
std::vector<contentListPair> CPlexSectionFanout::GetContentLists()
{
  CSingleLock lk(m_critical);
  std::vector<contentListPair> ret;
  BOOST_FOREACH(contentListPair p, m_fileLists)
    ret.push_back(p);

  return ret;
}

//////////////////////////////////////////////////////////////////////////////
int CPlexSectionFanout::LoadSection(const CStdString& url, int contentType)
{
  CPlexSectionLoadJob* job = new CPlexSectionLoadJob(url, contentType);
  return CJobManager::GetInstance().AddJob(job, this, CJob::PRIORITY_HIGH);
}

//////////////////////////////////////////////////////////////////////////////
CStdString CPlexSectionFanout::GetBestServerUrl(const CStdString& extraUrl)
{
  CStdString bestServerUrl;
  PlexServerPtr server = PlexServerManager::Get().bestServer();
  if (server)
  {
    if (!extraUrl.empty())
      bestServerUrl = PlexUtils::AppendPathToURL(server->url(), extraUrl);
    else
      bestServerUrl = server->url();

    if (!server->token.empty())
    {
      CURL urlWithToken(bestServerUrl);
      urlWithToken.SetOption("X-Plex-Token", server->token);
      return urlWithToken.Get();
    }

    return bestServerUrl;
  }

  bestServerUrl = "http://127.0.0.1:32400/";
  if (!extraUrl.empty())
    bestServerUrl = PlexUtils::AppendPathToURL(bestServerUrl, extraUrl);

  return bestServerUrl;
}

//////////////////////////////////////////////////////////////////////////////
void CPlexSectionFanout::Refresh()
{
  CPlexDirectory dir(true, false);

  CSingleLock lk(m_critical);
  m_fileLists.clear();

  CLog::Log(LOGDEBUG, "GUIWindowHome:SectionFanout:Refresh for %s", m_url.c_str());

  CURL trueUrl(m_url);

  if (trueUrl.GetProtocol() == "channel")
  {
    if (!g_advancedSettings.m_bHideFanouts)
    {
      CStdString filter = "channels/recentlyViewed?filter=" + trueUrl.GetHostName();
      LoadSection(GetBestServerUrl(filter), CONTENT_LIST_RECENTLY_ACCESSED);
    }

    /* We always show this as fanart */
    LoadSection(GetBestServerUrl("channels/arts"), CONTENT_LIST_FANART);
  }
  else if (trueUrl.GetProtocol() == "global")
  {
    if (g_guiSettings.GetBool("lookandfeel.enableglobalslideshow"))
      LoadSection(GetBestServerUrl("library/arts"), CONTENT_LIST_FANART);
  }
  else if (m_sectionType == PLEX_METADATA_MIXED)
  {
    if (!g_advancedSettings.m_bHideFanouts)
    {
      if (m_url.Find("queue") != -1)
        LoadSection(MyPlexManager::Get().getPlaylistUrl("/queue/unwatched"), CONTENT_LIST_QUEUE);
      else if (m_url.Find("recommendations") != -1)
        LoadSection(MyPlexManager::Get().getPlaylistUrl("/recommendations/unwatched"), CONTENT_LIST_QUEUE);
    }
  }
  else
  {
    if (!g_advancedSettings.m_bHideFanouts)
    {
      /* On slow/limited systems we don't want to have the full list */
#if defined(TARGET_RPI) || defined(TARGET_DARWIN_IOS)
      trueUrl.SetOption("X-Plex-Container-Start", "0");
      trueUrl.SetOption("X-Plex-Container-Size", "20");
#endif
      
      trueUrl.SetOption("unwatched", "1");
      trueUrl.SetFileName(PlexUtils::AppendPathToURL(trueUrl.GetFileName(), "recentlyAdded"));
      
      m_outstandingJobs.push_back(LoadSection(trueUrl.Get(), CONTENT_LIST_RECENTLY_ADDED));

      if (m_sectionType == PLEX_METADATA_MOVIE || m_sectionType == PLEX_METADATA_SHOW)
      {
        trueUrl = CURL(m_url);
        trueUrl.SetFileName(PlexUtils::AppendPathToURL(trueUrl.GetFileName(), "onDeck"));
        m_outstandingJobs.push_back(LoadSection(trueUrl.Get(), CONTENT_LIST_ON_DECK));
      }
    }

    /* We don't want to wait on the fanart, so don't add it to the outstandingjobs map */
    if (g_guiSettings.GetBool("lookandfeel.enableglobalslideshow"))
      LoadSection(PlexUtils::AppendPathToURL(m_url, "arts"), CONTENT_LIST_FANART);
  }
}

//////////////////////////////////////////////////////////////////////////////
void CPlexSectionFanout::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CPlexSectionLoadJob *load = (CPlexSectionLoadJob*)job;
  if (success)
  {
    m_fileLists[load->GetContentType()] = load->GetFileItemList();
    
    /* Pre-cache stuff */
    if (load->GetContentType() != CONTENT_LIST_FANART)
    {
      m_videoThumb.Load(*m_fileLists[load->GetContentType()].get());
    }
  }

  m_age.restart();

  vector<int>::iterator it = std::find(m_outstandingJobs.begin(), m_outstandingJobs.end(), jobID);
  if (it != m_outstandingJobs.end())
    m_outstandingJobs.erase(it);

  if (m_outstandingJobs.size() == 0 && load->GetContentType() != CONTENT_LIST_FANART)
  {
    CGUIMessage msg(GUI_MSG_PLEX_SECTION_LOADED, WINDOW_HOME, 300, m_sectionType);
    msg.SetStringParam(m_url);
    g_windowManager.SendThreadMessage(msg);
  }
  else if (load->GetContentType() == CONTENT_LIST_FANART)
  {
    CGUIMessage msg(GUI_MSG_PLEX_SECTION_LOADED, WINDOW_HOME, 300, CONTENT_LIST_FANART);
    msg.SetStringParam(m_url);
    g_windowManager.SendThreadMessage(msg);
  }
}

//////////////////////////////////////////////////////////////////////////////
void CPlexSectionFanout::Show()
{
  if (NeedsRefresh())
    Refresh();
  else
  {
    /* we are up to date, just send the messages */
    CGUIMessage msg(GUI_MSG_PLEX_SECTION_LOADED, WINDOW_HOME, 300, m_sectionType);
    msg.SetStringParam(m_url);
    g_windowManager.SendThreadMessage(msg);

    CGUIMessage msg2(GUI_MSG_PLEX_SECTION_LOADED, WINDOW_HOME, 300, CONTENT_LIST_FANART);
    msg2.SetStringParam(m_url);
    g_windowManager.SendThreadMessage(msg2);
  }
}

//////////////////////////////////////////////////////////////////////////////
bool CPlexSectionFanout::NeedsRefresh()
{
  int refreshTime = 5;
  if (m_sectionType == PLEX_METADATA_ALBUM ||
      m_sectionType == PLEX_METADATA_MIXED ||
      (m_sectionType >= PLEX_METADATA_CHANNEL_VIDEO &&
       m_sectionType <= PLEX_METADATA_CHANNEL_APPLICATION))
    refreshTime = 20;

  if (m_sectionType == PLEX_METADATA_GLOBAL_IMAGES)
    refreshTime = 100;

  CLog::Log(LOGDEBUG, "GUIWindowHome:SectionFanout:NeedsRefresh %s, age %f, refresh %s", m_url.c_str(), m_age.elapsed(), m_age.elapsed() > refreshTime ? "yes" : "no");
  return m_age.elapsed() > refreshTime;
}

//////////////////////////////////////////////////////////////////////////////
CGUIWindowHome::CGUIWindowHome(void) : CGUIWindow(WINDOW_HOME, "Home.xml"), m_globalArt(false), m_lastSelectedItem("Search")
{
  if (g_advancedSettings.m_iShowFirstRun != 77)
  {
    m_auxLoadingThread = new CAuxFanLoadThread();
    m_auxLoadingThread->Create();
  }
  AddSection("global://art", PLEX_METADATA_GLOBAL_IMAGES);
}

//////////////////////////////////////////////////////////////////////////////
bool CGUIWindowHome::OnAction(const CAction &action)
{
  /* > 9000 is the fans and 506 is preferences */
  if ((action.GetID() == ACTION_PREVIOUS_MENU || action.GetID() == ACTION_NAV_BACK) &&
      (GetFocusedControlID() > 9000 || GetFocusedControlID() == 506))
  {
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), 300);
    OnMessage(msg);

    return true;
  }
  
  if (action.GetID() == ACTION_CONTEXT_MENU)
  {
    return OnPopupMenu();
  }
  else if (action.GetID() == ACTION_PREVIOUS_MENU || action.GetID() == ACTION_PARENT_DIR)
  {
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), 300);
    OnMessage(msg);
    
    return true;
  }
  
  bool ret = CGUIWindow::OnAction(action);
  
  // See what's focused.
  if (GetFocusedControl() && GetFocusedControl()->GetID() == MAIN_MENU)
  {
    CGUIBaseContainer* pControl = (CGUIBaseContainer*)GetFocusedControl();
    if (pControl)
    {
      CGUIListItemPtr pItem = pControl->GetListItem(0);
      if (pItem)
      {
        m_lastSelectedItem = GetCurrentItemName();
        if (!ShowSection(pItem->GetProperty("sectionPath").asString()) && !m_globalArt)
        {
          HideAllLists();
          ShowSection("global://art");
        }
      }
    }
  }
  
  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CFileItemPtr CGUIWindowHome::GetCurrentListItem(int offset)
{
  CGUIBaseContainer* pControl = (CGUIBaseContainer* )GetControl(MAIN_MENU);
  if (pControl)
    return boost::static_pointer_cast<CFileItem>(pControl->GetListItem(offset));
  return CFileItemPtr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CFileItem* CGUIWindowHome::GetCurrentFileItem()
{
  CFileItemPtr listItem = GetCurrentListItem();
  if (listItem && listItem->IsFileItem())
    return (CFileItem*)listItem.get();
  return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowHome::OnPopupMenu()
{
  if (!GetFocusedControl())
    return false;
  
  int controlId = GetFocusedControl()->GetID();
  if (controlId == MAIN_MENU || controlId == POWER_MENU)
  {
    CContextButtons buttons;
    buttons.Add(CONTEXT_BUTTON_QUIT, 13009);

    if(g_powerManager.CanSuspend())
      buttons.Add(CONTEXT_BUTTON_SLEEP, 13011);

    if (g_powerManager.CanPowerdown())
      buttons.Add(CONTEXT_BUTTON_SHUTDOWN, 13005);

    int choice = CGUIDialogContextMenu::ShowAndGetChoice(buttons);

    if (choice == CONTEXT_BUTTON_SLEEP)
      CApplicationMessenger::Get().Suspend();

    if (choice == CONTEXT_BUTTON_QUIT)
      CApplicationMessenger::Get().Quit();

    if (choice == CONTEXT_BUTTON_SHUTDOWN)
      CApplicationMessenger::Get().Shutdown();

  }
  else if (controlId == CONTENT_LIST_ON_DECK ||
           controlId == CONTENT_LIST_RECENTLY_ADDED ||
           controlId == CONTENT_LIST_QUEUE ||
           controlId == CONTENT_LIST_RECOMMENDATIONS)
  {
    CGUIBaseContainer *container = (CGUIBaseContainer*)GetControl(controlId);
    CGUIListItemPtr item = container->GetListItem(0);
    if (item->IsFileItem())
    {
      bool updateFanOut = false;
      CFileItemPtr fileItem = boost::static_pointer_cast<CFileItem>(item);
      CContextButtons buttons;
      int type = (int)fileItem->GetProperty("typeNumber").asInteger();

      if (type == PLEX_METADATA_EPISODE)
        buttons.Add(CONTEXT_BUTTON_INFO, 20352);
      else if (type == PLEX_METADATA_MOVIE)
        buttons.Add(CONTEXT_BUTTON_INFO, 13346);

      if (((fileItem->IsRemoteSharedPlexMediaServerLibrary() == false) &&
          (fileItem->GetProperty("HasWatchedState").asBoolean() == true)) ||
          fileItem->HasProperty("ratingKey"))
      {
        CStdString viewOffset = item->GetProperty("viewOffset").asString();

        if (fileItem->GetVideoInfoTag()->m_playCount > 0 || viewOffset.size() > 0)
          buttons.Add(CONTEXT_BUTTON_MARK_UNWATCHED, 16104);
        if (fileItem->GetVideoInfoTag()->m_playCount == 0 || viewOffset.size() > 0)
          buttons.Add(CONTEXT_BUTTON_MARK_WATCHED, 16103);
      }

      int choice = CGUIDialogContextMenu::ShowAndGetChoice(buttons);

      if (choice == CONTEXT_BUTTON_INFO)
      {
        CGUIDialogVideoInfo* pDlgInfo = (CGUIDialogVideoInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_INFO);

        if (!pDlgInfo) return false;

        pDlgInfo->SetMovie(fileItem);
        pDlgInfo->DoModal();

        if (pDlgInfo->NeedRefresh() == false)
          return false;

        return true;
      }
      else if (choice == CONTEXT_BUTTON_MARK_UNWATCHED)
      {
        fileItem->MarkAsUnWatched();
        updateFanOut = true;
      }
      else if (choice == CONTEXT_BUTTON_MARK_WATCHED)
      {
        fileItem->MarkAsWatched();
        updateFanOut = true;
      }

      if (updateFanOut)
      {
        CFileItemPtr item = GetCurrentListItem();
        RefreshSection(item->GetProperty("sectionPath").asString(), item->GetProperty("typeNumber").asInteger());
      }

    }
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowHome::CheckTimer(const CStdString& strExisting, const CStdString& strNew, int title, int line1, int line2)
{
  bool bReturn;
  if (g_alarmClock.HasAlarm(strExisting) && strExisting != strNew)
  {
    if (CGUIDialogYesNo::ShowAndGetInput(title, line1, line2, 0, bReturn) == false)
    {
      return false;
    }
    else
    {
      g_alarmClock.Stop(strExisting, false);  
      return true;
    }
  }
  else
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
typedef pair<string, HostSourcesPtr> string_sources_pair;

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowHome::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() ==  GUI_MSG_WINDOW_DEINIT)
  {
    m_lastSelectedItem = GetCurrentItemName();
    HideAllLists();
    return true;
  }

  bool ret = CGUIWindow::OnMessage(message);

  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      if (!m_lastSelectedItem.empty())
        HideAllLists();

      if (m_lastSelectedItem == "Search")
        RefreshSection("global://art", PLEX_METADATA_GLOBAL_IMAGES);

      if (g_guiSettings.GetBool("backgroundmusic.bgmusicenabled"))
        g_backgroundMusicPlayer.PlayElevatorMusic();

    }

    case GUI_MSG_WINDOW_RESET:
    case GUI_MSG_UPDATE_MAIN_MENU:
    {
      UpdateSections();

      if (message.GetMessage() != GUI_MSG_UPDATE_MAIN_MENU)
        RefreshAllSections(false);
    }
      break;

    case GUI_MSG_PLEX_SECTION_LOADED:
    {
      int type = message.GetParam1();
      CStdString url = message.GetStringParam();
      CFileItem* currentFileItem = GetCurrentFileItem();

      CLog::Log(LOGDEBUG, "GUIWindowHome:OnMessage Plex Section loaded %s %d", url.c_str(), type);

      CStdString sectionToLoad;
      if (currentFileItem && currentFileItem->HasProperty("sectionPath"))
        sectionToLoad = currentFileItem->GetProperty("sectionPath").asString();
      if (m_lastSelectedItem != sectionToLoad)
        sectionToLoad = m_lastSelectedItem;

      if (type == CONTENT_LIST_FANART)
      {
        if (url == sectionToLoad || url == "global://art")
        {
          CFileItemListPtr list = GetContentListFromSection(url, CONTENT_LIST_FANART);
          if (list)
          {
            SET_CONTROL_VISIBLE(SLIDESHOW_MULTIIMAGE);

            CLog::Log(LOGDEBUG, "GUIWindowHome:OnMessage activating global fanart with %d photos", list->Size());
            CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), SLIDESHOW_MULTIIMAGE, 0, 0, list.get());
            OnMessage(msg);
          }
        }
      }
      else
      {
        if (url == sectionToLoad)
        {
          HideAllLists();

          BOOST_FOREACH(contentListPair p, GetContentListsFromSection(url))
          {
            if(p.second && p.second->Size() > 0)
            {
              CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), p.first, 0, 0, p.second.get());
              CLog::Log(LOGDEBUG, "GUIWindowHome::OnMessage sending BIND to %d", p.first);
              OnMessage(msg);
              SET_CONTROL_VISIBLE(p.first);
            }
            else
              SET_CONTROL_HIDDEN(p.first);
          }
        }
      }
    }

      break;

    case GUI_MSG_CLICKED:
    {
      m_lastSelectedItem = GetCurrentItemName();

      int iAction = message.GetParam1();
      if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_PLAYER_PLAY)
      {
        int iControl = message.GetSenderId();
        
        CGUIBaseContainer *container = (CGUIBaseContainer*)GetControl(iControl);
        if (container)
        {
          CGUIListItemPtr item = container->GetListItem(0);
          if (iAction == ACTION_SELECT_ITEM &&
              item &&
              (item->GetProperty("type").asString() == "movie" ||
               item->GetProperty("type").asString() == "episode"))
          {
            CBuiltins::Execute("XBMC.ActivateWindow(PlexPreplayVideo," + item->GetProperty("key").asString() + ",return)");
            return true;
          }
          
          PlayFileFromContainer(container);
          return true;
        }
      }
    }
      break;
  }

  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowHome::UpdateSections()
{
  // This will be our new list.
  vector<CGUIListItemPtr> newList;

  CLog::Log(LOGDEBUG, "CGUIWindowHome::UpdateSections");

  // Get the old list.
  CGUIBaseContainer* control = (CGUIBaseContainer* )GetControl(MAIN_MENU);
  if (control == 0)
    control = (CGUIBaseContainer* )GetControl(300);

  if (control)
  {
    vector<CGUIListItemPtr>& oldList = control->GetStaticItems();

    // First collect all the real items, minus the channel entries.
    BOOST_FOREACH(CGUIListItemPtr item, oldList)
    {
      // Collect the channel items. They may get removed after that, so we'll keep them around.
      CFileItem* fileItem = (CFileItem* )item.get();
      if (fileItem->m_iprogramCount == CHANNELS_VIDEO)
        m_videoChannelItem = item;
      else if (fileItem->m_iprogramCount == CHANNELS_MUSIC)
        m_musicChannelItem = item;
      else if (fileItem->m_iprogramCount == CHANNELS_PHOTO)
        m_photoChannelItem = item;
      else if (fileItem->m_iprogramCount == CHANNELS_APPLICATION)
        m_applicationChannelItem = item;
      else if (item->HasProperty("plex") == false)
        newList.push_back(item);
    }


    map<string, HostSourcesPtr> sourcesMap;
    CPlexSourceScanner::GetMap(sourcesMap);

    // Collect the channels, keeping track of how many there are.
    int numVideo = 0;
    int numPhoto = 0;
    int numMusic = 0;
    int numApplication = 0;

    BOOST_FOREACH(string_sources_pair nameSource, sourcesMap)
    {
      numVideo += nameSource.second->videoSources.size();
      numPhoto += nameSource.second->pictureSources.size();
      numMusic += nameSource.second->musicSources.size();
      numApplication += nameSource.second->applicationSources.size();
    }
    
    // Now collect the library sections.
    vector<CFileItemPtr> newSections;
    PlexLibrarySectionManager::Get().getOwnedSections(newSections);

    // Count the names.
    map<string, int> nameCounts;
    BOOST_FOREACH(CFileItemPtr section, newSections)
    {
      CStdString sectionName = section->GetLabel();
      ++nameCounts[sectionName.ToLower()];
    }

    // Add the queue if needed.
    CFileItemList queue;
    if (MyPlexManager::Get().getPlaylist(queue, "queue", true) && queue.Size() > 0)
    {
      CFileItemPtr queue = CFileItemPtr(new CFileItem(g_localizeStrings.Get(44021)));
      queue->SetProperty("type", "mixed");
      queue->SetProperty("typeNumber", PLEX_METADATA_MIXED);
      queue->SetProperty("key", MyPlexManager::Get().getPlaylistUrl("queue"));
      queue->SetPath(queue->GetProperty("key").asString());
      newSections.push_back(queue);
    }
    
    CFileItemList recommendations;
    if (MyPlexManager::Get().getPlaylist(recommendations, "recommendations", true) && recommendations.Size() > 0)
    {
      CFileItemPtr rec = CFileItemPtr(new CFileItem(g_localizeStrings.Get(44022)));
      rec->SetProperty("type", "mixed");
      rec->SetProperty("typeNumber", PLEX_METADATA_MIXED);
      rec->SetProperty("key", MyPlexManager::Get().getPlaylistUrl("recommendations"));
      rec->SetPath(rec->GetProperty("key").asString());
      newSections.push_back(rec);
    }

    // Add the shared content menu if needed.
    if (PlexLibrarySectionManager::Get().getNumSharedSections() > 0)
    {
      CFileItemPtr shared = CFileItemPtr(new CFileItem(g_localizeStrings.Get(44020)));
      shared->SetProperty("key", "plex://shared");
      shared->SetPath(shared->GetProperty("key").asString());
      newSections.push_back(shared);
    }

    // Now add the new ones.
    int id = 1000;
    BOOST_FOREACH(CFileItemPtr item, newSections)
    {
      CGUIStaticItemPtr newItem = CGUIStaticItemPtr(new CGUIStaticItem());
      newItem->SetLabel(item->GetLabel());
      newItem->SetProperty("plex", "1");

      if (item->GetProperty("key").asString().find("/library/sections") != string::npos)
        newItem->SetProperty("section", "1");

      CStdString sectionName = item->GetLabel();
      if (nameCounts[sectionName.ToLower()] > 1)
        newItem->SetLabel2(item->GetLabel2());

      // Adding the section for preloading the fans
      AddSection(item->GetPath(), (int)item->GetProperty("typeNumber").asInteger());

      if (item->GetProperty("key").asString().find("/shared") != string::npos)
      {
        CStdString path = "XBMC.ActivateWindow(MySharedContent," + item->GetPath() + ",return)";
        newItem->SetClickActions(CGUIAction("", path));
        newItem->SetPath(path);
      }
      else if (item->GetProperty("type").asString() == "artist")
      {
        CStdString path = "XBMC.ActivateWindow(MyMusicFiles," + item->GetPath() + ",return)";
        newItem->SetClickActions(CGUIAction("", path));
        newItem->SetPath(path);
      }
      else if (item->GetProperty("type").asString() == "photo")
      {
        CStdString path = "XBMC.ActivateWindow(MyPictures," + item->GetPath() + ",return)";
        newItem->SetClickActions(CGUIAction("", path));
        newItem->SetPath(path);
      }
      else
      {
        CStdString path = "XBMC.ActivateWindow(MyVideoFiles," + item->GetPath() + ",return)";
        newItem->SetClickActions(CGUIAction("", path));
        newItem->SetPath(path);
      }

      newItem->m_idepth = 0;
      if (item->HasArt(PLEX_ART_FANART))
        newItem->SetArt(PLEX_ART_FANART, item->GetArt(PLEX_ART_FANART));
      newItem->m_iprogramCount = id++;
      newItem->SetProperty("sectionPath", item->GetPath());

      newList.push_back(newItem);
    }

    // See what channel entries to add.
    if (numApplication > 0)
    {
      newList.push_back(m_applicationChannelItem);
      m_applicationChannelItem->SetProperty("sectionPath", "channel://application");
      AddSection("channel://application", PLEX_METADATA_CHANNEL_APPLICATION);
    }

    if (numVideo > 0)
    {
      newList.push_back(m_videoChannelItem);
      m_videoChannelItem->SetProperty("sectionPath", "channel://video");
      AddSection("channel://video", PLEX_METADATA_CHANNEL_VIDEO);
    }

    if (numPhoto > 0)
    {
      newList.push_back(m_photoChannelItem);
      m_photoChannelItem->SetProperty("sectionPath", "channel://photo");
      AddSection("channel://photo", PLEX_METADATA_CHANNEL_PHOTO);
    }

    if (numMusic > 0)
    {
      newList.push_back(m_musicChannelItem);
      m_musicChannelItem->SetProperty("sectionPath", "channel://music");
      AddSection("channel://music", PLEX_METADATA_CHANNEL_MUSIC);
    }

    // Replace 'em.
    control->SetStaticContent(newList);

    RestoreSection();
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowHome::HideAllLists()
{
  CLog::Log(LOGDEBUG, "CGUIWindowHome:HideAllLists");
  // Hide lists.
  short lists[] = {CONTENT_LIST_ON_DECK, CONTENT_LIST_RECENTLY_ACCESSED, CONTENT_LIST_RECENTLY_ADDED, CONTENT_LIST_QUEUE, CONTENT_LIST_RECOMMENDATIONS};
  BOOST_FOREACH(int id, lists)
  {
    SET_CONTROL_HIDDEN(id);
    SET_CONTROL_HIDDEN(id-1000);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowHome::AddSection(const CStdString &url, int type)
{
  if (m_sections.find(url) == m_sections.end())
  {
    CPlexSectionFanout* fan = new CPlexSectionFanout(url, type);
    m_sections[url] = fan;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowHome::RemoveSection(const CStdString &url)
{
  if (m_sections.find(url) != m_sections.end())
  {
    CPlexSectionFanout* fan = m_sections[url];
    m_sections.erase(url);
    delete fan;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
std::vector<contentListPair> CGUIWindowHome::GetContentListsFromSection(const CStdString &url)
{
  if (m_sections.find(url) != m_sections.end())
  {
    CPlexSectionFanout* section = m_sections[url];
    return section->GetContentLists();
  }
  return std::vector<contentListPair>();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CFileItemListPtr CGUIWindowHome::GetContentListFromSection(const CStdString &url, int contentType)
{
  if (m_sections.find(url) != m_sections.end())
  {
    CPlexSectionFanout* section = m_sections[url];
    CFileItemListPtr list = section->GetContentList(contentType);
    if ((list && list->Size() > 0) || contentType != CONTENT_LIST_FANART)
      return section->GetContentList(contentType);
  }

  if (contentType == CONTENT_LIST_FANART &&
      !g_guiSettings.GetBool("lookandfeel.enableglobalslideshow"))
  {
    /* Special case */
    CGUIBaseContainer *container = (CGUIBaseContainer*)GetControl(MAIN_MENU);
    CFileItemList* list = new CFileItemList();
    CFileItemPtr defaultItem = CFileItemPtr(new CFileItem(CPlexSectionFanout::GetBestServerUrl(":/resources/show-fanart.jpg"), false));

    if (!container)
      list->Add(defaultItem);

    BOOST_FOREACH(CGUIListItemPtr fileItem, container->GetItems())
    {
      if (fileItem->HasProperty("sectionPath") &&
          fileItem->GetProperty("sectionPath").asString() == url)
      {
        if (fileItem->HasArt(PLEX_ART_FANART))
          list->Add(CFileItemPtr(new CFileItem(fileItem->GetArt(PLEX_ART_FANART), false)));
        else
          list->Add(defaultItem);
        break;
      }
    }

    if (list->Size() == 0)
      list->Add(defaultItem);

    return CFileItemListPtr(list);
  }

  return CFileItemListPtr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowHome::RefreshSection(const CStdString &url, int type)
{
  if (m_sections.find(url) != m_sections.end())
  {
    CPlexSectionFanout* section = m_sections[url];
    return section->Refresh();
  }
  else
    AddSection(url, type);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
typedef std::pair<CStdString, CPlexSectionFanout*> nameSectionPair;
void CGUIWindowHome::RefreshAllSections(bool force)
{
  BOOST_FOREACH(nameSectionPair p, m_sections)
  {
    if (force || p.second->NeedsRefresh())
      p.second->Refresh();
  }

}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowHome::ShowSection(const CStdString &url)
{
  if (m_sections.find(url) != m_sections.end())
  {
    CPlexSectionFanout* section = m_sections[url];
    section->Show();
    
    if (url == "global://art")
      m_globalArt = true;
    else
      m_globalArt = false;
    
    return true;
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowHome::ShowCurrentSection()
{
  if (GetCurrentItemName(true))
    return ShowSection(GetCurrentItemName(true));
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CStdString CGUIWindowHome::GetCurrentItemName(bool onlySections)
{
  CStdString name;

  CFileItemPtr item = GetCurrentListItem();
  if (!item)
    return name;

  if (item->HasProperty("sectionPath"))
    name = item->GetProperty("sectionPath").asString();
  else if (!onlySections)
    name = item->GetLabel();

  return name;
}

///////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowHome::RestoreSection()
{
  CLog::Log(LOGDEBUG, "CGUIWindowHome:RestoreSection %s", m_lastSelectedItem.c_str());
  if (m_lastSelectedItem == GetCurrentItemName())
  {
    ShowSection(m_lastSelectedItem);
    return;
  }

  HideAllLists();

  CGUIBaseContainer *pControl = (CGUIBaseContainer*)GetControl(MAIN_MENU);
  if (pControl && !m_lastSelectedItem.empty())
  {
    int idx = 0;
    BOOST_FOREACH(CGUIListItemPtr i, pControl->GetItems())
    {
      if (i->IsFileItem())
      {
        CFileItem* fItem = (CFileItem*)i.get();
        if (m_lastSelectedItem == fItem->GetProperty("sectionPath").asString() ||
            m_lastSelectedItem == fItem->GetLabel())
        {
          CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), pControl->GetID(), idx+1, 0);
          g_windowManager.SendThreadMessage(msg);

          if (fItem->HasProperty("sectionPath"))
            ShowSection(fItem->GetProperty("sectionPath").asString());
        }
      }
      idx++;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////
void CAuxFanLoadThread::Process()
{
  while (!m_bStop)
  {
    CLog::Log(LOGDEBUG, "CAFL: sleeping %d seconds", m_numSeconds);
    boost::this_thread::sleep(boost::posix_time::seconds(m_numSeconds));

    if (g_windowManager.GetActiveWindow() == 10016)
      continue;

    if (g_guiSettings.GetString("myplex.token").empty() == true)
    {
      CGUIDialogOK::ShowAndGetInput(g_localizeStrings.Get(44200), g_localizeStrings.Get(44201), g_localizeStrings.Get(44202), "");
      CApplicationMessenger::Get().ExecBuiltIn("XBMC.ActivateWindow(16)");
    }
    else
    {
      MyPlexManager::Get().GetUserInfo();
      if (!MyPlexManager::Get().UserHaveSubscribed())
      {
        CGUIDialogOK::ShowAndGetInput(g_localizeStrings.Get(44203), g_localizeStrings.Get(44201), g_localizeStrings.Get(44202), "");
        CApplicationMessenger::Get().ExecBuiltIn("XBMC.ActivateWindow(16)");
      }
      else
      {
        // Since we had a successful login we can sleep longer
        m_numSeconds = 30 * 60;
      }
    }
  }
}

