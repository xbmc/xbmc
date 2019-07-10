/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Player.h"

#include "AddonUtils.h"
#include "Application.h"
#include "GUIInfoManager.h"
#include "GUIUserMessages.h"
#include "ListItem.h"
#include "PlayList.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "cores/IPlayer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/MediaSettings.h"
#include "utils/log.h"

using namespace KODI::MESSAGING;

namespace XBMCAddon
{
  namespace xbmc
  {
	PlayParameter Player::defaultPlayParameter;

    Player::Player(int _playerCore)
    {
      iPlayList = PLAYLIST_MUSIC;

      if (_playerCore != 0)
        CLog::Log(LOGERROR, "xbmc.Player: Requested non-default player. This behavior is deprecated, plugins may no longer specify a player");


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
        CMediaSettings::GetInstance().SetVideoStartWindowed(windowed);

        const AddonClass::Ref<xbmcgui::ListItem> listitem(plistitem);

        if (listitem.isSet())
        {
          // set m_strPath to the passed url
          listitem->item->SetPath(item.c_str());
          CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_PLAY, 0, 0, static_cast<void*>(new CFileItem(*listitem->item)));
        }
        else
        {
          CFileItemList *l = new CFileItemList; //don't delete,
          l->Add(std::make_shared<CFileItem>(item, false));
          CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_PLAY, -1, -1, static_cast<void*>(l));
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
      CMediaSettings::GetInstance().SetVideoStartWindowed(windowed);

