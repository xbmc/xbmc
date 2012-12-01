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

#pragma once

#include "cores/playercorefactory/PlayerCoreFactory.h"

#include "ListItem.h"
#include "PlayList.h"
#include "InfoTagVideo.h"
#include "Exception.h"
#include "music/tags/MusicInfoTag.h"
#include "AddonString.h"
#include "InfoTagMusic.h"
#include "AddonCallback.h"

#include "swighelper.h"

namespace XBMCAddon
{
  namespace xbmc
  {
    XBMCCOMMONS_STANDARD_EXCEPTION(PlayerException);

    /**
     * <pre>
     * Player class.
     * 
     * Player([core]) -- Creates a new Player with as default the xbmc music playlist.
     * 
     * core     : (optional) Use a specified playcore instead of letting xbmc decide the playercore to use.
     *          - xbmc.PLAYER_CORE_AUTO
     *          - xbmc.PLAYER_CORE_DVDPLAYER
     *          - xbmc.PLAYER_CORE_MPLAYER
     *          - xbmc.PLAYER_CORE_PAPLAYER
     *
     * This class is a merge of what was previously in xbmcmodule/player.h
     *  and xbmcmodule/PythonPlayer.h without the python references. The
     *  queuing and handling of asynchronous callbacks is done internal to
     *  this class.
     * </pre>
     */
    class Player : public AddonCallback, public IPlayerCallback
    {
    private:
      int iPlayList;
      EPLAYERCORES playerCore;

    public:
      /**
       * <pre>
       * Construct a Player proxying the given generated binding. The 
       *  construction of a Player needs to identify whether or not any 
       *  callbacks will be executed asynchronously or not.
       * </pre>
       */
      Player(int playerCore = EPC_NONE);
      virtual ~Player(void);

      /**
       * <pre>
       * playStream([item, listitem, windowed]) -- Play this item.
       * 
       * item           : [opt] string - filename or url.
       * listitem       : [opt] listitem - used with setInfo() to set different infolabels.
       * windowed       : [opt] bool - true=play video windowed, false=play users preference.(default)
       * 
       * *Note, If item is not given then the Player will try to play the current item
       *        in the current playlist.
       * 
       *        You can use the above as keywords for arguments and skip certain optional arguments.
       *        Once you use a keyword, all following arguments require the keyword.
       * 
       * example:
       *   - listitem = xbmcgui.ListItem('Ironman')
       *   - listitem.setInfo('video', {'Title': 'Ironman', 'Genre': 'Science Fiction'})
       *   - xbmc.Player( xbmc.PLAYER_CORE_MPLAYER ).play(url, listitem, windowed)\n
       * </pre>
       */
      void playStream(const String& item = emptyString, const XBMCAddon::xbmcgui::ListItem* listitem = NULL, bool windowed = false);

      /**
       * <pre>
       * playPlaylist([playlist, windowed]) -- Play this item.
       * 
       * playlist       : [opt] playlist.
       * windowed       : [opt] bool - true=play video windowed, false=play users preference.(default)
       * 
       * *Note, If playlist is not given then the Player will try to play the current item
       *        in the current playlist.
       * 
       *        You can use the above as keywords for arguments and skip certain optional arguments.
       *        Once you use a keyword, all following arguments require the keyword.
       * 
       * example:
       * </pre>
       */
      void playPlaylist(const PlayList* playlist = NULL, bool windowed = false);

      /**
       * <pre>
       * play() -- try to play the current item in the current playlist.
       *
       * windowed       : [opt] bool - true=play video windowed, false=play users preference.(default)
       * 
       * example:
       *   - xbmc.Player( xbmc.PLAYER_CORE_MPLAYER ).play()
       * </pre>
       */
      void playCurrent(bool windowed = false);

      /**
       * <pre>
       * stop() -- Stop playing.
       * </pre>
       */
      void stop();

      /**
       * <pre>
       * pause() -- Pause playing.
       * </pre>
       */
      void pause();

      /**
       * <pre>
       * playnext() -- Play next item in playlist.
       * </pre>
       */
      void playnext();

      /**
       * <pre>
       * playprevious() -- Play previous item in playlist.
       * </pre>
       */
      void playprevious();

      /**
       * <pre>
       * playselected() -- Play a certain item from the current playlist.
       * </pre>
       */
      void playselected(int selected);

      /**
       * <pre>
       * onPlayBackStarted() -- onPlayBackStarted method.
       * 
       * Will be called when xbmc starts playing a file
       * </pre>
       */
      // Player_OnPlayBackStarted
      virtual void onPlayBackStarted();


      /**
       * <pre>
       * onPlayBackEnded() -- onPlayBackEnded method.
       * 
       * Will be called when xbmc stops playing a file
       * </pre>
       */
      // Player_OnPlayBackEnded
      virtual void onPlayBackEnded();

      /**
       * <pre>
       * onPlayBackStopped() -- onPlayBackStopped method.
       * 
       * Will be called when user stops xbmc playing a file
       * </pre>
       */
      // Player_OnPlayBackStopped
      virtual void onPlayBackStopped();

      /**
       * <pre>
       * onPlayBackPaused() -- onPlayBackPaused method.
       * 
       * Will be called when user pauses a playing file
       * </pre>
       */
      // Player_OnPlayBackPaused
      virtual void onPlayBackPaused();

