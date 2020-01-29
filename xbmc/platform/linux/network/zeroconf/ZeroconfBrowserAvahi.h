/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "network/ZeroconfBrowser.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <map>
#include <memory>
#include <vector>

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/defs.h>
#include <avahi-common/thread-watch.h>

//platform specific implementation of  zeroconfbrowser interface using avahi
class CZeroconfBrowserAvahi : public CZeroconfBrowser
{
  public:
    CZeroconfBrowserAvahi();
    ~CZeroconfBrowserAvahi() override;

  private:
    ///implementation if CZeroconfBrowser interface
    ///@{
    bool doAddServiceType(const std::string& fcr_service_type) override;
    bool doRemoveServiceType(const std::string& fcr_service_type) override;

    std::vector<CZeroconfBrowser::ZeroconfService> doGetFoundServices() override;
    bool doResolveService(CZeroconfBrowser::ZeroconfService& fr_service, double f_timeout) override;
    ///@}

    /// avahi callbacks
    ///@{
    static void clientCallback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata);
    static void browseCallback(
                               AvahiServiceBrowser *b,
                               AvahiIfIndex interface,
                               AvahiProtocol protocol,
                               AvahiBrowserEvent event,
                               const char *name,
                               const char *type,
                               const char *domain,
                               AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
                               void* userdata);
    static void resolveCallback(
                         AvahiServiceResolver *r,
                         AvahiIfIndex interface,
                         AvahiProtocol protocol,
                         AvahiResolverEvent event,
                         const char *name,
                         const char *type,
                         const char *domain,
                         const char *host_name,
                         const AvahiAddress *address,
                         uint16_t port,
                         AvahiStringList *txt,
                         AvahiLookupResultFlags flags,
                         AVAHI_GCC_UNUSED void* userdata);
    //helpers
    bool createClient();
    static AvahiServiceBrowser* createServiceBrowser(const std::string& fcr_service_type, AvahiClient* fp_client, void* fp_userdata);

    //shared variables between avahi thread and interface
    AvahiClient* mp_client =  0 ;
    AvahiThreadedPoll* mp_poll =  0 ;
    // tBrowserMap maps service types the corresponding browser
    typedef std::map<std::string, AvahiServiceBrowser*> tBrowserMap;
    tBrowserMap m_browsers;

    // if a browser is in this set, it already sent an ALL_FOR_NOW message
    // (needed to bundle GUI_MSG_UPDATE_PATH messages
    std::set<AvahiServiceBrowser*> m_all_for_now_browsers;

    //this information is needed for avahi to resolve a service,
    //so unfortunately we'll only be able to resolve services already discovered
    struct AvahiSpecificInfos
    {
      AvahiIfIndex interface;
      AvahiProtocol protocol;
    };
    typedef std::map<CZeroconfBrowser::ZeroconfService, AvahiSpecificInfos> tDiscoveredServices;
    tDiscoveredServices m_discovered_services;
    CZeroconfBrowser::ZeroconfService m_resolving_service;
    CEvent m_resolved_event;
};
