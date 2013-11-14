#pragma once

#define BOOST_ASIO_DISABLE_IOCP 1; // IOCP reactor reads failed using boost 1.44.

#include <boost/lexical_cast.hpp>
#include <vector>

#include "plex/PlexUtils.h"
#include "Network/NetworkServiceBrowser.h"
#include "Network/PlexNetworkServiceAdvertiser.h"
#include "Client/PlexServerManager.h"
#include "settings/GUISettings.h"
#include "threads/Thread.h"

#include <boost/asio/deadline_timer.hpp>
#include <boost/system/error_code.hpp>

#include "PlexServerManager.h"

///
/// Plex specific service browser.
///
class CPlexNetworkServiceBrowser : public NetworkServiceBrowser
{
public:
  CPlexNetworkServiceBrowser(boost::asio::io_service& ioService, unsigned short port, int refreshTime=NS_BROWSE_REFRESH_INTERVAL)
  : NetworkServiceBrowser(ioService, port, refreshTime), m_addTimer(ioService, boost::posix_time::milliseconds(5000))
  {
  }

  /// Notify of a new service.
  virtual void handleServiceArrival(NetworkServicePtr& service);

  /// Notify of a service going away.
  virtual void handleServiceDeparture(NetworkServicePtr& service);

  virtual void handleNetworkChange(const vector<NetworkInterface> &interfaces);

private:
  void SetAddTimer();
  void HandleAddTimeout(const boost::system::error_code& e);
  CCriticalSection m_serversSection;
  std::map<CStdString, CPlexServerPtr> m_discoveredServers;
  boost::asio::deadline_timer m_addTimer;
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

  void Stop()
  {
    StopAdvertisement();
    
    m_pmsBrowser.reset();
    m_ioService.stop();
    
    StopThread(true);
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
