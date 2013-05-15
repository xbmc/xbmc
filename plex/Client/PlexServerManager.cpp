#include "PlexServerManager.h"

#include <boost/foreach.hpp>
#include <vector>
#include "utils/log.h"
#include "GUIMessage.h"
#include "GUIWindowManager.h"
#include "plex/PlexTypes.h"
#include "Client/PlexConnection.h"
#include "PlexServerDataLoader.h"

#include "Stopwatch.h"

using namespace std;

void
CPlexServerReachabilityThread::Process()
{
  bool success = false;
  if (m_force == true ||
      !m_server->GetActiveConnection())
  {
    success = m_server->UpdateReachability();
  }
  success = m_server->GetActiveConnection();

  g_plexServerManager.ServerReachabilityDone(m_server, success);
}

CPlexServerManager::CPlexServerManager()
{
  _myPlexServer = CPlexServerPtr(new CPlexServer("myplex", "myPlex", true));
  _myPlexServer->AddConnection(CPlexConnectionPtr(new CMyPlexConnection(CPlexConnection::CONNECTION_MYPLEX, "my.plexapp.com", 443)));

  _localServer = CPlexServerPtr(new CPlexServer("local", PlexUtils::GetHostName(), true));
  _localServer->AddConnection(CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MANUAL, "127.0.0.1", 32400)));
}

CPlexServerPtr
CPlexServerManager::FindByHostAndPort(const CStdString &host, int port)
{
  CSingleLock lk(m_serverManagerLock);

  BOOST_FOREACH(PlexServerPair p, m_serverMap)
  {
    vector<CPlexConnectionPtr> connections;
    p.second->GetConnections(connections);
    BOOST_FOREACH(CPlexConnectionPtr conn, connections)
    {
      if (conn->GetAddress().GetHostName().Equals(host) &&
          conn->GetAddress().GetPort() == port)
        return p.second;
    }
  }
  return CPlexServerPtr();
}

CPlexServerPtr
CPlexServerManager::FindByUUID(const CStdString &uuid)
{
  CSingleLock lk(m_serverManagerLock);

  if (uuid.Equals("myplex"))
    return _myPlexServer;

  if (uuid.Equals("local"))
    return _localServer;

  if (m_serverMap.find(uuid) != m_serverMap.end())
  {
    return m_serverMap.find(uuid)->second;
  }
  return CPlexServerPtr();
}

PlexServerList
CPlexServerManager::GetAllServers(CPlexServerOwnedModifier modifier) const
{
  CSingleLock lk(m_serverManagerLock);

  PlexServerList ret;

  BOOST_FOREACH(PlexServerPair p, m_serverMap)
  {
    if (modifier == SERVER_OWNED && p.second->GetOwned())
      ret.push_back(p.second);
    else if (modifier == SERVER_SHARED && !p.second->GetOwned())
      ret.push_back(p.second);
    else if (modifier == SERVER_ALL)
      ret.push_back(p.second);
  }

  return ret;
}

void
CPlexServerManager::MarkServersAsRefreshing()
{
  BOOST_FOREACH(PlexServerPair p, m_serverMap)
    p.second->MarkAsRefreshing();
}

void
CPlexServerManager::UpdateFromConnectionType(PlexServerList servers, int connectionType)
{
  CSingleLock lk(m_serverManagerLock);

  MarkServersAsRefreshing();
  BOOST_FOREACH(CPlexServerPtr p, servers)
    MergeServer(p);

  ServerRefreshComplete(connectionType);
  UpdateReachability();
}

void
CPlexServerManager::UpdateFromDiscovery(CPlexServerPtr server)
{
  CSingleLock lk(m_serverManagerLock);

  MergeServer(server);
  NotifyAboutServer(server);
  SetBestServer(server, false);
}

void
CPlexServerManager::MergeServer(CPlexServerPtr server)
{
  CSingleLock lk(m_serverManagerLock);

  if (m_serverMap.find(server->GetUUID()) != m_serverMap.end())
  {
    CPlexServerPtr existingServer = m_serverMap.find(server->GetUUID())->second;
    existingServer->Merge(server);
    CLog::Log(LOGDEBUG, "CPlexServerManager::MergeServer Merged %s with %d connection, now we have %d total connections.",
              server->GetName().c_str(), server->GetNumConnections(),
              existingServer->GetNumConnections());
  }
  else
  {
    m_serverMap[server->GetUUID()] = server;
    CLog::Log(LOGDEBUG, "CPlexServerManager::MergeServer Added a new server %s with %d connections", server->GetName().c_str(), server->GetNumConnections());
  }
}

void
CPlexServerManager::ServerRefreshComplete(int connectionType)
{
  vector<CStdString> serversToRemove;

  BOOST_FOREACH(PlexServerPair p, m_serverMap)
  {
    if (!p.second->MarkUpdateFinished(connectionType))
      serversToRemove.push_back(p.first);
  }

  BOOST_FOREACH(CStdString uuid, serversToRemove)
  {
    NotifyAboutServer(m_serverMap.find(uuid)->second, false);
    m_serverMap.erase(uuid);
  }
}

void
CPlexServerManager::UpdateReachability(bool force)
{
  CSingleLock lk(m_serverManagerLock);

  CLog::Log(LOGDEBUG, "CPlexServerManager::UpdateReachability Updating reachability (force=%s)", force ? "YES" : "NO");

  BOOST_FOREACH(PlexServerPair p, m_serverMap)
    new CPlexServerReachabilityThread(p.second, false);
}

void
CPlexServerManager::SetBestServer(CPlexServerPtr server, bool force)
{
  CSingleLock lk(m_serverManagerLock);
  if (!m_bestServer || force || m_bestServer == server)
  {
    m_bestServer = server;

    CGUIMessage msg(GUI_MSG_PLEX_BEST_SERVER_UPDATED, 0, 0);
    msg.SetStringParam(server->GetUUID());
    g_windowManager.SendThreadMessage(msg);
  }
}

void
CPlexServerManager::ClearBestServer()
{
  m_bestServer.reset();
}

void CPlexServerManager::ServerReachabilityDone(CPlexServerPtr server, bool success)
{
  if (success)
  {
    if (server->GetOwned() &&
        (server->GetServerClass().empty() || !server->GetServerClass().Equals(PLEX_SERVER_CLASS_SECONDARY)))
    {
      SetBestServer(server, false);
      NotifyAboutServer(server);
    }
    else if (m_bestServer==server)
    {
      ClearBestServer();
    }
  }
}

void
CPlexServerManager::NotifyAboutServer(CPlexServerPtr server, bool added)
{
  CGUIMessage msg(GUI_MSG_PLEX_SERVER_NOTIFICATION, 0, 0, added ? 1 : 0);
  msg.SetStringParam(server->GetUUID());
  g_windowManager.SendThreadMessage(msg);

  if (added)
    g_plexServerDataLoader.LoadDataFromServer(server);
}
