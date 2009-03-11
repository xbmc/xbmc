#if (defined(_LINUX) || ! defined(__APPLE__))

#import "ZeroconfAvahi.h"
#include <string>
#include <iostream>
#include <sstream>
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



CZeroconfAvahi::CZeroconfAvahi(): mp_client(0), mp_poll (0), mp_webserver_group(0), m_publish_webserver(false){
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
	//does this work? -> www.avahi.org/ticket/251
	avahi_threaded_poll_stop(mp_poll);
	if(mp_webserver_group) 
		avahi_entry_group_free(mp_webserver_group);
	std::cerr << "...client" <<std::endl;
	avahi_client_free(mp_client); 
	std::cerr << "...poll" <<std::endl;
	avahi_threaded_poll_free(mp_poll);
	std::cerr << "finished " << std::endl;
}

void CZeroconfAvahi::doPublishWebserver(int f_port){
	ScopedEventLoopBlock l_block(mp_poll);
	m_publish_webserver = true;
	m_port = f_port;
	if( avahi_client_get_state(mp_client) ==  AVAHI_CLIENT_S_RUNNING ){
		std::cerr << "client already in running state. adding webserver" <<std::endl;
		addWebserverService();
	} else {
		std::cerr << "queuing webserver for publishing" <<std::endl;
	}

}

void  CZeroconfAvahi::doRemoveWebserver(){
	ScopedEventLoopBlock l_block(mp_poll);
	m_publish_webserver = false;
	if(!mp_webserver_group){
		std::cerr << "Webserver not published. Nothing todo" << std::endl;
	} else {
		std::cerr << "Removing webserver service..." << std::endl;
		avahi_entry_group_reset(mp_webserver_group);
	}
}

void CZeroconfAvahi::doStop(){
	doRemoveWebserver();
}

void CZeroconfAvahi::clientCallback(AvahiClient* fp_client, AvahiClientState f_state, void* fp_data){
	CZeroconfAvahi* p_instance = static_cast<CZeroconfAvahi*>(fp_data);
	switch(f_state){
		case AVAHI_CLIENT_S_RUNNING:
			std::cout << "Client's up and running!" << std::endl;
			if(p_instance->m_publish_webserver){
				p_instance->addWebserverService();
			}			
			break;
		case AVAHI_CLIENT_FAILURE:
			std::cerr << "Avahi client failure: " << avahi_strerror(avahi_client_errno(fp_client)) << ". Recreating client.."<< std::endl;
			//We were forced to disconnect from server. now recreate the client object
			if(p_instance->mp_webserver_group){
				avahi_entry_group_reset(p_instance->mp_webserver_group);
			}
			p_instance->createClient();
			
			break;
		case AVAHI_CLIENT_S_COLLISION:
		case AVAHI_CLIENT_S_REGISTERING:
			//HERE WE SHOULD REMOVE ALL OF OUR SERVICES AND "RESCHEDULE" them for later addition
			std::cerr << "uiuui; coll or reg, anyways, remove our groups" <<std::endl;
			if(p_instance->mp_webserver_group){
				std::cerr << "webserver removed" <<std::endl;
				avahi_entry_group_reset(p_instance->mp_webserver_group);
			}
			break;
		case AVAHI_CLIENT_CONNECTING:
			std::cerr << "avahi server not available. But may become later..." << std::endl;
			break;
	}
}

void CZeroconfAvahi::groupCallback(AvahiEntryGroup *fp_group, AvahiEntryGroupState f_state, void *){
	std::cerr << "groupCallback " << fp_group << " websrvgrp: " << std::endl;
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
			std::cerr << "Entry group uncomitted or regstering... " << std::endl;		

		;
	}
}

std::string CZeroconfAvahi::assembleWebserverServiceName(){
	std::stringstream ss;
	ss << GetWebserverPublishPrefix() << '@';
	
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

void CZeroconfAvahi::addWebserverService()
{
	std::cerr << "CZeroconfAvahi::addWebserverService()" << std::endl;
	if(mp_webserver_group)
		avahi_entry_group_reset(mp_webserver_group);
	else if (!(mp_webserver_group = avahi_entry_group_new(mp_client, &CZeroconfAvahi::groupCallback, NULL))){
		std::cerr << "avahi_entry_group_new() failed:" << avahi_strerror(avahi_client_errno(mp_client)) << std::endl;
		mp_webserver_group = 0;
		return;
	}
		
	if (avahi_entry_group_is_empty(mp_webserver_group)) {
		int ret;
		if ((ret = avahi_entry_group_add_service(mp_webserver_group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, AvahiPublishFlags(0), assembleWebserverServiceName().c_str(), 
				 "_http._tcp", NULL, NULL, m_port, NULL) < 0))
		{
			if (ret == AVAHI_ERR_COLLISION){
				std::cerr << "Ouch name collision :/ FIXME!!" << std::endl; //TODO
				return;
			}
			std::cerr << "Failed to add _http._tcp service: "<< avahi_strerror(ret) << std::endl;
			return;
		}
		/* Tell the server to register the service */
		if ((ret = avahi_entry_group_commit(mp_webserver_group)) < 0) {
			std::cerr << "Failed to commit entry group: " << avahi_strerror(ret) << std::endl;
			//TODO what now? reset the group?
		}
	}
}

#endif