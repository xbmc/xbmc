/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include <memory>
#include <map>

#include "network/ZeroconfBrowser.h"
#include "threads/Thread.h"
#include "threads/CriticalSection.h"

#include <CoreFoundation/CoreFoundation.h>
#if defined(TARGET_DARWIN_OSX)
  #include <CoreServices/CoreServices.h>
#else
  #include <CFNetwork/CFNetServices.h>
#endif

//platform specific implementation of  zeroconfbrowser interface using native os x APIs
class CZeroconfBrowserOSX : public CZeroconfBrowser
{
public:
  CZeroconfBrowserOSX();
  ~CZeroconfBrowserOSX();

private:
  ///implementation if CZeroconfBrowser interface
  ///@{
  virtual bool doAddServiceType(const std::string &fcr_service_type);
  virtual bool doRemoveServiceType(const std::string &fcr_service_type);

  virtual std::vector<CZeroconfBrowser::ZeroconfService> doGetFoundServices();
  virtual bool doResolveService(CZeroconfBrowser::ZeroconfService &fr_service, double f_timeout);
  ///@}

  /// browser callback
  static void BrowserCallback(CFNetServiceBrowserRef browser, CFOptionFlags flags, CFTypeRef domainOrService, CFStreamError *error, void *info);
  /// resolve callback
  static void ResolveCallback(CFNetServiceRef theService, CFStreamError *error, void *info);

  /// adds the service to list of found services
  void addDiscoveredService(CFNetServiceBrowserRef browser, CFOptionFlags flags, ZeroconfService const &fcr_service);
  /// removes the service from list of found services
  void removeDiscoveredService(CFNetServiceBrowserRef browser, CFOptionFlags flags, ZeroconfService const &fcr_service);
  
  //CF runloop ref; we're using main-threads runloop
  CFRunLoopRef m_runloop;
  
  //shared variables (with guard)
  //TODO: split the guard for discovered, resolved access?
  CCriticalSection m_data_guard;
  // tBrowserMap maps service types the corresponding browser
  typedef std::map<std::string, CFNetServiceBrowserRef> tBrowserMap;
  tBrowserMap m_service_browsers;
  //tDiscoveredServicesMap maps browsers to their discovered services + a ref-count for each service
  //ref-count is needed, because a service might pop up more than once, if there's more than one network-iface
  typedef std::map<CFNetServiceBrowserRef, std::vector<std::pair<ZeroconfService, unsigned int> > > tDiscoveredServicesMap;
  tDiscoveredServicesMap m_discovered_services;
};
