#pragma once

#define BOOST_ASIO_DISABLE_IOCP 1; // IOCP reactor reads failed using boost 1.44.

#include <boost/lexical_cast.hpp>
#include <vector>

#include "plex/PlexUtils.h"
#include "Network/NetworkServiceBrowser.h"
#include "PlexSourceScanner.h"
#include "PlexNetworkServiceAdvertiser.h"
#include "Client/PlexServerManager.h"
#include "settings/GUISettings.h"
#include "threads/Thread.h"

///
/// Plex specific service browser.
///
class CPlexNetworkServiceBrowser : public NetworkServiceBrowser
{
public:
  CPlexNetworkServiceBrowser(boost::asio::io_service& ioService, unsigned short port, int refreshTime=NS_BROWSE_REFRESH_INTERVAL)
  : NetworkServiceBrowser(ioService, port, refreshTime)
  {
  }

  /// Notify of a new service.
  virtual void handleServiceArrival(NetworkServicePtr& service);

  /// Notify of a service going away.
  virtual void handleServiceDeparture(NetworkServicePtr& service);

private:
  CCriticalSection m_serversSection;
  PlexServerMap m_discoveredServers;
};

///
/// Network service manager
///
class CPlexServiceListener : public CThread
{
public:
  CPlexServiceListener() : CThread("PlexServiceListener")
  {
    Create();
  }

  void Process();

  void OnExit()
  {
    StopAdvertisement();
    m_ioService.stop();
  }

  void StopAdvertisement()
  {
    if (m_plexAdvertiser)
    {
      dprintf("NetworkService: shutting down player advertisement");
      m_plexAdvertiser->stop();
      m_plexAdvertiser.reset();
    }
  }

  void StartAdvertisement()
  {
    // Player advertiser.
    if(g_guiSettings.GetBool("services.plexplayer"))
    {
      dprintf("NetworkService: starting player advertisement");
      m_plexAdvertiser = NetworkServiceAdvertiserPtr(new PlexNetworkServiceAdvertiser(m_ioService));
      m_plexAdvertiser->start();
    }
  }

  void ScanNow() { m_pmsBrowser->scanNow(); }

private:
  boost::asio::io_service     m_ioService;
  NetworkServiceBrowserPtr    m_pmsBrowser;
  NetworkServiceAdvertiserPtr m_plexAdvertiser;
};
