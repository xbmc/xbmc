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

///helper struct to block event loop for modifications
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
  //TODO
//	ScopedEventLoopBlock l_block(mp_poll);
//	m_publish_webserver = true;
//	m_port = f_port;
//	if( mp_client && avahi_client_get_state(mp_client) ==  AVAHI_CLIENT_S_RUNNING ){
//		std::cerr << "client already in running state. adding webserver" <<std::endl;
//		addWebserverService();
//	} else {
//		std::cerr << "queued webserver for publishing" <<std::endl;
//	}
}

bool CZeroconfAvahi::doRemoveService(const std::string& fcr_ident){
  //TODO
//	ScopedEventLoopBlock l_block(mp_poll);
//	m_publish_webserver = false;
//	if(!mp_webserver_group)
//		return;
//	if(avahi_entry_group_is_empty(mp_webserver_group)){
//		std::cerr << "Webserver not published. Nothing todo" << std::endl;
//	} else {
//		std::cerr << "Removing webserver service..." << std::endl;
//		avahi_entry_group_reset(mp_webserver_group);
//	}  
}

bool CZeroconfAvahi::doHasService(const std::string& fcr_ident){
  //TODO
}

void CZeroconfAvahi::doStop(){
   	doRemoveWebserver();
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
        updateServices();
			break;
		case AVAHI_CLIENT_FAILURE:
			std::cerr << "Avahi client failure: " << avahi_strerror(avahi_client_errno(fp_client)) << ". Recreating client.."<< std::endl;
			//We were forced to disconnect from server. now recreate the client object
      //TODO
//            if(p_instance->mp_webserver_group){
//                avahi_entry_group_free(p_instance->mp_webserver_group);
//                p_instance->mp_webserver_group = 0;
//            }
      p_instance->mp_client = 0;
			p_instance->createClient();
			break;
		case AVAHI_CLIENT_S_COLLISION:
		case AVAHI_CLIENT_S_REGISTERING:
			//HERE WE SHOULD REMOVE ALL OF OUR SERVICES AND "RESCHEDULE" them for later addition
			std::cerr << "uiuui; coll or reg, anyways, remove our groups" <<std::endl;
        //TODO
//			if(p_instance->mp_webserver_group){
//				std::cerr << "webserver removed" <<std::endl;
//				avahi_entry_group_reset(p_instance->mp_webserver_group);
//			}
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

void CZeroconfAvahi::updateServices()
{
//	std::cerr << "CZeroconfAvahi::addWebserverService()" << std::endl;
//	if(mp_webserver_group)
//		avahi_entry_group_reset(mp_webserver_group);
//	else if (!(mp_webserver_group = avahi_entry_group_new(mp_client, &CZeroconfAvahi::groupCallback, this))){
//		std::cerr << "avahi_entry_group_new() failed:" << avahi_strerror(avahi_client_errno(mp_client)) << std::endl;
//		mp_webserver_group = 0;
//		return;
//	}
//		
//	if (avahi_entry_group_is_empty(mp_webserver_group)) {
//		int ret;
//		if ((ret = avahi_entry_group_add_service(mp_webserver_group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, AvahiPublishFlags(0), assembleWebserverServiceName().c_str(), 
//				 "_http._tcp", NULL, NULL, m_port, NULL) < 0))
//		{
//			if (ret == AVAHI_ERR_COLLISION){
//				std::cerr << "Ouch name collision :/ FIXME!!" << std::endl; //TODO
//				return;
//			}
//			std::cerr << "Failed to add _http._tcp service: "<< avahi_strerror(ret) << std::endl;
//			return;
//		}
//		/* Tell the server to register the service */
//		if ((ret = avahi_entry_group_commit(mp_webserver_group)) < 0) {
//			std::cerr << "Failed to commit entry group: " << avahi_strerror(ret) << std::endl;
//			//TODO what now? reset the group?
//		}
//	}
}

#endif