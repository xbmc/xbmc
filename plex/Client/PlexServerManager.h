#pragma once

#include <map>
#include <vector>

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

  // This constructor is mainly used by tests, it inserts the server
  // into the server map.
  CPlexServerManager(const CPlexServerPtr& server);

  CPlexServerPtr GetBestServer() const
  {
    CSingleLock lk(m_serverManagerLock);
    return m_bestServer;
  }

  void SetBestServer(CPlexServerPtr server, bool force);
  void ClearBestServer();

  CPlexServerPtr FindByUUID(const CStdString &uuid);
  CPlexServerPtr FindFromItem(const CFileItemPtr& item);
  CPlexServerPtr FindFromItem(const CFileItem& item);

  PlexServerList GetAllServers(CPlexServerOwnedModifier modifier = SERVER_ALL,
                               bool onlyActive = false) const;

  virtual void UpdateFromConnectionType(const PlexServerList& servers, int connectionType);
  void UpdateFromDiscovery(const CPlexServerPtr& server);
  void MarkServersAsRefreshing();
  CPlexServerPtr MergeServer(const CPlexServerPtr& server);
  void ServerRefreshComplete(int connectionType);
  virtual void UpdateReachability(bool force = false);

  void ServerReachabilityDone(const CPlexServerPtr& server, bool success=false);
  bool HasAnyServerWithActiveConnection() const;

  void RemoveAllServers();

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

  virtual void NotifyAboutServer(const CPlexServerPtr& server, bool added = true);

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

