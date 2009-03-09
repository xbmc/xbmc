#pragma once

#if (defined(_LINUX) || ! defined(__APPLE__))

#include <memory>
#include "Zeroconf.h"

#include <avahi-client/client.h>

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
		//this is where the client calls us if something in the state happens
		static void clientCallback(AvahiClient* fp_client, AvahiClientState f_state, void*);

		//don't access these without stopping the client thread
		//see http://avahi.org/wiki/RunningAvahiClientAsThread
		//and check struct ScopedEventLoopBlock for details
		AvahiClient* mp_client;
		AvahiThreadedPoll* mp_poll;
};

#endif