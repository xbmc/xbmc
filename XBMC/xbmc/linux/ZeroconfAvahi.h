#pragma once

#if (defined(_LINUX) && ! defined(__APPLE__))

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
  virtual bool doPublishService(const std::string& fcr_identifier,
                                const std::string& fcr_type,
                                const std::string& fcr_name,
                                unsigned int f_port);
  
  virtual bool doRemoveService(const std::string& fcr_ident);
  
  //doHas is ugly ...
  virtual bool doHasService(const std::string& fcr_ident);
  
  virtual void doStop();
  
private:
	///this is where the client calls us if state changes
	static void clientCallback(AvahiClient* fp_client, AvahiClientState f_state, void*);
	///here we get notified of group changes
	static void groupCallback(AvahiEntryGroup *fp_group, AvahiEntryGroupState f_state, void *);
  //shutdown callback; works around a problem in avahi < 0.6.24 see destructor for details
	static void shutdownCallback(AvahiTimeout *fp_e, void *);

	///helper to assemble the announced name
  std::string assemblePublishedName(const std::string& fcr_prefix);
	
	///creates the avahi client;
	///@return true on success
	bool createClient();
	
	//don't access stuff below without stopping the client thread
	//see http://avahi.org/wiki/RunningAvahiClientAsThread
	//and use struct ScopedEventLoopBlock
  //goes through a list of todos and publishs them
  void updateServices();
  
	AvahiClient* mp_client;
	AvahiThreadedPoll* mp_poll;

	unsigned int m_port;
	
	// 3 variables below are needed for workaround of avahi bug (see destructor for details)
	bool m_shutdown;
	pthread_t m_thread_id;
};

#endif