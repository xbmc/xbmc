/*
 *      Copyright (C) 2012-2013 Team XBMC
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
#include <dns_sd.h>

//platform specific implementation of  zeroconfbrowser interface using native os x APIs
class CZeroconfBrowserMDNS : public CZeroconfBrowser
{
public:
  CZeroconfBrowserMDNS();
  ~CZeroconfBrowserMDNS();

private:
  ///implementation if CZeroconfBrowser interface
  ///@{
  virtual bool doAddServiceType(const std::string& fcr_service_type);
  virtual bool doRemoveServiceType(const std::string& fcr_service_type);

  virtual std::vector<CZeroconfBrowser::ZeroconfService> doGetFoundServices();
  virtual bool doResolveService(CZeroconfBrowser::ZeroconfService& fr_service, double f_timeout);
  ///@}

  /// browser callback
  static void DNSSD_API BrowserCallback(DNSServiceRef browser,
                                        DNSServiceFlags flags,
                                        uint32_t interfaceIndex,
                                        DNSServiceErrorType errorCode,
                                        const char *serviceName,
                                        const char *regtype,
                                        const char *replyDomain,
                                        void *context);
  /// GetAddrInfo callback
  static void DNSSD_API GetAddrInfoCallback(DNSServiceRef                    sdRef,
                                            DNSServiceFlags                  flags,
                                            uint32_t                         interfaceIndex,
                                            DNSServiceErrorType              errorCode,
                                            const char                       *hostname,
                                            const struct sockaddr            *address,
                                            uint32_t                         ttl,
                                            void                             *context
                                            );

  /// resolve callback
  static void DNSSD_API ResolveCallback(DNSServiceRef                       sdRef,
                                        DNSServiceFlags                     flags,
                                        uint32_t                            interfaceIndex,
                                        DNSServiceErrorType                 errorCode,
                                        const char                          *fullname,
                                        const char                          *hosttarget,
                                        uint16_t                            port,        /* In network byte order */
                                        uint16_t                            txtLen,
                                        const unsigned char                 *txtRecord,
                                        void                                *context
                                        );

  /// adds the service to list of found services
  void addDiscoveredService(DNSServiceRef browser, CZeroconfBrowser::ZeroconfService const& fcr_service);
  /// removes the service from list of found services
  void removeDiscoveredService(DNSServiceRef browser, CZeroconfBrowser::ZeroconfService const& fcr_service);
  // win32: process replies from the bonjour daemon
  void ProcessResults();

  //shared variables (with guard)
  CCriticalSection m_data_guard;
  // tBrowserMap maps service types the corresponding browser
  typedef std::map<std::string, DNSServiceRef> tBrowserMap;
  tBrowserMap m_service_browsers;
  //tDiscoveredServicesMap maps browsers to their discovered services + a ref-count for each service
  //ref-count is needed, because a service might pop up more than once, if there's more than one network-iface
  typedef std::map<DNSServiceRef, std::vector<std::pair<ZeroconfService, unsigned int> > > tDiscoveredServicesMap;
  tDiscoveredServicesMap m_discovered_services;
  DNSServiceRef m_browser;
  CZeroconfBrowser::ZeroconfService m_resolving_service;
  CEvent m_resolved_event;
  CEvent m_addrinfo_event;
};
