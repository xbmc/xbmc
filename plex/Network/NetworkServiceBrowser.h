/*
 *  Copyright (C) 2010 Plex, Inc.   
 *
 *  Created on: Dec 16, 2010
 *      Author: Elan Feingold
 */

#pragma once

#include <set>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/timer.hpp>

#include "NetworkInterface.h"
#include "NetworkServiceBase.h"
#include "NetworkService.h"

#include "filesystem/CurlFile.h"

class NetworkServiceBrowser;
typedef boost::shared_ptr<NetworkServiceBrowser> NetworkServiceBrowserPtr;
typedef pair<boost::asio::ip::address, NetworkServicePtr> address_service_pair;
 
/////////////////////////////////////////////////////////////////////////////
class NetworkServiceBrowser : public NetworkServiceBase
{
 public:
  
  /// Constructor.
  NetworkServiceBrowser(boost::asio::io_service& ioService, unsigned short port, int refreshTime=NS_BROWSE_REFRESH_INTERVAL)
   : NetworkServiceBase(ioService)
   , m_port(port)
   , m_timer(ioService, boost::posix_time::milliseconds(10))
   , m_deletionTimer(ioService, boost::posix_time::milliseconds(1500))
   , m_refreshTime(refreshTime)
  {
    // Add a timer which we'll use to send out search requests.
    try { m_timer.async_wait(boost::bind(&NetworkServiceBrowser::handleTimeout, this)); }
    catch (std::exception&) { eprintf("Unable to create timer."); }
  }
  
  // Destructor.
  virtual ~NetworkServiceBrowser() {}
  
  /// Notify of a new service.
  virtual void handleServiceArrival(NetworkServicePtr& service) 
  {
    dprintf("NetworkServiceBrowser: SERVICE arrived: %s", service->address().to_string().c_str());
  }
  
  /// Notify of a service going away.
  virtual void handleServiceDeparture(NetworkServicePtr& service) 
  {
    dprintf("NetworkServiceBrowser: SERVICE departed after not being seen for %f seconds: %s", service->timeSinceLastSeen(), service->address().to_string().c_str());
  }

  /// Notify of a service update.
  virtual void handleServiceUpdate(NetworkServicePtr& service)
  {
    dprintf("NetworkServiceBrowser: SERVICE updated: %s", service->address().to_string().c_str());
  }

  /// See if server is still reachable
  virtual bool handleServiceIsReachable(NetworkServicePtr& service)
  {
    dprintf("NetworkServiceBrowser: SERVICE reachability: %s (will always be false)", service->address().to_string().c_str());
    return false;
  }
  
  /// Copy out the current service list.
  map<boost::asio::ip::address, NetworkServicePtr> copyServices()
  {
    boost::mutex::scoped_lock lk(m_mutex);
    
    map<boost::asio::ip::address, NetworkServicePtr> ret;
    typedef pair<boost::asio::ip::address, NetworkServicePtr> address_service_pair;
    BOOST_FOREACH(address_service_pair pair, m_services)
      ret[pair.first] = pair.second;
    
    return ret;
  }
  
 private:

  /// Handle network change.
  virtual void handleNetworkChange(const vector<NetworkInterface>& interfaces)
  {
    dprintf("Network change for browser, closing %ld browse sockets (%ld interfaces)", m_sockets.size(), interfaces.size());

    // Close the old one.
    BOOST_FOREACH(udp_socket_ptr socket, m_sockets)
      socket->close();
    
    // Create the new multicast receiver and bind to the designated port for receiving broadcast updates.
    if (m_multicastSocket)
      m_multicastSocket->close();
    
    m_multicastSocket = udp_socket_ptr(new boost::asio::ip::udp::socket(m_ioService));
    setupMulticastListener(m_multicastSocket, "0.0.0.0", NS_BROADCAST_ADDR, m_port+1);
    m_multicastSocket->async_receive_from(boost::asio::buffer(m_data, NS_MAX_PACKET_SIZE), m_endpoint, boost::bind(&NetworkServiceBrowser::handleRead, this, m_multicastSocket, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, 0));
    
    // Now create the browse sockets.
    m_sockets.clear();
    m_ignoredAddresses.clear();

    int interfaceIndex = 0;
    BOOST_FOREACH(const NetworkInterface& xface, interfaces)
    {
      // Don't add virtual interfaces.
      if (xface.name()[0] != 'v' && boost::starts_with(xface.address(), "169.254.") == false)
      {
        dprintf("NetworkService: Browsing on interface %s.", xface.address().c_str());
        
        // Create the new socket, and bind to any port. It doesn't matter, we just need to be able to receive
        // a UDP reply packet.
        //
        udp_socket_ptr socket = udp_socket_ptr(new boost::asio::ip::udp::socket(m_ioService));
        setupMulticastListener(socket, xface.address(), NS_BROADCAST_ADDR, 0, true);
        m_sockets.push_back(socket);
        
        // Wait for data.
        socket->async_receive_from(boost::asio::buffer(m_data, NS_MAX_PACKET_SIZE), m_endpoint, boost::bind(&NetworkServiceBrowser::handleRead, this, socket, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, interfaceIndex));
        interfaceIndex++;
      }
      else
      {
        // Sometimes we get packets from these interfaces, not sure why, but we'll ignore them.
        m_ignoredAddresses.insert(xface.address());
      }
    }
  }
  
