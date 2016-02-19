#pragma once
/*
 *      Copyright (C) 2015 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "kodi/api2/gui/ListItem.hpp"
#include "kodi/api2/player/PlayList.hpp"

namespace V2
{
namespace KodiAPI
{

  namespace Player
  {

  /// \defgroup CPP_V2_KodiAPI_Player_CPlayer
  /// \ingroup CPP_V2_KodiAPI_Player
  /// @{
  /// @brief <b>Creates a new Player</b>
  ///
  /// This player use callbacks from Kodi to add-on who are normally used defined
  /// with  this  class as parent and the all from add-on needed virtual function
  /// must be present on child.
  ///
  /// Also  is  it  possible  to  use  this  class  complete  independent  in two
  /// cases,  one  is  when  no  callbacks are  needed  and  the  other  is  with
  /// own  defined  functions  for  them  and  reported to  Kodi with a  call  of
  /// \ref KodiAPI::Player::CPlayer::SetIndependentCallbacks   where  the  needed
  /// callback functions are given with address to Kodi.
  ///
  /// It has the header \ref AddonPlayer.h "#include <kodi/api2/player/AddonPlayer.h>"
  /// be included to enjoy it.
  ///
  class CPlayer
  {
  public:
    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Class constructor
    ///
    CPlayer();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Class Destructor
    ///
    virtual ~CPlayer();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief To retrieve the supported media type formats on Kodi
    /// by selected media type (File ends e.g. .mp3 or .mpg).
    ///
    /// @param[in] mediaType            Media type to use
    /// @return                         Supported Media types as fileendings
    ///
    static std::string GetSupportedMedia(AddonPlayListType mediaType);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Play a item from filename.
    ///
    /// @param[in] item                string - filename, url or
    ///                                playlist.
    /// @param[in] windowed            [opt] bool - true=play video windowed,
    ///                                false=play users preference.(default)
    ///
    bool Play(const std::string& item, bool windowed = false);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Play a item from a GUI list entry.
    ///
    /// @param[in] listitem            listitem - used with setInfo() to
    ///                                set different infolabels.
    /// @param[in] windowed            [opt] bool - true=play video windowed,
    ///                                false=play users preference.(default)
    ///
    bool Play(const V2::KodiAPI::GUI::CListItem* listitem, bool windowed = false);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Play a items from playlist.
    ///
    /// @param[in] list                list source to play
    /// @param[in] windowed            [opt] bool - true=play video windowed,
    ///                                false=play users preference.(default)
    /// @param[in] startpos            [opt] int - starting position when
    ///                                playing a playlist. Default = -1
    ///
    bool Play(const CPlayList* list, bool windowed = false, int startpos = -1);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Stop playing.
    ///
    void Stop();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Pause or resume playing if already paused.
    ///
    void Pause();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Play next item in playlist.
    ///
    void PlayNext();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Play previous item in playlist.
    ///
    void PlayPrevious();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Play a certain item from the current playlist.
    ///
    /// @param[in] selected             Position in current playlist
    ///
    void PlaySelected(int selected);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Check Kodi is playing something.
    ///
    /// @return                        True if Kodi is playing a file.
    ///
    bool IsPlaying();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Check for playing audio.
    ///
    /// @return                        True if Kodi is playing an audio file.
    ///
    bool IsPlayingAudio();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Check for playing video.
    ///
    /// @return                        True if Kodi is playing a video.
    ///
    bool IsPlayingVideo();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Check for playing radio data system (RDS).
    ///
    /// @return                        True if kodi is playing a radio data
    ///                                system (RDS).
    ///
    bool IsPlayingRDS();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Returns the current playing file as a string.
    ///
    /// @note For LiveTV, returns a __pvr://__ url which is not translatable
    /// to an OS specific file or external url.
    ///
    /// @return                        Playing filename
    ///
    bool GetPlayingFile(std::string& file);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief To get total playing time.
    ///
    /// Returns the total time of the current playing media in seconds.
    /// This is only accurate to the full second.
    ///
    /// @return                        Total time of the current playing media
    ///
    double GetTotalTime();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Get playing time.
    ///
    /// Returns the current time of the current playing media as fractional seconds.
    ///
    /// @return                        Current time as fractional seconds
    ///
    double GetTime();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Seek time.
    ///
    /// Seeks the specified amount of time as fractional seconds.
    /// The time specified is relative to the beginning of the currently.
    /// playing media file.
    ///
    /// @param[in] seekTime            Time to seek as fractional seconds
    ///
    void SeekTime(double seekTime);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Get Video stream names
    ///
    /// @param[out] streams            List of Video streams as name
    /// @return                        true if successed, otherwise false
    ///
    bool GetAvailableVideoStreams(std::vector<std::string> &streams);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Set Video Stream.
    ///
    /// @param[in] iStream             [int] Video stream to select for play
    ///
    void SetVideoStream(int iStream);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Get Audio stream names
    ///
    /// @param[out] streams            List of audio streams as name
    /// @return                        true if successed, otherwise false
    ///
    bool GetAvailableAudioStreams(std::vector<std::string> &streams);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Set Audio Stream.
    ///
    /// @param[in] iStream             [int] Audio stream to select for play
    ///
    void SetAudioStream(int iStream);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Get Subtitle stream names.
    ///
    /// @param[out] streams            List of subtitle streams as name
    /// @return                        true if successed, otherwise false
    ///
    bool GetAvailableSubtitleStreams(std::vector<std::string> &streams);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Set Subtitle Stream.
    ///
    /// @param[in] iStream             [int] Subtitle stream to select for play
    ///
    void SetSubtitleStream(int iStream);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Enable / disable subtitles.
    ///
    /// @param[in] visible             [boolean] True for visible subtitles.
    ///
    void ShowSubtitles(bool bVisible);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Get subtitle stream name.
    ///
    /// @param[out] name               The name of currently used subtitle
    /// @return                        true if successed and subtitle present
    ///
    bool GetCurrentSubtitleName(std::string& name);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Set subtitle file and enable subtitles.
    ///
    /// @param[in] subPath              File to use as source ofsubtitles
    ///
    void AddSubtitle(const std::string& subPath);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @defgroup CPP_V2_KodiAPI_Player_CPlayer_callbacks Callback functions from Kodi to add-on
    /// \ingroup CPP_V2_KodiAPI_Player_CPlayer
    /// @brief Functions to handle control callbacks from Kodi
    ///
    /// @link CPP_V2_KodiAPI_Player_CPlayer Go back to normal functions from CPP_V2_KodiAPI_Player_CPlayer@endlink
    ///


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer_callbacks
    /// @brief OnPlayBackStarted method.
    ///
    /// Will be called when Kodi starts playing a file.
    ///
    /// @note Function becomes only called from Kodi itself
    ///
    virtual void OnPlayBackStarted() { }
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer_callbacks
    /// @brief onPlayBackEnded method.
    ///
    /// Will be called when Kodi stops playing a file.
    ///
    /// @note Function becomes only called from Kodi itself
    ///
    virtual void OnPlayBackEnded() { }
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer_callbacks
    /// @brief onPlayBackStopped method.
    ///
    /// Will be called when user stops Kodi playing a file.
    ///
    /// @note Function becomes only called from Kodi itself
    ///
    virtual void OnPlayBackStopped() { }
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer_callbacks
    /// @brief onPlayBackPaused method.
    ///
    /// Will be called when user pauses a playing file.
    ///
    /// @note Function becomes only called from Kodi itself
    ///
    virtual void OnPlayBackPaused() { }
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer_callbacks
    /// @brief onPlayBackResumed method.
    ///
    /// Will be called when user resumes a paused file.
    ///
    /// @note Function becomes only called from Kodi itself
    ///
    virtual void OnPlayBackResumed() { }
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer_callbacks
    /// @brief onQueueNextItem method.
    ///
    /// Will be called when user queues the next item.
    ///
    /// @note Function becomes only called from Kodi itself
    ///
    virtual void OnQueueNextItem() { }
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer_callbacks
    /// @brief onPlayBackSpeedChanged method.
    ///
    /// Will be called when players speed changes (eg. user FF/RW).
    ///
    /// @param[in] speed               [integer] Current speed of player
    ///
    /// @note Function becomes only called from Kodi itself
    ///
    virtual void OnPlayBackSpeedChanged(int iSpeed) { }
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer_callbacks
    /// @brief onPlayBackSeek method.
    ///
    /// Will be called when user seeks to a time.
    ///
    /// @param[in] time                [integer] Time to seek to
    /// @param[in] seekOffset          [integer] ?
    ///
    /// @note Function becomes only called from Kodi itself
    ///
    virtual void OnPlayBackSeek(int iTime, int seekOffset) { }
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer_callbacks
    /// @brief onPlayBackSeekChapter method.
    ///
    /// Will be called when user performs a chapter seek.
    ///
    /// @param[in] chapter             [integer] Chapter to seek to
    ///
    /// @note Function becomes only called from Kodi itself.
    ///
    virtual void OnPlayBackSeekChapter(int iChapter) { }
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_Player_CPlayer_callbacks
    /// @brief If the class is used independent (with "new CPP_V2_KodiAPI_Player_CPlayer")
    /// and not as parent (with "cCLASS_own : KodiAPI::Player::CPlayer") from own must
    /// be the callback from Kodi to add-on overdriven with own functions!
    ///
    void SetIndependentCallbacks(
        PLAYERHANDLE     cbhdl,
        void      (*CBOnPlayBackStarted)     (PLAYERHANDLE cbhdl),
        void      (*CBOnPlayBackEnded)       (PLAYERHANDLE cbhdl),
        void      (*CBOnPlayBackStopped)     (PLAYERHANDLE cbhdl),
        void      (*CBOnPlayBackPaused)      (PLAYERHANDLE cbhdl),
        void      (*CBOnPlayBackResumed)     (PLAYERHANDLE cbhdl),
        void      (*CBOnQueueNextItem)       (PLAYERHANDLE cbhdl),
        void      (*CBOnPlayBackSpeedChanged)(PLAYERHANDLE cbhdl, int iSpeed),
        void      (*CBOnPlayBackSeek)        (PLAYERHANDLE cbhdl, int iTime, int seekOffset),
        void      (*CBOnPlayBackSeekChapter) (PLAYERHANDLE cbhdl, int iChapter));
    //--------------------------------------------------------------------------

    IMPL_ADDON_PLAYER;
  };
  /// @}
  }; /* namespace Player */

}; /* namespace KodiAPI */
}; /* namespace V2 */
