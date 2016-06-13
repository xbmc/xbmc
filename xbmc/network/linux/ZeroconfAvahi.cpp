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

#include "PlatformDefs.h"
#include "ZeroconfAvahi.h"

#ifdef HAS_AVAHI

#include <string>
#include <iostream>
#include <sstream>
#include <cassert>
#include <avahi-client/client.h>
#include <avahi-common/thread-watch.h>
#include <avahi-common/alternative.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>

#include <unistd.h> //gethostname

#include <utils/log.h>

///helper RAII-struct to block event loop for modifications
struct ScopedEventLoopBlock
{
  ScopedEventLoopBlock(AvahiThreadedPoll* fp_poll):mp_poll(fp_poll)
  {
    avahi_threaded_poll_lock(mp_poll);
  }

  ~ScopedEventLoopBlock()
  {
    avahi_threaded_poll_unlock(mp_poll);
  }
private:
  AvahiThreadedPoll* mp_poll;
};

///helper to store information on howto create an avahi-group to publish
struct CZeroconfAvahi::ServiceInfo
{
  ServiceInfo(const std::string& fcr_type, const std::string& fcr_name,
              unsigned int f_port, AvahiStringList *txt, AvahiEntryGroup* fp_group = 0):
    m_type(fcr_type), m_name(fcr_name), m_port(f_port), mp_txt(txt), mp_group(fp_group)
  {
  }

  std::string m_type;
  std::string m_name;
  unsigned int m_port;
  AvahiStringList* mp_txt;
  AvahiEntryGroup* mp_group;
};

CZeroconfAvahi::CZeroconfAvahi(): mp_client(0), mp_poll (0), m_shutdown(false),m_thread_id(0)
{
    if (! (mp_poll = avahi_threaded_poll_new()))
    {
      CLog::Log(LOGERROR, "CZeroconfAvahi::CZeroconfAvahi(): Could not create threaded poll object");
      //! @todo throw exception?
      return;
    }

    if (!createClient())
    {
      CLog::Log(LOGERROR, "CZeroconfAvahi::CZeroconfAvahi(): Could not create client");
      //yeah, what if not? but should always succeed
    }

    //start event loop thread
    if (avahi_threaded_poll_start(mp_poll) < 0)
    {
      CLog::Log(LOGERROR, "CZeroconfAvahi::CZeroconfAvahi(): Failed to start avahi client thread");
    }
}

CZeroconfAvahi::~CZeroconfAvahi()
{
  CLog::Log(LOGDEBUG, "CZeroconfAvahi::~CZeroconfAvahi() Going down! cleaning up...");

  if (mp_poll)
  {
    //normally we would stop the avahi thread here and do our work, but
    //it looks like this does not work -> www.avahi.org/ticket/251
    //so instead of calling
    //avahi_threaded_poll_stop(mp_poll);
    //we set m_shutdown=true, post an event and wait for it to stop itself
    struct timeval tv = { 0, 0 }; //! @todo does tv survive the thread?
    AvahiTimeout* lp_timeout;
    {
      ScopedEventLoopBlock l_block(mp_poll);
      const AvahiPoll* cp_apoll = avahi_threaded_poll_get(mp_poll);
      m_shutdown = true;
      lp_timeout = cp_apoll->timeout_new(cp_apoll,
                                         &tv,
                                         shutdownCallback,
                                         this);
    }

    //now wait for the thread to stop
    assert(m_thread_id);
    pthread_join(m_thread_id, NULL);
    avahi_threaded_poll_get(mp_poll)->timeout_free(lp_timeout);
  }

  //free the client (frees all browsers, groups, ...)
  if (mp_client)
    avahi_client_free(mp_client);
  if (mp_poll)
    avahi_threaded_poll_free(mp_poll);
}

