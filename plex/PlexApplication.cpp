/*
 *  PlexApplication.cpp
 *  XBMC
 *
 *  Created by Jamie Kirkpatrick on 20/01/2011.
 *  Copyright 2011 Plex Inc. All rights reserved.
 *
 */

#include "PlexNetworkServices.h"
#include "PlexApplication.h"
#include "BackgroundMusicPlayer.h"
#include "GUIUserMessages.h"
#include "ManualServerScanner.h"
#include "MediaSource.h"
#include "plex/Helper/PlexHelper.h"
#include "MyPlexManager.h"
#include "AdvancedSettings.h"

////////////////////////////////////////////////////////////////////////////////
void
PlexApplication::Start()
{

  m_autoUpdater = new CPlexAutoUpdate("http://plexapp.com/appcast/plexht/appcast.xml");

  if (g_advancedSettings.m_bEnableGDM)
    m_serviceListener = PlexServiceListener::Create();

  // Make sure we always scan for localhost.
  ManualServerScanner::Get().addServer("127.0.0.1");
  
  // Add the manual server if it exists and is enabled.
  if (g_guiSettings.GetBool("plexmediaserver.manualaddress"))
  {
    string address = g_guiSettings.GetString("plexmediaserver.address");
    if (PlexUtils::IsValidIP(address))
      ManualServerScanner::Get().addServer(address);
  }

  MyPlexManager::Get().scanAsync();
}

////////////////////////////////////////////////////////////////////////////////
PlexApplication::~PlexApplication()
{
  delete m_autoUpdater;
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
      g_backgroundMusicPlayer.SetTheme(message.GetStringParam());
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
      PlexHelper::GetInstance().Restart();
    }
};
#endif

////////////////////////////////////////////////////////////////////////////////
void PlexApplication::OnWakeUp()
{
  /* Scan servers */
  m_serviceListener->scanNow();
  MyPlexManager::Get().scanAsync();

#ifdef TARGET_DARWIN_OSX
  CRemoteRestartThread* hack = new CRemoteRestartThread;
  hack->Create(true);
#endif

}
