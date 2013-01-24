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
#include "filesystem/SpecialProtocol.h"
#include "filesystem/DirectoryFactory.h"
#include "PlayList.h"
#include "PlayListPlayer.h"

using namespace XFILE;

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
  /* TODO: We want to add fading out here in the future */
  Die();
}

////////////////////////////////////////////////////////////////////////////////
void BackgroundMusicPlayer::Die()
{
  if (m_player)
  {
    m_player->UnRegisterAudioCallback();
    delete m_player;
    m_player = NULL;
  }
}

////////////////////////////////////////////////////////////////////////////////
void BackgroundMusicPlayer::PlayElevatorMusic(bool force)
{
  if (g_application.IsPlaying())
    return;

  InitPlayer();

  if (m_bgPlaylist.Size() < 1)
    if (!InitElevatorMusic())
      return;

  if (m_player->IsPaused() && !force)
  {
    m_player->Pause();
    return;
  }

  if (m_player->IsPlaying() && !force)
    return;

  CFileItemPtr item = m_bgPlaylist.Get(m_position);
  CLog::Log(LOGDEBUG, "BMP: playing %s", item->GetPath().c_str());
  new BackgroundMusicOpener(m_player, item);
}

////////////////////////////////////////////////////////////////////////////////
void BackgroundMusicPlayer::PauseElevatorMusic()
{
  if (m_player && m_player->IsPlaying())
    m_player->Pause();
}

////////////////////////////////////////////////////////////////////////////////
bool BackgroundMusicPlayer::InitElevatorMusic()
{
  CStdString url = CSpecialProtocol::TranslatePath("special://userdata/BackgroundMusic");
  CLog::Log(LOGDEBUG, "BMP: Looking for bgmusic in %s", url.c_str());
  IDirectory* dir = CDirectoryFactory::Create(url);

  m_bgPlaylist.Clear();
  m_position = 0;

  if (dir->GetDirectory(url, m_bgPlaylist))
  {
    CLog::Log(LOGDEBUG, "BMP: we have %d files for background music", m_bgPlaylist.Size());
    if (m_bgPlaylist.Size() > 0)
    {
      m_bgPlaylist.Randomize();
      return true;
    }
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
void BackgroundMusicPlayer::InitPlayer()
{
  if (!m_player)
  {
    m_player = (PAPlayer*)CPlayerCoreFactory::CreatePlayer(EPC_PAPLAYER, *this);
    m_player->RegisterAudioCallback(this);

    int volPercent = g_guiSettings.GetInt("backgroundmusic.bgmusicvolume");
    m_player->SetVolume(volPercent / 100.0);
  }
}


////////////////////////////////////////////////////////////////////////////////
void BackgroundMusicPlayer::PlayCurrentTheme()
{
  InitPlayer();

  if (m_theme.size() && m_player)
  {
    if (m_player->IsPlaying())
      m_player->Pause();

    CFileItemPtr theme(new CFileItem(m_theme, false));
    theme->SetMimeType("audio/mp3");

    new BackgroundMusicOpener(m_player, theme);
  }
  else if (m_player)
  {
    m_player->Pause();
    m_player->CloseFile();

    if (g_guiSettings.GetBool("backgroundmusic.bgmusicenabled"))
      PlayElevatorMusic(true);
  }
}

////////////////////////////////////////////////////////////////////////////////
void BackgroundMusicPlayer::OnQueueNextItem()
{
  if (m_position + 1 < m_bgPlaylist.Size())
    m_position ++;
  else
    InitElevatorMusic();
  m_player->QueueNextFile(*m_bgPlaylist.Get(m_position).get());
}

#if 0
void BackgroundMusicPlayer::OnPlayBackEnded()
{
  if (m_position + 1 < m_bgPlaylist.Size())
    m_position ++;
  else
    m_bgPlaylist.Clear();

  PlayElevatorMusic();
}
#endif