  /// Send out the search request.
  void sendSearch()
  {
    // Send the search message.
    string msg = NS_SEARCH_MSG;
    boost::asio::ip::udp::endpoint broadcastEndpoint(NS_BROADCAST_ADDR, m_port);
    try 
    {
      // Yoohoo! Anyone there?
      BOOST_FOREACH(udp_socket_ptr socket, m_sockets)
        socket->send_to(boost::asio::buffer(msg, msg.size()), broadcastEndpoint);
      
      // Set a timer after which we'll declare services dead.
      m_deletionTimer.expires_at(m_timer.expires_at() + boost::posix_time::milliseconds(NS_REMOVAL_INTERVAL));
      m_deletionTimer.async_wait(boost::bind(&NetworkServiceBrowser::handleDeletionTimeout, this));
    }
    catch (std::exception& e)
    {
      wprintf("NetworkServiceBrowser: Error sending out discover packet: %s", e.what());
    }
  }
  
  /// Find a network service by resource identifier.
  NetworkServicePtr findServiceByIdentifier(const string& identifier)
  {
    BOOST_FOREACH(address_service_pair pair, m_services)
    {
      if (!pair.second)
        eprintf("How did a null service make it in for %s???", pair.first.to_string().c_str());
        
      if (pair.second && pair.second->getParam("Resource-Identifier") == identifier)
        return pair.second;
    }
    
    return NetworkServicePtr();
  }
  
  /// Handle incoming data.
  void handleRead(const udp_socket_ptr& socket, const boost::system::error_code& error, size_t bytes, int interfaceIndex)
  {
    if (!error)
    {
      // Parse out the parameters.
      string data = string(m_data, bytes);
      string cmd;
      map<string, string> params;
      parse(data, params, cmd);

      m_mutex.lock();
      
      // Get the source address of the packet. If it's a local address (meaning coming from
      // this machine), then replace it with 127.0.0.1. The reason for this is that we want
      // communication with the server to use localhost or private address, and not a public
      // address, which this address could potentially be, if the machine is acting as a gateway.
      //
      boost::asio::ip::address address = m_endpoint.address();
      if (NetworkInterface::IsLocalAddress(address.to_string()))
      {
        //dprintf("NetworkService: Changing %s to localhost.", address.to_string().c_str());
        address = boost::asio::ip::address::from_string("127.0.0.1");
      }
      
      // Look up the service.
      NetworkServicePtr service;
      if (m_services.find(address) != m_services.end())
        service = m_services[address];
      
      bool notifyAdd = false;
      bool notifyDel = false;
      bool notifyUpdate = boost::starts_with(cmd, "UPDATE");
      
      if (m_ignoredAddresses.find(address.to_string()) != m_ignoredAddresses.end())
      {
        // Ignore this packet.
        iprintf("NetworkService: Ignoring a packet from this uninteresting interface %s.", address.to_string().c_str());
      }
      else if (boost::starts_with(cmd, "BYE"))
      {
        // Whack it.
        notifyDel = true;
        m_services.erase(address);
      }
      else
      {
        // Determine if the service is new and save it, and notify.
        if (service)
        {
          // Freshen the service.
          service->freshen(params);
        }
        else
        {
          // If we can find it via identifier, replace the old one if it had a larger interface index.
          // We don't replace interface index 0, which should always be loopback.
          //
          NetworkServicePtr oldService = findServiceByIdentifier(params["Resource-Identifier"]);
          if (oldService)
          {
            dprintf("NetworkService: Have an old server at index %d and address %s (we just got packet from %s, index %d)", oldService->interfaceIndex(), oldService->address().to_string().c_str(), address.to_string().c_str(), interfaceIndex);
            
            // Whack the old one and treat it as an update.
            m_services.erase(oldService->address());
            notifyUpdate = true;
          }
          else
          {
            // It's brand new, we'll treat it as an add.
            notifyAdd = true;
          }
          
          // Add the new mapping.
          service = NetworkServicePtr(new NetworkService(address, interfaceIndex, params));
          m_services[address] = service;
        }
      }
        
      m_mutex.unlock();
      
      // If we're going to, notify.
      if (service)
      {
        if (notifyAdd)
          handleServiceArrival(service);
        else if (notifyDel)
          handleServiceDeparture(service);
        else if (notifyUpdate)
          handleServiceUpdate(service);
      }
    }
    else
    {
      eprintf("Network Service: Error in browser handle read: %d (%s) socket=%d", error.value(), error.message().c_str(), socket->native());
      usleep(1000 * 100);
    }
    
    // If the socket is open, keep receiving (On XP we need to abandon a socket for 10022 - An invalid argument was supplied - as well).
    if (socket->is_open() && error.value() != 10022)
      socket->async_receive_from(boost::asio::buffer(m_data, NS_MAX_PACKET_SIZE), m_endpoint, boost::bind(&NetworkServiceBrowser::handleRead, this, socket, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, interfaceIndex));
    else
      iprintf("Network Service: Abandoning browse socket, it was closed.");
  }
  
