#include "PlexSectionFanout.h"
#include <boost/foreach.hpp>
#include "GUIMessage.h"
#include "Client/PlexServerManager.h"
#include "settings/GUISettings.h"
#include "settings/AdvancedSettings.h"
#include "VideoThumbLoader.h"
#include "Key.h"
#include "guilib/GUIWindowManager.h"
#include "Playlists/PlexPlayQueueManager.h"
#include "PlayListPlayer.h"

using namespace XFILE;
using namespace std;

//////////////////////////////////////////////////////////////////////////////
CPlexSectionFanout::CPlexSectionFanout(const CStdString& url, SectionTypes sectionType,
                                       bool useGlobalSlideshow)
  : m_sectionType(sectionType),
    m_needsRefresh(false),
    m_url(url),
    m_useGlobalSlideshow(useGlobalSlideshow)
{
  Refresh();
}

//////////////////////////////////////////////////////////////////////////////
void CPlexSectionFanout::GetContentList(int type, CFileItemList& list)
{
  CSingleLock lk(m_critical);
  if (m_fileLists.find(type) != m_fileLists.end())
    list.Copy(*m_fileLists[type], true);
}

//////////////////////////////////////////////////////////////////////////////
void CPlexSectionFanout::GetContentTypes(std::vector<int>& lists)
{
  CSingleLock lk(m_critical);
  BOOST_FOREACH(contentListPair p, m_fileLists)
    lists.push_back(p.first);
}

//////////////////////////////////////////////////////////////////////////////
int CPlexSectionFanout::LoadSection(const CURL& url, int contentType)
{
  CPlexSectionFetchJob* job = new CPlexSectionFetchJob(url, contentType);
  return CJobManager::GetInstance().AddJob(job, this, CJob::PRIORITY_HIGH);
}

