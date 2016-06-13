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

#pragma once

#include <vector>

#include "ListItem.h"
#include "PlayList.h"
#include "InfoTagVideo.h"
#include "Exception.h"
#include "AddonString.h"
#include "InfoTagMusic.h"
#include "InfoTagRadioRDS.h"
#include "AddonCallback.h"
#include "Alternative.h"

#include "swighelper.h"

#include "cores/IPlayerCallback.h"

namespace XBMCAddon
{
  namespace xbmc
  {
    XBMCCOMMONS_STANDARD_EXCEPTION(PlayerException);

    typedef Alternative<String, const PlayList* > PlayParameter;

    // This class is a merge of what was previously in xbmcmodule/player.h
    //  and xbmcmodule/PythonPlayer.h without the python references. The
    //  queuing and handling of asynchronous callbacks is done internal to
    //  this class.

    //
    /// \defgroup python_Player Player
    /// \ingroup python_xbmc
    /// @{
    /// @brief **Kodi's player.**
    ///
    /// \python_class{ xbmc.Player() }
    ///
    /// To become and create the class to play something.
    ///
    ///
    ///
    ///--------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// xbmc.Player().play(url, listitem, windowed)
    /// ...
    /// ~~~~~~~~~~~~~
    //
    class Player : public AddonCallback, public IPlayerCallback
    {
    private:
      int iPlayList;

      void playStream(const String& item = emptyString, const XBMCAddon::xbmcgui::ListItem* listitem = NULL, bool windowed = false);
      void playPlaylist(const PlayList* playlist = NULL,
                        bool windowed = false, int startpos=-1);
      void playCurrent(bool windowed = false);

    public:
#if !defined SWIG && !defined DOXYGEN_SHOULD_SKIP_THIS
      static PlayParameter defaultPlayParameter;
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS
      // Construct a Player proxying the given generated binding. The 
      //  construction of a Player needs to identify whether or not any 
      //  callbacks will be executed asynchronously or not.
      Player(int playerCore = 0);
      virtual ~Player(void);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ play([item, listitem, windowed, startpos]) }
      ///-----------------------------------------------------------------------
      /// @brief Play a item.
      ///
      /// @param item                [opt] string - filename, url or playlist
      /// @param listitem            [opt] listitem - used with setInfo() to set
      ///                            different infolabels.
      /// @param windowed            [opt] bool - true=play video windowed,
      ///                            false=play users preference.(default)
      /// @param startpos            [opt] int - starting position when playing
      ///                            a playlist. Default = -1
      ///
      /// @note If  item  is  not  given then  the Player  will try  to play the
      ///       current item in the current playlist.\n
      ///       \n
      ///       You can use the above as keywords for arguments and skip certain
      ///       optional arguments.\n
      ///       Once  you use  a keyword,  all following  arguments require  the
      ///       keyword.
      ///
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// listitem = xbmcgui.ListItem('Ironman')
      /// listitem.setInfo('video', {'Title': 'Ironman', 'Genre': 'Science Fiction'})
      /// xbmc.Player().play(url, listitem, windowed)
      /// xbmc.Player().play(playlist, listitem, windowed, startpos)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      play(...);
#else
      void play(const PlayParameter& item = Player::defaultPlayParameter,
                const XBMCAddon::xbmcgui::ListItem* listitem = NULL, bool windowed = false, int startpos = -1);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ stop() }
      ///-----------------------------------------------------------------------
      /// Stop playing.
      ///
      stop();
#else
      void stop();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ pause() }
      ///-----------------------------------------------------------------------
      /// Pause or resume playing if already paused.
      ///
      pause();
#else
      void pause();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ playnext() }
      ///-----------------------------------------------------------------------
      /// Play next item in playlist.
      ///
      playnext();
#else
      void playnext();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ playprevious() }
      ///-----------------------------------------------------------------------
      /// Play previous item in playlist.
      ///
      playprevious();
#else
      void playprevious();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ playselected(selected) }
      ///-----------------------------------------------------------------------
      /// Play a certain item from the current playlist.
      ///
      /// @param selected   Integer - Item to select
      ///
      playselected(...);
#else
      void playselected(int selected);
#endif

      //
      /// @defgroup python_PlayerCB Callback functions from Kodi to Add-On
      /// \ingroup python_Player
      /// @{
      /// @brief **Callback functions.**
      ///
      /// Functions to handle control callbacks from Kodi to Add-On.
      ///
      /// ----------------------------------------------------------------------
      ///
      /// @link python_Player Go back to normal functions from player@endlink
      //

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_PlayerCB
      /// @brief \python_func{ onPlayBackStarted() }
      ///-----------------------------------------------------------------------
      /// onPlayBackStarted method.
      ///
      /// Will be called when Kodi starts playing a file.
      ///
      onPlayBackStarted();
#else
      virtual void onPlayBackStarted();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_PlayerCB
      /// @brief \python_func{ onPlayBackEnded() }
      ///-----------------------------------------------------------------------
      /// onPlayBackEnded method.
      ///
      /// Will be called when Kodi stops playing a file.
      ///
      onPlayBackEnded();