  /// Handle the deletion timer.
  void handleDeletionTimeout()
  {
    vector<NetworkServicePtr> deleteMe;
    
    {
      boost::mutex::scoped_lock lk(m_mutex);

      // Look for services older than the deletion interval.
      BOOST_FOREACH(address_service_pair pair, m_services)
      {
        if (pair.second->timeSinceLastSeen()*1000 > NS_DEAD_SERVER_TIME)
        {
          if (handleServiceIsReachable(pair.second))
          {
            pair.second->freshen();
            CLog::Log(LOGDEBUG, "We couldn't get a response from %s but we can still reach it, freshing it.", pair.second->address().to_string().c_str());
            continue;
          }
          else
          {
            deleteMe.push_back(pair.second);
          }
        }
      }
      
      // Whack em.
      BOOST_FOREACH(NetworkServicePtr& service, deleteMe)
        m_services.erase(service->address());
    }

    // Now notify, outside the boost::mutex.
    BOOST_FOREACH(NetworkServicePtr& service, deleteMe)
      handleServiceDeparture(service);
    
#if 0
    // Dump services.
    dprintf("SERVICES:");
    dprintf("==========================================");
    BOOST_FOREACH(address_service_pair pair, m_services)
    {
      dprintf(" -> Service: %s (%f seconds old)", pair.first.to_string().c_str(), pair.second->timeSinceCreation());
      map<string, string> params = pair.second->getParams();
      BOOST_FOREACH(string_pair pair, params)
        dprintf("    * %s: %s", pair.first.c_str(), pair.second.c_str());
    }
#endif
  }
  
  /// Handle the timer.
  void handleTimeout()
  {
    // Send a new request.
    sendSearch();
    
    // Wait again.
    m_timer.expires_at(m_timer.expires_at() + boost::posix_time::milliseconds(m_refreshTime));
    m_timer.async_wait(boost::bind(&NetworkServiceBrowser::handleTimeout, this));
  }
  
  ///////////////////////////////////////////////////////////////////////////////////////////////////
  void parse(const string& data, map<string, string>& msg, string& verb)
  {
    // Split into lines.
    vector<string> lines;
    split(lines, data, boost::is_any_of("\r\n"));

    if (lines.size() > 1)
      verb = lines[0];

    bool lastWasBlank = false;
    BOOST_FOREACH(string line, lines)
    {
      // See if we hit CRLF.
      if (lastWasBlank && line.size() == 0)
        break;
      
      // If it looks like a real header, split it and add to map.
      if (line.size() > 0 && line.find(":") != string::npos)
      {
        vector<string> nameValue;
        boost::split(nameValue, line, boost::is_any_of(":"));
        if (nameValue.size() == 2)
          msg[nameValue[0]] = nameValue[1].substr(1);
      } 
      
      lastWasBlank = (line.size() == 0);
    }
  }
  
  unsigned short                   m_port;
  vector<udp_socket_ptr>           m_sockets;
  udp_socket_ptr                   m_multicastSocket;
  std::set<std::string>            m_ignoredAddresses;
  boost::mutex                     m_mutex;
  boost::asio::deadline_timer      m_timer;
  boost::asio::deadline_timer      m_deletionTimer;
  int                              m_refreshTime;
  boost::asio::ip::udp::endpoint   m_endpoint;
  char                             m_data[NS_MAX_PACKET_SIZE];
  map<boost::asio::ip::address, NetworkServicePtr>  m_services;
};
