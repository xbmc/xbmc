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
#include "BackgroundMusicPlayer.h"
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

#include "AutoUpdate/PlexAutoUpdate.h"

////////////////////////////////////////////////////////////////////////////////
void
PlexApplication::Start()
{
  dataLoader = new CPlexServerDataLoader;
  serverManager = new CPlexServerManager;
  myPlexManager = new CMyPlexManager;
  remoteSubscriberManager = new CPlexRemoteSubscriberManager;
  mediaServerClient = new CPlexMediaServerClient;
  backgroundMusicPlayer = new BackgroundMusicPlayer;
  analytics = new CPlexAnalytics;
  
  ANNOUNCEMENT::CAnnouncementManager::AddAnnouncer(this);

  m_autoUpdater = new CPlexAutoUpdate("http://plexapp.com/appcast/plexht/appcast.xml");

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
      g_plexApplication.backgroundMusicPlayer->SetTheme(message.GetStringParam());
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
  m_autoUpdater->ForceCheckInBackground();
}

////////////////////////////////////////////////////////////////////////////////////////
void PlexApplication::Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  if (flag == ANNOUNCEMENT::System && stricmp(sender, "xbmc") == 0 && stricmp(message, "onQuit") == 0)
  {
    CLog::Log(LOGINFO, "CPlexApplication shutting down!");
    
    m_serviceListener->Stop();
    m_serviceListener.reset();
    
    myPlexManager->Stop();
    delete myPlexManager;

    serverManager->Stop();
    delete serverManager;

    dataLoader->Stop();
    delete dataLoader;
    
    mediaServerClient->CancelJobs();
    delete mediaServerClient;
    mediaServerClient = NULL;
    
    delete remoteSubscriberManager;
    
    backgroundMusicPlayer->Die();
    delete backgroundMusicPlayer;
    
    delete m_autoUpdater;
  }
}
