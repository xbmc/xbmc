#if (defined(_LINUX) && ! defined(__APPLE__))

#import "ZeroconfAvahi.h"
#include <string>
#include <iostream>
#include <sstream>
#include <cassert>
#include <avahi-client/client.h>
#include <avahi-common/thread-watch.h>
#include <avahi-common/error.h>
#include <unistd.h> //gethostname

//TODO: fix log output to xbmc methods
//TODO: throw exception in constructor?

///helper RAII-struct to block event loop for modifications
struct ScopedEventLoopBlock{
	ScopedEventLoopBlock(AvahiThreadedPoll* fp_poll):mp_poll(fp_poll){
		avahi_threaded_poll_lock(mp_poll);
	}
	~ScopedEventLoopBlock(){
		avahi_threaded_poll_unlock(mp_poll);
	}
	private:
		AvahiThreadedPoll* mp_poll;
};

///helper to store information on howto create an avahi-group to publish
struct CZeroconfAvahi::ServiceInfo{
  ServiceInfo(const std::string& fcr_type, const std::string& fcr_name, unsigned int f_port, AvahiEntryGroup* fp_group = 0):
    m_type(fcr_type), m_name(fcr_name), m_port(f_port), mp_group(fp_group){
  }
  
  std::string m_type;
  std::string m_name;
  unsigned int m_port;
  
  AvahiEntryGroup* mp_group;
};

CZeroconfAvahi::CZeroconfAvahi(): mp_client(0), mp_poll (0), m_shutdown(false),m_thread_id(0)
{
	if(! (mp_poll = avahi_threaded_poll_new())){
		std::cerr << "Ouch. Could not create threaded poll object" << std::endl;
		return;
	}
	if(!createClient()){
		//yeah, what if not? 
		//TODO
	}
	//start event loop thread
	if(avahi_threaded_poll_start(mp_poll) < 0){
		std::cerr << "Ouch. Failed to start avahi client thread" << std::endl;
	}
}

CZeroconfAvahi::~CZeroconfAvahi(){
	std::cerr << "CZeroconfAvahi::~CZeroconfAvahi() Going down! cleaning up.. " <<std::endl;
	
    if(mp_poll){
        //normally we would stop the avahi thread here and do our work, but
        //it looks like this does not work -> www.avahi.org/ticket/251
        //so instead of calling
        //avahi_threaded_poll_stop(mp_poll);
        //we set m_shutdown=true, post an event and wait for it to stop itself
        struct timeval tv = { 0, 0 }; //TODO: does tv survive the thread?
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
    if(mp_client)
        avahi_client_free(mp_client); 
    if(mp_poll)
        avahi_threaded_poll_free(mp_poll);
}

bool CZeroconfAvahi::doPublishService(const std::string& fcr_identifier,
                              const std::string& fcr_type,
                              const std::string& fcr_name,
                              unsigned int f_port)
{
  std::cerr << " CZeroconfAvahi::doPublishService" <<std::endl;

  ScopedEventLoopBlock l_block(mp_poll);
  tServiceMap::iterator it = m_services.find(fcr_identifier);
  if(it != m_services.end()){
    //fcr_identifier exists, no update functionality yet, so exit
    return false;
  } 

  //create service info and add it to service map
  tServiceMap::mapped_type p_service_info(new CZeroconfAvahi::ServiceInfo(fcr_type, fcr_name, f_port));
  it = m_services.insert(it, std::make_pair(fcr_identifier, p_service_info));

  //if client is already running, directly try to add the new service
	if( mp_client && avahi_client_get_state(mp_client) ==  AVAHI_CLIENT_S_RUNNING ){
    //client's already running, add this new service
		addService(p_service_info);
  } else {
		std::cerr << "queued webserver for publishing" <<std::endl;
  }
  return true;
}

bool CZeroconfAvahi::doRemoveService(const std::string& fcr_ident){
  std::cerr << "CZeroconfAvahi::doRemoveService named:" << fcr_ident << std::endl;
  ScopedEventLoopBlock l_block(mp_poll);
  tServiceMap::iterator it = m_services.find(fcr_ident);
  if(it == m_services.end()){
    return false;
  }
  if(it->second->mp_group){
    avahi_entry_group_free(it->second->mp_group);
    it->second->mp_group = 0;
  } 
  m_services.erase(it);
  return true;
}

bool CZeroconfAvahi::doHasService(const std::string& fcr_ident){
  return (m_services.find(fcr_ident) != m_services.end());
}

void CZeroconfAvahi::doStop(){
  for(tServiceMap::const_iterator it = m_services.begin(); it != m_services.end(); ++it){
    if(it->second->mp_group){
      avahi_entry_group_free(it->second->mp_group);
      it->second->mp_group = 0;
    }
  }
  m_services.clear();
}

void CZeroconfAvahi::clientCallback(AvahiClient* fp_client, AvahiClientState f_state, void* fp_data){
	CZeroconfAvahi* p_instance = static_cast<CZeroconfAvahi*>(fp_data);
	//store our thread ID and check for shutdown -> check details in destructor
	p_instance->m_thread_id = pthread_self();
    if(p_instance->m_shutdown){
        avahi_threaded_poll_quit(p_instance->mp_poll);
        return;
    }
	switch(f_state){
		case AVAHI_CLIENT_S_RUNNING:
        std::cout << "Client's up and running!" << std::endl;
        p_instance->updateServices();
			break;
		case AVAHI_CLIENT_FAILURE:
			std::cerr << "Avahi client failure: " << avahi_strerror(avahi_client_errno(fp_client)) << ". Recreating client.."<< std::endl;
			//We were forced to disconnect from server. now free and recreate the client object
      avahi_client_free(p_instance->mp_client);
      p_instance->mp_client = 0;
      //freeing the client also frees all groups and browsers, pointers are undefined afterwards, so fix that now
      for(tServiceMap::const_iterator it = p_instance->m_services.begin(); it != p_instance->m_services.end(); ++it){
        it->second->mp_group = 0;
      } 
			p_instance->createClient();
			break;
		case AVAHI_CLIENT_S_COLLISION:
		case AVAHI_CLIENT_S_REGISTERING:
			//HERE WE SHOULD REMOVE ALL OF OUR SERVICES AND "RESCHEDULE" them for later addition
			std::cerr << "uiuui; coll or reg, anyways, reset our groups" <<std::endl;
        for(tServiceMap::const_iterator it = p_instance->m_services.begin(); it != p_instance->m_services.end(); ++it){
          if(it->second->mp_group)
            avahi_entry_group_reset(it->second->mp_group);
        }
			break;
		case AVAHI_CLIENT_CONNECTING:
			std::cerr << "avahi server not available. But may become later..." << std::endl;
			break;
	}
}

void CZeroconfAvahi::groupCallback(AvahiEntryGroup *fp_group, AvahiEntryGroupState f_state, void * fp_data){
	CZeroconfAvahi* p_instance = static_cast<CZeroconfAvahi*>(fp_data);
	//store our thread ID and check for shutdown -> check details in destructor
	p_instance->m_thread_id = pthread_self();
    if(p_instance->m_shutdown){
        avahi_threaded_poll_quit(p_instance->mp_poll);
        return;
    }
	switch (f_state) {
		case AVAHI_ENTRY_GROUP_ESTABLISHED :
			/* The entry group has been established successfully */
			std::cerr << "Service successfully established." << std::endl;
			break;

		case AVAHI_ENTRY_GROUP_COLLISION :
			std::cerr << "Service name collision... FIXME!." << std::endl;
			//TODO handle collision rename, etc and directly readd them; don't free, but just change name and recommit
			break;

		case AVAHI_ENTRY_GROUP_FAILURE :
			std::cerr << "Entry group failure: "<< avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(fp_group))) << " FIXME!!" << std::endl;		
			//Some kind of failure happened while we were registering our services <--- and that means exactly what!?!
			//TODO do we need to reset that group?
			break;

		case AVAHI_ENTRY_GROUP_UNCOMMITED:
		case AVAHI_ENTRY_GROUP_REGISTERING:
			//TODO
			std::cerr << "Entry group uncomitted or registering... " << std::endl;		

		;
	}
}

