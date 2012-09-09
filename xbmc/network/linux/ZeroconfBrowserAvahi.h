#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "system.h"
#ifdef HAS_AVAHI

#include <memory>
#include <map>

#include "network/ZeroconfBrowser.h"
#include "threads/Thread.h"
#include "threads/CriticalSection.h"

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/thread-watch.h>
#include <avahi-common/defs.h>

//platform specific implementation of  zeroconfbrowser interface using avahi
class CZeroconfBrowserAvahi : public CZeroconfBrowser
{
  public:
    CZeroconfBrowserAvahi();
    ~CZeroconfBrowserAvahi();

  private:
    ///implementation if CZeroconfBrowser interface
    ///@{
    virtual bool doAddServiceType(const CStdString& fcr_service_type);
    virtual bool doRemoveServiceType(const CStdString& fcr_service_type);

    virtual std::vector<CZeroconfBrowser::ZeroconfService> doGetFoundServices();
    virtual bool doResolveService(CZeroconfBrowser::ZeroconfService& fr_service, double f_timeout);
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
    //helper to workaround avahi bug
    static void shutdownCallback(AvahiTimeout *fp_e, void *fp_data);
    //helpers
    bool createClient();
    static AvahiServiceBrowser* createServiceBrowser(const CStdString& fcr_service_type, AvahiClient* fp_client, void* fp_userdata);

    //shared variables between avahi thread and interface
    AvahiClient* mp_client;
    AvahiThreadedPoll* mp_poll;
    // tBrowserMap maps service types the corresponding browser
    typedef std::map<CStdString, AvahiServiceBrowser*> tBrowserMap;
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

    //2 variables below are needed for workaround of avahi bug (see destructor for details)
    bool m_shutdown;
    pthread_t m_thread_id;
};

#endif //HAS_AVAHI
