/*
 *  Copyright (C) 2010 Plex, Inc.   
 *
 *  Created on: Dec 18, 2010
 *      Author: Jamie Kirkpatrick
 */

#pragma once

#define BOOST_ASIO_DISABLE_IOCP 1; // IOCP reactor reads failed using boost 1.44.

#include <boost/lexical_cast.hpp>

#include "plex/PlexUtils.h"
#include "Network/NetworkServiceBrowser.h"
#include "PlexSourceScanner.h"
#include "PlexNetworkServiceAdvertiser.h"
#include "PlexServerManager.h"

class PlexServiceListener;
typedef boost::shared_ptr < PlexServiceListener > PlexServiceListenerPtr;
typedef boost::shared_ptr < boost::thread > ThreadPtr;

///
/// Plex specific service browser.
///
class PlexNetworkServiceBrowser : public NetworkServiceBrowser
{
public:
  PlexNetworkServiceBrowser(boost::asio::io_service& ioService, unsigned short port, int refreshTime=NS_BROWSE_REFRESH_INTERVAL)
    : NetworkServiceBrowser(ioService, port, refreshTime)
  {
  }

  /// Notify of a new service.
  virtual void handleServiceArrival(NetworkServicePtr& service) 
  {
    map<string, string> params = service->getParams();
    for (map<string, string>::iterator param = params.begin(); param != params.end(); ++param)
    {
      string name = param->first;
      string value = param->second;
      CLog::CLog().Log(LOGINFO, "%s -> %s", param->first.c_str(), param->second.c_str());
    }
    
    // Scan the host.
    dprintf("NetworkServiceBrowser: SERVICE arrived: %s", service->address().to_string().c_str());
    PlexServerManager::Get().addServer(service->getResourceIdentifier(), service->getParam("Name"), service->address().to_string(), service->port());
  }
  
  /// Notify of a service going away.
  virtual void handleServiceDeparture(NetworkServicePtr& service) 
  {
    dprintf("NetworkServiceBrowser: SERVICE departed after not being seen for %f seconds: %s", service->timeSinceLastSeen(), service->address().to_string().c_str());
    PlexServerManager::Get().removeServer(service->getResourceIdentifier(), service->getParam("Name"), service->address().to_string(), service->port());
  }
  
  /// Notify of a service update.
  virtual void handleServiceUpdate(NetworkServicePtr& service)
  {
    // Update the server.
    dprintf("NetworkServiceBrowser: SERVICE updated: %s", service->address().to_string().c_str());
    time_t updatedAt = 0;
    if (service->hasParam("Updated-At"))
      updatedAt = boost::lexical_cast<time_t>(service->getParam("Updated-At"));
    
    PlexServerManager::Get().updateServer(service->getResourceIdentifier(), service->getParam("Name"), service->address().to_string(), service->port(), updatedAt);
  }

  /// Check reachability
  virtual bool handleServiceIsReachable(NetworkServicePtr &service)
  {
    return PlexServerManager::Get().checkServerReachability(service->getResourceIdentifier(), service->address().to_string(), service->port());
  }
};

///
/// Network service manager
/// 
class PlexServiceListener 
{
public:
  static PlexServiceListenerPtr Create()
  {
    return PlexServiceListenerPtr( new PlexServiceListener );
  }
  
  ~PlexServiceListener()
  {
    stop();
  }

  void start()
  {
    dprintf("NetworkService: Initializing.");
    
    // We start watching for changes in here.
    NetworkInterface::WatchForChanges();
       
    // Server browser.
    m_pmsBrowser = NetworkServiceBrowserPtr(new PlexNetworkServiceBrowser(m_ioService, NS_PLEX_MEDIA_SERVER_PORT));
    
    // Player advertiser.
    m_plexAdvertiser = NetworkServiceAdvertiserPtr(new PlexNetworkServiceAdvertiser(m_ioService));
    m_plexAdvertiser->start();
    
    // Start the I/O service in its own thread.
    m_ptrThread = ThreadPtr(new boost::thread(boost::bind(&boost::asio::io_service::run, &m_ioService)));
  }

  void stop()
  {
    m_plexAdvertiser->stop();
    
    m_ioService.stop();
    if (m_ptrThread)
	  {
		  m_ptrThread->join();
      m_ptrThread.reset();
	  }
  }

 private:
 
  PlexServiceListener()
  {
    start();
  }

  boost::asio::io_service     m_ioService;
  NetworkServiceBrowserPtr    m_pmsBrowser;
  NetworkServiceAdvertiserPtr m_plexAdvertiser;
  ThreadPtr                   m_ptrThread;
};

