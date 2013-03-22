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


class CPlexServerReachabilityJob : public CJob
{
public:
  CPlexServerReachabilityJob(CPlexServerPtr server, bool force)
  {
    m_force = force;
    m_server = server;
  }
  bool DoWork();
  bool m_force;
  CPlexServerPtr m_server;
};

class CPlexServerManager : public CJobQueue
{
public:
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

  PlexServerList GetAllServers() const;

  void UpdateFromConnectionType(PlexServerList servers, int connectionType);
  void UpdateFromDiscovery(CPlexServerPtr server);
  void MarkServersAsRefreshing();
  void MergeServer(CPlexServerPtr server);
  void ServerRefreshComplete(int connectionType);
  void UpdateReachability(bool force = false);

  virtual void OnJobComplete(unsigned int jobId, bool succeed, CJob* job);

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
