#include "PlexNetworkServiceBrowser.h"

#include <vector>

using namespace std;

void
CPlexNetworkServiceBrowser::handleServiceArrival(NetworkServicePtr &service)
{
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer(service->getResourceIdentifier(), service->getParam("Name"), true));
  CPlexConnection* conn = new CPlexConnection(CPlexConnection::CONNECTION_DISCOVERED, service->address().to_string(), service->port());
  server->AddConnection(CPlexConnectionPtr(conn));
  server->UpdateReachability();

  g_plexServerManager.UpdateFromDiscovery(server);

  CSingleLock lk(m_serversSection);
  m_discoveredServers[server->GetUUID()] = server;
  dprintf("CPlexNetworkServiceBrowser::handleServiceArrival %s arrived", service->address().to_string().c_str());
}

void
CPlexNetworkServiceBrowser::handleServiceDeparture(NetworkServicePtr &service)
{
  CSingleLock lk(m_serversSection);
  /* Remove the server from m_discoveredServers and then tell ServerManager to update it's state */
  if (m_discoveredServers.find(service->getResourceIdentifier()) != m_discoveredServers.end())
    m_discoveredServers.erase(service->getResourceIdentifier());

  PlexServerList list;
  BOOST_FOREACH(PlexServerPair p, m_discoveredServers)
    list.push_back(p.second);

  g_plexServerManager.UpdateFromConnectionType(list, CPlexConnection::CONNECTION_DISCOVERED);
}

void
CPlexNetworkServiceBrowser::handleScanCompleted()
{
  CSingleLock lk(m_serversSection);
  PlexServerList list;
  BOOST_FOREACH(PlexServerPair p, m_discoveredServers)
  list.push_back(p.second);

  g_plexServerManager.UpdateFromConnectionType(list, CPlexConnection::CONNECTION_DISCOVERED);
}

void
CPlexServiceListener::Process()
{
  dprintf("CPlexServiceListener: Initializing.");

  // We start watching for changes in here.
  NetworkInterface::WatchForChanges();

  // Server browser.
  m_pmsBrowser = NetworkServiceBrowserPtr(new CPlexNetworkServiceBrowser(m_ioService, NS_PLEX_MEDIA_SERVER_PORT));

  // Player
  StartAdvertisement();

  // Start the I/O service in its own thread.
  m_ioService.run();
}
