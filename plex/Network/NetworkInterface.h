/*
 *  Copyright (C) 2010 Plex, Inc.   
 *
 *  Created on: Jan 3, 2011
 *      Author: Elan Feingold
 */

#pragma once

#include <boost/asio.hpp>

#include <vector>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>

#include "PlexLog.h"

using namespace std;

class NetworkInterface
{
  typedef boost::function<void(vector<NetworkInterface>&)> callback_function;
  
 public:
  
  NetworkInterface(int index, const string& name, const string& address, bool loopback, const string& netmask)
    : m_index(index)
    , m_name(name)
    , m_address(address)
    , m_loopback(loopback)
    , m_netmask(netmask) {}

  virtual ~NetworkInterface() {}
  
  /// Called on initialization to start watching for interface changes.
  static void WatchForChanges();
  
  /// Return a list of the interfaces.
  static void GetAll(vector<NetworkInterface>& interfaces);
  
  /// Returns a list of the known interfaces, without actually polling.
  static void GetCachedList(vector<NetworkInterface>& interfaces)
  {
    g_mutex.lock();
    interfaces.assign(g_interfaces.begin(), g_interfaces.end());
    g_mutex.unlock();
  }
  
  /// Retrieve netmask for interface
  static string GetNetmaskForInterfaceWithAddress(const string& addr)
  {
    vector<NetworkInterface> interfaces;
    GetCachedList(interfaces);
    
    BOOST_FOREACH(NetworkInterface& xface, interfaces)
    {
      if (xface.address() == addr)
      {
        return xface.netmask();
      }
    }

    /// default to something that is pretty standard
    return string("255.255.255.0");
  }

  /// Get the primary IP address.
  static string GetPrimaryAddress()
  {
    string address;
    
    vector<NetworkInterface> interfaces;
    GetCachedList(interfaces);
    
    BOOST_FOREACH(NetworkInterface& xface, interfaces)
    {
      if (xface.loopback() == false)
      {
        address = xface.address();
        break;
      }
    }
    
    return address;
  }
  
  /// See if this address is local.
  static bool IsLocalAddress(const string& address);
  
  /// Register an observer of changes.
  static void RegisterObserver(callback_function observer)
  {
    // Save the observer.
    g_mutex.lock();
    g_observers.push_back(observer);
    observer(g_interfaces);
    g_mutex.unlock();
  }
  
  /// Called when a network change occurs.
  static void NotifyOfNetworkChange(bool forceNotify=false)
  {
    dprintf("NetworkInterface: Notified of network changed (force=%d)", forceNotify);

    // Get current list.
    vector<NetworkInterface> interfaces;
    GetAll(interfaces);
    
    boost::mutex::scoped_lock lk(g_mutex);
    
    // See if anything really changed.
    bool changed = (g_interfaces.size() != interfaces.size());
    for (size_t i=0; changed == false && i<interfaces.size(); i++)
      changed = !(interfaces[i] == g_interfaces[i]); 
    
    // Notify observers if we're actually changed.
    if (changed || forceNotify)
    {
      if (changed)
      {
        dprintf("Network interfaces:");
        BOOST_FOREACH(NetworkInterface& xface, interfaces)
          dprintf(" * %d %s (%s) (loopback: %d)", xface.index(), xface.name().c_str(), xface.address().c_str(), xface.loopback());
      }

      // Call the observers.
      BOOST_FOREACH(callback_function func, g_observers)
        func(interfaces);
      
      // Save the new list.
      g_interfaces.assign(interfaces.begin(), interfaces.end());
    }
    else
    {
      dprintf("Network change notification but nothing changed.");
    }
  }

  int           index() const { return m_index; }
  const string& name() const { return m_name; }
  const string& address() const { return m_address; }
  bool          loopback() const { return m_loopback; }
  const string& netmask() const { return m_netmask; }

  /// Get the broadcast address.
  string broadcast() const
  {
    boost::asio::ip::address_v4 address = boost::asio::ip::address_v4::from_string(m_address);
    boost::asio::ip::address_v4 netmask = boost::asio::ip::address_v4::from_string(m_netmask);
    
    /* this code differs from the one in PMS because PHT ships with a older version of boost that has
     * a bug that makes localhost addresses return bogus broadcast addresses */
    boost::asio::ip::address_v4 broadcast = boost::asio::ip::address_v4(address.to_ulong() | (netmask.to_ulong() ^ 0xFFFFFFFF));

    return broadcast.to_string();
  }

  /// Equality test.
  bool operator==(const NetworkInterface& rhs)
  {
    return (   index() == rhs.index() &&
                name() == rhs.name() &&
             address() == rhs.address() &&
            loopback() == rhs.loopback()   );
  }
  
 private:
  
  static vector<NetworkInterface> g_interfaces;
  static vector<callback_function> g_observers;
  static boost::mutex g_mutex;
  
  int    m_index;
  string m_name;
  string m_address;
  string m_netmask;
  bool   m_loopback;
};