#else
      virtual void onPlayBackEnded();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_PlayerCB
      /// @brief \python_func{ onPlayBackStopped() }
      ///-----------------------------------------------------------------------
      /// onPlayBackStopped method.
      ///
      /// Will be called when user stops Kodi playing a file.
      ///
      onPlayBackStopped();
#else
      virtual void onPlayBackStopped();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_PlayerCB
      /// @brief \python_func{ onPlayBackPaused() }
      ///-----------------------------------------------------------------------
      /// onPlayBackPaused method.
      ///
      /// Will be called when user pauses a playing file.
      ///
      onPlayBackPaused();
#else
      virtual void onPlayBackPaused();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_PlayerCB
      /// @brief \python_func{ onPlayBackResumed() }
      ///-----------------------------------------------------------------------
      /// onPlayBackResumed method.
      ///
      /// Will be called when user resumes a paused file.
      ///
      onPlayBackResumed();
#else
      virtual void onPlayBackResumed();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_PlayerCB
      /// @brief \python_func{ onQueueNextItem() }
      ///-----------------------------------------------------------------------
      /// onQueueNextItem method.
      ///
      /// Will be called when user queues the next item.
      ///
      onQueueNextItem();
#else
      virtual void onQueueNextItem();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_PlayerCB
      /// @brief \python_func{ onPlayBackSpeedChanged(speed) }
      ///-----------------------------------------------------------------------
      /// onPlayBackSpeedChanged method.
      ///
      /// Will be called when players speed changes (eg. user FF/RW).
      ///
      /// @param speed               [integer] Current speed of player
      ///
      /// @note Negative speed means player is rewinding, 1 is normal playback
      /// speed.
      ///
      onPlayBackSpeedChanged(int speed);
#else
      virtual void onPlayBackSpeedChanged(int speed);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_PlayerCB
      /// @brief \python_func{ onPlayBackSeek(time, seekOffset) }
      ///-----------------------------------------------------------------------
      /// onPlayBackSeek method.
      ///
      /// Will be called when user seeks to a time.
      ///
      /// @param time                [integer] Time to seek to
      /// @param seekOffset          [integer] ?
      ///
      onPlayBackSeek(...);
#else
      virtual void onPlayBackSeek(int time, int seekOffset);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_PlayerCB
      /// @brief \python_func{ onPlayBackSeekChapter(chapter) }
      ///-----------------------------------------------------------------------
      /// onPlayBackSeekChapter method.
      ///
      /// Will be called when user performs a chapter seek.
      ///
      /// @param chapter             [integer] Chapter to seek to
      ///
      onPlayBackSeekChapter(...);
#else
      virtual void onPlayBackSeekChapter(int chapter);
#endif
      /// @}

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ isPlaying() }
      ///-----------------------------------------------------------------------
      /// Check Kodi is playing something.
      ///
      /// @return                    True if Kodi is playing a file.
      ///
      isPlaying();
#else
      bool isPlaying();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ isPlayingAudio() }
      ///-----------------------------------------------------------------------
      /// Check for playing audio.
      ///
      /// @return                    True if Kodi is playing an audio file.
      ///
      isPlayingAudio();
#else
      bool isPlayingAudio();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ isPlayingVideo() }
      ///-----------------------------------------------------------------------
      /// Check for playing video.
      ///
      /// @return                    True if Kodi is playing a video.
      ///
      isPlayingVideo();
#else
      bool isPlayingVideo();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ isPlayingRDS() }
      ///-----------------------------------------------------------------------
      /// Check for playing radio data system (RDS).
      ///
      /// @return                    True if kodi is playing a radio data
      ///                            system (RDS).
      ///
      isPlayingRDS();
#else
      bool isPlayingRDS();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ getPlayingFile() }
      ///-----------------------------------------------------------------------
      /// Returns the current playing file as a string.
      ///
      /// @note For LiveTV, returns a __pvr://__ url which is not translatable
      /// to an OS specific file or external url.
      ///
      /// @return                    Playing filename
      /// @throws Exception          If player is not playing a file.
      ///
      getPlayingFile();
#else
      String getPlayingFile();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ getTime() }
      ///-----------------------------------------------------------------------
      /// Get playing time.
      ///
      /// Returns the current time of the current playing media as fractional
      /// seconds.
      ///
      /// @return                    Current time as fractional seconds
      /// @throws Exception          If player is not playing a file.
      ///
      getTime();
#else
      double getTime();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ seekTime(seekTime) }
      ///-----------------------------------------------------------------------
      /// Seek time.
      ///
      /// Seeks the specified amount of time as fractional seconds.
      /// The time specified is relative to the beginning of the currently.
      /// playing media file.
      ///
      /// @param seekTime            Time to seek as fractional seconds
      /// @throws Exception          If player is not playing a file.
      ///
      seekTime(...);
#else
      void seekTime(double seekTime);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ setSubtitles(subtitleFile) }
      ///-----------------------------------------------------------------------
      /// Set subtitle file and enable subtitles.
      ///
      /// @param subtitleFile        File to use as source ofsubtitles
      ///
      setSubtitles(...);
