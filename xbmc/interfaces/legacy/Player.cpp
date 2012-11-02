/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Player.h"
#include "ListItem.h"
#include "PlayList.h"
#include "PlayListPlayer.h"
#include "settings/Settings.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "GUIInfoManager.h"
#include "AddonUtils.h"
#include "utils/LangCodeExpander.h"
#include "utils/log.h"

namespace XBMCAddon
{
  namespace xbmc
  {
    Player::Player(int _playerCore): AddonCallback("Player")
    {
      iPlayList = PLAYLIST_MUSIC;

      if (_playerCore == EPC_DVDPLAYER ||
          _playerCore == EPC_MPLAYER ||
          _playerCore == EPC_PAPLAYER)
        playerCore = (EPLAYERCORES)_playerCore;
      else
        playerCore = EPC_NONE;

      // now that we're done, register hook me into the system
      if (languageHook)
      {
        DelayedCallGuard dc(languageHook);
        languageHook->registerPlayerCallback(this);
      }
    }

    Player::~Player()
    {
      deallocating();

      // we're shutting down so unregister me.
      if (languageHook)
      {
        DelayedCallGuard dc(languageHook);
        languageHook->unregisterPlayerCallback(this);
      }
    }

    void Player::playStream(const String& item, const xbmcgui::ListItem* plistitem, bool windowed)
    {
      TRACE;
      DelayedCallGuard dc(languageHook);
      if (!item.empty())
      {
        // set fullscreen or windowed
        g_settings.m_bStartVideoWindowed = windowed;

        // force a playercore before playing
        g_application.m_eForcedNextPlayer = playerCore;

        const AddonClass::Ref<xbmcgui::ListItem> listitem(plistitem);

        if (listitem.isSet())
        {
          // set m_strPath to the passed url
          listitem->item->SetPath(item.c_str());
          CApplicationMessenger::Get().PlayFile((const CFileItem)(*(listitem->item)), false);
        }
        else
        {
          CFileItem nextitem(item, false);
          CApplicationMessenger::Get().MediaPlay(nextitem.GetPath());
        }
      }
      else
        playCurrent(windowed);
    }

    void Player::playCurrent(bool windowed)
    {
      TRACE;
      DelayedCallGuard dc(languageHook);
      // set fullscreen or windowed
      g_settings.m_bStartVideoWindowed = windowed;

      // force a playercore before playing
      g_application.m_eForcedNextPlayer = playerCore;

      // play current file in playlist
      if (g_playlistPlayer.GetCurrentPlaylist() != iPlayList)
        g_playlistPlayer.SetCurrentPlaylist(iPlayList);
      CApplicationMessenger::Get().PlayListPlayerPlay(g_playlistPlayer.GetCurrentSong());
    }

    void Player::playPlaylist(const PlayList* playlist, bool windowed)
    {
      TRACE;
      DelayedCallGuard dc(languageHook);
      if (playlist != NULL)
      {
        // set fullscreen or windowed
        g_settings.m_bStartVideoWindowed = windowed;

        // force a playercore before playing
        g_application.m_eForcedNextPlayer = playerCore;

        // play a python playlist (a playlist from playlistplayer.cpp)
        iPlayList = playlist->getPlayListId();
        g_playlistPlayer.SetCurrentPlaylist(iPlayList);
        CApplicationMessenger::Get().PlayListPlayerPlay();
      }
      else
        playCurrent(windowed);
    }

    void Player::stop()
    {
      TRACE;
      CApplicationMessenger::Get().MediaStop();
    }

    void Player::pause()
    {
      TRACE;
      CApplicationMessenger::Get().MediaPause();
    }

    void Player::playnext()
    {
      TRACE;
      DelayedCallGuard dc(languageHook);
      // force a playercore before playing
      g_application.m_eForcedNextPlayer = playerCore;

      CApplicationMessenger::Get().PlayListPlayerNext();
    }

    void Player::playprevious()
    {
      TRACE;
      DelayedCallGuard dc(languageHook);
      // force a playercore before playing
      g_application.m_eForcedNextPlayer = playerCore;

      CApplicationMessenger::Get().PlayListPlayerPrevious();
    }

