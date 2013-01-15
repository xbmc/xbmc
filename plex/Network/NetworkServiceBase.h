#pragma once

#include <boost/asio.hpp>

#include "Network/NetworkInterface.h"
#include "Network/NetworkService.h"

typedef boost::shared_ptr<boost::asio::ip::udp::socket> udp_socket_ptr;
typedef pair<string, udp_socket_ptr> address_socket_pair;

class NetworkServiceBase
{
 protected:

  /// Constructor.
  NetworkServiceBase(boost::asio::io_service& ioService)
    : m_ioService(ioService)
    , m_timer(ioService, boost::posix_time::seconds(2))
    , m_firstChange(true)
  {
    // Register for network changes.
    dprintf("%p: Creating new Network Service and registering for notifications.", this);
    NetworkInterface::RegisterObserver(boost::bind(&NetworkServiceBase::onNetworkChanged, this, _1));
  }
  
  /// Destructor.
  virtual ~NetworkServiceBase() {}

  /// Utility to set up a listener.
  void setupListener(const udp_socket_ptr& socket, const string& bindAddress, unsigned short port)
  {
    boost::asio::ip::udp::endpoint listenEndpoint(boost::asio::ip::address::from_string(bindAddress), port);
    socket->open(listenEndpoint.protocol());
    
    // Bind.
    try { socket->bind(listenEndpoint); }
    catch (std::exception& ex) { eprintf("NetworkService: Couldn't bind to port %d: %s", port, ex.what()); }
    
    // Reuse.
    try { socket->set_option(boost::asio::ip::udp::socket::reuse_address(true)); }
    catch (std::exception& ex) { eprintf("NetworkService: Couldn't reuse address: %s", ex.what()); }    
  }
  
  /// Utility to set up a multicast listener/broadcaster for a single interface.
  void setupMulticastListener(const udp_socket_ptr& socket, const string& bindAddress, const boost::asio::ip::address& groupAddr, unsigned short port, bool outboundInterface = false)
  {
    // Create the server socket.
    dprintf("NetworkService: Setting up multicast listener on %s:%d (outbound: %d)", bindAddress.c_str(), port, outboundInterface);
    
    // Bind.
    setupListener(socket, bindAddress, port);
    
    // Enable loopback.
    socket->set_option(boost::asio::ip::multicast::enable_loopback(true));
    
    // Join the multicast group after leaving it (just in case).
    try { socket->set_option(boost::asio::ip::multicast::leave_group(groupAddr)); } 
    catch (std::exception&) { }
    try { socket->set_option(boost::asio::ip::multicast::join_group(groupAddr)); }
    catch (std::exception& ex) { eprintf("NetworkService: Couldn't join multicast group: %s", ex.what()); }
    
    if (outboundInterface)
    {
      // Send out multicast packets on the specified interface.
      boost::asio::ip::address_v4 localInterface = boost::asio::ip::address_v4::from_string(bindAddress);
      boost::asio::ip::multicast::outbound_interface option(localInterface);
      try { socket->set_option(option); }
      catch (std::exception&) { eprintf("NetworkService: Unable to set option on socket."); }
    }
  }

  /// For subclasses to fill in.
  virtual void handleNetworkChange(const vector<NetworkInterface>& interfaces) = 0;

  boost::asio::io_service& m_ioService;
  boost::mutex m_mutex;
  boost::asio::deadline_timer m_timer;
  bool m_firstChange;

 private:

  void onNetworkChanged(const vector<NetworkInterface>& interfaces)
  {
    boost::mutex::scoped_lock lk(m_mutex);
    
    dprintf("%p: NetworkService got notification of changed network (first change: %d)", this, m_firstChange);
    
    if (m_firstChange)
    {
      // Dispatch the notification in an ASIO thread's context.
      dprintf("NetworkService: Quick dispatch of network change.");
      m_ioService.dispatch(boost::bind(&NetworkServiceBase::handleNetworkChange, this, interfaces));
      m_firstChange = false;
    }
    else
    {
      // Dispatch the change with a two second delay.
      dprintf("NetworkService: Dispatch network change after two second delay.");
      m_timer.expires_from_now(boost::posix_time::seconds(2));
      m_timer.async_wait(boost::bind(&NetworkServiceBase::handleNetworkChange, this, interfaces));
    }
  }
};
