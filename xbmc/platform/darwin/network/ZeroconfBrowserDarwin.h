/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <map>
#include <string>
#include <vector>

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
class CZeroconfBrowserDarwin : public CZeroconfBrowser
{
public:
  CZeroconfBrowserDarwin();
  ~CZeroconfBrowserDarwin() override;

private:
  ///implementation if CZeroconfBrowser interface
  ///@{
  bool doAddServiceType(const std::string& fcr_service_type) override;
  bool doRemoveServiceType(const std::string& fcr_service_type) override;

  std::vector<CZeroconfBrowser::ZeroconfService> doGetFoundServices() override;
  bool doResolveService(CZeroconfBrowser::ZeroconfService& fr_service, double f_timeout) override;
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
  CFRunLoopRef m_runloop = nullptr;

  //shared variables (with guard)
  //! @todo split the guard for discovered, resolved access?
  CCriticalSection m_data_guard;
  // tBrowserMap maps service types the corresponding browser
  typedef std::map<std::string, CFNetServiceBrowserRef> tBrowserMap;
  tBrowserMap m_service_browsers;
  //tDiscoveredServicesMap maps browsers to their discovered services + a ref-count for each service
  //ref-count is needed, because a service might pop up more than once, if there's more than one network-iface
  typedef std::map<CFNetServiceBrowserRef, std::vector<std::pair<ZeroconfService, unsigned int> > > tDiscoveredServicesMap;
  tDiscoveredServicesMap m_discovered_services;
};