bool CZeroconfAvahi::doPublishService(const std::string& fcr_identifier,
                              const std::string& fcr_type,
                              const std::string& fcr_name,
                              unsigned int f_port,
                              const std::vector<std::pair<std::string, std::string> >&  txt)
{
  CLog::Log(LOGDEBUG, "CZeroconfAvahi::doPublishService identifier: %s type: %s name:%s port:%i", fcr_identifier.c_str(), fcr_type.c_str(), fcr_name.c_str(), f_port);

  ScopedEventLoopBlock l_block(mp_poll);
  tServiceMap::iterator it = m_services.find(fcr_identifier);
  if (it != m_services.end())
  {
    //fcr_identifier exists, no update functionality yet, so exit
    return false;
  }

  //txt records to AvahiStringList
  AvahiStringList *txtList = NULL;
  for(std::vector<std::pair<std::string, std::string> >::const_iterator it=txt.begin(); it!=txt.end(); ++it)
  {
    txtList = avahi_string_list_add_pair(txtList, it->first.c_str(), it->second.c_str());
  }

  //create service info and add it to service map
  tServiceMap::mapped_type p_service_info(new CZeroconfAvahi::ServiceInfo(fcr_type, fcr_name, f_port, txtList));
  it = m_services.insert(it, std::make_pair(fcr_identifier, p_service_info));

  //if client is already running, directly try to add the new service
  if ( mp_client && avahi_client_get_state(mp_client) ==  AVAHI_CLIENT_S_RUNNING )
  {
    //client's already running, add this new service
    addService(p_service_info, mp_client);
  }
  else
  {
    CLog::Log(LOGDEBUG, "CZeroconfAvahi::doPublishService: client not running, queued for publishing");
  }
  return true;
}

bool CZeroconfAvahi::doForceReAnnounceService(const std::string& fcr_identifier)
{
  bool ret = false;
  ScopedEventLoopBlock l_block(mp_poll);
  tServiceMap::iterator it = m_services.find(fcr_identifier);
  if (it != m_services.end() && it->second->mp_group)
  {
    // to force a reannounce on avahi its enough to reverse the txtrecord list
    it->second->mp_txt = avahi_string_list_reverse(it->second->mp_txt);

    // this will trigger the reannouncement
    if ((avahi_entry_group_update_service_txt_strlst(it->second->mp_group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, AvahiPublishFlags(0),
                                              it->second->m_name.c_str(),
                                              it->second->m_type.c_str(), NULL, it->second->mp_txt)) >= 0)
      ret = true;
  }

  return ret;
}

bool CZeroconfAvahi::doRemoveService(const std::string& fcr_ident)
{
  CLog::Log(LOGDEBUG, "CZeroconfAvahi::doRemoveService named: %s", fcr_ident.c_str());
  ScopedEventLoopBlock l_block(mp_poll);
  tServiceMap::iterator it = m_services.find(fcr_ident);
  if (it == m_services.end())
  {
    return false;
  }
  
  if (it->second->mp_group)
  {
    avahi_entry_group_free(it->second->mp_group);
    it->second->mp_group = 0;
  }
  
  if(it->second->mp_txt)
  {
    avahi_string_list_free(it->second->mp_txt);
    it->second->mp_txt = NULL;
  }
  
  m_services.erase(it);
  return true;
}

void CZeroconfAvahi::doStop()
{
  ScopedEventLoopBlock l_block(mp_poll);
  for(tServiceMap::const_iterator it = m_services.begin(); it != m_services.end(); ++it)
  {
    if (it->second->mp_group)
    {
      avahi_entry_group_free(it->second->mp_group);
      it->second->mp_group = 0;
    }
    
    if(it->second->mp_txt)
    {
      avahi_string_list_free(it->second->mp_txt);
      it->second->mp_txt = NULL;
    }
  }
  m_services.clear();
}

