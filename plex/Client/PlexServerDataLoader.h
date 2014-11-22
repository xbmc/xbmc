#pragma once

#include "Client/PlexServer.h"
#include "Client/PlexServerManager.h"

#include "GlobalsHandling.h"
#include "JobManager.h"

#include <boost/shared_ptr.hpp>
#include <map>

#include "FileItem.h"
#include "plex/PlexTypes.h"
#include "threads/Timer.h"
#include "FileSystem/PlexDirectory.h"

/* Maps UUID->list of sections for the section */
typedef std::map<CStdString, CFileItemListPtr> ServerDataMap;
typedef std::pair<CStdString, CFileItemListPtr> ServerDataPair;
typedef std::map<CStdString, CPlexServerPtr> ServerMap;

class CPlexServerDataLoader : public CJobQueue,
                              public IPlexGlobalTimeout,
                              public boost::enable_shared_from_this<CPlexServerDataLoader>
{
public:
  CPlexServerDataLoader();

  void LoadDataFromServer(const CPlexServerPtr& server);
  void RemoveServer(const CPlexServerPtr& server);

  CFileItemListPtr GetSectionsForUUID(const CStdString& uuid);
  CFileItemListPtr GetSectionsForServer(const CPlexServerPtr& server)
  {
    return GetSectionsForUUID(server->GetUUID());
  }
  CFileItemListPtr GetChannelsForUUID(const CStdString& uuid);
  CFileItemListPtr GetChannelsForServer(const CPlexServerPtr& server)
  {
    return GetChannelsForUUID(server->GetUUID());
  }

  CFileItemListPtr GetAllSections() const;
  CFileItemListPtr GetAllSharedSections() const;
  CFileItemListPtr GetAllChannels() const;
  
  bool AnyOwendServerHasPlaylists()
  {
    CSingleLock lk(m_dataLock);
    
    PlexServerList list = g_plexApplication.serverManager->GetAllServers(CPlexServerManager::SERVER_OWNED);
    BOOST_FOREACH(const CPlexServerPtr& server, list)
    {
      if (ServerHasPlaylist(server))
        return true;
    }
    return false;
  }
  bool ServerUUIDHasPlaylist(const CStdString& uuid)
  {
    if (m_serverHasPlaylist.find(uuid) == m_serverHasPlaylist.end())
      return false;
    
    return m_serverHasPlaylist[uuid];
  }
  
  bool ServerHasPlaylist(const CPlexServerPtr& server)
  {
    if (!server)
      return false;
    
    return ServerUUIDHasPlaylist(server->GetUUID());
  }

  bool SectionHasFilters(const CURL& section);

  bool HasChannels() const
  {
    return m_channelMap.size() > 0;
  }
  bool HasSharedSections() const
  {
    return m_sharedSectionsMap.size() > 0;
  }

  void OnJobComplete(unsigned int jobID, bool success, CJob* job);

  void Stop();

  CFileItemPtr GetSection(const CURL& sectionUrl);
  EPlexDirectoryType GetSectionType(const CURL& sectionUrl);

  CStdString TimerName() const
  {
    return "serverDataLoader";
  }

  void Refresh()
  {
    CSingleLock lk(m_dataLock);
    m_forceRefresh = true;
    g_plexApplication.timer->RestartTimeout(5, this);
  }

private:
  bool m_stopped;
  void OnTimeout();

  CCriticalSection m_dataLock;
  CCriticalSection m_serverLock;

  ServerMap m_servers;

  ServerDataMap m_sectionMap;
  ServerDataMap m_channelMap;
  ServerDataMap m_sharedSectionsMap;
  std::map<CStdString, bool> m_serverHasPlaylist;

  bool m_forceRefresh;
};

typedef boost::shared_ptr<CPlexServerDataLoader> CPlexServerDataLoaderPtr;

class CPlexServerDataLoaderJob : public CJob
{
public:
  CPlexServerDataLoaderJob(const CPlexServerPtr& server, const CPlexServerDataLoaderPtr& loader)
    : m_server(server), m_loader(loader), m_abort(false)
  {
  }

  bool DoWork();
  CFileItemListPtr FetchList(const CStdString& path);

  CPlexServerPtr m_server;
  CFileItemListPtr m_sectionList;
  CFileItemListPtr m_channelList;
  CFileItemListPtr m_playlistList;

  /* we retain this so it won't go away from under us */
  CPlexServerDataLoaderPtr m_loader;
  XFILE::CPlexDirectory m_dir;

  virtual void Cancel()
  {
    m_abort = true;
    m_dir.CancelDirectory();
  }

  virtual bool operator==(const CJob* job) const
  {
    CPlexServerDataLoaderJob* oJob = (CPlexServerDataLoaderJob*)job;
    if (oJob->m_server->Equals(m_server))
      return true;
    return false;
  }

  bool m_abort;
  void loadPreferences();
};
