/*
 *  PlexApplication.h
 *  XBMC
 *
 *  Created by Jamie Kirkpatrick on 20/01/2011.
 *  Copyright 2011 Plex Inc. All rights reserved.
 *
 */

#pragma once

#include <boost/shared_ptr.hpp>

#include "guilib/IMsgTargetCallback.h"
#include "threads/Thread.h"
#include "GlobalsHandling.h"
#include "interfaces/IAnnouncer.h"

#ifdef TARGET_DARWIN_OSX
#include "Helper/PlexHTHelper.h"
#endif

class CMyPlexManager;
class CPlexServerManager;
class CPlexRemoteSubscriberManager;
class CPlexMediaServerClient;
class CPlexServerDataLoader;
class CPlexAutoUpdate;
class BackgroundMusicPlayer;
class CPlexAnalytics;

class CPlexServiceListener;
typedef boost::shared_ptr<CPlexServiceListener> CPlexServiceListenerPtr;

///
/// The hub of all Plex goodness.
///
class PlexApplication : public IMsgTargetCallback, public ANNOUNCEMENT::IAnnouncer
{
public:
  PlexApplication() : myPlexManager(NULL), serverManager(NULL), remoteSubscriberManager(NULL), mediaServerClient(NULL), dataLoader(NULL), backgroundMusicPlayer(NULL) {};
  void Start();

  /// Destructor
  virtual ~PlexApplication() {}
  
  /// Handle internal messages.
  virtual bool OnMessage(CGUIMessage& message);

  void OnWakeUp();

  void ForceVersionCheck();
  CPlexServiceListenerPtr GetServiceListener() const { return m_serviceListener; }
  
  
  CMyPlexManager *myPlexManager;
  CPlexServerManager *serverManager;
  CPlexRemoteSubscriberManager *remoteSubscriberManager;
  CPlexMediaServerClient *mediaServerClient;
  CPlexServerDataLoader *dataLoader;
  BackgroundMusicPlayer *backgroundMusicPlayer;
  CPlexAnalytics *analytics;
      
private:
  /// Members
  CPlexServiceListenerPtr m_serviceListener;
  CPlexAutoUpdate* m_autoUpdater;
  
  virtual void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);
};

XBMC_GLOBAL_REF(PlexApplication, g_plexApplication);
#define g_plexApplication XBMC_GLOBAL_USE(PlexApplication)
