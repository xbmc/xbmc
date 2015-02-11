#pragma once
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

#include "system.h"

#ifdef HAS_AVAHI

#include <memory>
#include <map>
#include <vector>
#include <string>
#include "network/Zeroconf.h"

#include <memory>
#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-common/defs.h>

struct AvahiThreadedPoll;

class CZeroconfAvahi : public CZeroconf
{
public:
  CZeroconfAvahi();
  ~CZeroconfAvahi();

protected:
  //implement base CZeroConf interface
  virtual bool doPublishService(const std::string& fcr_identifier,
                                const std::string& fcr_type,
                                const std::string& fcr_name,
                                unsigned int f_port,
                                const std::vector<std::pair<std::string, std::string> >& txt);

  virtual bool doForceReAnnounceService(const std::string& fcr_identifier);
  virtual bool doRemoveService(const std::string& fcr_ident);

  virtual void doStop();

private:
  ///this is where the client calls us if state changes
  static void clientCallback(AvahiClient* fp_client, AvahiClientState f_state, void*);
  ///here we get notified of group changes
  static void groupCallback(AvahiEntryGroup *fp_group, AvahiEntryGroupState f_state, void *);
  //shutdown callback; works around a problem in avahi < 0.6.24 see destructor for details
  static void shutdownCallback(AvahiTimeout *fp_e, void *);

  ///creates the avahi client;
  ///@return true on success
  bool createClient();

  //don't access stuff below without stopping the client thread
  //see http://avahi.org/wiki/RunningAvahiClientAsThread
  //and use struct ScopedEventLoopBlock

  //helper struct for holding information about creating a service / AvahiEntryGroup
  //we have to hold that as it's needed to recreate the service
  class ServiceInfo;
  typedef std::map<std::string, std::shared_ptr<ServiceInfo> > tServiceMap;

  //goes through a list of todos and publishs them (takes the client a param, as it might be called from
  // from the callbacks)
  void updateServices(AvahiClient* fp_client);
  //helper that actually does the work of publishing
  void addService(tServiceMap::mapped_type fp_service_info, AvahiClient* fp_client);

  AvahiClient* mp_client;
  AvahiThreadedPoll* mp_poll;

  //this holds all published and unpublished services including info on howto create them
  tServiceMap m_services;

  //2 variables below are needed for workaround of avahi bug (see destructor for details)
  bool m_shutdown;
  pthread_t m_thread_id;
};

#endif // HAS_AVAHI