    void Player::playselected(int selected)
    {
      TRACE;
      DelayedCallGuard dc(languageHook);
      // force a playercore before playing
      g_application.m_eForcedNextPlayer = playerCore;

      if (g_playlistPlayer.GetCurrentPlaylist() != iPlayList)
      {
        g_playlistPlayer.SetCurrentPlaylist(iPlayList);
      }
      g_playlistPlayer.SetCurrentSong(selected);

      CApplicationMessenger::Get().PlayListPlayerPlay(selected);
      //g_playlistPlayer.Play(selected);
      //CLog::Log(LOGNOTICE, "Current Song After Play: %i", g_playlistPlayer.GetCurrentSong());
    }

    void Player::OnPlayBackStarted()
    { 
      TRACE;
      invokeCallback(new CallbackFunction<Player>(this,&Player::onPlayBackStarted));
    }

    void Player::OnPlayBackEnded()
    { 
      TRACE;
      invokeCallback(new CallbackFunction<Player>(this,&Player::onPlayBackEnded));
    }

    void Player::OnPlayBackStopped()
    { 
      TRACE;
      invokeCallback(new CallbackFunction<Player>(this,&Player::onPlayBackStopped));
    }

    void Player::OnPlayBackPaused()
    { 
      TRACE;
      invokeCallback(new CallbackFunction<Player>(this,&Player::onPlayBackPaused));
    }

    void Player::OnPlayBackResumed()
    { 
      TRACE;
      invokeCallback(new CallbackFunction<Player>(this,&Player::onPlayBackResumed));
    }

    void Player::OnQueueNextItem()
    { 
      TRACE;
      invokeCallback(new CallbackFunction<Player>(this,&Player::onQueueNextItem));
    }

    void Player::OnPlayBackSpeedChanged(int speed)
    { 
      TRACE;
      invokeCallback(new CallbackFunction<Player,int>(this,&Player::onPlayBackSpeedChanged,speed));
    }

    void Player::OnPlayBackSeek(int time, int seekOffset)
    { 
      TRACE;
      invokeCallback(new CallbackFunction<Player,int,int>(this,&Player::onPlayBackSeek,time,seekOffset));
    }

    void Player::OnPlayBackSeekChapter(int chapter)
    { 
      TRACE;
      invokeCallback(new CallbackFunction<Player,int>(this,&Player::onPlayBackSeekChapter,chapter));
    }

    void Player::onPlayBackStarted()
    {
      TRACE;
    }

    void Player::onPlayBackEnded()
    {
      TRACE;
    }

    void Player::onPlayBackStopped()
    {
      TRACE;
    }

    void Player::onPlayBackPaused()
    {
      TRACE;
    }

    void Player::onPlayBackResumed()
    {
      TRACE;
    }

    void Player::onQueueNextItem()
    {
      TRACE;
    }

    void Player::onPlayBackSpeedChanged(int speed)
    { 
      TRACE;
    }

    void Player::onPlayBackSeek(int time, int seekOffset)
    { 
      TRACE;
    }

    void Player::onPlayBackSeekChapter(int chapter)
    { 
      TRACE;
    }

    bool Player::isPlaying()
    {
      TRACE;
      return g_application.IsPlaying();
    }

    bool Player::isPlayingAudio()
    {
      TRACE;
      return g_application.IsPlayingAudio();
    }

    bool Player::isPlayingVideo()
    {
      TRACE;
      return g_application.IsPlayingVideo();
    }

    String Player::getPlayingFile() throw (PlayerException)
    {
      TRACE;
      if (!g_application.IsPlaying())
        throw PlayerException("XBMC is not playing any file");

      return g_application.CurrentFile();
    }

    InfoTagVideo* Player::getVideoInfoTag() throw (PlayerException)
    {
      TRACE;
      if (!g_application.IsPlayingVideo())
        throw PlayerException("XBMC is not playing any videofile");

      const CVideoInfoTag* movie = g_infoManager.GetCurrentMovieTag();
      if (movie)
        return new InfoTagVideo(*movie);

      return new InfoTagVideo();
    }

    InfoTagMusic* Player::getMusicInfoTag() throw (PlayerException)
    {
      TRACE;
      if (g_application.IsPlayingVideo() || !g_application.IsPlayingAudio())
        throw PlayerException("XBMC is not playing any music file");

      const MUSIC_INFO::CMusicInfoTag* tag = g_infoManager.GetCurrentSongTag();
      if (tag)
        return new InfoTagMusic(*tag);

      return new InfoTagMusic();
    }