      /**
       * <pre>
       * onPlayBackResumed() -- onPlayBackResumed method.
       * 
       * Will be called when user resumes a paused file
       * </pre>
       */
      // Player_OnPlayBackResumed
      virtual void onPlayBackResumed();

      /**
       * <pre>
       * onQueueNextItem() -- onQueueNextItem method.
       * 
       * Will be called when user queues the next item
       * </pre>
       */
      virtual void onQueueNextItem();

      /**
       * <pre>
       * onPlayBackSpeedChanged(speed) -- onPlayBackSpeedChanged method.
       * 
       * speed          : integer - current speed of player.
       * 
       * *Note, negative speed means player is rewinding, 1 is normal playback speed.
       * 
       * Will be called when players speed changes. (eg. user FF/RW)
       * </pre>
       */
      virtual void onPlayBackSpeedChanged(int speed);

      /**
       * <pre>
       * onPlayBackSeek(time, seekOffset) -- onPlayBackSeek method.
       * 
       * time           : integer - time to seek to.
       * seekOffset     : integer - ?.
       * 
       * Will be called when user seeks to a time
       * </pre>
       */
      virtual void onPlayBackSeek(int time, int seekOffset);

      /**
       * <pre>
       * onPlayBackSeekChapter(chapter) -- onPlayBackSeekChapter method.
       * 
       * chapter        : integer - chapter to seek to.
       * 
       * Will be called when user performs a chapter seek
       * </pre>
       */
      virtual void onPlayBackSeekChapter(int chapter);

      /**
       * <pre>
       * isPlaying() -- returns True is xbmc is playing a file.
       * </pre>
       */
      // Player_IsPlaying
      bool isPlaying();

      /**
       * <pre>
       * isPlayingAudio() -- returns True is xbmc is playing an audio file.
       * </pre>
       */
      // Player_IsPlayingAudio
      bool isPlayingAudio();

      /**
       * <pre>
       * isPlayingVideo() -- returns True if xbmc is playing a video.
       * </pre>
       */
      // Player_IsPlayingVideo
      bool isPlayingVideo();

      /**
       * <pre>
       * getPlayingFile() -- returns the current playing file as a string.
       * 
       * Throws: Exception, if player is not playing a file.
       * </pre>
       */
      // Player_GetPlayingFile
      String getPlayingFile() throw (PlayerException);

      /**
       * <pre>
       * getTime() -- Returns the current time of the current playing media as fractional seconds.
       * 
       * Throws: Exception, if player is not playing a file.
       * </pre>
       */
      // Player_GetTime
      double getTime() throw(PlayerException);

      /**
       * <pre>
       * seekTime() -- Seeks the specified amount of time as fractional seconds.
       *               The time specified is relative to the beginning of the
       *               currently playing media file.
       * 
       * Throws: Exception, if player is not playing a file.
       * </pre>
       */
      // Player_SeekTime
      void seekTime(double seekTime) throw(PlayerException);

      /**
       * <pre>
       * setSubtitles() -- set subtitle file and enable subtitles\n
       * </pre>
       */
      // Player_SetSubtitles
      void setSubtitles(const char* subtitleFile);

      // Player_ShowSubtitles
      /**
       * <pre>
       * showSubtitles(visible) -- enable/disable subtitles
       * 
       * visible        : boolean - True for visible subtitles.
       * example:
       * - xbmc.Player().showSubtitles(True)
       * </pre>
       */
      void showSubtitles(bool bVisible);

      /**
       * <pre>
       * getSubtitles() -- get subtitle stream name
       * </pre>
       */
      // Player_GetSubtitles
      String getSubtitles();

      /**
       * <pre>
       * DisableSubtitles() -- disable subtitles\n
       * </pre>
       */
      // Player_DisableSubtitles
      void disableSubtitles();

      // Player_getAvailableSubtitleStreams
      /**
       * <pre>
       * getAvailableSubtitleStreams() -- get Subtitle stream names
       * </pre>
       */
      std::vector<String>* getAvailableSubtitleStreams();

      // Player_setSubtitleStream
      /**
       * <pre>
       * setSubtitleStream(stream) -- set Subtitle Stream 
       * 
       * stream           : int
       * 
       * example:
       *   - setSubtitleStream(1)
       * </pre>
       */
      void setSubtitleStream(int iStream);

      /**
       * <pre>
       * getVideoInfoTag() -- returns the VideoInfoTag of the current playing Movie.
       * 
       * Throws: Exception, if player is not playing a file or current file is not a movie file.
       * </pre>
       */
      InfoTagVideo* getVideoInfoTag() throw (PlayerException);

      /**
       * <pre>
       * getMusicInfoTag() -- returns the MusicInfoTag of the current playing 'Song'.
       * 
       * Throws: Exception, if player is not playing a file or current file is not a music file.
       * </pre>
       */
      // Player_GetMusicInfoTag
      InfoTagMusic* getMusicInfoTag() throw (PlayerException);

      /**
       * <pre>
       *getTotalTime() -- Returns the total time of the current playing media in
       *                  seconds.  This is only accurate to the full second.
       *
       *Throws: Exception, if player is not playing a file.
       * </pre>
       */
      double getTotalTime() throw (PlayerException);

      std::vector<String>* getAvailableAudioStreams();

      /**
       * <pre>
       * setAudioStream(stream) -- set Audio Stream 
       *
       * stream           : int
       *
       * example:
       *    - setAudioStream(1)
       * </pre>
       */
      void setAudioStream(int iStream);

#ifndef SWIG
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

