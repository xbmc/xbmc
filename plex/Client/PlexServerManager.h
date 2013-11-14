#pragma once

#include <map>
#include <vector>

#include "PlexServer.h"
#include "PlexConnection.h"
#include "JobManager.h"

#include "PlexManualServerManager.h"

#define PLEX_SERVER_MANAGER_XML_FORMAT_VERSION 1
#define PLEX_SERVER_MANAGER_XML_FILE "special://profile/plexservermanager.xml"

class CPlexServerReachabilityThread;

class CPlexServerManager : public boost::enable_shared_from_this<CPlexServerManager>
{
public:
  enum CPlexServerOwnedModifier
  {
    SERVER_OWNED,
    SERVER_SHARED,
    SERVER_ALL
  };

  CPlexServerManager();

  CPlexServerPtr GetBestServer() const
  {
    CSingleLock lk(m_serverManagerLock);
    return m_bestServer;
  }

  void SetBestServer(CPlexServerPtr server, bool force);
  void ClearBestServer();

  CPlexServerPtr FindByUUID(const CStdString &uuid);
  CPlexServerPtr FindByHostAndPort(const CStdString &host, int port);
  CPlexServerPtr FindFromItem(CFileItemPtr item);

  PlexServerList GetAllServers(CPlexServerOwnedModifier modifier = SERVER_ALL) const;

  void UpdateFromConnectionType(PlexServerList servers, int connectionType);
  void UpdateFromDiscovery(CPlexServerPtr server);
  void MarkServersAsRefreshing();
  void MergeServer(CPlexServerPtr server);
  void ServerRefreshComplete(int connectionType);
  void UpdateReachability(bool force = false);

  void ServerReachabilityDone(CPlexServerPtr server, bool success=false);
  bool HasAnyServerWithActiveConnection() const;

  void save();
  void load();
  
  void Stop();
  
  bool IsRunningReachabilityTests() const { return m_reachabilityThreads.size() > 0; }
  
  CPlexManualServerManager m_manualServerManager;

private:
  CPlexServerPtr _myPlexServer;
  CPlexServerPtr _localServer;
  CPlexServerPtr _nodeServer;
  bool m_stopped;

  void NotifyAboutServer(CPlexServerPtr server, bool added = true);

  CCriticalSection m_serverManagerLock;
  CPlexServerPtr m_bestServer;
  PlexServerMap m_serverMap;
  
  CEvent m_reachabilityTestEvent;
  bool m_updateRechabilityForced;
  
  std::map<CStdString, CPlexServerReachabilityThread*> m_reachabilityThreads;
};

typedef boost::shared_ptr<CPlexServerManager> CPlexServerManagerPtr;

class CPlexServerReachabilityThread : public CThread
{
  public:
    CPlexServerReachabilityThread(CPlexServerManagerPtr serverManager, CPlexServerPtr server)
      : CThread("ServerReachability: " + server->GetName()), m_server(server), m_serverManager(serverManager)
    {
      Create(true);
    }

    void Process();

    CPlexServerPtr m_server;
    CPlexServerManagerPtr m_serverManager;
};