void CZeroconfAvahi::clientCallback(AvahiClient* fp_client, AvahiClientState f_state, void* fp_data)
{
  CZeroconfAvahi* p_instance = static_cast<CZeroconfAvahi*>(fp_data);

  //store our thread ID and check for shutdown -> check details in destructor
  p_instance->m_thread_id = pthread_self();

  if (p_instance->m_shutdown)
  {
    avahi_threaded_poll_quit(p_instance->mp_poll);
    return;
  }
  switch(f_state)
  {
  case AVAHI_CLIENT_S_RUNNING:
    CLog::Log(LOGDEBUG, "CZeroconfAvahi::clientCallback: client is up and running");
    p_instance->updateServices(fp_client);
    break;

  case AVAHI_CLIENT_FAILURE:
    CLog::Log(LOGINFO, "CZeroconfAvahi::clientCallback: client failure. avahi-daemon stopped? Recreating client...");
    //We were forced to disconnect from server. now free and recreate the client object
    avahi_client_free(fp_client);
    p_instance->mp_client = 0;
    //freeing the client also frees all groups and browsers, pointers are undefined afterwards, so fix that now
    for(tServiceMap::const_iterator it = p_instance->m_services.begin(); it != p_instance->m_services.end(); ++it)
    {
      it->second->mp_group = 0;
    }
    p_instance->createClient();
    break;

  case AVAHI_CLIENT_S_COLLISION:
  case AVAHI_CLIENT_S_REGISTERING:
    //HERE WE SHOULD REMOVE ALL OF OUR SERVICES AND "RESCHEDULE" them for later addition
    CLog::Log(LOGDEBUG, "CZeroconfAvahi::clientCallback: uiuui; coll or reg, anyways, resetting groups");
    for(tServiceMap::const_iterator it = p_instance->m_services.begin(); it != p_instance->m_services.end(); ++it)
    {
      if (it->second->mp_group)
        avahi_entry_group_reset(it->second->mp_group);
    }
    break;

  case AVAHI_CLIENT_CONNECTING:
    CLog::Log(LOGINFO, "CZeroconfAvahi::clientCallback: avahi server not available. But may become later...");
    break;
  }
}

void CZeroconfAvahi::groupCallback(AvahiEntryGroup *fp_group, AvahiEntryGroupState f_state, void * fp_data)
{
  CZeroconfAvahi* p_instance = static_cast<CZeroconfAvahi*>(fp_data);
  //store our thread ID and check for shutdown -> check details in destructor
  p_instance->m_thread_id = pthread_self();
  if (p_instance->m_shutdown)
  {
    avahi_threaded_poll_quit(p_instance->mp_poll);
    return;
  }

  switch (f_state)
  {
  case AVAHI_ENTRY_GROUP_ESTABLISHED :
    // The entry group has been established successfully
    CLog::Log(LOGDEBUG, "CZeroconfAvahi::groupCallback: Service successfully established");
    break;

  case AVAHI_ENTRY_GROUP_COLLISION :
  {
    //need to find the ServiceInfo struct for this group
    tServiceMap::iterator it = p_instance->m_services.begin();
    for(; it != p_instance->m_services.end(); ++it)
    {
      if (it->second->mp_group == fp_group)
        break;
    }
    if( it != p_instance->m_services.end() ) {
      char* alt_name = avahi_alternative_service_name( it->second->m_name.c_str() );
      it->second->m_name = alt_name;
      avahi_free(alt_name);
      CLog::Log(LOGNOTICE, "CZeroconfAvahi::groupCallback: Service name collision. Renamed to: %s", it->second->m_name.c_str());
      p_instance->addService(it->second, p_instance->mp_client);
    }
    break;
  }

  case AVAHI_ENTRY_GROUP_FAILURE:
    CLog::Log(LOGERROR, "CZeroconfAvahi::groupCallback: Entry group failure: %s ",
              (fp_group) ?
              avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(fp_group)))
              : "Unknown");
    //free the group and set to 0 so it may be added later
    if (fp_group)
    {
      //need to find the ServiceInfo struct for this group
      tServiceMap::iterator it = p_instance->m_services.begin();
      for (; it != p_instance->m_services.end(); ++it)
      {
        if (it->second->mp_group == fp_group)
        {
          avahi_entry_group_free(it->second->mp_group);
          it->second->mp_group = 0;
          if (it->second->mp_txt)
          {
            avahi_string_list_free(it->second->mp_txt);
            it->second->mp_txt = NULL;
          }
          break;
        }
      }
    }
    break;

  case AVAHI_ENTRY_GROUP_UNCOMMITED:
  case AVAHI_ENTRY_GROUP_REGISTERING:
  default:
    break;
  }
}

