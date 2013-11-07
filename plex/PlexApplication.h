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
#include "threads/Timer.h"
#include "network/UdpClient.h"
#include "FileItem.h"

#ifdef TARGET_DARWIN_OSX
#include "Helper/PlexHTHelper.h"
#endif

class CMyPlexManager;

class CPlexServerManager;
typedef boost::shared_ptr<CPlexServerManager> CPlexServerManagerPtr;

class CPlexRemoteSubscriberManager;

class CPlexMediaServerClient;
typedef boost::shared_ptr<CPlexMediaServerClient> CPlexMediaServerClientPtr;

class CPlexServerDataLoader;
typedef boost::shared_ptr<CPlexServerDataLoader> CPlexServerDataLoaderPtr;

class CPlexAutoUpdate;
class BackgroundMusicPlayer;
class CPlexAnalytics;

class CPlexServiceListener;
typedef boost::shared_ptr<CPlexServiceListener> CPlexServiceListenerPtr;

class CPlexTimelineManager;
typedef boost::shared_ptr<CPlexTimelineManager> CPlexTimelineManagerPtr;

class CPlexThemeMusicPlayer;
typedef boost::shared_ptr<CPlexThemeMusicPlayer> CPlexThemeMusicPlayerPtr;

class CPlexThumbCacher;

class CPlexFilterManager;
typedef boost::shared_ptr<CPlexFilterManager> CPlexFilterManagerPtr;

///
/// The hub of all Plex goodness.
///
class PlexApplication : public IMsgTargetCallback, public ANNOUNCEMENT::IAnnouncer, public ITimerCallback, public CUdpClient
{
public:
  PlexApplication() : myPlexManager(NULL), remoteSubscriberManager(NULL), m_networkLoggingTimer(this) {};
  void Start();

  /// Destructor
  virtual ~PlexApplication() {}
  
  /// Handle internal messages.
  virtual bool OnMessage(CGUIMessage& message);

  void OnWakeUp();

  void ForceVersionCheck();
  CPlexServiceListenerPtr GetServiceListener() const { return m_serviceListener; }
  
  CFileItemPtr m_preplayItem;
  
  CMyPlexManager *myPlexManager;
  CPlexServerManagerPtr serverManager;
  CPlexRemoteSubscriberManager *remoteSubscriberManager;
  CPlexMediaServerClientPtr mediaServerClient;
  CPlexServerDataLoaderPtr dataLoader;
  CPlexThemeMusicPlayerPtr themeMusicPlayer;
  CPlexAnalytics *analytics;
  CPlexAutoUpdate* autoUpdater;
  CPlexTimelineManagerPtr timelineManager;  
  CPlexThumbCacher *thumbCacher;
  CPlexFilterManagerPtr filterManager;

  void setNetworkLogging(bool);
  void OnTimeout();
  void sendNetworkLog(int level, const std::string& logline);

private:
  /// Members
  CPlexServiceListenerPtr m_serviceListener;
  CTimer m_networkLoggingTimer;
  CStdString m_ipAddress;
  
  virtual void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);
};

XBMC_GLOBAL_REF(PlexApplication, g_plexApplication);
#define g_plexApplication XBMC_GLOBAL_USE(PlexApplication)