      // play current file in playlist
      if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() != iPlayList)
        CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(iPlayList);
      CApplicationMessenger::GetInstance().SendMsg(TMSG_PLAYLISTPLAYER_PLAY, CServiceBroker::GetPlaylistPlayer().GetCurrentSong());
    }

    void Player::playPlaylist(const PlayList* playlist, bool windowed, int startpos)
    {
      XBMC_TRACE;
      DelayedCallGuard dc(languageHook);
      if (playlist != NULL)
      {
        // set fullscreen or windowed
        CMediaSettings::GetInstance().SetVideoStartWindowed(windowed);

        // play a python playlist (a playlist from playlistplayer.cpp)
        iPlayList = playlist->getPlayListId();
        CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(iPlayList);
        if (startpos > -1)
          CServiceBroker::GetPlaylistPlayer().SetCurrentSong(startpos);
        CApplicationMessenger::GetInstance().SendMsg(TMSG_PLAYLISTPLAYER_PLAY, startpos);
      }
      else
        playCurrent(windowed);
    }

    void Player::stop()
    {
      XBMC_TRACE;
      CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_STOP);
    }

    void Player::pause()
    {
      XBMC_TRACE;
      CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_PAUSE);
    }

    void Player::playnext()
    {
      XBMC_TRACE;
      DelayedCallGuard dc(languageHook);

      CApplicationMessenger::GetInstance().SendMsg(TMSG_PLAYLISTPLAYER_NEXT);
    }

    void Player::playprevious()
    {
      XBMC_TRACE;
      DelayedCallGuard dc(languageHook);

      CApplicationMessenger::GetInstance().SendMsg(TMSG_PLAYLISTPLAYER_PREV);
    }

    void Player::playselected(int selected)
    {
      XBMC_TRACE;
      DelayedCallGuard dc(languageHook);

      if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() != iPlayList)
      {
        CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(iPlayList);
      }
      CServiceBroker::GetPlaylistPlayer().SetCurrentSong(selected);

      CApplicationMessenger::GetInstance().SendMsg(TMSG_PLAYLISTPLAYER_PLAY, selected);
      //CServiceBroker::GetPlaylistPlayer().Play(selected);
      //CLog::Log(LOGNOTICE, "Current Song After Play: %i", CServiceBroker::GetPlaylistPlayer().GetCurrentSong());
    }

    void Player::OnPlayBackStarted(const CFileItem &file)
    {
      // We only have fileItem due to us having to
      // implement the interface, we can't send it to python
      // as we're not able to serialize it.
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player>(this, &Player::onPlayBackStarted));
    }

    void Player::OnAVStarted(const CFileItem &file)
    {
      // We only have fileItem due to us having to
      // implement the interface, we can't send it to python
      // as we're not able to serialize it.
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player>(this, &Player::onAVStarted));
    }

    void Player::OnAVChange()
    {
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player>(this, &Player::onAVChange));
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

    void Player::OnPlayBackError()
    {
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player>(this,&Player::onPlayBackError));
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

    void Player::OnPlayBackSeek(int64_t time, int64_t seekOffset)
    {
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player,int,int>(this,&Player::onPlayBackSeek,static_cast<int>(time),static_cast<int>(seekOffset)));
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

    void Player::onAVStarted()
    {
      XBMC_TRACE;
    }

    void Player::onAVChange()
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

    void Player::onPlayBackError()
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
      return g_application.GetAppPlayer().IsPlaying();
    }

    bool Player::isPlayingAudio()
    {
      XBMC_TRACE;
      return g_application.GetAppPlayer().IsPlayingAudio();
    }

    bool Player::isPlayingVideo()
    {
      XBMC_TRACE;
      return g_application.GetAppPlayer().IsPlayingVideo();
    }

    bool Player::isPlayingRDS()
    {
      XBMC_TRACE;
      return g_application.GetAppPlayer().IsPlayingRDS();
    }

    bool Player::isExternalPlayer()
    {
      XBMC_TRACE;
      return g_application.GetAppPlayer().IsExternalPlaying();
    }

    String Player::getPlayingFile()
    {
      XBMC_TRACE;
      if (!g_application.GetAppPlayer().IsPlaying())
        throw PlayerException("XBMC is not playing any file");

      return g_application.CurrentFileItem().GetDynPath();
    }

    InfoTagVideo* Player::getVideoInfoTag()
    {
      XBMC_TRACE;
      if (!g_application.GetAppPlayer().IsPlayingVideo())
        throw PlayerException("XBMC is not playing any videofile");

      const CVideoInfoTag* movie = CServiceBroker::GetGUI()->GetInfoManager().GetCurrentMovieTag();
      if (movie)
        return new InfoTagVideo(*movie);

      return new InfoTagVideo();
    }

    InfoTagMusic* Player::getMusicInfoTag()
    {
      XBMC_TRACE;
      if (g_application.GetAppPlayer().IsPlayingVideo() || !g_application.GetAppPlayer().IsPlayingAudio())
        throw PlayerException("XBMC is not playing any music file");

      const MUSIC_INFO::CMusicInfoTag* tag = CServiceBroker::GetGUI()->GetInfoManager().GetCurrentSongTag();
      if (tag)
        return new InfoTagMusic(*tag);

      return new InfoTagMusic();
    }

    void Player::updateInfoTag(const XBMCAddon::xbmcgui::ListItem* item)
    {
      XBMC_TRACE;
      if (!g_application.GetAppPlayer().IsPlaying())
        throw PlayerException("Kodi is not playing any file");

      CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, item->item);
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
    }

    InfoTagRadioRDS* Player::getRadioRDSInfoTag()
    {
      XBMC_TRACE;
      if (g_application.GetAppPlayer().IsPlayingVideo() || !g_application.GetAppPlayer().IsPlayingRDS())
        throw PlayerException("Kodi is not playing any music file with RDS");

      std::shared_ptr<CFileItem> item = g_application.CurrentFileItemPtr();
      if (item && item->HasPVRChannelInfoTag())
        return new InfoTagRadioRDS(item->GetPVRChannelInfoTag());

      return new InfoTagRadioRDS();
    }

    double Player::getTotalTime()
    {
      XBMC_TRACE;
      if (!g_application.GetAppPlayer().IsPlaying())
        throw PlayerException("XBMC is not playing any media file");

      return g_application.GetTotalTime();
    }

    double Player::getTime()
    {
      XBMC_TRACE;
      if (!g_application.GetAppPlayer().IsPlaying())
        throw PlayerException("XBMC is not playing any media file");

      return g_application.GetTime();
    }

    void Player::seekTime(double pTime)
    {
      XBMC_TRACE;
      if (!g_application.GetAppPlayer().IsPlaying())
        throw PlayerException("XBMC is not playing any media file");

      g_application.SeekTime( pTime );
    }

    void Player::setSubtitles(const char* cLine)
    {
      XBMC_TRACE;
      if (g_application.GetAppPlayer().HasPlayer())
      {
        g_application.GetAppPlayer().AddSubtitle(cLine);
      }
    }

    void Player::showSubtitles(bool bVisible)
    {
      XBMC_TRACE;
      if (g_application.GetAppPlayer().HasPlayer())
      {
        g_application.GetAppPlayer().SetSubtitleVisible(bVisible != 0);
      }
    }

    String Player::getSubtitles()
    {
      XBMC_TRACE;
      if (g_application.GetAppPlayer().HasPlayer())
      {
        SubtitleStreamInfo info;
        g_application.GetAppPlayer().GetSubtitleStreamInfo(CURRENT_STREAM, info);

        if (info.language.length() > 0)
          return info.language;
        else
          return info.name;
      }

      return NULL;
    }

    std::vector<String> Player::getAvailableSubtitleStreams()
    {
      if (g_application.GetAppPlayer().HasPlayer())
      {
        int subtitleCount = g_application.GetAppPlayer().GetSubtitleCount();
        std::vector<String> ret(subtitleCount);
        for (int iStream=0; iStream < subtitleCount; iStream++)
        {
          SubtitleStreamInfo info;
          g_application.GetAppPlayer().GetSubtitleStreamInfo(iStream, info);

          if (info.language.length() > 0)
            ret[iStream] = info.language;
          else
            ret[iStream] = info.name;
        }
        return ret;
      }

      return std::vector<String>();
    }

    void Player::setSubtitleStream(int iStream)
    {
      if (g_application.GetAppPlayer().HasPlayer())
      {
        int streamCount = g_application.GetAppPlayer().GetSubtitleCount();
        if(iStream < streamCount)
        {
          g_application.GetAppPlayer().SetSubtitle(iStream);
          g_application.GetAppPlayer().SetSubtitleVisible(true);
        }
      }
    }

    std::vector<String> Player::getAvailableAudioStreams()
    {
      if (g_application.GetAppPlayer().HasPlayer())
      {
        int streamCount = g_application.GetAppPlayer().GetAudioStreamCount();
        std::vector<String> ret(streamCount);
        for (int iStream=0; iStream < streamCount; iStream++)
        {
          AudioStreamInfo info;
          g_application.GetAppPlayer().GetAudioStreamInfo(iStream, info);

          if (info.language.length() > 0)
            ret[iStream] = info.language;
          else
            ret[iStream] = info.name;
        }
        return ret;
      }

      return std::vector<String>();
    }

    void Player::setAudioStream(int iStream)
    {
      if (g_application.GetAppPlayer().HasPlayer())
      {
        int streamCount = g_application.GetAppPlayer().GetAudioStreamCount();
        if(iStream < streamCount)
          g_application.GetAppPlayer().SetAudioStream(iStream);
      }
    }

    std::vector<String> Player::getAvailableVideoStreams()
    {
      int streamCount = g_application.GetAppPlayer().GetVideoStreamCount();
      std::vector<String> ret(streamCount);
      for (int iStream = 0; iStream < streamCount; ++iStream)
      {
        VideoStreamInfo info;
        g_application.GetAppPlayer().GetVideoStreamInfo(iStream, info);

        if (info.language.length() > 0)
          ret[iStream] = info.language;
        else
          ret[iStream] = info.name;
      }
      return ret;
    }

    void Player::setVideoStream(int iStream)
    {
      int streamCount = g_application.GetAppPlayer().GetVideoStreamCount();
      if (iStream < streamCount)
        g_application.GetAppPlayer().SetVideoStream(iStream);
    }
  }
}