void CZeroconfAvahi::shutdownCallback(AvahiTimeout *fp_e, void *fp_data){
	CZeroconfAvahi* p_instance = static_cast<CZeroconfAvahi*>(fp_data);
    //should only be called on shutdown 
    if(p_instance->m_shutdown){
        avahi_threaded_poll_quit(p_instance->mp_poll);
    }
}

std::string CZeroconfAvahi::assemblePublishedName(const std::string& fcr_prefix){
	std::stringstream ss;
	ss << fcr_prefix << '@';
	
	//get our hostname
	char lp_hostname[256];
	if(gethostname(lp_hostname, sizeof(lp_hostname))){
		//TODO
		std::cerr << "could not get hostname.. hm... waaaah! PANIC! " << std::endl;
		ss << "DummyThatCantResolveItsName";
	} else {
		ss << lp_hostname;
	}
	return ss.str();
}

bool CZeroconfAvahi::createClient()
{
	if(mp_client){
		avahi_client_free(mp_client);
	}
	mp_client = avahi_client_new(avahi_threaded_poll_get(mp_poll), 
                                 AVAHI_CLIENT_NO_FAIL, &clientCallback,this,0);
	if(!mp_client)
	{
		std::cerr << "Ouch. Failed to create avahi client" << std::endl;
		mp_client = 0;
		return false;
	}
	return true;
}

void CZeroconfAvahi::updateServices(){
  for(tServiceMap::const_iterator it = m_services.begin(); it != m_services.end(); ++it){
    if(!it->second->mp_group){
      addService(it->second);
    }
  }
}

void CZeroconfAvahi::addService(tServiceMap::mapped_type fp_service_info){
  std::cerr << "CZeroconfAvahi::addService()" << std::endl;
  if(fp_service_info->mp_group)
    avahi_entry_group_reset(fp_service_info->mp_group);
  else if (!(fp_service_info->mp_group = avahi_entry_group_new(mp_client, &CZeroconfAvahi::groupCallback, this))){
    std::cerr << "avahi_entry_group_new() failed:" << avahi_strerror(avahi_client_errno(mp_client)) << std::endl;
    fp_service_info->mp_group = 0;
    return;
  }
  assert(avahi_entry_group_is_empty(fp_service_info->mp_group));

  int ret;
  if ((ret = avahi_entry_group_add_service(fp_service_info->mp_group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, AvahiPublishFlags(0), 
                                           assemblePublishedName(fp_service_info->m_name).c_str(), 
                                           fp_service_info->m_type.c_str(), NULL, NULL, fp_service_info->m_port, NULL) < 0))
  {
    if (ret == AVAHI_ERR_COLLISION){
      std::cerr << "Ouch name collision :/ FIXME!!" << std::endl; //TODO
      return;
    }
    std::cerr << "Failed to add service named " << fp_service_info << "@$(HOSTNAME) type: " 
              << fp_service_info->m_type << " on port " << fp_service_info->m_port << "Error was: "<< avahi_strerror(ret) << std::endl;
    return;
  }
  /* Tell the server to register the service */
  if ((ret = avahi_entry_group_commit(fp_service_info->mp_group)) < 0) {
    std::cerr << "Failed to commit entry group: " << avahi_strerror(ret) << std::endl;
    //TODO what now? reset the group? free it?
  }
}
#endif