/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Player.h"
#include "ListItem.h"
#include "PlayList.h"
#include "PlayListPlayer.h"
#include "settings/MediaSettings.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "GUIInfoManager.h"
#include "AddonUtils.h"
#include "utils/log.h"
#include "cores/IPlayer.h"

namespace XBMCAddon
{
  namespace xbmc
  {
	PlayParameter Player::defaultPlayParameter;

    Player::Player(int _playerCore)
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
        languageHook->RegisterPlayerCallback(this);
      }
    }

    Player::~Player()
    {
      deallocating();

      // we're shutting down so unregister me.
      if (languageHook)
      {
        DelayedCallGuard dc(languageHook);
        languageHook->UnregisterPlayerCallback(this);
      }
    }

    void Player::play(const Alternative<String, const PlayList* > & item,
                      const XBMCAddon::xbmcgui::ListItem* listitem, bool windowed, int startpos)
    {
      XBMC_TRACE;

      if (&item == &defaultPlayParameter)
        playCurrent(windowed);
      else if (item.which() == XBMCAddon::first)
        playStream(item.former(), listitem, windowed);
      else // item is a PlayListItem
        playPlaylist(item.later(),windowed,startpos);
    }

    void Player::playStream(const String& item, const xbmcgui::ListItem* plistitem, bool windowed)
    {
      XBMC_TRACE;
      DelayedCallGuard dc(languageHook);
      if (!item.empty())
      {
        // set fullscreen or windowed
        CMediaSettings::Get().SetVideoStartWindowed(windowed);

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
      XBMC_TRACE;
      DelayedCallGuard dc(languageHook);
      // set fullscreen or windowed
      CMediaSettings::Get().SetVideoStartWindowed(windowed);

      // force a playercore before playing
      g_application.m_eForcedNextPlayer = playerCore;

      // play current file in playlist
      if (g_playlistPlayer.GetCurrentPlaylist() != iPlayList)
        g_playlistPlayer.SetCurrentPlaylist(iPlayList);
      CApplicationMessenger::Get().PlayListPlayerPlay(g_playlistPlayer.GetCurrentSong());
    }

    void Player::playPlaylist(const PlayList* playlist, bool windowed, int startpos)
    {
      XBMC_TRACE;
      DelayedCallGuard dc(languageHook);
      if (playlist != NULL)
      {
        // set fullscreen or windowed
        CMediaSettings::Get().SetVideoStartWindowed(windowed);

        // force a playercore before playing
        g_application.m_eForcedNextPlayer = playerCore;

        // play a python playlist (a playlist from playlistplayer.cpp)
        iPlayList = playlist->getPlayListId();
        g_playlistPlayer.SetCurrentPlaylist(iPlayList);
        if (startpos > -1)
          g_playlistPlayer.SetCurrentSong(startpos);
        CApplicationMessenger::Get().PlayListPlayerPlay(startpos);
      }
      else
        playCurrent(windowed);
    }

    void Player::stop()
    {
      XBMC_TRACE;
      CApplicationMessenger::Get().MediaStop();
    }

    void Player::pause()
    {
      XBMC_TRACE;
      CApplicationMessenger::Get().MediaPause();
    }

    void Player::playnext()
    {
      XBMC_TRACE;
      DelayedCallGuard dc(languageHook);
      // force a playercore before playing
      g_application.m_eForcedNextPlayer = playerCore;

      CApplicationMessenger::Get().PlayListPlayerNext();
    }

    void Player::playprevious()
    {
      XBMC_TRACE;
      DelayedCallGuard dc(languageHook);
      // force a playercore before playing
      g_application.m_eForcedNextPlayer = playerCore;

      CApplicationMessenger::Get().PlayListPlayerPrevious();
    }

    void Player::playselected(int selected)
    {
      XBMC_TRACE;
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
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player>(this,&Player::onPlayBackStarted));
    }

    void Player::OnPlayBackEnded()
    { 
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player>(this,&Player::onPlayBackEnded));
    }

    void Player::OnPlayBackStopped()
    { 
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player>(this,&Player::onPlayBackStopped));
    }

    void Player::OnPlayBackPaused()
    { 
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player>(this,&Player::onPlayBackPaused));
    }

    void Player::OnPlayBackResumed()
    { 
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player>(this,&Player::onPlayBackResumed));
    }

    void Player::OnQueueNextItem()
    { 
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player>(this,&Player::onQueueNextItem));
    }

    void Player::OnPlayBackSpeedChanged(int speed)
    { 
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player,int>(this,&Player::onPlayBackSpeedChanged,speed));
    }

    void Player::OnPlayBackSeek(int time, int seekOffset)
    { 
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player,int,int>(this,&Player::onPlayBackSeek,time,seekOffset));
    }

    void Player::OnPlayBackSeekChapter(int chapter)
    { 
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player,int>(this,&Player::onPlayBackSeekChapter,chapter));
    }

    void Player::onPlayBackStarted()
    {
      XBMC_TRACE;
    }

    void Player::onPlayBackEnded()
    {
      XBMC_TRACE;
    }

    void Player::onPlayBackStopped()
    {
      XBMC_TRACE;
    }

    void Player::onPlayBackPaused()
    {
      XBMC_TRACE;
    }

    void Player::onPlayBackResumed()
    {
      XBMC_TRACE;
    }

    void Player::onQueueNextItem()
    {
      XBMC_TRACE;
    }

    void Player::onPlayBackSpeedChanged(int speed)
    { 
      XBMC_TRACE;
    }

    void Player::onPlayBackSeek(int time, int seekOffset)
    { 
      XBMC_TRACE;
    }

    void Player::onPlayBackSeekChapter(int chapter)
    { 
      XBMC_TRACE;
    }

    bool Player::isPlaying()
    {
      XBMC_TRACE;
      return g_application.m_pPlayer->IsPlaying();
    }

    bool Player::isPlayingAudio()
    {
      XBMC_TRACE;
      return g_application.m_pPlayer->IsPlayingAudio();
    }

    bool Player::isPlayingVideo()
    {
      XBMC_TRACE;
      return g_application.m_pPlayer->IsPlayingVideo();
    }

    String Player::getPlayingFile()
    {
      XBMC_TRACE;
      if (!g_application.m_pPlayer->IsPlaying())
        throw PlayerException("XBMC is not playing any file");

      return g_application.CurrentFile();
    }

    InfoTagVideo* Player::getVideoInfoTag()
    {
      XBMC_TRACE;
      if (!g_application.m_pPlayer->IsPlayingVideo())
        throw PlayerException("XBMC is not playing any videofile");

      const CVideoInfoTag* movie = g_infoManager.GetCurrentMovieTag();
      if (movie)
        return new InfoTagVideo(*movie);

      return new InfoTagVideo();
    }

    InfoTagMusic* Player::getMusicInfoTag()
    {
      XBMC_TRACE;
      if (g_application.m_pPlayer->IsPlayingVideo() || !g_application.m_pPlayer->IsPlayingAudio())
        throw PlayerException("XBMC is not playing any music file");

      const MUSIC_INFO::CMusicInfoTag* tag = g_infoManager.GetCurrentSongTag();
      if (tag)
        return new InfoTagMusic(*tag);

      return new InfoTagMusic();
    }

    double Player::getTotalTime()
    {
      XBMC_TRACE;
      if (!g_application.m_pPlayer->IsPlaying())
        throw PlayerException("XBMC is not playing any media file");

      return g_application.GetTotalTime();
    }

    double Player::getTime()
    {
      XBMC_TRACE;
      if (!g_application.m_pPlayer->IsPlaying())
        throw PlayerException("XBMC is not playing any media file");

      return g_application.GetTime();
    }

    void Player::seekTime(double pTime)
    {
      XBMC_TRACE;
      if (!g_application.m_pPlayer->IsPlaying())
        throw PlayerException("XBMC is not playing any media file");

      g_application.SeekTime( pTime );
    }

    void Player::setSubtitles(const char* cLine)
    {
      XBMC_TRACE;
      if (g_application.m_pPlayer->HasPlayer())
      {
        g_application.m_pPlayer->AddSubtitle(cLine);
      }
    }

    void Player::showSubtitles(bool bVisible)
    {
      XBMC_TRACE;
      if (g_application.m_pPlayer->HasPlayer())
      {
        g_application.m_pPlayer->SetSubtitleVisible(bVisible != 0);
      }
    }

    String Player::getSubtitles()
    {
      XBMC_TRACE;
      if (g_application.m_pPlayer->HasPlayer())
      {
        SPlayerSubtitleStreamInfo info;
        g_application.m_pPlayer->GetSubtitleStreamInfo(g_application.m_pPlayer->GetSubtitle(), info);

        if (info.language.length() > 0)
          return info.language;
        else
          return info.name;
      }

      return NULL;
    }

    void Player::disableSubtitles()
    {
      XBMC_TRACE;
      CLog::Log(LOGWARNING,"'xbmc.Player().disableSubtitles()' is deprecated and will be removed in future releases, please use 'xbmc.Player().showSubtitles(false)' instead");
      if (g_application.m_pPlayer->HasPlayer())
      {
        g_application.m_pPlayer->SetSubtitleVisible(false);
      }
    }

    std::vector<String>* Player::getAvailableSubtitleStreams()
    {
      if (g_application.m_pPlayer->HasPlayer())
      {
        int subtitleCount = g_application.m_pPlayer->GetSubtitleCount();
        std::vector<String>* ret = new std::vector<String>(subtitleCount);
        for (int iStream=0; iStream < subtitleCount; iStream++)
        {
          SPlayerSubtitleStreamInfo info;
          g_application.m_pPlayer->GetSubtitleStreamInfo(iStream, info);

          if (info.language.length() > 0)
            (*ret)[iStream] = info.language;
          else
            (*ret)[iStream] = info.name;
        }
        return ret;
      }

      return NULL;
    }

    void Player::setSubtitleStream(int iStream)
    {
      if (g_application.m_pPlayer->HasPlayer())
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
      if (g_application.m_pPlayer->HasPlayer())
      {
        int streamCount = g_application.m_pPlayer->GetAudioStreamCount();
        std::vector<String>* ret = new std::vector<String>(streamCount);
        for (int iStream=0; iStream < streamCount; iStream++)
        {
          SPlayerAudioStreamInfo info;
          g_application.m_pPlayer->GetAudioStreamInfo(iStream, info);

          if (info.language.length() > 0)
            (*ret)[iStream] = info.language;
          else
            (*ret)[iStream] = info.name;
        }
        return ret;
      }
    
      return NULL;
    } 

    void Player::setAudioStream(int iStream)
    {
      if (g_application.m_pPlayer->HasPlayer())
      {
        int streamCount = g_application.m_pPlayer->GetAudioStreamCount();
        if(iStream < streamCount)
          g_application.m_pPlayer->SetAudioStream(iStream);
      }
    }  
  }
}

