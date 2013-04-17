#pragma once

#include <map>
#include <vector>

#include "GlobalsHandling.h"

#include "PlexServer.h"
#include "PlexConnection.h"
#include "JobManager.h"

typedef std::vector<CPlexServerPtr> PlexServerList;
typedef std::map<CStdString, CPlexServerPtr> PlexServerMap;
typedef std::pair<CStdString, CPlexServerPtr> PlexServerPair;


class CPlexServerReachabilityThread : public CThread
{
  public:
    CPlexServerReachabilityThread(CPlexServerPtr server, bool force)
      : CThread("ServerReachability: " + server->GetName()), m_server(server), m_force(force)
    {
      Create(true);
    }

    void Process();

    CPlexServerPtr m_server;
    bool m_force;
};

class CPlexServerManager
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
    CSingleLock lk(m_bestServerLock);
    return m_bestServer;
  }

  void SetBestServer(CPlexServerPtr server, bool force);
  void ClearBestServer();

  CPlexServerPtr FindByUUID(const CStdString &uuid);
  CPlexServerPtr FindByHostAndPort(const CStdString &host, int port);

  PlexServerList GetAllServers(CPlexServerOwnedModifier modifier = SERVER_ALL) const;

  void UpdateFromConnectionType(PlexServerList servers, int connectionType);
  void UpdateFromDiscovery(CPlexServerPtr server);
  void MarkServersAsRefreshing();
  void MergeServer(CPlexServerPtr server);
  void ServerRefreshComplete(int connectionType);
  void UpdateReachability(bool force = false);

  void ServerReachabilityDone(CPlexServerPtr server, bool success);

private:
  CPlexServerPtr _myPlexServer;
  CPlexServerPtr _localServer;

  void NotifyAboutServer(CPlexServerPtr server, bool added = true);

  CCriticalSection m_bestServerLock;
  CPlexServerPtr m_bestServer;

  CCriticalSection m_serverMapLock;
  PlexServerMap m_serverMap;
};

XBMC_GLOBAL_REF(CPlexServerManager, g_plexServerManager);
#define g_plexServerManager XBMC_GLOBAL_USE(CPlexServerManager)
