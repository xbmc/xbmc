#include "PlexNetworkServiceBrowser.h"
#include "PlexApplication.h"
#include "PlexMediaServerClient.h"

#include <vector>

using namespace std;

void
CPlexNetworkServiceBrowser::handleServiceArrival(NetworkServicePtr &service)
{
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer(service->getResourceIdentifier(), service->getParam("Name"), true));
  
  int port = 32400;
  try {
    port = boost::lexical_cast<int>(service->getParam("Port"));
  } catch (...) {
    eprintf("CPlexNetworkServiceBrowser::handleServiceArrival failed to get port?");
  }
  
  CPlexConnectionPtr conn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_DISCOVERED,
                                                                   service->address().to_string(),
                                                                   port));
  server->AddConnection(conn);
  server->UpdateReachability();

  g_plexApplication.serverManager->UpdateFromDiscovery(server);

  if (!server || server->GetUUID().empty())
    return;

  CSingleLock lk(m_serversSection);
  m_discoveredServers[server->GetUUID()] = server;
  dprintf("CPlexNetworkServiceBrowser::handleServiceArrival %s arrived", service->address().to_string().c_str());

  /* Update or reset our timer */
  SetAddTimer();
}

void
CPlexNetworkServiceBrowser::handleServiceDeparture(NetworkServicePtr &service)
{
  CLog::Log(LOGDEBUG, "CPlexNetworkServiceBrowser::handleServiceDeparture departing with server %s last seen %f", service->getResourceIdentifier().c_str(), service->timeSinceLastSeen());
  CSingleLock lk(m_serversSection);
  /* Remove the server from m_discoveredServers and then tell ServerManager to update it's state */
  if (m_discoveredServers.find(service->getResourceIdentifier()) != m_discoveredServers.end())
    m_discoveredServers.erase(service->getResourceIdentifier());

  PlexServerList list;
  BOOST_FOREACH(PlexServerPair p, m_discoveredServers)
    list.push_back(p.second);

  CLog::Log(LOGDEBUG, "CPlexNetworkServiceBrowser::handleServiceDeparture we have %lu servers from GDM", list.size());
  g_plexApplication.serverManager->UpdateFromConnectionType(list, CPlexConnection::CONNECTION_DISCOVERED);

  SetAddTimer();
}

void CPlexNetworkServiceBrowser::handleNetworkChange(const vector<NetworkInterface> &interfaces)
{
  NetworkServiceBrowser::handleNetworkChange(interfaces);

  // refresh myPlex as well
  g_plexApplication.myPlexManager->Refresh();

  // publish our device to plex
  g_plexApplication.mediaServerClient->publishDevice();
}

void
CPlexNetworkServiceBrowser::SetAddTimer()
{
  m_addTimer.expires_from_now(boost::posix_time::milliseconds(5000));
  m_addTimer.async_wait(boost::bind(&CPlexNetworkServiceBrowser::HandleAddTimeout, this, boost::asio::placeholders::error));
}

void
CPlexNetworkServiceBrowser::HandleAddTimeout(const boost::system::error_code &e)
{
  if (e == boost::asio::error::operation_aborted)
    return;

  CSingleLock lk(m_serversSection);
  PlexServerList list;
  BOOST_FOREACH(PlexServerPair p, m_discoveredServers)
    list.push_back(p.second);

  g_plexApplication.serverManager->UpdateFromConnectionType(list, CPlexConnection::CONNECTION_DISCOVERED);
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
