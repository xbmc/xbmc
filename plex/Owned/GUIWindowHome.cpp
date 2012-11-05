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
#include "PlexContentWorker.h"
#include "PlexDirectory.h"
#include "PlexSourceScanner.h"
#include "PlexLibrarySectionManager.h"
#include "threads/SingleLock.h"
#include "PlexUtils.h"

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

CGUIWindowHome::CGUIWindowHome(void) : CGUIWindow(WINDOW_HOME, "Home.xml")
  , m_globalArt(true)
  , m_lastSelectedItemKey("Search")
{
  // Create the worker. We're not going to destroy it because whacking it on exit can cause problems.
  m_workerManager = new PlexContentWorkerManager();
}

CGUIWindowHome::~CGUIWindowHome(void)
{
}

bool CGUIWindowHome::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_PREVIOUS_MENU && GetFocusedControlID() > 9000)
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
        // Save the selected menu item and if it's changed, get new lists.
        if (SaveSelectedMenuItem() == true)
        {
          // Hide lists.
          HideAllLists();
          
          // OK, let's load it after a delay.
          CFileItem* pFileItem = (CFileItem* )pItem.get();
          m_workerManager->cancelPending();
          CStdString path = pFileItem->GetPath();
          if (path.empty())
            path = pFileItem->GetLabel();
          m_pendingSelectItemKey = path;
          m_lastSelectedItemKey.clear();
          m_contentLoadTimer.StartZero();
        }
      }
    }
  }
  
  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowHome::SaveSelectedMenuItem()
{
  bool changed = false;
  
  CGUIBaseContainer* pControl = (CGUIBaseContainer* )GetControl(MAIN_MENU);
  if (pControl)
  {
    CGUIListItemPtr item = pControl->GetListItem(0);
    if (item->IsFileItem())
    {
      CFileItem* fileItem = (CFileItem* )item.get();
      
      CStdString path = fileItem->GetPath();
      if (path.empty())
        /* No path, means that this is a static item from the skin, store the label instead */
        path = fileItem->GetLabel();

      // See if it's changed.
      if (m_lastSelectedItemKey != path)
        changed = true;
      
      // Save the current selection.
      m_lastSelectedItemKey = path;
    }
  }
  
  return changed;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowHome::RestoreSelectedMenuItem()
{
  CGUIBaseContainer* pControl = (CGUIBaseContainer* )GetControl(MAIN_MENU);
  if (pControl && m_lastSelectedItemKey.empty() == false)
  {
    int selectionItem = -1;
    
    // Figure out the ID with the selected key.
    int i = 0;
    BOOST_FOREACH(CGUIListItemPtr item, pControl->GetItems())
    {
      CFileItem* fileItem = (CFileItem* )item.get();
      if (fileItem->GetPath() == m_lastSelectedItemKey ||
          (fileItem->GetLabel() == m_lastSelectedItemKey && m_lastSelectedItemKey.empty() == false))
      {
        selectionItem = i;
        break;
      }      
      i++;
    }
    
    if (selectionItem != -1)
    {
      CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), pControl->GetID(), selectionItem+1, 0);
      g_windowManager.SendThreadMessage(msg);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowHome::KeyHaveFanout(const CStdString& key)
{
  int id = LookupIDFromKey(key);
  if (id < 1)
    return false;

  /* Channels has a ID less than 4 and sections greater than 1000 */
  if (id <= 4 || id > 1000)
    return true;

  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int CGUIWindowHome::LookupIDFromKey(const std::string& key)
{
  CGUIBaseContainer* pControl = (CGUIBaseContainer* )GetControl(MAIN_MENU);
  if (pControl)
  {
    // Figure out the ID with the selected key.
    BOOST_FOREACH(CGUIListItemPtr item, pControl->GetItems())
    {
      CFileItem* fileItem = (CFileItem* )item.get();
      if (fileItem->GetPath() == key || fileItem->GetLabel() == key)
        return fileItem->m_iprogramCount;
    }
  }
  
  return -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowHome::UpdateContentForSelectedItem(const std::string& key)
{
  // Hide lists.
  HideAllLists();

  if (m_lastSelectedItemKey == key)
  {
    // Bind the lists.
    BOOST_FOREACH(int_list_pair pair, m_contentLists)
    {
      int controlID = pair.first;
      CFileItemListPtr list = pair.second.list;
    
      CGUIBaseContainer* control = (CGUIBaseContainer* )GetControl(controlID);
      if (control && list->Size() > 0)
      {
        // Bind the list.
        CGUIMessage msg(GUI_MSG_LABEL_BIND, MAIN_MENU, controlID, 0, 0, list.get());
        OnMessage(msg);
        
        // Make sure it's visible.
        SET_CONTROL_VISIBLE(controlID);
        SET_CONTROL_VISIBLE(controlID-1000);
        
        // Load thumbs.
        m_contentLists[controlID].loader->Load(*list.get());
      }
    }
  }
  else
  {
    bool globalArt = true;
    PlexServerPtr bestServer = PlexServerManager::Get().bestServer();
    CStdString bestServerUrl;
    if(bestServer)
      bestServerUrl.Format("http://%s:%d/", bestServer->address, bestServer->port);
    else
      bestServerUrl = "http://127.0.0.1:32400/";
    
    // Clear old lists.
    m_contentLists.clear();
    
    // Cancel any pending requests.
    m_workerManager->cancelPending();
    
    // Depending on what's selected, get the appropriate content.
    int itemID = LookupIDFromKey(key);
    if (itemID >= 1000)
    {
      // A library section.
      string sectionUrl = m_idToSectionUrlMap[itemID];
      int typeID = m_idToSectionTypeMap[itemID];
      
      if (typeID == PLEX_METADATA_MIXED)
      {
        // Queue.
        m_contentLists[CONTENT_LIST_QUEUE] = Group(kVIDEO_LOADER);
        m_workerManager->enqueue(WINDOW_HOME, MyPlexManager::Get().getPlaylistUrl("queue/unwatched"), CONTENT_LIST_QUEUE);
        
        // Recommendations.
        m_contentLists[CONTENT_LIST_RECOMMENDATIONS] = Group(kVIDEO_LOADER);
        m_workerManager->enqueue(WINDOW_HOME, MyPlexManager::Get().getPlaylistUrl("recommendations/unwatched"), CONTENT_LIST_RECOMMENDATIONS);
      }
      else if (boost::ends_with(sectionUrl, "shared") == false)
      {
        // Recently added.
        m_contentLists[CONTENT_LIST_RECENTLY_ADDED] = Group(typeID == PLEX_METADATA_ALBUM ? kMUSIC_LOADER : kVIDEO_LOADER);
        m_workerManager->enqueue(WINDOW_HOME, PlexUtils::AppendPathToURL(sectionUrl, "recentlyAdded?unwatched=1"), CONTENT_LIST_RECENTLY_ADDED);

        if (typeID == PLEX_METADATA_SHOW || typeID == PLEX_METADATA_MOVIE)
        {
          // On deck.
          m_contentLists[CONTENT_LIST_ON_DECK] = Group(kVIDEO_LOADER);
          m_workerManager->enqueue(WINDOW_HOME, PlexUtils::AppendPathToURL(sectionUrl, "onDeck"), CONTENT_LIST_ON_DECK);
        }

        // Asynchronously fetch the fanart for the section.
        globalArt = false;
        m_globalArt = false;
        m_workerManager->enqueue(WINDOW_HOME, PlexUtils::AppendPathToURL(sectionUrl, "arts"), CONTENT_LIST_FANART);
      }
    }
    else if (itemID >= 1 && itemID <= 4)
    {
      CStdString filter = (itemID==1) ? "video" : (itemID==2) ? "music" : (itemID==3) ? "photo" : "application";

      // Recently accessed.
      m_contentLists[CONTENT_LIST_RECENTLY_ACCESSED] = Group(kVIDEO_LOADER);
      m_workerManager->enqueue(WINDOW_HOME, bestServerUrl + "channels/recentlyViewed?filter=" + filter, CONTENT_LIST_RECENTLY_ACCESSED);
    }

    // If we need to, load global art.
    if (globalArt && m_globalArt == false)
    {
      m_globalArt = true;
      
      if (g_guiSettings.GetBool("lookandfeel.enableglobalslideshow") == true)
        m_workerManager->enqueue(WINDOW_HOME, bestServerUrl + "library/arts", CONTENT_LIST_FANART);
      else
        SET_CONTROL_HIDDEN(SLIDESHOW_MULTIIMAGE);
    }
    
    // Remember what the last one was.
    m_lastSelectedItemKey = key;
  }
}

bool CGUIWindowHome::OnPopupMenu()
{
  if (!GetFocusedControl())
    return false;
  
  int controlId = GetFocusedControl()->GetID();
  if (controlId == MAIN_MENU || controlId == POWER_MENU)
  {
    CGUIBaseContainer* pControl = (CGUIBaseContainer*)GetFocusedControl();
    CGUIListItemPtr pItem = pControl->GetListItem(pControl->GetSelectedItem());
    int itemId = pControl->GetSelectedItemID();
    
    float menuOffset = 0;
    int iHeading, iInfo, iAlreadySetMsg;
    CStdString sAlarmName, sAction;
    
    if (controlId == POWER_MENU) menuOffset = 180;
    
    switch (itemId) 
    {
      case QUIT_ITEM:
        iHeading = 40300;
        iInfo = 40310;
        iAlreadySetMsg = 40320;
        sAlarmName = "plex_quit_timer";
        sAction = "quit";
        break;
      case SLEEP_ITEM:       
        iHeading = 40301;
        iInfo = 40311;
        iAlreadySetMsg = 40321;
        sAlarmName = "plex_sleep_timer";
        sAction = "sleepsystem";
        break;
      case SHUTDOWN_ITEM:
        iHeading = 40302;
        iInfo = 40312;
        iAlreadySetMsg = 40322;
        sAlarmName = "plex_shutdown_timer";
        sAction = "shutdownsystem";
        break;
      case SLEEP_DISPLAY_ITEM:
        iHeading = 40303;
        iInfo = 40312;
        iAlreadySetMsg = 40323;
        sAlarmName = "plex_sleep_display_timer";
        sAction = "sleepdisplay";
      default:
        return false;
        break;
    }
    
    // Check to see if any timers already exist
    if (!CheckTimer("plex_quit_timer", sAlarmName, 40325, 40315, iAlreadySetMsg) ||
        !CheckTimer("plex_sleep_timer", sAlarmName, 40325, 40316, iAlreadySetMsg) ||
        !CheckTimer("plex_shutdown_timer", sAlarmName, 40325, 40317, iAlreadySetMsg) ||
        !CheckTimer("plex_sleep_display_timer", sAlarmName, 40325, 40318, iAlreadySetMsg))
      return false;
    
    
    int iTime;
    if (g_alarmClock.HasAlarm(sAlarmName))
    {
      iTime = (int)(g_alarmClock.GetRemaining(sAlarmName)/60);
      iHeading += 5; // Change the title to "Change" not "Set".
    }
    else
    {
      iTime = 0;
    }
    
    iTime = CGUIDialogTimer::ShowAndGetInput(iHeading, iInfo, iTime);
    
    // Dialog cancelled
    if (iTime == -1) return false;
    
    // If the alarm's already been set, cancel it
    if (g_alarmClock.HasAlarm(sAlarmName))
      g_alarmClock.Stop(sAlarmName, false);
    
    // Start a new alarm
    if (iTime > 0)
      g_alarmClock.Start(sAlarmName, float(iTime*60), sAction, false);
    
    // Focus the main menu again
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), MAIN_MENU);
    CSingleTryLock lock(g_graphicsContext);
    if(lock.IsOwner())
      CGUIWindow::OnMessage(msg);
    else
      g_windowManager.SendThreadMessage(msg, GetID());
    
    return true;
    
    if (pControl->GetSelectedItemID() == 1)
    {
      g_alarmClock.Start ("plex_quit_timer", 5, "ShutDown", false);      
    }
  }
  return false;
}

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

typedef pair<string, HostSourcesPtr> string_sources_pair;

static bool compare(CFileItemPtr first, CFileItemPtr second)
{
  return first->GetLabel() < second->GetLabel();
}

bool CGUIWindowHome::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() ==  GUI_MSG_WINDOW_DEINIT)
  {
    // Save the selected menu item.
    SaveSelectedMenuItem();
    
    // Cancel pending tasks and hide.
    m_workerManager->cancelPending();
    HideAllLists();
  }

  bool ret = CGUIWindow::OnMessage(message);
  
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_INIT:
  {
    if (m_lastSelectedItemKey.empty())
      HideAllLists();
  }
    
  case GUI_MSG_WINDOW_RESET:
  case GUI_MSG_UPDATE_MAIN_MENU:
  {
    // This will be our new list.
    vector<CGUIListItemPtr> newList;
    
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
      
      // Now collect all the added items.
      CPlexSourceScanner::Lock();
      
      map<string, HostSourcesPtr>& sourcesMap = CPlexSourceScanner::GetMap();

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
      
      CPlexSourceScanner::Unlock();

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
        CFileItemPtr queue = CFileItemPtr(new CFileItem(g_localizeStrings.Get(19021)));
        queue->SetProperty("type", "mixed");
        queue->SetProperty("typeNumber", PLEX_METADATA_MIXED);
        queue->SetProperty("key", MyPlexManager::Get().getPlaylistUrl("queue"));
        queue->SetPath(queue->GetProperty("key").asString());
        newSections.push_back(queue);
      }
      
      // Add the shared content menu if needed.
      if (PlexLibrarySectionManager::Get().getNumSharedSections() > 0)
      {
        CFileItemPtr shared = CFileItemPtr(new CFileItem(g_localizeStrings.Get(19020)));
        shared->SetProperty("key", "plex://shared");
        shared->SetPath(shared->GetProperty("key").asString());
        newSections.push_back(shared);
      }
            
      // Clear the maps.
      m_idToSectionUrlMap.clear();
      m_idToSectionTypeMap.clear();

      // Now add the new ones.
      bool itemStillExists = false;
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

        // Save the map from ID to library section ID.
        m_idToSectionUrlMap[id] = item->GetPath();
        m_idToSectionTypeMap[id] = item->GetProperty("typeNumber").asInteger();
        
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
        //newItem->SetQuickFanart(item->GetQuickFanart());
        newItem->SetArt(PLEX_ART_FANART, item->GetArt(PLEX_ART_FANART));
        newItem->m_iprogramCount = id++;

        newList.push_back(newItem);
        
        // See if it matches the selected item.
        if (newItem->GetPath() == m_lastSelectedItemKey)
          itemStillExists = true;
      }
      
      // See what channel entries to add.
      if (numApplication > 0)
        newList.push_back(m_applicationChannelItem);
      
      if (numVideo > 0)
        newList.push_back(m_videoChannelItem);

      if (numPhoto > 0)
        newList.push_back(m_photoChannelItem);

      if (numMusic > 0)
        newList.push_back(m_musicChannelItem);
      
      // Replace 'em.
      control->SetStaticContent(newList);
      
      // Restore selection.
      RestoreSelectedMenuItem();
      
      // See if the item for which we were showing the right hand lists still exists.
      if (itemStillExists == false)
      {
        // Whack the right hand side.
        HideAllLists();
        m_workerManager->cancelPending();
        m_contentLists.clear();
      }
    }

    if (message.GetMessage() != GUI_MSG_UPDATE_MAIN_MENU)
    {
      // Reload if needed.
      if (KeyHaveFanout(m_lastSelectedItemKey))
      {
        m_pendingSelectItemKey = m_lastSelectedItemKey;
        m_lastSelectedItemKey.clear();
        m_contentLoadTimer.StartZero();
      }
      else
      {
        if (g_guiSettings.GetBool("lookandfeel.enableglobalslideshow") == true)
        {
          PlexServerPtr bestServer = PlexServerManager::Get().bestServer();
          CStdString artUrl;
          if (bestServer)
            artUrl.Format("http://%s:%d/library/arts", bestServer->address, bestServer->port);
          else
            artUrl = "http://127.0.0.1:32400/library/arts";

          m_workerManager->enqueue(WINDOW_HOME, artUrl, CONTENT_LIST_FANART);
        }
        else
          SET_CONTROL_HIDDEN(SLIDESHOW_MULTIIMAGE);
      }
    }
  }
  break;
  
  case GUI_MSG_SEARCH_HELPER_COMPLETE:
  {
    PlexContentWorkerPtr worker = m_workerManager->find(message.GetParam1());
    if (worker)
    {
      CFileItemListPtr results = worker->getResults();
      int controlID = message.GetParam2();
      //printf("Processing results from worker: %d (context: %d).\n", worker->getID(), controlID);

      // Copy the items across.
      if (m_contentLists.find(controlID) != m_contentLists.end())
      {
        m_contentLists[controlID].list = results;
        
        CGUIBaseContainer* control = (CGUIBaseContainer* )GetControl(controlID);
        if (control && results->Size() > 0)
        {
          // Bind the list.
          CGUIMessage msg(GUI_MSG_LABEL_BIND, MAIN_MENU, controlID, 0, 0, m_contentLists[controlID].list.get());
          OnMessage(msg);
          
          // Make sure it's visible.
          SET_CONTROL_VISIBLE(controlID);
          SET_CONTROL_VISIBLE(controlID-1000);
          
          // Load thumbs.
          m_contentLists[controlID].loader->Load(*m_contentLists[controlID].list.get());
        }
      }
      else if (controlID == CONTENT_LIST_FANART)
      {
        if (results->Size() > 0)
        {
          // Send the right slideshow information over to the multiimage.
          CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), SLIDESHOW_MULTIIMAGE, 0, 0, results.get());
          OnMessage(msg);
          
          // Make it visible.
          SET_CONTROL_VISIBLE(SLIDESHOW_MULTIIMAGE);
        }
      }
      
      m_workerManager->destroy(worker->getID());
    }
  }
  break;
  
  case GUI_MSG_CLICKED:
  {
    int iAction = message.GetParam1();
    if (iAction == ACTION_SELECT_ITEM)
    {
      int iControl = message.GetSenderId();
      PlayFileFromContainer(GetControl(iControl));
    }
  }
  break;
  }
  
  return ret;
}

void CGUIWindowHome::SaveStateBeforePlay(CGUIBaseContainer* container)
{
}

void CGUIWindowHome::HideAllLists()
{
  // Hide lists.
  short lists[] = {CONTENT_LIST_ON_DECK, CONTENT_LIST_RECENTLY_ACCESSED, CONTENT_LIST_RECENTLY_ADDED, CONTENT_LIST_QUEUE, CONTENT_LIST_RECOMMENDATIONS};
  BOOST_FOREACH(int id, lists)
  {
    SET_CONTROL_HIDDEN(id);
    SET_CONTROL_HIDDEN(id-1000);
  }
}

void CGUIWindowHome::Render()
{
  if (m_pendingSelectItemKey.empty() == false && m_contentLoadTimer.IsRunning() && m_contentLoadTimer.GetElapsedMilliseconds() > 300)
  {
    UpdateContentForSelectedItem(m_pendingSelectItemKey);
    m_pendingSelectItemKey.clear();
    m_contentLoadTimer.Stop();
  }
  
  CGUIWindow::Render();
}
