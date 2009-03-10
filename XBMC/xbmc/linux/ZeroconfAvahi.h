#pragma once

#if (defined(_LINUX) || ! defined(__APPLE__))

#include <memory>
#include "Zeroconf.h"

#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-common/defs.h>
struct AvahiThreadedPoll;

class CZeroconfAvahi : public CZeroconf{
public:
    CZeroconfAvahi();
    ~CZeroconfAvahi();
    
protected:
    //implement base CZeroConf interface
	virtual void doPublishWebserver(int f_port);
	virtual void doRemoveWebserver();
	virtual void doStop();

private:
	//this is where the client calls us if state changes
	static void clientCallback(AvahiClient* fp_client, AvahiClientState f_state, void*);
	//here we get notified of group changes
	static void groupCallback(AvahiEntryGroup *fp_group, AvahiEntryGroupState f_state, void *);
	//helper to assemble the announced name
	static std::string assembleWebserverServiceName();
	
	//don't access these without stopping the client thread
	//see http://avahi.org/wiki/RunningAvahiClientAsThread
	//and use struct ScopedEventLoopBlock
	AvahiClient* mp_client;
	AvahiThreadedPoll* mp_poll;
	AvahiEntryGroup* mp_webserver_group;
};

#endif