/*
 *  Copyright (C) 2010 Plex, Inc.   
 *
 *  Created on: Dec 18, 2010
 *      Author: Elan Feingold
 */

#pragma once

#include <boost/enable_shared_from_this.hpp>

#include "BonjourRequestHandler.h"
#include "Common.h"
#include "Database.h"
#include "LibrarySection.h"
#include "LibraryUpdater.h"
#include "NetworkServiceAdvertiser.h"
#include "Preferences.h"
#include "Serializable.h"
#include "GUIInfoManager.h"

#ifdef _WIN32
extern int getUpdatedAt();
#endif

/////////////////////////////////////////////////////////////////////////////
class PlexMediaServer : public Serializable
{
  DEFINE_CLASS_NAME(PlexMediaServer);
  
  void serialize(ostream& out)
  {
    SERIALIZE_ELEMENT_START(out, className());
    {
      SERIALIZE_ELEMENT_START(out, "Library");
      
      // Library sections.
      Database db;
      vector<LibrarySectionPtr> sections = LibrarySection::FindAll(db);
      BOOST_FOREACH(LibrarySectionPtr section, sections)
      {
        section->setAttribute("refreshing", LibraryUpdater::GetUpdater().isUpdatingSection(section->id) ? "1" : "0");
        section->serialize(out, false);
      }
      
      SERIALIZE_ELEMENT_END(out, "Library");
    }

    SERIALIZE_FULL_CLASS_END(out);
  }
};

/////////////////////////////////////////////////////////////////////////////
class NetworkServiceAdvertiserPMS : public NetworkServiceAdvertiser,
                                    public PreferenceObserver,
                                    public boost::enable_shared_from_this<NetworkServiceAdvertiserPMS>
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
    Preferences::Get()->addPreferenceObserver(PREF_FRIENDLY_NAME, shared_from_this());    
  }
  
  virtual void doStop()
  {
    Preferences::Get()->removePreferenceObserver(PREF_FRIENDLY_NAME, shared_from_this());
  }
  
  /// For subclasses to fill in.
  virtual void createReply(map<string, string>& headers) 
  {
    headers["Name"] = GetMachineName();
    headers["Port"] = "32400";
    headers["Version"] = g_infoManager.GetVersion();
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
    return BonjourRequestHandler::Get()->GetMachineIdentifier();
  }
  
  /// For subclasses to fill in.
  virtual string getBody()
  {
    PlexMediaServer pms;
    return pms.toReply().content;
  }
  
  /// Overridden from PreferenceObserver.
  virtual void preferenceModified(const string& prefKey)
  {
    if (prefKey == PREF_FRIENDLY_NAME)
      update("serverMod=name");
  }
};
