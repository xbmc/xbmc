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

#include "Client/MyPlex/MyPlexManager.h"
#include "PlexDirectory.h"
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

#include "Client/PlexServerManager.h"
#include "Client/PlexServerDataLoader.h"

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
CPlexSectionFanout::CPlexSectionFanout(const CStdString &url, SectionTypes sectionType)
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
int CPlexSectionFanout::LoadSection(const CURL &url, int contentType)
{
  CPlexSectionLoadJob* job = new CPlexSectionLoadJob(url, contentType);
  return CJobManager::GetInstance().AddJob(job, this, CJob::PRIORITY_HIGH);
}

//////////////////////////////////////////////////////////////////////////////
CStdString CPlexSectionFanout::GetBestServerUrl(const CStdString& extraUrl)
{
  CPlexServerPtr server = g_plexServerManager.GetBestServer();
  if (server)
    return server->BuildPlexURL(extraUrl).Get();

  CPlexServerPtr local = g_plexServerManager.FindByUUID("local");
  return local->BuildPlexURL(extraUrl).Get();
}

//////////////////////////////////////////////////////////////////////////////
void CPlexSectionFanout::Refresh()
{
  CPlexDirectory dir;

  CSingleLock lk(m_critical);
  m_fileLists.clear();

  CLog::Log(LOGDEBUG, "GUIWindowHome:SectionFanout:Refresh for %s", m_url.Get().c_str());

  CURL trueUrl(m_url);

  if (trueUrl.GetProtocol() == "global")
  {
    if (g_guiSettings.GetBool("lookandfeel.enableglobalslideshow"))
      LoadSection(GetBestServerUrl("library/arts"), CONTENT_LIST_FANART);
  }
  else if (m_sectionType == SECTION_TYPE_QUEUE)
  {
    /*if (!g_advancedSettings.m_bHideFanouts)
    {
      if (m_url.Find("queue") != -1)
        LoadSection(MyPlexManager::Get().getPlaylistUrl("/queue/unwatched"), CONTENT_LIST_QUEUE);
      else if (m_url.Find("recommendations") != -1)
        LoadSection(MyPlexManager::Get().getPlaylistUrl("/recommendations/unwatched"), CONTENT_LIST_QUEUE);
    }*/
  }
  else if (m_sectionType == SECTION_TYPE_CHANNELS)
  {
    if (!g_advancedSettings.m_bHideFanouts)
      LoadSection(GetBestServerUrl("channels/recentlyViewed"), CONTENT_LIST_RECENTLY_ACCESSED);

    /* We always show this as fanart */
    LoadSection(GetBestServerUrl("channels/arts"), CONTENT_LIST_FANART);
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
      PlexUtils::AppendPathToURL(trueUrl, "recentlyAdded");
      
      m_outstandingJobs.push_back(LoadSection(trueUrl.Get(), CONTENT_LIST_RECENTLY_ADDED));

      if (m_sectionType == SECTION_TYPE_MOVIE || m_sectionType == SECTION_TYPE_SHOW)
      {
        trueUrl = CURL(m_url);
        PlexUtils::AppendPathToURL(trueUrl, "onDeck");
        m_outstandingJobs.push_back(LoadSection(trueUrl.Get(), CONTENT_LIST_ON_DECK));
      }
    }

    /* We don't want to wait on the fanart, so don't add it to the outstandingjobs map */
    if (g_guiSettings.GetBool("lookandfeel.enableglobalslideshow"))
    {
      CURL artsUrl(m_url);
      PlexUtils::AppendPathToURL(artsUrl, "arts");
      LoadSection(artsUrl, CONTENT_LIST_FANART);
    }
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
    msg.SetStringParam(m_url.Get());
    g_windowManager.SendThreadMessage(msg);
  }
  else if (load->GetContentType() == CONTENT_LIST_FANART)
  {
    CGUIMessage msg(GUI_MSG_PLEX_SECTION_LOADED, WINDOW_HOME, 300, CONTENT_LIST_FANART);
    msg.SetStringParam(m_url.Get());
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
    msg.SetStringParam(m_url.Get());
    g_windowManager.SendThreadMessage(msg);

    CGUIMessage msg2(GUI_MSG_PLEX_SECTION_LOADED, WINDOW_HOME, 300, CONTENT_LIST_FANART);
    msg2.SetStringParam(m_url.Get());
    g_windowManager.SendThreadMessage(msg2);
  }
}

//////////////////////////////////////////////////////////////////////////////
bool CPlexSectionFanout::NeedsRefresh()
{
  int refreshTime = 5;
  if (m_sectionType == SECTION_TYPE_ALBUM ||
      m_sectionType == SECTION_TYPE_QUEUE ||
      m_sectionType >= SECTION_TYPE_CHANNELS)
    refreshTime = 20;

  if (m_sectionType == SECTION_TYPE_GLOBAL_FANART)
    refreshTime = 100;

  CLog::Log(LOGDEBUG, "GUIWindowHome:SectionFanout:NeedsRefresh %s, age %f, refresh %s", m_url.Get().c_str(), m_age.elapsed(), m_age.elapsed() > refreshTime ? "yes" : "no");
  return m_age.elapsed() > refreshTime;
}

