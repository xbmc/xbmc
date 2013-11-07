/*
 *  PlexApplication.cpp
 *  XBMC
 *
 *  Created by Jamie Kirkpatrick on 20/01/2011.
 *  Copyright 2011 Plex Inc. All rights reserved.
 *
 */

#include "Client/PlexNetworkServiceBrowser.h"
#include "PlexApplication.h"
#include "GUIUserMessages.h"
#include "MediaSource.h"
#include "plex/Helper/PlexHTHelper.h"
#include "Client/MyPlex/MyPlexManager.h"
#include "AdvancedSettings.h"
#include "plex/CrashReporter/CrashSubmitter.h"

#include "Client/PlexServerManager.h"
#include "Client/PlexServerDataLoader.h"
#include "Remote/PlexRemoteSubscriberManager.h"
#include "Client/PlexMediaServerClient.h"
#include "PlexApplication.h"
#include "interfaces/AnnouncementManager.h"
#include "PlexAnalytics.h"
#include "Client/PlexTimelineManager.h"
#include "PlexThemeMusicPlayer.h"
#include "VideoThumbLoader.h"
#include "PlexFilterManager.h"

#include "AutoUpdate/PlexAutoUpdate.h"

////////////////////////////////////////////////////////////////////////////////
void
PlexApplication::Start()
{
  dataLoader = CPlexServerDataLoaderPtr(new CPlexServerDataLoader);
  serverManager = CPlexServerManagerPtr(new CPlexServerManager);
  myPlexManager = new CMyPlexManager;
  remoteSubscriberManager = new CPlexRemoteSubscriberManager;
  mediaServerClient = CPlexMediaServerClientPtr(new CPlexMediaServerClient);
  analytics = new CPlexAnalytics;
  timelineManager = CPlexTimelineManagerPtr(new CPlexTimelineManager);
  themeMusicPlayer = CPlexThemeMusicPlayerPtr(new CPlexThemeMusicPlayer);
  thumbCacher = new CPlexThumbCacher;
  filterManager = CPlexFilterManagerPtr(new CPlexFilterManager);
  
  ANNOUNCEMENT::CAnnouncementManager::AddAnnouncer(this);

  autoUpdater = new CPlexAutoUpdate;

  serverManager->load();

  new CrashSubmitter;

  if (g_advancedSettings.m_bEnableGDM)
    m_serviceListener = CPlexServiceListenerPtr(new CPlexServiceListener);
  
  // Add the manual server if it exists and is enabled.
  if (g_guiSettings.GetBool("plexmediaserver.manualaddress"))
  {
    string address = g_guiSettings.GetString("plexmediaserver.address");
    if (PlexUtils::IsValidIP(address))
    {
      PlexServerList list;
      CPlexServerPtr server = CPlexServerPtr(new CPlexServer("", address, 32400));
      list.push_back(server);
      g_plexApplication.serverManager->UpdateFromConnectionType(list, CPlexConnection::CONNECTION_MANUAL);
    }
  }

  myPlexManager->Create();
}

////////////////////////////////////////////////////////////////////////////////
bool PlexApplication::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_APP_ACTIVATED:
    case GUI_MSG_APP_DEACTIVATED:
    {
      CLog::Log(LOGDEBUG,"Plex Application: Handling message %d", message.GetMessage());
      return true;
    }
    case GUI_MSG_BG_MUSIC_SETTINGS_UPDATED:
    {
      return true;
    }
    case GUI_MSG_BG_MUSIC_THEME_UPDATED:
    {
//      g_plexApplication.backgroundMusicPlayer->SetTheme(message.GetStringParam());
      return true;
    }
  }
  
  return false;
}

#ifdef TARGET_DARWIN_OSX
// Hack
class CRemoteRestartThread : public CThread
{
  public:
    CRemoteRestartThread() : CThread("RemoteRestart") {}
    void Process()
    {
      // This blocks until the helper is restarted
      PlexHTHelper::GetInstance().Restart();
    }
};
#endif

////////////////////////////////////////////////////////////////////////////////
void PlexApplication::OnWakeUp()
{
  /* Scan servers */
  m_serviceListener->ScanNow();
  myPlexManager->Poke();
  
#ifdef TARGET_DARWIN_OSX
  CRemoteRestartThread* hack = new CRemoteRestartThread;
  hack->Create(true);
#endif

}

////////////////////////////////////////////////////////////////////////////////////////
void PlexApplication::ForceVersionCheck()
{
  autoUpdater->ForceVersionCheckInBackground();
}

////////////////////////////////////////////////////////////////////////////////////////
void PlexApplication::Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  if (flag == ANNOUNCEMENT::System && stricmp(sender, "xbmc") == 0 && stricmp(message, "onQuit") == 0)
  {
    CLog::Log(LOGINFO, "CPlexApplication shutting down!");

    themeMusicPlayer->stop();
    
    m_serviceListener->Stop();
    m_serviceListener.reset();
    
    myPlexManager->Stop();
    delete myPlexManager;

    serverManager->Stop();
    serverManager.reset();

    dataLoader->Stop();
    dataLoader.reset();

    timelineManager->Stop();
    timelineManager.reset();
    
    mediaServerClient->CancelJobs();
    mediaServerClient.reset();

    filterManager->saveFiltersToDisk();
    filterManager.reset();
    
    delete remoteSubscriberManager;
    
//    backgroundMusicPlayer->Die();
//    delete backgroundMusicPlayer;
    
    delete autoUpdater;

    delete thumbCacher;
  }
}
