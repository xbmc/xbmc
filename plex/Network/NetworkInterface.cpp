/*
 *  Copyright (C) 2010 Plex, Inc.   
 *
 *  Created on: Jan 3, 2011
 *      Author: Elan Feingold
 */

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#ifdef __APPLE__
#include <SystemConfiguration/SystemConfiguration.h>
#endif

#include "NetworkInterface.h"

#if defined(__APPLE__) || defined(__linux__) || defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <arpa/inet.h>

using namespace boost;

vector<NetworkInterface::callback_function> NetworkInterface::g_observers;
vector<NetworkInterface> NetworkInterface::g_interfaces;
boost::mutex NetworkInterface::g_mutex;

///////////////////////////////////////////////////////////////////////////////////////////////////
void NetworkInterface::GetAll(vector<NetworkInterface>& interfaces)
{
  struct ifaddrs *ifa;
  getifaddrs(&ifa);
  
  for (struct ifaddrs* pInterface = ifa; pInterface; pInterface = pInterface->ifa_next)
  {
    // Look for IPv4 interfaces which are UP and capable of multicast.
    if (pInterface->ifa_flags & IFF_UP && pInterface->ifa_addr && 
    #ifdef __APPLE__
        pInterface->ifa_flags & IFF_MULTICAST &&
    #endif
        !(pInterface->ifa_flags & IFF_POINTOPOINT) &&
        pInterface->ifa_addr->sa_family == AF_INET)
    {
      // Get the address as a string.
      char str[INET_ADDRSTRLEN];
      struct sockaddr_in *ifa_addr = (struct sockaddr_in *)pInterface->ifa_addr;
      inet_ntop(AF_INET, &ifa_addr->sin_addr, str, INET_ADDRSTRLEN);

      // Get the netmask as a string.
      char netmask[INET_ADDRSTRLEN];
      struct sockaddr_in *ifa_msk = (struct sockaddr_in *)pInterface->ifa_netmask;
      inet_ntop(AF_INET, &ifa_msk->sin_addr, netmask, INET_ADDRSTRLEN);

      // The index.
      int index = if_nametoindex(pInterface->ifa_name);
      
      interfaces.push_back(NetworkInterface(index, pInterface->ifa_name, str, pInterface->ifa_flags & IFF_LOOPBACK, netmask));
    }
  }
  
  // Free the structure.
  freeifaddrs(ifa);
}

#endif

#ifdef __APPLE__

///////////////////////////////////////////////////////////////////////////////////////////////////
void NetworkChanged(SCDynamicStoreRef store, CFArrayRef changedKeys, void *context)
{
  NetworkInterface::NotifyOfNetworkChange();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void NetworkInterface::WatchForChanges()
{
  // Get the initial list.
  NotifyOfNetworkChange();
  
  SCDynamicStoreRef store    = SCDynamicStoreCreate(NULL, CFSTR("Plex:WatchForNetworkChanges"), NetworkChanged, 0);
  CFMutableArrayRef keys     = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
  CFMutableArrayRef patterns = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
    
  // Interface list
  CFStringRef key = SCDynamicStoreKeyCreateNetworkInterface(NULL, kSCDynamicStoreDomainState);
  CFArrayAppendValue(keys, key);
  CFRelease(key);

  // IPv4.
  key = SCDynamicStoreKeyCreateNetworkGlobalEntity(NULL, kSCDynamicStoreDomainState, kSCEntNetIPv4);
  CFArrayAppendValue(keys, key);
  CFRelease(key);

  CFStringRef pattern = SCDynamicStoreKeyCreateNetworkServiceEntity(NULL, kSCDynamicStoreDomainState, kSCCompAnyRegex, kSCEntNetIPv4);
  CFArrayAppendValue(patterns, pattern);
  CFRelease(pattern);

  pattern = SCDynamicStoreKeyCreateNetworkInterfaceEntity(NULL, kSCDynamicStoreDomainState, kSCCompAnyRegex, kSCEntNetIPv4);
  CFArrayAppendValue(patterns, pattern);
  CFRelease(pattern);
  
  // Link.
  pattern = SCDynamicStoreKeyCreateNetworkInterfaceEntity(NULL, kSCDynamicStoreDomainState, kSCCompAnyRegex, kSCEntNetLink);
  CFArrayAppendValue(patterns, pattern);
  CFRelease(pattern);
  
  // AirPort (e.g. BSSID)
  pattern = SCDynamicStoreKeyCreateNetworkInterfaceEntity(NULL, kSCDynamicStoreDomainState, kSCCompAnyRegex, kSCEntNetAirPort);
  CFArrayAppendValue(patterns, pattern);
  CFRelease(pattern);
  
  // Set the keys and patterns.
  if (!SCDynamicStoreSetNotificationKeys(store, keys, patterns))
    eprintf("SCDynamicStoreSetNotificationKeys failed: %s", SCErrorString(SCError()));

  CFRunLoopSourceRef source = SCDynamicStoreCreateRunLoopSource(NULL, store, -1);
  if (!source) 
    eprintf("SCDynamicStoreCreateRunLoopSource failed: %s", SCErrorString(SCError()));

  CFRunLoopAddSource(CFRunLoopGetMain(), source, kCFRunLoopDefaultMode);
  CFRelease(source);
  
  CFRelease(patterns);
  CFRelease(keys);
}

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
bool NetworkInterface::IsLocalAddress(const string& address)
{
  boost::mutex::scoped_lock lk(g_mutex);
  
  BOOST_FOREACH(const NetworkInterface& xface, g_interfaces)
    if (xface.address() == address)
      return true;
  
  return false;
}