//////////////////////////////////////////////////////////////////////////////
CGUIWindowHome::CGUIWindowHome(void) : CGUIWindow(WINDOW_HOME, "Home.xml"), m_globalArt(false), m_lastSelectedItem("Search")
{
  AddSection("global://art", SECTION_TYPE_GLOBAL_FANART);
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
      EPlexDirectoryType type = (EPlexDirectoryType)fileItem->GetPlexDirectoryType();

      if (type == PLEX_DIR_TYPE_EPISODE)
        buttons.Add(CONTEXT_BUTTON_INFO, 20352);
      else if (type == PLEX_DIR_TYPE_MOVIE)
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
        RefreshSection(item->GetProperty("sectionPath").asString(),
                       CGUIWindowHome::GetSectionTypeFromDirectoryType(item->GetPlexDirectoryType()));
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
        RefreshSection("global://art", SECTION_TYPE_GLOBAL_FANART);

      if (g_guiSettings.GetBool("backgroundmusic.bgmusicenabled"))
        g_backgroundMusicPlayer.PlayElevatorMusic();

    }

    case GUI_MSG_PLEX_BEST_SERVER_UPDATED:
    case GUI_MSG_WINDOW_RESET:
    case GUI_MSG_PLEX_SERVER_DATA_LOADED:
    {
      UpdateSections();

      if (message.GetMessage() != GUI_MSG_PLEX_SERVER_DATA_LOADED)
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
          if (PlexUtils::CurrentSkinHasPreplay() &&
              iAction == ACTION_SELECT_ITEM &&
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

void CGUIWindowHome::UpdateSections()
{
  CLog::Log(LOGDEBUG, "CGUIWindowHome::UpdateSections");

  CGUIBaseContainer* control = (CGUIBaseContainer*)GetControl(MAIN_MENU);
  if (!control)
  {
    CLog::Log(LOGWARNING, "CGUIWindowHome::UpdateSections can't find MAIN_MENU control");
    return;
  }

  vector<CGUIListItemPtr>& oldList = control->GetStaticItems();
  CFileItemListPtr sections = g_plexServerDataLoader.GetAllSections();
  vector<CGUIListItemPtr> newList;
  bool haveChannelItem = false;

  for (int i = 0; i < oldList.size(); i ++)
  {
    CGUIListItemPtr item = oldList[i];
    if (!item->HasProperty("plex"))
      newList.push_back(item);

    if (item->HasProperty("plex_channel_item") && g_plexServerDataLoader.HasChannels())
    {
      haveChannelItem = true;
      newList.push_back(item);
    }
  }

  for (int i = 0; i < sections->Size(); i ++)
  {
    CFileItemPtr sectionItem = sections->Get(i);
    CGUIStaticItemPtr item = CGUIStaticItemPtr(new CGUIStaticItem);
    item->SetLabel(sectionItem->GetLabel());
    item->SetProperty("plex", true);
    item->SetProperty("sectionPath", sectionItem->GetPath());

    AddSection(sectionItem->GetPath(),
               CGUIWindowHome::GetSectionTypeFromDirectoryType(sectionItem->GetPlexDirectoryType()));

    CStdString path("XBMC.ActivateWindow");
    if (sectionItem->GetProperty("type").asString() == "artist")
      path += "(MyMusicFiles, " + sectionItem->GetPath() + ",return)";
    else if (sectionItem->GetProperty("type").asString() == "photo")
      path += "(MyPictureFiles," + sectionItem->GetPath() + ",return)";
    else
      path += "(MyVideoFiles," + sectionItem->GetPath() + ",return)";

    item->SetPath(path);
    item->SetClickActions(CGUIAction("", path));
    newList.push_back(item);

  }

  if (!haveChannelItem && g_plexServerDataLoader.HasChannels())
  {
    /* We need the channel button as well */
    CGUIStaticItemPtr item = CGUIStaticItemPtr(new CGUIStaticItem);
    item->SetLabel(g_localizeStrings.Get(52102));
    item->SetProperty("plex_channel_item", true);
    item->SetProperty("plex", true);
    item->SetProperty("sectionPath", "plexserver://channels");

    item->SetPath("");
    item->SetClickActions(CGUIAction("", ""));
    newList.push_back(item);

    AddSection("plexserver://channels", SECTION_TYPE_CHANNELS);
  }

  CFileItemListPtr sharedSections = g_plexServerDataLoader.GetAllSharedSections();

  if (sharedSections->Size() > 0)
  {
    CGUIStaticItemPtr item = CGUIStaticItemPtr(new CGUIStaticItem);
    item->SetLabel(g_localizeStrings.Get(44020));
    item->SetProperty("plex", true);
    item->SetProperty("sectionPath", "plexserver://shared");
    item->SetPath("XBMC.ActivateWindow(MySharedContent,plexserver://shared,return)");
    item->SetClickActions(CGUIAction("", item->GetPath()));
    newList.push_back(item);
  }

  control->SetStaticContent(newList);

  RestoreSection();

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
void CGUIWindowHome::AddSection(const CStdString &url, SectionTypes type)
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
void CGUIWindowHome::RefreshSection(const CStdString &url, SectionTypes type)
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
SectionTypes
CGUIWindowHome::GetSectionTypeFromDirectoryType(EPlexDirectoryType dirType)
{
  if (dirType == PLEX_DIR_TYPE_MOVIE)
    return SECTION_TYPE_MOVIE;
  else if (dirType == PLEX_DIR_TYPE_SHOW)
    return SECTION_TYPE_SHOW;
  else if (dirType == PLEX_DIR_TYPE_ALBUM)
    return SECTION_TYPE_ALBUM;
  else if (dirType == PLEX_DIR_TYPE_PHOTO)
    return SECTION_TYPE_PHOTOS;
  else if (dirType == PLEX_DIR_TYPE_ARTIST)
    return SECTION_TYPE_ALBUM;
  else
  {
    CLog::Log(LOGINFO, "CGUIWindowHome::GetSectionTypeFromDirectoryType not handling DirectoryType %d", (int)dirType);
    return SECTION_TYPE_MOVIE;
  }
}

