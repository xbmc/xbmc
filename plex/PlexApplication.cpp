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
#include "PlexApplicationWin.h"
#include "BackgroundMusicPlayer.h"
#include "GUIUserMessages.h"
#include "ManualServerScanner.h"
#include "MediaSource.h"

BackgroundMusicPlayerPtr bgMusicPlayer;

////////////////////////////////////////////////////////////////////////////////
PlexApplicationPtr PlexApplication::Create()
{
#ifdef _WIN32
  return PlexApplicationPtr(new PlexApplicationWin());
#else
  return PlexApplicationPtr(new PlexApplication());
#endif
}

////////////////////////////////////////////////////////////////////////////////
PlexApplication::PlexApplication()
{
  // We don't want the background music player whacked on exit (destructor issues), so we'll keep a reference.
  m_serviceListener = PlexServiceListener::Create();
  bgMusicPlayer = m_bgMusicPlayer = BackgroundMusicPlayer::Create();
  
  // Make sure we always scan for localhost.
  ManualServerScanner::Get().addServer("127.0.0.1");
  
  // Add the manual server if it exists and is enabled.
  if (g_guiSettings.GetBool("plexmediaserver.manualaddress"))
  {
    string address = g_guiSettings.GetString("plexmediaserver.address");
    if (PlexUtils::IsValidIP(address))
      ManualServerScanner::Get().addServer(address);
  }
}

////////////////////////////////////////////////////////////////////////////////
PlexApplication::~PlexApplication()
{
  bgMusicPlayer.reset();
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
      m_bgMusicPlayer->SetTheme(message.GetStringParam());
      return true;
    }
  }
  
  return false;
}

////////////////////////////////////////////////////////////////////////////////
void PlexApplication::SetGlobalVolume(int volume)
{
	m_bgMusicPlayer->SetGlobalVolumeAsPercent(volume);
}
