//
//  BackgroundMusicPlayer.cpp
//  Plex
//
//  Created by Jamie Kirkpatrick on 29/01/2011.
//  Copyright 2011 Kirk Consulting Limited. All rights reserved.
//

#include "Application.h"
#include "BackgroundMusicPlayer.h"
#include "FileItem.h"
#include "GUISettings.h"
#include "GUIUserMessages.h"
#include "GUIWindowManager.h"
#include "Settings.h"
#include "log.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"

////////////////////////////////////////////////////////////////////////////////
BackgroundMusicPlayerPtr BackgroundMusicPlayer::Create()
{
  return BackgroundMusicPlayerPtr(new BackgroundMusicPlayer());
}

////////////////////////////////////////////////////////////////////////////////
void BackgroundMusicPlayer::SendThemeChangeMessage(const CStdString& theme)
{
  CGUIMessage msg(GUI_MSG_BG_MUSIC_THEME_UPDATED, 0, 0);
  msg.SetStringParam(theme);
  g_windowManager.SendMessage(msg); 
}

////////////////////////////////////////////////////////////////////////////////
BackgroundMusicPlayer::BackgroundMusicPlayer()
{
  m_globalVolume = 100;
  m_player.reset(CPlayerCoreFactory::CreatePlayer(EPC_DVDPLAYER, *this));
  m_player->RegisterAudioCallback(this);
}

////////////////////////////////////////////////////////////////////////////////
BackgroundMusicPlayer::~BackgroundMusicPlayer()
{
  m_player->UnRegisterAudioCallback();
}

////////////////////////////////////////////////////////////////////////////////
void BackgroundMusicPlayer::SetGlobalVolumeAsPercent(int volume)
{
  return;
}

////////////////////////////////////////////////////////////////////////////////
void BackgroundMusicPlayer::SetTheme(const CStdString& theme)
{
  // Disabled?
  if (g_guiSettings.GetBool("backgroundmusic.thememusicenabled") == false)
    return;
  
  // Same theme?
  if (m_theme.Equals(theme))
    return;
  
  // Playing music/video already?
  if (g_application.IsPlayingAudio() == true || g_application.IsPlayingVideo() == true)
    return;

  // Play the new theme.
  CLog::Log(LOGDEBUG,"Background music theme changed to: %s", theme.c_str());
  m_theme = theme;
  PlayCurrentTheme();
}

////////////////////////////////////////////////////////////////////////////////
void BackgroundMusicPlayer::PlayCurrentTheme()
{
  if (m_theme.size())
    m_player->OpenFile(CFileItem(m_theme, false), CPlayerOptions());
  else if (m_player->IsPlaying())
  	m_player->CloseFile();
}

////////////////////////////////////////////////////////////////////////////////
void BackgroundMusicPlayer::OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample)
{
  int volPercent = g_guiSettings.GetInt("backgroundmusic.bgmusicvolume");
  int volDB = (int)(2000.0 * log10((float)volPercent/100.0));
  
  m_player->SetVolume(volDB);
}