    double Player::getTotalTime() throw (PlayerException)
    {
      TRACE;
      if (!g_application.IsPlaying())
        throw PlayerException("XBMC is not playing any media file");

      return g_application.GetTotalTime();
    }

    double Player::getTime() throw (PlayerException)
    {
      TRACE;
      if (!g_application.IsPlaying())
        throw PlayerException("XBMC is not playing any media file");

      return g_application.GetTime();
    }

    void Player::seekTime(double pTime) throw (PlayerException)
    {
      TRACE;
      if (!g_application.IsPlaying())
        throw PlayerException("XBMC is not playing any media file");

      g_application.SeekTime( pTime );
    }

    void Player::setSubtitles(const char* cLine)
    {
      TRACE;
      if (g_application.m_pPlayer)
      {
        int nStream = g_application.m_pPlayer->AddSubtitle(cLine);
        if(nStream >= 0)
        {
          g_application.m_pPlayer->SetSubtitle(nStream);
          g_application.m_pPlayer->SetSubtitleVisible(true);
        }
      }
    }

    void Player::showSubtitles(bool bVisible)
    {
      TRACE;
      if (g_application.m_pPlayer)
      {
        g_settings.m_currentVideoSettings.m_SubtitleOn = bVisible != 0;
        g_application.m_pPlayer->SetSubtitleVisible(bVisible != 0);
      }
    }

    String Player::getSubtitles()
    {
      TRACE;
      if (g_application.m_pPlayer)
      {
        int i = g_application.m_pPlayer->GetSubtitle();
        CStdString strName;
        g_application.m_pPlayer->GetSubtitleName(i, strName);

        if (strName == "Unknown(Invalid)")
          strName = "";
        return strName;
      }

      return NULL;
    }

    void Player::disableSubtitles()
    {
      TRACE;
      CLog::Log(LOGWARNING,"'xbmc.Player().disableSubtitles()' is deprecated and will be removed in future releases, please use 'xbmc.Player().showSubtitles(false)' instead");
      if (g_application.m_pPlayer)
      {
        g_settings.m_currentVideoSettings.m_SubtitleOn = false;
        g_application.m_pPlayer->SetSubtitleVisible(false);
      }
    }

    std::vector<String>* Player::getAvailableSubtitleStreams()
    {
      if (g_application.m_pPlayer)
      {
        int subtitleCount = g_application.m_pPlayer->GetSubtitleCount();
        std::vector<String>* ret = new std::vector<String>(subtitleCount);
        for (int iStream=0; iStream < subtitleCount; iStream++)
        {
          CStdString strName;
          CStdString FullLang;
          g_application.m_pPlayer->GetSubtitleName(iStream, strName);
          if (!g_LangCodeExpander.Lookup(FullLang, strName))
            FullLang = strName;
          (*ret)[iStream] = FullLang;
        }
        return ret;
      }

      return NULL;
    }

    void Player::setSubtitleStream(int iStream)
    {
      if (g_application.m_pPlayer)
      {
        int streamCount = g_application.m_pPlayer->GetSubtitleCount();
        if(iStream < streamCount)
        {
          g_application.m_pPlayer->SetSubtitle(iStream);
          g_application.m_pPlayer->SetSubtitleVisible(true);
        }
      }
    }

    std::vector<String>* Player::getAvailableAudioStreams()
    {
      if (g_application.m_pPlayer)
      {
        int streamCount = g_application.m_pPlayer->GetAudioStreamCount();
        std::vector<String>* ret = new std::vector<String>(streamCount);
        for (int iStream=0; iStream < streamCount; iStream++)
        {  
          CStdString strName;
          CStdString FullLang;
          g_application.m_pPlayer->GetAudioStreamLanguage(iStream, strName);
          g_LangCodeExpander.Lookup(FullLang, strName);
          if (FullLang.IsEmpty())
            g_application.m_pPlayer->GetAudioStreamName(iStream, FullLang);
          (*ret)[iStream] = FullLang;
        }
        return ret;
      }
    
      return NULL;
    } 

    void Player::setAudioStream(int iStream)
    {
      if (g_application.m_pPlayer)
      {
        int streamCount = g_application.m_pPlayer->GetAudioStreamCount();
        if(iStream < streamCount)
          g_application.m_pPlayer->SetAudioStream(iStream);
      }
    }  
  }
}