//////////////////////////////////////////////////////////////////////////////
CStdString CPlexSectionFanout::GetBestServerUrl(const CStdString& extraUrl)
{
  CPlexServerPtr server = g_plexApplication.serverManager->GetBestServer();
  if (server)
    return server->BuildPlexURL(extraUrl).Get();

  CPlexServerPtr local = g_plexApplication.serverManager->FindByUUID("local");
  return local->BuildPlexURL(extraUrl).Get();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexSectionFanout::ShowPlayQueue()
{
  CFileItemList pqList;
  if (!g_plexApplication.playQueueManager->getCurrentPlayQueue(pqList))
    return;

  int type = g_plexApplication.playQueueManager->getCurrentPlayQueuePlaylist();

  int listType = (type == PLAYLIST_VIDEO) ?
                   CONTENT_LIST_PLAYQUEUE_VIDEO : CONTENT_LIST_PLAYQUEUE_MUSIC;

  std::pair<int, CFileItemList*> p;
  BOOST_FOREACH(p, m_fileLists)
    delete p.second;

  m_fileLists.clear();

  CFileItemList* list = new CFileItemList;
  list->Copy(pqList, false);

  int currentPos = g_playlistPlayer.GetCurrentSong();
  if (currentPos == -1)
    currentPos = pqList.GetProperty("playQueueSelectedItemOffset").asInteger(0);

  for (int i = currentPos; i < pqList.Size(); i ++)
    list->Add(pqList.Get(i));

  m_fileLists[listType] = list;

  CLog::Log(LOGDEBUG, "CPlexSectionFanout::ShowPlayQueue showing playqueue %d with %d items",
            listType, list->Size());

  CGUIMessage msg(GUI_MSG_PLEX_SECTION_LOADED, WINDOW_HOME, 300, m_sectionType);
  msg.SetStringParam(m_url.Get());
  g_windowManager.SendThreadMessage(msg);
}

//////////////////////////////////////////////////////////////////////////////
void CPlexSectionFanout::Refresh()
{
  CPlexDirectory dir;

  CSingleLock lk(m_critical);

  CLog::Log(LOGDEBUG, "GUIWindowHome:SectionFanout:Refresh for %s", m_url.Get().c_str());

  CURL trueUrl(m_url);

  if (trueUrl.GetProtocol() == "plexserver" &&
      trueUrl.GetHostName() == "playqueue")
  {
    ShowPlayQueue();
    LoadSection(GetBestServerUrl("library/arts"), CONTENT_LIST_FANART);
  }
  else if (trueUrl.GetProtocol() == "global")
  {
    if (g_guiSettings.GetBool("lookandfeel.enableglobalslideshow"))
      LoadSection(GetBestServerUrl("library/arts"), CONTENT_LIST_FANART);
  }
  else if (m_sectionType == SECTION_TYPE_QUEUE)
  {
    if (!g_advancedSettings.m_bHideFanouts)
    {
      PlexUtils::AppendPathToURL(trueUrl, "queue/unwatched");
      m_outstandingJobs.push_back(LoadSection(trueUrl, CONTENT_LIST_QUEUE));
      trueUrl = CURL(m_url);
      PlexUtils::AppendPathToURL(trueUrl, "recommendations/unwatched");
      m_outstandingJobs.push_back(LoadSection(trueUrl, CONTENT_LIST_RECOMMENDATIONS));
    }
  }
  else if (m_sectionType == SECTION_TYPE_CHANNELS)
  {
    if (!g_advancedSettings.m_bHideFanouts)
      m_outstandingJobs.push_back(
      LoadSection(GetBestServerUrl("channels/recentlyViewed"), CONTENT_LIST_RECENTLY_ACCESSED));

    /* We always show this as fanart */
    m_outstandingJobs.push_back(
    LoadSection(GetBestServerUrl("channels/arts"), CONTENT_LIST_FANART));
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

      if (m_sectionType != SECTION_TYPE_ALBUM)
        trueUrl.SetOption("unwatched", "1");

#if 0
      if (m_sectionType == SECTION_TYPE_SHOW)
      {
        trueUrl.SetOption("stack", "1");
        trueUrl.SetOption("includeParentData", "1");
      }
#endif

      PlexUtils::AppendPathToURL(trueUrl, "recentlyAdded");

      m_outstandingJobs.push_back(LoadSection(trueUrl.Get(), CONTENT_LIST_RECENTLY_ADDED));

      if (m_sectionType == SECTION_TYPE_MOVIE || m_sectionType == SECTION_TYPE_SHOW ||
          m_sectionType == SECTION_TYPE_HOME_MOVIE)
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

      if (m_useGlobalSlideshow)
        artsUrl = GetBestServerUrl("library/arts");
      else
        PlexUtils::AppendPathToURL(artsUrl, "arts");

      LoadSection(artsUrl, CONTENT_LIST_FANART);
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
void CPlexSectionFanout::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  CPlexSectionFetchJob* load = (CPlexSectionFetchJob*)job;
  if (success)
  {
    CSingleLock lk(m_critical);

    // check if the section content has changed
    if (load->DirectoryChanged())
    {
      int type = load->m_contentType;
      if (m_fileLists.find(type) != m_fileLists.end() && m_fileLists[type] != NULL)
        delete m_fileLists[type];

      CFileItemList* newList = new CFileItemList;
      newList->Assign(load->m_items, false);

      /* HACK HACK HACK */
      if (m_sectionType == SECTION_TYPE_HOME_MOVIE)
      {
        for (int i = 0; i < newList->Size(); i++)
        {
          newList->Get(i)->SetProperty("type", "clip");
          newList->Get(i)->SetPlexDirectoryType(PLEX_DIR_TYPE_CLIP);
        }
      }

      m_fileLists[type] = newList;

      /* Pre-cache stuff */
      if (type != CONTENT_LIST_FANART)
        g_plexApplication.thumbCacher->Load(*newList);
    }
  }

  m_age.restart();

  vector<int>::iterator it = std::find(m_outstandingJobs.begin(), m_outstandingJobs.end(), jobID);
  if (it != m_outstandingJobs.end())
    m_outstandingJobs.erase(it);

  if (m_outstandingJobs.size() == 0 && load->m_contentType != CONTENT_LIST_FANART)
  {
    CGUIMessage msg(GUI_MSG_PLEX_SECTION_LOADED, WINDOW_HOME, 300, m_sectionType);
    msg.SetStringParam(m_url.Get());
    g_windowManager.SendThreadMessage(msg);
  }
  else if (load->m_contentType == CONTENT_LIST_FANART)
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
  if (m_needsRefresh || m_sectionType == SECTION_TYPE_PLAYQUEUE)
  {
    m_needsRefresh = false;
    return true;
  }

  int refreshTime = 5;
  if (m_sectionType == SECTION_TYPE_ALBUM || m_sectionType == SECTION_TYPE_QUEUE ||
      m_sectionType >= SECTION_TYPE_CHANNELS)
    refreshTime = 20;

  if (m_sectionType == SECTION_TYPE_GLOBAL_FANART)
    refreshTime = 3600;

  return m_age.elapsed() > refreshTime;
}

///////////////////////////////////////////////////////////////////////////////////////////
CPlexSectionFanout::SectionTypes CPlexSectionFanout::GetSectionTypeFromDirectoryType(EPlexDirectoryType dirType)
{
  if (dirType == PLEX_DIR_TYPE_MOVIE)
    return SECTION_TYPE_MOVIE;
  else if (dirType == PLEX_DIR_TYPE_SHOW)
    return SECTION_TYPE_SHOW;
  else if (dirType == PLEX_DIR_TYPE_ALBUM)
    return SECTION_TYPE_ALBUM;
  else if (dirType == PLEX_DIR_TYPE_PHOTOALBUM || dirType == PLEX_DIR_TYPE_PHOTO)
    return SECTION_TYPE_PHOTOS;
  else if (dirType == PLEX_DIR_TYPE_ARTIST)
    return SECTION_TYPE_ALBUM;
  else if (dirType == PLEX_DIR_TYPE_PLAYLIST)
    return SECTION_TYPE_QUEUE;
  else if (dirType == PLEX_DIR_TYPE_HOME_MOVIES)
    return SECTION_TYPE_HOME_MOVIE;
  else
  {
    CLog::Log(LOGINFO, "CGUIWindowHome::GetSectionTypeFromDirectoryType not handling DirectoryType %d", (int)dirType);
    return SECTION_TYPE_MOVIE;
  }
}
