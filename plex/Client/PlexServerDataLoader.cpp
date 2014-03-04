#include "PlexServerDataLoader.h"
#include "FileSystem/PlexDirectory.h"
#include "GUIWindowManager.h"
#include "GUIMessage.h"
#include "settings/GUISettings.h"

#include "PlexTypes.h"

#include <boost/foreach.hpp>

#include "utils/log.h"
#include "guilib/LocalizeStrings.h"

using namespace XFILE;

#define SECTION_REFRESH_INTERVAL 5 * 60 * 1000
#define SHARED_SERVER_REFRESH 10 * 60 * 1000

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexServerDataLoader::CPlexServerDataLoader() : CJobQueue(false, 4, CJob::PRIORITY_NORMAL), m_stopped(false)
{
  g_plexApplication.timer.SetTimeout(SECTION_REFRESH_INTERVAL, this);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexServerDataLoader::LoadDataFromServer(const CPlexServerPtr &server)
{
  if (m_stopped)
    return;

  CSingleLock lk(m_serverLock);
  
  if (m_servers.find(server->GetUUID()) == m_servers.end())
  {
    m_servers[server->GetUUID()] = server;
    CLog::Log(LOGDEBUG, "CPlexServerDataLoader::LoadDataFromServer loading data for server %s", server->GetName().c_str());
    AddJob(new CPlexServerDataLoaderJob(server, shared_from_this()));
    
    g_plexApplication.timer.RestartTimeout(SECTION_REFRESH_INTERVAL, this);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexServerDataLoader::RemoveServer(const CPlexServerPtr &server)
{
  if (m_stopped)
    return;

  CSingleLock lk(m_dataLock);
  
  if (m_sectionMap.find(server->GetUUID()) != m_sectionMap.end())
  {
    CLog::Log(LOG_LEVEL_DEBUG, "CPlexServerDataLoader::RemoveServer from sectionMap %s", server->GetName().c_str());
    m_sectionMap.erase(server->GetUUID());
  }

  if (m_sharedSectionsMap.find(server->GetUUID()) != m_sharedSectionsMap.end())
  {
    CLog::Log(LOG_LEVEL_DEBUG, "CPlexServerDataLoader::RemoveServer from sharedSectionMap %s", server->GetName().c_str());
    m_sharedSectionsMap.erase(server->GetUUID());
  }

  if (m_channelMap.find(server->GetUUID()) != m_channelMap.end())
  {
    CLog::Log(LOG_LEVEL_DEBUG, "CPlexServerDataLoader::RemoveServer from channelMap %s", server->GetName().c_str());
    m_channelMap.erase(server->GetUUID());
  }

  if (m_servers.find(server->GetUUID()) != m_servers.end())
    m_servers.erase(server->GetUUID());

  CGUIMessage msg(GUI_MSG_PLEX_SERVER_DATA_UNLOADED, PLEX_DATA_LOADER, 0);
  msg.SetStringParam(server->GetUUID());
  g_windowManager.SendThreadMessage(msg);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexServerDataLoader::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CPlexServerDataLoaderJob *j = (CPlexServerDataLoaderJob*)job;
  if (success && !m_stopped)
  {
    CSingleLock lk(m_dataLock);
    if (j->m_sectionList) {
      CFileItemListPtr sectionList = j->m_sectionList;
      sectionList->SetProperty("serverUUID", j->m_server->GetUUID());
      sectionList->SetProperty("serverName", j->m_server->GetName());

      if (j->m_server->GetOwned())
        m_sectionMap[j->m_server->GetUUID()] = sectionList;
      else
        m_sharedSectionsMap[j->m_server->GetUUID()] = sectionList;
    }
    
    if (j->m_channelList)
    {
      m_channelMap[j->m_server->GetUUID()] = j->m_channelList;
      m_channelMap[j->m_server->GetUUID()]->SetProperty("serverUUID", j->m_server->GetUUID());
      m_channelMap[j->m_server->GetUUID()]->SetProperty("serverName", j->m_server->GetName());
    }

    j->m_server->DidRefresh();

    CGUIMessage msg(GUI_MSG_PLEX_SERVER_DATA_LOADED, PLEX_DATA_LOADER, 0);
    msg.SetStringParam(j->m_server->GetUUID());
    g_windowManager.SendThreadMessage(msg);
  }
  else
    CLog::Log(LOGDEBUG, "CPlexServerDataLoader::OnJobComplete fail");

  CJobQueue::OnJobComplete(jobID, success, job);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CFileItemListPtr CPlexServerDataLoader::GetSectionsForUUID(const CStdString &uuid)
{
  CSingleLock lk(m_dataLock);

  if (m_sectionMap.find(uuid) != m_sectionMap.end())
    return m_sectionMap[uuid];

  /* not found in our server map, check shared servers */
  if (m_sharedSectionsMap.find(uuid) != m_sharedSectionsMap.end())
    return m_sharedSectionsMap[uuid];

  return CFileItemListPtr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CFileItemListPtr CPlexServerDataLoader::GetChannelsForUUID(const CStdString &uuid)
{
  CSingleLock lk(m_dataLock);
  if (m_channelMap.find(uuid) != m_channelMap.end())
    return m_channelMap[uuid];
  return CFileItemListPtr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CFileItemListPtr CPlexServerDataLoaderJob::FetchList(const CStdString& path)
{
  CURL url = m_server->BuildPlexURL(path);
  CFileItemList* list = new CFileItemList;

  if (m_dir.GetDirectory(url.Get(), *list))
    return CFileItemListPtr(list);

  delete list;
  return CFileItemListPtr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexServerDataLoaderJob::DoWork()
{
  if (m_server->GetUUID() != "myplex")
  {
    m_sectionList = FetchList("/library/sections");
    if (m_server->GetOwned())
      m_channelList = FetchList("/channels/all");
  }
  else
  {
    m_sectionList = CFileItemListPtr(new CFileItemList);
    CFileItemPtr myPlexSection = CFileItemPtr(new CFileItem("plexserver://myplex/pms/playlists"));
    myPlexSection->SetProperty("serverName", "myPlex");
    myPlexSection->SetProperty("serverUUID", "myplex");
    myPlexSection->SetPath("plexserver://myplex/pms/playlists");
    myPlexSection->SetLabel(g_localizeStrings.Get(44021));
    myPlexSection->SetPlexDirectoryType(PLEX_DIR_TYPE_PLAYLIST);
    m_sectionList->Add(myPlexSection);
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CFileItemListPtr CPlexServerDataLoader::GetAllSharedSections() const
{
  CSingleLock lk(m_dataLock);
  CFileItemList* list = new CFileItemList;

  BOOST_FOREACH(ServerDataPair pair, m_sharedSectionsMap)
  {
    for (int i = 0; i < pair.second->Size(); i++)
    {
      CFileItemPtr item = pair.second->Get(i);
      item->SetProperty("serverName", pair.second->GetProperty("serverName"));
      item->SetProperty("serverUUID", pair.second->GetProperty("serverUUID"));
      list->Add(item);
    }
  }

  return CFileItemListPtr(list);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CFileItemListPtr CPlexServerDataLoader::GetAllSections() const
{
  CSingleLock lk(m_dataLock);
  CFileItemList* list = new CFileItemList;
  std::map<std::string, CFileItemPtr> sectionNameMap;

  BOOST_FOREACH(ServerDataPair pair, m_sectionMap)
  {
    if (!pair.second)
      continue;

    for (int i = 0; i < pair.second->Size(); i++)
    {
      CFileItemPtr item = pair.second->Get(i);
      if (item)
      {
        item->SetProperty("serverName", pair.second->GetProperty("serverName"));
        item->SetProperty("serverUUID", pair.second->GetProperty("serverUUID"));
        list->Add(item);

        if (sectionNameMap.find(item->GetLabel()) != sectionNameMap.end())
        {
          sectionNameMap[item->GetLabel()]->SetProperty("SectionNameCollision", "yes");
          item->SetProperty("sectionNameCollision", "yes");
        }

        sectionNameMap[item->GetLabel()] = item;
      }
    }
  }

  return CFileItemListPtr(list);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CFileItemListPtr CPlexServerDataLoader::GetAllChannels() const
{
  CSingleLock lk(m_dataLock);
  CFileItemList* list = new CFileItemList;

  BOOST_FOREACH(ServerDataPair pair, m_channelMap)
  {
    for (int i = 0; i < pair.second->Size(); i++)
    {
      CFileItemPtr item = pair.second->Get(i);
      item->SetProperty("serverName", pair.second->GetProperty("serverName"));
      item->SetProperty("serverUUID", pair.second->GetProperty("serverUUID"));
      list->Add(item);
    }
  }

  return CFileItemListPtr(list);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexServerDataLoader::OnTimeout()
{
  CSingleLock lk(m_serverLock);

  std::pair<CStdString, CPlexServerPtr> p;
  BOOST_FOREACH(p, m_servers)
  {
    if (p.second->GetUUID() != "myplex")
    {
      if (p.second->GetOwned() || p.second->GetLastRefreshed() > SHARED_SERVER_REFRESH)
      {
        CLog::Log(LOGDEBUG, "CPlexServerDataLoader::OnTimeout refreshing data for %s", p.second->GetName().c_str());
        AddJob(new CPlexServerDataLoaderJob(p.second, shared_from_this()));
      }
    }
  }

  g_plexApplication.timer.SetTimeout(SECTION_REFRESH_INTERVAL, this);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexServerDataLoader::Stop()
{
  g_plexApplication.timer.RemoveTimeout(this);
  m_stopped = true;

  CancelJobs();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CFileItemPtr CPlexServerDataLoader::GetSection(const CURL &sectionUrl)
{
  CFileItemListPtr sections = GetSectionsForUUID(sectionUrl.GetHostName());
  if (sections && sections->Size() > 0)
  {
    for (int i = 0; i < sections->Size(); i ++)
    {
      CFileItemPtr item = sections->Get(i);
      if (item && item->GetPath() == sectionUrl.Get())
      {
        if (item->GetPlexDirectoryType() == PLEX_DIR_TYPE_MOVIE &&
            item->GetProperty("agent").asString() == "com.plexapp.agents.none")
          item->SetPlexDirectoryType(PLEX_DIR_TYPE_HOME_MOVIES);
        return item;
      }
    }
  }
  return CFileItemPtr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexServerDataLoader::SectionHasFilters(const CURL &sectionUrl)
{
  CFileItemPtr item = GetSection(sectionUrl);
  if (item)
    return item->GetProperty("filters").asBoolean();

  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
EPlexDirectoryType CPlexServerDataLoader::GetSectionType(const CURL &sectionUrl)
{
  CFileItemPtr item = GetSection(sectionUrl);
  if (item)
    return item->GetPlexDirectoryType();

  return PLEX_DIR_TYPE_UNKNOWN;
}
