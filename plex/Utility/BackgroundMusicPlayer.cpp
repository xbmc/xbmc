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
  m_player = NULL;
}

////////////////////////////////////////////////////////////////////////////////
BackgroundMusicPlayer::~BackgroundMusicPlayer()
{
  if (m_player)
    delete m_player;
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
void BackgroundMusicPlayer::FadeOutAndDie()
{
  // Fixme when they have nice fading in XBMC
  m_player->UnRegisterAudioCallback();
  delete m_player;
  m_player = NULL;
}

////////////////////////////////////////////////////////////////////////////////
void BackgroundMusicPlayer::PlayCurrentTheme()
{
  if (!m_player)
  {
    m_player = CPlayerCoreFactory::CreatePlayer(EPC_DVDPLAYER, *this);
    m_player->RegisterAudioCallback(this);
  }

  if (m_theme.size() && m_player)
    m_player->OpenFile(CFileItem(m_theme, false), CPlayerOptions());
  else
    FadeOutAndDie();
}

////////////////////////////////////////////////////////////////////////////////
void BackgroundMusicPlayer::OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample)
{
  int volPercent = g_guiSettings.GetInt("backgroundmusic.bgmusicvolume");
  m_player->SetVolume(volPercent / 100.0);
}
