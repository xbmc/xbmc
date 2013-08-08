/*
 *  Copyright (C) 2010 Plex, Inc.   
 *
 *  Created on: Dec 18, 2010
 *      Author: Elan Feingold
 */

#pragma once

#include <boost/enable_shared_from_this.hpp>

#include "GdmRequestHandler.h"
#include "Common.h"
#include "Database.h"
#include "LibrarySection.h"
#include "LibraryUpdater.h"
#include "NetworkServiceAdvertiser.h"
#include "Preferences.h"
#include "Serializable.h"
#include "MachineId.h"

#ifdef _WIN32
extern int getUpdatedAt();
#endif

/////////////////////////////////////////////////////////////////////////////
class NetworkServiceAdvertiserPMS : public NetworkServiceAdvertiser,
                                    public IServerEventSink
{
 public:
  
  /// Constructor.
  NetworkServiceAdvertiserPMS(boost::asio::io_service& ioService, const boost::asio::ip::address& groupAddr, unsigned short port)
    : NetworkServiceAdvertiser(ioService, groupAddr, port)
  {
  }
  
  /// Destructor.
  virtual ~NetworkServiceAdvertiserPMS() 
  {
  }
  
 protected:
  
  /// From NetworkServiceAdvertiser.
  virtual void doStart()
  {
    ServerEventManager::Get().registerSink(EVENT_PREFERENCE_MODIFIED, this);
  }
  
  virtual void doStop()
  {
    ServerEventManager::Get().unregisterSink(EVENT_PREFERENCE_MODIFIED, this);
  }
  
  /// For subclasses to fill in.
  virtual void createReply(map<string, string>& headers) 
  {
    headers["Name"] = GetMachineName();
    headers["Port"] = "32400";
    headers["Version"] = Cocoa_GetAppVersion();
    headers["Updated-At"] = lexical_cast<string>(getUpdatedAt());
  }
  
  /// For subclasses to fill in.
  virtual string getType()
  {
    return "plex/media-server";
  }
  
  /// For subclasses to fill in.
  virtual string getResourceIdentifier()
  {
    return GetMachineIdentifier();
  }
  
  /// For subclasses to fill in.
  virtual string getBody()
  {
    return "";
  }
  
  /// Overridden from PreferenceObserver.
  virtual void preferenceModified(const string& prefKey)
  {
    if (prefKey == PREF_FRIENDLY_NAME)
      update("serverMod=name");
  }
};
