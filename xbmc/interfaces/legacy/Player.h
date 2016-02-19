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
    /// @brief <b>Kodi's player class.</b>
    ///
    /// <b><c>xbmc.Player()</c></b>
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

      ///
      /// \ingroup python_Player
      /// @brief Play a item.
      ///
      /// @param[in] item                [opt] string - filename, url or
      ///                                playlist.
      /// @param[in] listitem            [opt] listitem - used with setInfo() to
      ///                                set different infolabels.
      /// @param[in] windowed            [opt] bool - true=play video windowed,
      ///                                false=play users preference.(default)
      /// @param[in] startpos            [opt] int - starting position when
      ///                                playing a playlist. Default = -1
      ///
      /// @note If item is not given then the Player will try to play the current
      ///      item in the current playlist.\n
      ///       \n
      ///       You can use the above as keywords for arguments and skip certain.
      ///      optional arguments.\n
      ///       Once you use a keyword, all following arguments require the keyword.
      ///
      ///
      ///
      ///--------------------------------------------------------------------------
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
      void play(const PlayParameter& item = Player::defaultPlayParameter,
#ifndef DOXYGEN_SHOULD_SKIP_THIS
                const XBMCAddon::xbmcgui::
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
                ListItem* listitem = NULL,
                bool windowed = false,
                int startpos = -1);

      ///
      /// \ingroup python_Player
      /// @brief Stop playing.
      ///
      void stop();

      ///
      /// \ingroup python_Player
      /// @brief Pause or resume playing if already paused.
      ///
      void pause();

      ///
      /// \ingroup python_Player
      /// @brief Play next item in playlist.
      ///
      void playnext();

      ///
      /// \ingroup python_Player
      /// @brief Play previous item in playlist.
      ///
      void playprevious();

      ///
      /// \ingroup python_Player
      /// @brief Play a certain item from the current playlist.
      ///
      void playselected(int selected);

      //
      /// @defgroup python_PlayerCB Callback functions from Kodi to Add-On
      /// \ingroup python_Player
      /// @{
      /// @brief __Callback functions.__
      ///
      /// Functions to handle control callbacks from Kodi to Add-On.
      ///
      /// ------------------------------------------------------------------------
      ///
      /// @link python_Player Go back to normal functions from player@endlink
      //

      ///
      /// \ingroup python_PlayerCB
      /// @brief onPlayBackStarted method.
      ///
      /// Will be called when Kodi starts playing a file.
      ///
      /// @param[in] self                Own base class pointer
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual void onPlayBackStarted();
#else
      void onPlayBackStarted(void* self); // Python function style as doxygen part (not used to build!)
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

      ///
      /// \ingroup python_PlayerCB
      /// @brief onPlayBackEnded method.
      ///
      /// Will be called when Kodi stops playing a file.
      ///
      /// @param[in] self                Own base class pointer
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual void onPlayBackEnded();
#else
      void onPlayBackEnded(void* self); // Python function style as doxygen part (not used to build!)
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

      ///
      /// \ingroup python_PlayerCB
      /// @brief onPlayBackStopped method.
      ///
      /// Will be called when user stops Kodi playing a file.
      ///
      /// @param[in] self                Own base class pointer
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual void onPlayBackStopped();
#else
      void onPlayBackStopped(void* self); // Python function style as doxygen part (not used to build!)
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

      ///
      /// \ingroup python_PlayerCB
      /// @brief onPlayBackPaused method.
      ///
      /// Will be called when user pauses a playing file.
      ///
      /// @param[in] self                Own base class pointer
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual void onPlayBackPaused();
#else
      void onPlayBackPaused(void* self); // Python function style as doxygen part (not used to build!)
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

      ///
      /// \ingroup python_PlayerCB
      /// @brief onPlayBackResumed method.
      ///
      /// Will be called when user resumes a paused file.
      ///
      /// @param[in] self                Own base class pointer
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual void onPlayBackResumed();
#else
      void onPlayBackResumed(void* self); // Python function style as doxygen part (not used to build!)
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

      ///
      /// \ingroup python_PlayerCB
      /// @brief onQueueNextItem method.
      ///
      /// Will be called when user queues the next item.
      ///
      /// @param[in] self                Own base class pointer
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual void onQueueNextItem();
#else
      void onQueueNextItem(void* self); // Python function style as doxygen part (not used to build!)
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

      ///
      /// \ingroup python_PlayerCB
      /// @brief onPlayBackSpeedChanged method.
      ///
      /// Will be called when players speed changes (eg. user FF/RW).
      ///
      /// @param[in] self                Own base class pointer
      /// @param[in] speed               [integer] Current speed of player
      ///
      /// @note Negative speed means player is rewinding, 1 is normal playback
      /// speed.
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual void onPlayBackSpeedChanged(int speed);
#else
      void onPlayBackSpeedChanged(void* self, int speed); // Python function style as doxygen part (not used to build!)
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

      ///
      /// \ingroup python_PlayerCB
      /// @brief onPlayBackSeek method.
      ///
      /// Will be called when user seeks to a time.
      ///
      /// @param[in] self                Own base class pointer
      /// @param[in] time                [integer] Time to seek to
      /// @param[in] seekOffset          [integer] ?
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual void onPlayBackSeek(int time, int seekOffset);
#else
      void onPlayBackSeek(void* self, int time, int seekOffset); // Python function style as doxygen part (not used to build!)
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

      ///
      /// \ingroup python_PlayerCB
      /// @brief onPlayBackSeekChapter method.
      ///
      /// Will be called when user performs a chapter seek.
      ///
      /// @param[in] self                Own base class pointer
      /// @param[in] chapter             [integer] Chapter to seek to
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual void onPlayBackSeekChapter(int chapter);
#else
      void onPlayBackSeekChapter(void* self, int chapter); // Python function style as doxygen part (not used to build!)
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      /// @}

      ///
      /// \ingroup python_Player
      /// @brief Check Kodi is playing something.
      ///
      /// @return                        True if Kodi is playing a file.
      ///
      bool isPlaying();

      ///
      /// \ingroup python_Player
      /// @brief Check for playing audio.
      ///
      /// @return                        True if Kodi is playing an audio file.
      ///
      bool isPlayingAudio();

      ///
      /// \ingroup python_Player
      /// @brief Check for playing video.
      ///
      /// @return                        True if Kodi is playing a video.
      ///
      bool isPlayingVideo();

      ///
      /// \ingroup python_Player
      /// @brief Check for playing radio data system (RDS).
      ///
      /// @return                        True if kodi is playing a radio data
      ///                                system (RDS).
      ///
      bool isPlayingRDS();

      ///
      /// \ingroup python_Player
      /// @brief Returns the current playing file as a string.
      ///
      /// @note For LiveTV, returns a __pvr://__ url which is not translatable
      /// to an OS specific file or external url.
      ///
      /// @return                        Playing filename
      /// @throws Exception, if player is not playing a file.
      ///
      String getPlayingFile();

      ///
      /// \ingroup python_Player
      /// @brief Get playing time.
      ///
      /// Returns the current time of the current playing media as fractional seconds.
      ///
      /// @return                        Current time as fractional seconds
      /// @throws Exception              If player is not playing a file.
      ///
      double getTime();

      ///
      /// \ingroup python_Player
      /// @brief Seek time.
      ///
      /// Seeks the specified amount of time as fractional seconds.
      /// The time specified is relative to the beginning of the currently.
      /// playing media file.
      ///
      /// @param[in] seekTime            Time to seek as fractional seconds
      /// @throws Exception              If player is not playing a file.
      ///
      void seekTime(double seekTime);

      ///
      /// \ingroup python_Player
      /// @brief Set subtitle file and enable subtitles.
      ///
      /// @param[in] subtitleFile        File to use as source ofsubtitles
      ///
      void setSubtitles(const char* subtitleFile);

      ///
      /// \ingroup python_Player
      /// @brief Enable / disable subtitles.
      ///
      /// @param[in] visible             [boolean] True for visible subtitles.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// xbmc.Player().showSubtitles(True)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      void showSubtitles(bool bVisible);

      ///
      /// \ingroup python_Player
      /// @brief Get subtitle stream name.
      ///
      /// @return                        Stream name
      ///
      String getSubtitles();

      ///
      /// \ingroup python_Player
      /// @brief Disable subtitles.
      ///
      void disableSubtitles();

      ///
      /// \ingroup python_Player
      /// @brief Get Subtitle stream names.
      ///
      /// @return                        List of subtitle streams as name
      ///
      std::vector<String> getAvailableSubtitleStreams();

      ///
      /// \ingroup python_Player
      /// @brief Set Subtitle Stream.
      ///
      /// @param[in] iStream             [int] Subtitle stream to select for play
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// xbmc.Player().setSubtitleStream(1)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      void setSubtitleStream(int iStream);

      ///
      /// \ingroup python_Player
      /// @brief To get video info tag.
      ///
      /// Returns the VideoInfoTag of the current playing Movie.
      ///
      /// @return                        Video info tag
      /// @throws Exception              If player is not playing a file or
      ///                                current file is not a movie file.
      ///
      InfoTagVideo* getVideoInfoTag();

      ///
      /// \ingroup python_Player
      /// @brief To get music info tag.
      ///
      /// Returns the MusicInfoTag of the current playing 'Song'.
      ///
      /// @return                        Music info tag
      /// @throws Exception              If player is not playing a file or
      ///                                current file is not a music file.
      ///
      InfoTagMusic* getMusicInfoTag();

      ///
      /// \ingroup python_Player
      /// @brief To get Radio RDS info tag
      ///
      /// Returns the RadioRDSInfoTag of the current playing 'Radio Song if.
      /// present'.
      ///
      /// @return                        Radio RDS info tag
      /// @throws Exception              If player is not playing a file or
      ///                                current file is not a rds file.
      ///
      InfoTagRadioRDS* getRadioRDSInfoTag() throw (PlayerException);

      ///
      /// \ingroup python_Player
      /// @brief To get total playing time.
      ///
      /// Returns the total time of the current playing media in seconds.
      /// This is only accurate to the full second.
      ///
      /// @return                        Total time of the current playing media
      /// @throws Exception              If player is not playing a file.
      ///
      double getTotalTime();

      ///
      /// \ingroup python_Player
      /// @brief Get Audio stream names
      ///
      /// @return                        List of audio streams as name
      ///
      std::vector<String> getAvailableAudioStreams();

      ///
      /// \ingroup python_Player
      /// @brief Set Audio Stream.
      ///
      /// @param[in] iStream             [int] Audio stream to select for play
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// xbmc.Player().setAudioStream(1)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      void setAudioStream(int iStream);

      ///
      /// \ingroup python_Player
      /// @brief Get Video stream names
      ///
      /// @return                        List of video streams as name
      ///
      std::vector<String> getAvailableVideoStreams();

      ///
      /// \ingroup python_Player
      /// @brief Set Video Stream.
      ///
      /// @param[in] iStream             [int] Video stream to select for play
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// xbmc.Player().setVideoStream(1)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      void setVideoStream(int iStream);

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