#else
      void setSubtitles(const char* subtitleFile);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ showSubtitles(visible) }
      ///-----------------------------------------------------------------------
      /// Enable / disable subtitles.
      ///
      /// @param visible             [boolean] True for visible subtitles.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// xbmc.Player().showSubtitles(True)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      showSubtitles(...);
#else
      void showSubtitles(bool bVisible);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ getSubtitles() }
      ///-----------------------------------------------------------------------
      /// Get subtitle stream name.
      ///
      /// @return                    Stream name
      ///
      getSubtitles();
#else
      String getSubtitles();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ getAvailableSubtitleStreams() }
      ///-----------------------------------------------------------------------
      /// Get Subtitle stream names.
      ///
      /// @return                    List of subtitle streams as name
      ///
      getAvailableSubtitleStreams();
#else
      std::vector<String> getAvailableSubtitleStreams();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ setSubtitleStream(stream) }
      ///-----------------------------------------------------------------------
      /// Set Subtitle Stream.
      ///
      /// @param iStream             [int] Subtitle stream to select for play
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// xbmc.Player().setSubtitleStream(1)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setSubtitleStream(...);
#else
      void setSubtitleStream(int iStream);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ getVideoInfoTag() }
      ///-----------------------------------------------------------------------
      /// To get video info tag.
      ///
      /// Returns the VideoInfoTag of the current playing Movie.
      ///
      /// @return                    Video info tag
      /// @throws Exception          If player is not playing a file or current
      ///                            file is not a movie file.
      ///
      getVideoInfoTag();
#else
      InfoTagVideo* getVideoInfoTag();
#endif

      // Player_GetMusicInfoTag
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ getMusicInfoTag() }
      ///-----------------------------------------------------------------------
      /// To get music info tag.
      ///
      /// Returns the MusicInfoTag of the current playing 'Song'.
      ///
      /// @return                    Music info tag
      /// @throws Exception          If player is not playing a file or current
      ///                            file is not a music file.
      ///
      getMusicInfoTag();
#else
      InfoTagMusic* getMusicInfoTag();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ getRadioRDSInfoTag() }
      ///-----------------------------------------------------------------------
      /// To get Radio RDS info tag
      ///
      /// Returns the RadioRDSInfoTag of the current playing 'Radio Song if.
      /// present'.
      ///
      /// @return                    Radio RDS info tag
      /// @throws Exception          If player is not playing a file or current
      ///                            file is not a rds file.
      ///
      getRadioRDSInfoTag();
#else
      InfoTagRadioRDS* getRadioRDSInfoTag() throw (PlayerException);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ getTotalTime() }
      ///-----------------------------------------------------------------------
      /// To get total playing time.
      ///
      /// Returns the total time of the current playing media in seconds.
      /// This is only accurate to the full second.
      ///
      /// @return                    Total time of the current playing media
      /// @throws Exception          If player is not playing a file.
      ///
      getTotalTime();
#else
      double getTotalTime();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ getAvailableAudioStreams() }
      ///-----------------------------------------------------------------------
      /// Get Audio stream names
      ///
      /// @return                    List of audio streams as name
      ///
      getAvailableAudioStreams();
#else
      std::vector<String> getAvailableAudioStreams();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ setAudioStream(stream) }
      ///-----------------------------------------------------------------------
      /// Set Audio Stream.
      ///
      /// @param iStream             [int] Audio stream to select for play
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// xbmc.Player().setAudioStream(1)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setAudioStream(...);
#else
      void setAudioStream(int iStream);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ getAvailableVideoStreams() }
      ///-----------------------------------------------------------------------
      /// Get Video stream names
      ///
      /// @return                    List of video streams as name
      ///
      getAvailableVideoStreams();
#else
      std::vector<String> getAvailableVideoStreams();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Player
      /// @brief \python_func{ setVideoStream(stream) }
      ///-----------------------------------------------------------------------
      /// Set Video Stream.
      ///
      /// @param iStream             [int] Video stream to select for play
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// xbmc.Player().setVideoStream(1)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setVideoStream(...);
#else
      void setVideoStream(int iStream);
#endif

#if !defined SWIG && !defined DOXYGEN_SHOULD_SKIP_THIS
      SWIGHIDDENVIRTUAL void OnPlayBackStarted();
      SWIGHIDDENVIRTUAL void OnPlayBackEnded();
      SWIGHIDDENVIRTUAL void OnPlayBackStopped();
      SWIGHIDDENVIRTUAL void OnPlayBackPaused();
      SWIGHIDDENVIRTUAL void OnPlayBackResumed();
      SWIGHIDDENVIRTUAL void OnQueueNextItem();
      SWIGHIDDENVIRTUAL void    OnPlayBackSpeedChanged(int iSpeed);
      SWIGHIDDENVIRTUAL void    OnPlayBackSeek(int iTime, int seekOffset);
      SWIGHIDDENVIRTUAL void    OnPlayBackSeekChapter(int iChapter);
#endif

    protected:
    };
  }
}