void CZeroconfAvahi::shutdownCallback(AvahiTimeout *fp_e, void *fp_data)
{
  CZeroconfAvahi* p_instance = static_cast<CZeroconfAvahi*>(fp_data);
  //should only be called on shutdown
  if (p_instance->m_shutdown)
  {
    avahi_threaded_poll_quit(p_instance->mp_poll);
  }
}

bool CZeroconfAvahi::createClient()
{
    if (mp_client)
    {
      avahi_client_free(mp_client);
    }
    mp_client = avahi_client_new(avahi_threaded_poll_get(mp_poll),
                                 AVAHI_CLIENT_NO_FAIL, &clientCallback,this,0);
    if (!mp_client)
      return false;
    return true;
}

void CZeroconfAvahi::updateServices(AvahiClient* fp_client)
{
  for(tServiceMap::const_iterator it = m_services.begin(); it != m_services.end(); ++it)
  {
    if (!it->second->mp_group)
      addService(it->second, fp_client);
  }
}

void CZeroconfAvahi::addService(tServiceMap::mapped_type fp_service_info, AvahiClient* fp_client)
{
  assert(fp_client);
  CLog::Log(LOGDEBUG, "CZeroconfAvahi::addService() named: %s type: %s port:%i", fp_service_info->m_name.c_str(), fp_service_info->m_type.c_str(), fp_service_info->m_port);
  //create the group if it doesn't exist
  if (!fp_service_info->mp_group)
  {
    if (!(fp_service_info->mp_group = avahi_entry_group_new(fp_client, &CZeroconfAvahi::groupCallback, this)))
    {
      CLog::Log(LOGDEBUG, "CZeroconfAvahi::addService() avahi_entry_group_new() failed: %s", avahi_strerror(avahi_client_errno(fp_client)));
      fp_service_info->mp_group = 0;
      return;
    }
  }


  // add entries to the group if it's empty
  int ret;
  if (avahi_entry_group_is_empty(fp_service_info->mp_group))
  {
    if ((ret = avahi_entry_group_add_service_strlst(fp_service_info->mp_group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, AvahiPublishFlags(0),
                                             fp_service_info->m_name.c_str(),
                                             fp_service_info->m_type.c_str(), NULL, NULL, fp_service_info->m_port, fp_service_info->mp_txt)) < 0)
    {
      if (ret == AVAHI_ERR_COLLISION)
      {
        char* alt_name = avahi_alternative_service_name(fp_service_info->m_name.c_str());
        fp_service_info->m_name = alt_name;
        avahi_free(alt_name);
        CLog::Log(LOGNOTICE, "CZeroconfAvahi::addService: Service name collision. Renamed to: %s", fp_service_info->m_name.c_str());
        addService(fp_service_info, fp_client);
        return;
      }
      CLog::Log(LOGERROR, "CZeroconfAvahi::addService(): failed to add service named:%s@$(HOSTNAME) type:%s port:%i. Error:%s :/ FIXME!",
                fp_service_info->m_name.c_str(), fp_service_info->m_type.c_str(), fp_service_info->m_port,  avahi_strerror(ret));
      return;
    }
  }

  // Tell the server to register the service
  if ((ret = avahi_entry_group_commit(fp_service_info->mp_group)) < 0)
  {
    CLog::Log(LOGERROR, "CZeroconfAvahi::addService(): Failed to commit entry group! Error:%s",  avahi_strerror(ret));
    //! @todo what now? reset the group? free it?
  }
}

#endif // HAS_AVAHI

