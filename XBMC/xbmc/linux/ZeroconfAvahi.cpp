#if (defined(_LINUX) || ! defined(__APPLE__))

#import "ZeroconfAvahi.h"
#include <string>
#include <iostream>
#include <avahi-client/client.h>
#include <avahi-common/thread-watch.h>
#include <avahi-common/error.h>

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



CZeroconfAvahi::CZeroconfAvahi(){
	if(! (mp_poll = avahi_threaded_poll_new())){
		std::cerr << "Ouch. Could not create threaded poll object" << std::endl;
		return;
	}
	mp_client = avahi_client_new(avahi_threaded_poll_get(mp_poll), 
				AVAHI_CLIENT_NO_FAIL, &clientCallback,0,0);
	if(!mp_client)
	{
		std::cerr << "Ouch. Failed to create avahi client" << std::endl;
		return;
	}
	//start event loop thread
	if(avahi_threaded_poll_start(mp_poll) < 0){
		std::cerr << "Ouch. Failed to start avahi client thread" << std::endl;
	}
}

CZeroconfAvahi::~CZeroconfAvahi(){
		avahi_threaded_poll_stop(mp_poll);
		avahi_client_free(mp_client);
		avahi_threaded_poll_free(mp_poll);
}

void CZeroconfAvahi::doPublishWebserver(int f_port){
}

void  CZeroconfAvahi::doRemoveWebserver(){
}

void CZeroconfAvahi::doStop(){
}

void CZeroconfAvahi::clientCallback(AvahiClient* fp_client, AvahiClientState f_state, void*){
	
	switch(f_state){
		case AVAHI_CLIENT_S_RUNNING:
			std::cout << "Client's up and running!" << std::endl;
			break;
		case AVAHI_CLIENT_FAILURE:
			std::cerr << "Avahi client failure: " << avahi_strerror(avahi_client_errno(fp_client)) << std::endl;
			break;
		case AVAHI_CLIENT_S_COLLISION:
		case AVAHI_CLIENT_S_REGISTERING:
			break;
		case AVAHI_CLIENT_CONNECTING:
			break;
	}
}

#endif