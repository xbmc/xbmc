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

#include "network/Network.h"
#include "system.h"
#include "CompileInfo.h"
#include "GUIInfoManager.h"
#include "view/GUIViewState.h"
#include "windows/GUIMediaWindow.h"
#include "dialogs/GUIDialogKeyboardGeneric.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogProgress.h"
#include "Application.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "utils/Weather.h"
#include "PartyModeManager.h"
#include "addons/Visualisation.h"
#include "input/ButtonTranslator.h"
#include "utils/AlarmClock.h"
#include "LangInfo.h"
#include "utils/SystemInfo.h"
#include "guilib/GUITextBox.h"
#include "guilib/GUIControlGroupList.h"
#include "pictures/GUIWindowSlideShow.h"
#include "pictures/PictureInfoTag.h"
#include "music/tags/MusicInfoTag.h"
#include "guilib/IGUIContainer.h"
#include "guilib/GUIWindowManager.h"
#include "PlayListPlayer.h"
#include "playlists/PlayList.h"
#include "profiles/ProfilesManager.h"
#include "windowing/WindowingFactory.h"
#include "powermanagement/PowerManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SkinSettings.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/StereoscopicsManager.h"
#include "utils/CharsetConverter.h"
#include "utils/CPUInfo.h"
#include "utils/SortUtils.h"
#include "utils/StringUtils.h"
#include "utils/MathUtils.h"
#include "utils/SeekHandler.h"
#include "URL.h"
#include "addons/Skin.h"
#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include "cores/DataCacheCore.h"
#include "guiinfo/GUIInfoLabels.h"
#include "messaging/ApplicationMessenger.h"

// stuff for current song
#include "music/MusicInfoLoader.h"

#include "GUIUserMessages.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "storage/MediaManager.h"
#include "utils/TimeUtils.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/channels/PVRRadioRDSInfoTag.h"
#include "epg/EpgContainer.h"
#include "pvr/recordings/PVRRecording.h"

#include "addons/AddonManager.h"
#include "interfaces/info/InfoBool.h"
#include "video/VideoThumbLoader.h"
#include "music/MusicThumbLoader.h"
#include "video/VideoDatabase.h"
#include "cores/IPlayer.h"
#include "cores/AudioEngine/Engines/ActiveAE/AudioDSPAddons/ActiveAEDSP.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/VideoPlayer/VideoRenderers/BaseRenderer.h"
#include "interfaces/info/InfoExpression.h"

#if defined(TARGET_DARWIN_OSX)
#include "platform/darwin/osx/smc.h"
#include "linux/LinuxResourceCounter.h"
static CLinuxResourceCounter m_resourceCounter;
#endif

#ifdef TARGET_POSIX
#include "linux/XMemUtils.h"
#endif

#define SYSHEATUPDATEINTERVAL 60000

using namespace XFILE;
using namespace MUSIC_INFO;
using namespace ADDON;
using namespace PVR;
using namespace INFO;
using namespace EPG;

class CSetCurrentItemJob : public CJob
{
  CFileItemPtr m_itemCurrentFile;
public:
  CSetCurrentItemJob(const CFileItemPtr item) : m_itemCurrentFile(item) { }
  ~CSetCurrentItemJob(void) {}

  bool DoWork(void)
  {
    g_infoManager.SetCurrentItemJob(m_itemCurrentFile);
    return true;
  }
};

CGUIInfoManager::CGUIInfoManager(void) :
    Observable()
{
  m_lastSysHeatInfoTime = -SYSHEATUPDATEINTERVAL;  // make sure we grab CPU temp on the first pass
  m_fanSpeed = 0;
  m_AfterSeekTimeout = 0;
  m_seekOffset = 0;
  m_nextWindowID = WINDOW_INVALID;
  m_prevWindowID = WINDOW_INVALID;
  m_stringParameters.push_back("__ZZZZ__");   // to offset the string parameters by 1 to assure that all entries are non-zero
  m_currentFile = new CFileItem;
  m_currentSlide = new CFileItem;
  m_frameCounter = 0;
  m_lastFPSTime = 0;
  m_playerShowTime = false;
  m_playerShowInfo = false;
  m_fps = 0.0f;
  ResetLibraryBools();
}

CGUIInfoManager::~CGUIInfoManager(void)
{
  delete m_currentFile;
  delete m_currentSlide;
}

bool CGUIInfoManager::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_NOTIFY_ALL)
  {
    if (message.GetParam1() == GUI_MSG_UPDATE_ITEM && message.GetItem())
    {
      CFileItemPtr item = std::static_pointer_cast<CFileItem>(message.GetItem());
      if (m_currentFile->IsSamePath(item.get()))
      {
        m_currentFile->UpdateInfo(*item);
        return true;
      }
    }
  }
  return false;
}

/// \brief Translates a string as given by the skin into an int that we use for more
/// efficient retrieval of data. Can handle combined strings on the form
/// Player.Caching + VideoPlayer.IsFullscreen (Logical and)
/// Player.HasVideo | Player.HasAudio (Logical or)
int CGUIInfoManager::TranslateString(const std::string &condition)
{
  // translate $LOCALIZE as required
  std::string strCondition(CGUIInfoLabel::ReplaceLocalize(condition));
  return TranslateSingleString(strCondition);
}

typedef struct
{
  const char *str;
  int  val;
} infomap;

/// \page modules__General__List_of_gui_access List of GUI access messages
/// \tableofcontents
///
/// \section modules__General__List_of_gui_access_Description Description
/// Skins can use boolean conditions with the <b><visible></b> tag or with condition
/// attributes. Scripts can read boolean conditions with
/// <b>xbmc.getCondVisibility(condition)</b>.
///
/// Skins can use infolabels with <b>$INFO[infolabel]</b> or the <b><info></b> tag. Scripts
/// can read infolabels with <b>xbmc.getInfoLabel('infolabel')</b>.


const infomap string_bools[] =   {{ "isempty",          STRING_IS_EMPTY },
                                  { "isequal",          STRING_IS_EQUAL },
                                  { "startswith",       STRING_STARTS_WITH },
                                  { "endswith",         STRING_ENDS_WITH },
                                  { "contains",         STRING_CONTAINS }};

const infomap integer_bools[] =  {{ "isequal",          INTEGER_IS_EQUAL },
                                  { "isgreater",        INTEGER_GREATER_THAN },
                                  { "isgreaterorequal", INTEGER_GREATER_OR_EQUAL },
                                  { "isless",           INTEGER_LESS_THAN },
                                  { "islessorequal",    INTEGER_LESS_OR_EQUAL }};

/// \page modules__General__List_of_gui_access
/// \section modules__General__List_of_gui_access_Player Player
/// @{
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Player.HasAudio`</b>,
///                  \anchor Player_HasAudio
///                  _boolean_,
///     Returns true if the player has an audio file.
///   }
///   \table_row3{   <b>`Player.HasMedia`</b>,
///                  \anchor Player_HasMedia
///                  _boolean_,
///     Returns true if the player has an audio or video file.
///   }
///   \table_row3{   <b>`Player.HasVideo`</b>,
///                  \anchor Player_HasVideo
///                  _boolean_,
///     Returns true if the player has a video file.
///   }
///   \table_row3{   <b>`Player.Paused`</b>,
///                  \anchor Player_Paused
///                  _boolean_,
///     Returns true if the player is paused.
///   }
///   \table_row3{   <b>`Player.Playing`</b>,
///                  \anchor Player_Playing
///                  _boolean_,
///     Returns true if the player is currently playing (ie not ffwding\,
///     rewinding or paused.)
///   }
///   \table_row3{   <b>`Player.Rewinding`</b>,
///                  \anchor Player_Rewinding
///                  _boolean_,
///     Returns true if the player is rewinding.
///   }
///   \table_row3{   <b>`Player.Rewinding2x`</b>,
///                  \anchor Player_Rewinding2x
///                  _boolean_,
///     Returns true if the player is rewinding at 2x.
///   }
///   \table_row3{   <b>`Player.Rewinding4x`</b>,
///                  \anchor Player_Rewinding4x
///                  _boolean_,
///     Returns true if the player is rewinding at 4x.
///   }
///   \table_row3{   <b>`Player.Rewinding8x`</b>,
///                  \anchor Player_Rewinding8x
///                  _boolean_,
///     Returns true if the player is rewinding at 8x.
///   }
///   \table_row3{   <b>`Player.Rewinding16x`</b>,
///                  \anchor Player_Rewinding16x
///                  _boolean_,
///     Returns true if the player is rewinding at 16x.
///   }
///   \table_row3{   <b>`Player.Rewinding32x`</b>,
///                  \anchor Player_Rewinding32x
///                  _boolean_,
///     Returns true if the player is rewinding at 32x.
///   }
///   \table_row3{   <b>`Player.Forwarding`</b>,
///                  \anchor Player_Forwarding
///                  _boolean_,
///     Returns true if the player is fast forwarding.
///   }
///   \table_row3{   <b>`Player.Forwarding2x`</b>,
///                  \anchor Player_Forwarding2x
///                  _boolean_,
///     Returns true if the player is fast forwarding at 2x.
///   }
///   \table_row3{   <b>`Player.Forwarding4x`</b>,
///                  \anchor Player_Forwarding4x
///                  _boolean_,
///     Returns true if the player is fast forwarding at 4x.
///   }
///   \table_row3{   <b>`Player.Forwarding8x`</b>,
///                  \anchor Player_Forwarding8x
///                  _boolean_,
///     Returns true if the player is fast forwarding at 8x.
///   }
///   \table_row3{   <b>`Player.Forwarding16x`</b>,
///                  \anchor Player_Forwarding16x
///                  _boolean_,
///     Returns true if the player is fast forwarding at 16x.
///   }
///   \table_row3{   <b>`Player.Forwarding32x`</b>,
///                  \anchor Player_Forwarding32x
///                  _boolean_,
///     Returns true if the player is fast forwarding at 32x.
///   }
///   \table_row3{   <b>`Player.CanRecord`</b>,
///                  \anchor Player_CanRecord
///                  _boolean_,
///     Returns true if the player can record the current internet stream.
///   }
///   \table_row3{   <b>`Player.Recording`</b>,
///                  \anchor Player_Recording
///                  _boolean_,
///     Returns true if the player is recording the current internet stream.
///   }
///   \table_row3{   <b>`Player.Caching`</b>,
///                  \anchor Player_Caching
///                  _boolean_,
///     Returns true if the player is current re-caching data (internet based
///     video playback).
///   }
///   \table_row3{   <b>`Player.DisplayAfterSeek`</b>,
///                  \anchor Player_DisplayAfterSeek
///                  _boolean_,
///     Returns true for the first 2.5 seconds after a seek.
///   }
///   \table_row3{   <b>`Player.Seekbar`</b>,
///                  \anchor Player_Seekbar
///                  _integer_,
///     Returns amount of percent of one seek to other position
///   }
///   \table_row3{   <b>`Player.Seeking`</b>,
///                  \anchor Player_Seeking
///                  _boolean_,
///     Returns true if a seek is in progress
///   }
///   \table_row3{   <b>`Player.ShowTime`</b>,
///                  \anchor Player_ShowTime
///                  _boolean_,
///     Returns true if the user has requested the time to show (occurs in video
///     fullscreen)
///   }
///   \table_row3{   <b>`Player.ShowInfo`</b>,
///                  \anchor Player_ShowInfo
///                  _boolean_,
///     Returns true if the user has requested the song info to show (occurs in
///     visualisation fullscreen and slideshow)
///   }
///   \table_row3{   <b>`Player.ShowCodec`</b>,
///                  \anchor Player_ShowCodec
///                  _boolean_,
///     Returns true if the user has requested the codec to show (occurs in
///     visualisation and video fullscreen)
///   }
///   \table_row3{   <b>`Player.Title`</b>,
///                  \anchor Player_Title
///                  _boolean_,
///     Returns the musicplayer title for audio and the videoplayer title for
///     videos.
///   }
///   \table_row3{   <b>`Player.Muted`</b>,
///                  \anchor Player_Muted
///                  _boolean_,
///     Returns true if the volume is muted.
///   }
///   \table_row3{   <b>`Player.HasDuration`</b>,
///                  \anchor Player_HasDuration
///                  _boolean_,
///     Returns true if Media isn't a true stream
///   }
///   \table_row3{   <b>`Player.Passthrough`</b>,
///                  \anchor Player_Passthrough
///                  _boolean_,
///     Returns true if the player is using audio passthrough.
///   }
///   \table_row3{   <b>`Player.CacheLevel`</b>,
///                  \anchor Player_CacheLevel
///                  _string_,
///     Get the used cache level as string with an integer number
///   }
///   \table_row3{   <b>`Player.Progress`</b>,
///                  \anchor Player_Progress
///                  _integer_,
///     Returns the progress position as percent
///   }
///   \table_row3{   <b>`Player.ProgressCache`</b>,
///                  \anchor Player_ProgressCache
///                  _boolean_,
///     Shows how much of the file is cached above current play percentage
///   }
///   \table_row3{   <b>`Player.Volume`</b>,
///                  \anchor Player_Volume
///                  _string_,
///     Returns the current player volume with the format `%2.1f dB`
///   }
///   \table_row3{   <b>`Player.SubtitleDelay`</b>,
///                  \anchor Player_SubtitleDelay
///                  _string_,
///     Return the used subtitle delay with the format `%2.3f s`
///   }
///   \table_row3{   <b>`Player.AudioDelay`</b>,
///                  \anchor Player_AudioDelay
///                  _string_,
///     Return the used audio delay with the format `%2.3f s`
///   }
///   \table_row3{   <b>`Player.Chapter`</b>,
///                  \anchor Player_Chapter
///                  _integer_,
///     Current chapter of current playing media
///   }
///   \table_row3{   <b>`Player.ChapterCount`</b>,
///                  \anchor Player_ChapterCount
///                  _integer_,
///     Total number of chapters of current playing media
///   }
///   \table_row3{   <b>`Player.ChapterName`</b>,
///                  \anchor Player_ChapterName
///                  _string_,
///     Return the name of currently used chapter if available
///   }
///   \table_row3{   <b>`Player.Folderpath`</b>,
///                  \anchor Player_Folderpath
///                  _path_,
///     Shows the full path of the currently playing song or movie
///   }
///   \table_row3{   <b>`Player.FilenameAndPath`</b>,
///                  \anchor FilenameAndPath
///                  _boolean_,
///     Shows the full path with filename of the currently playing song or movie
///   }
///   \table_row3{   <b>`Player.Filename`</b>,
///                  \anchor Player_Filename
///                  _path_,
///     Returns the filename of the currently playing media.
///   }
///   \table_row3{   <b>`Player.IsInternetStream`</b>,
///                  \anchor Player_IsInternetStream
///                  _boolean_,
///     Returns true if the player is playing an internet stream.
///   }
///   \table_row3{   <b>`Player.PauseEnabled`</b>,
///                  \anchor Player_PauseEnabled
///                  _boolean_,
///     Returns true if played stream is paused
///   }
///   \table_row3{   <b>`Player.SeekEnabled`</b>,
///                  \anchor Player_SeekEnabled
///                  _boolean_,
///     Returns true if seek on playing is enabled
///   }
///   \table_row3{   <b>`Player.ChannelPreviewActive`</b>,
///                  \anchor Player_ChannelPreviewActive
///                  _boolean_,
///     Returns true if pvr channel preview is active (used channel tag different
///     from played tag)
///   }
/// \table_end
/// @}
const infomap player_labels[] =  {{ "hasmedia",         PLAYER_HAS_MEDIA },           // bools from here
                                  { "hasaudio",         PLAYER_HAS_AUDIO },
                                  { "hasvideo",         PLAYER_HAS_VIDEO },
                                  { "playing",          PLAYER_PLAYING },
                                  { "paused",           PLAYER_PAUSED },
                                  { "rewinding",        PLAYER_REWINDING },
                                  { "forwarding",       PLAYER_FORWARDING },
                                  { "rewinding2x",      PLAYER_REWINDING_2x },
                                  { "rewinding4x",      PLAYER_REWINDING_4x },
                                  { "rewinding8x",      PLAYER_REWINDING_8x },
                                  { "rewinding16x",     PLAYER_REWINDING_16x },
                                  { "rewinding32x",     PLAYER_REWINDING_32x },
                                  { "forwarding2x",     PLAYER_FORWARDING_2x },
                                  { "forwarding4x",     PLAYER_FORWARDING_4x },
                                  { "forwarding8x",     PLAYER_FORWARDING_8x },
                                  { "forwarding16x",    PLAYER_FORWARDING_16x },
                                  { "forwarding32x",    PLAYER_FORWARDING_32x },
                                  { "canrecord",        PLAYER_CAN_RECORD },
                                  { "recording",        PLAYER_RECORDING },
                                  { "displayafterseek", PLAYER_DISPLAY_AFTER_SEEK },
                                  { "caching",          PLAYER_CACHING },
                                  { "seekbar",          PLAYER_SEEKBAR },
                                  { "seeking",          PLAYER_SEEKING },
                                  { "showtime",         PLAYER_SHOWTIME },
                                  { "showcodec",        PLAYER_SHOWCODEC },
                                  { "showinfo",         PLAYER_SHOWINFO },
                                  { "title",            PLAYER_TITLE },
                                  { "muted",            PLAYER_MUTED },
                                  { "hasduration",      PLAYER_HASDURATION },
                                  { "passthrough",      PLAYER_PASSTHROUGH },
                                  { "cachelevel",       PLAYER_CACHELEVEL },          // labels from here
                                  { "progress",         PLAYER_PROGRESS },
                                  { "progresscache",    PLAYER_PROGRESS_CACHE },
                                  { "volume",           PLAYER_VOLUME },
                                  { "subtitledelay",    PLAYER_SUBTITLE_DELAY },
                                  { "audiodelay",       PLAYER_AUDIO_DELAY },
                                  { "chapter",          PLAYER_CHAPTER },
                                  { "chaptercount",     PLAYER_CHAPTERCOUNT },
                                  { "chaptername",      PLAYER_CHAPTERNAME },
                                  { "folderpath",       PLAYER_PATH },
                                  { "filenameandpath",  PLAYER_FILEPATH },
                                  { "filename",         PLAYER_FILENAME },
                                  { "isinternetstream", PLAYER_ISINTERNETSTREAM },
                                  { "pauseenabled",     PLAYER_CAN_PAUSE },
                                  { "seekenabled",      PLAYER_CAN_SEEK },
                                  { "channelpreviewactive", PLAYER_IS_CHANNEL_PREVIEW_ACTIVE},
                                  { "tempoenabled", PLAYER_SUPPORTS_TEMPO},
                                  { "istempo", PLAYER_IS_TEMPO},
                                  { "playspeed", PLAYER_PLAYSPEED}};

/// \page modules__General__List_of_gui_access
/// @{
/// \table_start
///   \table_row3{   <b>`Player.Art(fanart)`</b>,
///                  \anchor Player_Art_fanart
///                  _string_,
///     Fanart Image of the currently playing episode's parent TV show
///   }
///   \table_row3{   <b>`Player.Art(thumb)`</b>,
///                  \anchor Player_Art_thumb
///                  _string_,
///     Returns the thumbnail image of the currently playing item.
///   }
///   \table_row3{   <b>`Player.Art(poster)`</b>,
///                  \anchor Player_Art_poster
///                  _string_,
///     Returns the poster of the currently playing movie.
///   }
///   \table_row3{   <b>`Player.Art(tvshow.poster)`</b>,
///                  \anchor Player_Art_tvshowposter
///                  _string_,
///     Returns the tv show poster of the currently playing episode's parent TV show.
///   }
///   \table_row3{   <b>`Player.Art(tvshow.banner)`</b>,
///                  \anchor Player_Art_tvshowbanner
///                  _string_,
///     Returns the tv show banner of the currently playing episode's parent TV show.
///   }
/// \table_end
/// @}
const infomap player_param[] =   {{ "art",              PLAYER_ITEM_ART }};

/// \page modules__General__List_of_gui_access
/// @{
/// \table_start
///   \table_row3{   <b>`Player.SeekTime`</b>,
///                  \anchor Player_SeekTime
///                  _string_,
///     Time to which the user is seeking
///   }
///   \table_row3{   <b>`Player.SeekOffset`</b>,
///                  \anchor Player_SeekOffset
///                  _string_,
///     Indicates the seek offset after a seek press (eg user presses
///     BigStepForward\, player.seekoffset returns +10:00)
///   }
///   \table_row3{   <b>`Player.SeekOffset(format)`</b>,
///                  \anchor Player_SeekOffset_format
///                  _string_,
///     Shows hours (hh)\, minutes (mm) or seconds (ss). Also supported: (hh:mm)\,
///     (mm:ss)\, (hh:mm:ss)\, (hh:mm:ss).
///   }
///   \table_row3{   <b>`Player.SeekStepSize`</b>,
///                  \anchor Player_SeekStepSize
///                  _string_,
///     Displays the seek step size. (v15 addition)
///   }
///   \table_row3{   <b>`Player.TimeRemaining`</b>,
///                  \anchor Player_TimeRemaining
///                  _string_,
///     Remaining time of current playing media
///   }
///   \table_row3{   <b>`Player.TimeRemaining(format)`</b>,
///                  \anchor Player_TimeRemaining_format
///                  _string_,
///     Shows hours (hh)\, minutes (mm) or seconds (ss). When 12 hour clock is
///     used (xx) will return AM/PM. Also supported: (hh:mm)\, (mm:ss)\,
///     (hh:mm:ss)\, (hh:mm:ss).
///   }
///   \table_row3{   <b>`Player.TimeSpeed`</b>,
///                  \anchor Player_TimeSpeed
///                  _string_,
///     Both the time and the playspeed formatted up. eg 1:23 (2x)
///   }
///   \table_row3{   <b>`Player.Time`</b>,
///                  \anchor Player_Time
///                  _string_,
///     Elapsed time of current playing media
///   }
///   \table_row3{   <b>`Player.Time(format)`</b>,
///                  \anchor Player_Time_format
///                  _string_,
///     Shows hours (hh)\, minutes (mm) or seconds (ss). When 12 hour clock is
///     used (xx) will return AM/PM. Also supported: (hh:mm)\, (mm:ss)\,
///     (hh:mm:ss)\, (hh:mm:ss).
///   }
///   \table_row3{   <b>`Player.Duration`</b>,
///                  \anchor Player_Duration
///                  _string_,
///     Total duration of the current playing media
///   }
///   \table_row3{   <b>`Player.Duration(format)`</b>,
///                  \anchor Player_Duration_format
///                  _string_,
///     Shows hours (hh)\, minutes (mm) or seconds (ss). When 12 hour clock is used
///     (xx) will return AM/PM. Also supported: (hh:mm)\, (mm:ss)\, (hh:mm:ss)\,
///     (hh:mm:ss).
///   }
///   \table_row3{   <b>`Player.FinishTime`</b>,
///                  \anchor Player_FinishTime
///                  _string_,
///     Time playing media will end
///   }
///   \table_row3{   <b>`Player.FinishTime(format)`</b>,
///                  \anchor Player_FinishTime_format
///                  _string_,
///     Shows hours (hh)\, minutes (mm) or seconds (ss). When 12 hour clock is
///     used (xx) will return AM/PM. Also supported: (hh:mm)\, (mm:ss)\,
///     (hh:mm:ss)\, (hh:mm:ss).
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}
const infomap player_times[] =   {{ "seektime",         PLAYER_SEEKTIME },
                                  { "seekoffset",       PLAYER_SEEKOFFSET },
                                  { "seekstepsize",     PLAYER_SEEKSTEPSIZE },
                                  { "timeremaining",    PLAYER_TIME_REMAINING },
                                  { "timespeed",        PLAYER_TIME_SPEED },
                                  { "time",             PLAYER_TIME },
                                  { "duration",         PLAYER_DURATION },
                                  { "finishtime",       PLAYER_FINISH_TIME },
                                  { "starttime",        PLAYER_START_TIME}};

/// \page modules__General__List_of_gui_access
/// \section modules__General__List_of_gui_access_Weather Weather
/// @{
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Weather.IsFetched`</b>,
///                  \anchor Weather_IsFetched
///                  _boolean_,
///     Returns true if the weather data has been downloaded.
///   }
///   \table_row3{   <b>`Weather.Conditions`</b>,
///                  \anchor Weather_Conditions
///                  _string_,
///     Current weather conditions – this is looked up in a background process.
///   }
///   \table_row3{   <b>`Weather.Temperature`</b>,
///                  \anchor Weather_Temperature
///                  _string_,
///     Current weather temperature
///   }
///   \table_row3{   <b>`Weather.Location`</b>,
///                  \anchor Weather_Location
///                  _string_,
///     City/town which the above two items are for
///   }
///   \table_row3{   <b>`Weather.fanartcode`</b>,
///                  \anchor Weather_fanartcode
///                  _string_,
///     Current weather fanartcode.
///   }
///   \table_row3{   <b>`Weather.plugin`</b>,
///                  \anchor Weather_plugin
///                  _string_,
///     Current weather plugin.
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}
const infomap weather[] =        {{ "isfetched",        WEATHER_IS_FETCHED },
                                  { "conditions",       WEATHER_CONDITIONS },         // labels from here
                                  { "temperature",      WEATHER_TEMPERATURE },
                                  { "location",         WEATHER_LOCATION },
                                  { "fanartcode",       WEATHER_FANART_CODE },
                                  { "plugin",           WEATHER_PLUGIN }};

/// \page modules__General__List_of_gui_access
/// \section modules__General__List_of_gui_access_System System
/// @{
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`System.AlarmLessOrEqual(alarmname\,seconds)`</b>,
///                  \anchor System_AlarmLessOrEqual
///                  _boolean_,
///     Returns true if the alarm with `alarmname` has less or equal to `seconds`
///     left. Standard use would be system.alarmlessorequal(shutdowntimer\,119)\,
///     which would return true when the shutdowntimer has less then 2 minutes
///     left.
///   }
///   \table_row3{   <b>`System.HasNetwork`</b>,
///                  \anchor System_HasNetwork
///                  _boolean_,
///     Returns true if the ethernet cable is plugged in.
///   }
///   \table_row3{   <b>`System.HasMediadvd`</b>,
///                  \anchor System_HasMediadvd
///                  _boolean_,
///     Returns true if there is a CD or DVD in the DVD-ROM drive.
///   }
///   \table_row3{   <b>`System.DVDReady`</b>,
///                  \anchor System_DVDReady
///                  _boolean_,
///     Returns true if disc is ready to use.
///   }
///   \table_row3{   <b>`System.TrayOpen`</b>,
///                  \anchor System_TrayOpen
///                  _boolean_,
///     Returns true if discs tray is open
///   }
///   \table_row3{   <b>`System.HasLocks`</b>,
///                  \anchor System_HasLocks
///                  _boolean_,
///     Todo
///   }
///   \table_row3{   <b>`System.IsMaster`</b>,
///                  \anchor System_IsMaster
///                  _boolean_,
///     Todo
///   }
///   \table_row3{   <b>`System.ShowExitButton`</b>,
///                  \anchor System_ShowExitButton
///                  _boolean_,
///     Todo
///   }
///   \table_row3{   <b>`System.DPMSActive`</b>,
///                  \anchor System_DPMSActive
///                  _boolean_,
///     Todo
///   }
///   \table_row3{   <b>`System.IdleTime(time)`</b>,
///                  \anchor System_IdleTime
///                  _boolean_,
///     Returns true if Kodi has had no input for ?time? amount of seconds.
///   }
///   \table_row3{   <b>`System.IsStandalone`</b>,
///                  \anchor System_IsStandalone
///                  _boolean_,
///     Returns true if Kodi is running in standalone mode.
///   }
///   \table_row3{   <b>`System.IsFullscreen`</b>,
///                  \anchor System_IsFullscreen
///                  _boolean_,
///     Returns true if Kodi is running fullscreen.
///   }
///   \table_row3{   <b>`System.LoggedOn`</b>,
///                  \anchor System_LoggedOn
///                  _boolean_,
///     Returns true if a user is currently logged on under a profile
///   }
///   \table_row3{   <b>`System.HasLoginScreen`</b>,
///                  \anchor System_HasLoginScreen
///                  _boolean_,
///     Returns true if the profile login screen is enabled
///   }
///   \table_row3{   <b>`System.HasPVR`</b>,
///                  \anchor System_HasPVR
///                  _boolean_,
///     Returns true if PVR is supported from Kodi
///     \note normally always true
///   }
///   \table_row3{   <b>`System.HasADSP`</b>,
///                  \anchor System_HasADSP
///                  _boolean_,
///     Returns true if ADSP is supported from Kodi
///     \note normally always true
///   }
///   \table_row3{   <b>`System.HasCMS`</b>,
///                  \anchor System_HasCMS
///                  _boolean_,
///     Returns true if colour management is supported from Kodi
///     \note currently only supported for OpenGL
///   }
///   \table_row3{   <b>`System.HasModalDialog`</b>,
///                  \anchor System_HasModalDialog
///                  _boolean_,
///     Returns true true if a modal dialog is visible
///   }
///   \table_row3{   <b>`System.Time(startTime\,endTime)`</b>,
///                  \anchor System_Time
///                  _boolean_,
///     Returns true if the current system time is >= startTime and < endTime.
///     endTime is optional. Time must be specified in the format HH:mm\, using
///     a 24 hour clock.
///   }
///   \table_row3{   <b>`System.Date(startDate\,endDate)`</b>,
///                  \anchor System_Date
///                  _boolean_,
///     Returns true if the current system date is >= startDate and < endDate.
///     endDate is optional. Date must be specified in the format MM-DD.
///   }
///   \table_row3{   <b>`System.Platform.Linux`</b>,
///                  \anchor System_PlatformLinux
///                  _boolean_,
///     Returns true if Kodi is running on a linux/unix based computer.
///   }
///   \table_row3{   <b>`System.Platform.Linux.RaspberryPi`</b>,
///                  \anchor System_PlatformLinuxRaspberryPi
///                  _boolean_,
///     Returns true if Kodi is running on a Raspberry Pi.
///   }
///   \table_row3{   <b>`System.Platform.Windows`</b>,
///                  \anchor System_PlatformWindows
///                  _boolean_,
///     Returns true if Kodi is running on a windows based computer.
///   }
///   \table_row3{   <b>`System.Platform.OSX`</b>,
///                  \anchor System_PlatformOSX
///                  _boolean_,
///     Returns true if Kodi is running on an OSX based computer.
///   }
///   \table_row3{   <b>`System.Platform.IOS`</b>,
///                  \anchor System_PlatformIOS
///                  _boolean_,
///     Returns true if Kodi is running on an IOS device.
///   }
///   \table_row3{   <b>`System.Platform.Darwin`</b>,
///                  \anchor System_PlatformDarwin
///                  _boolean_,
///     Returns true if Kodi is running on an OSX or IOS system.
///   }
///   \table_row3{   <b>`System.Platform.ATV2`</b>,
///                  \anchor System_PlatformATV2
///                  _boolean_,
///     Returns true if Kodi is running on an atv2.
///   }
///   \table_row3{   <b>`System.Platform.Android`</b>,
///                  \anchor System_PlatformAndroid
///                  _boolean_,
///     Returns true if Kodi is running on an android device.
///   }
///   \table_row3{   <b>`System.CanPowerDown`</b>,
///                  \anchor System_CanPowerDown
///                  _boolean_,
///     Returns true if Kodi can powerdown the system.
///   }
///   \table_row3{   <b>`System.CanSuspend`</b>,
///                  \anchor System_CanSuspend
///                  _boolean_,
///     Returns true if Kodi can suspend the system.
///   }
///   \table_row3{   <b>`System.CanHibernate`</b>,
///                  \anchor System_CanHibernate
///                  _boolean_,
///     Returns true if Kodi can hibernate the system.
///   }
///   \table_row3{   <b>`System.HasHiddenInput`</b>,
///                  \anchor System_HasHiddenInput
///                  _boolean_,
///     Return true when to osd keyboard/numeric dialog requests a
///     password/pincode.
///   }
///   \table_row3{   <b>`System.CanReboot`</b>,
///                  \anchor System_CanReboot
///                  _boolean_,
///     Returns true if Kodi can reboot the system.
///   }
///   \table_row3{   <b>`System.ScreenSaverActive`</b>,
///                  \anchor System_ScreenSaverActive
///                  _boolean_,
///     Returns true if ScreenSaver is active.
///   }
///   \table_row3{   <b>`System.IsInhibit`</b>,
///                  \anchor System_IsInhibit
///                  _boolean_,
///     Returns true when shutdown on idle is disabled.
///   }
///   \table_row3{   <b>`System.HasShutdown`</b>,
///                  \anchor System_HasShutdown
///                  _boolean_,
///     Returns true when shutdown on idle is enabled.
///   }
///   \table_row3{   <b>`System.Time`</b>,
///                  \anchor System_Time
///                  _string_,
///     Current time
///   }
///   \table_row3{   <b>`System.Time(format)`</b>,
///                  \anchor System_Time_format
///                  _string_,
///     Shows hours (hh)\, minutes (mm) or seconds (ss). When 12 hour clock is
///     used (xx) will return AM/PM. Also supported: (hh:mm)\, (mm:ss)\,
///     (hh:mm:ss)\, (hh:mm:ss). (xx) option added after dharma
///   }
///   \table_row3{   <b>`System.Date`</b>,
///                  \anchor System_Date
///                  _string_,
///     Current date
///   }
///   \table_row3{   <b>`System.Date(format)`</b>,
///                  \anchor System_Date_format
///                  _string_,
///     Show current date using format\, available markings: d (day of month
///     1-31)\, dd (day of month 01-31)\, ddd (short day of the week Mon-Sun)\,
///     DDD (long day of the week Monday-Sunday)\, m (month 1-12)\, mm (month
///     01-12)\, mmm (short month name Jan-Dec)\, MMM (long month name January -
///     December)\, yy (2-digit year)\, yyyy (4-digit year). Added after dharma.
///   }
///   \table_row3{   <b>`System.AlarmPos`</b>,
///                  \anchor System_AlarmPos
///                  _string_,
///     Shutdown Timer position
///   }
///   \table_row3{   <b>`System.BatteryLevel`</b>,
///                  \anchor System_BatteryLevel
///                  _string_,
///     Returns the remaining battery level in range 0-100
///   }
///   \table_row3{   <b>`System.FreeSpace`</b>,
///                  \anchor System_FreeSpace
///                  _string_,
///     Total Freespace on the drive
///   }
///   \table_row3{   <b>`System.UsedSpace`</b>,
///                  \anchor System_UsedSpace
///                  _string_,
///     Total Usedspace on the drive
///   }
///   \table_row3{   <b>`System.TotalSpace`</b>,
///                  \anchor System_TotalSpace
///                  _string_,
///     Totalspace on the drive
///   }
///   \table_row3{   <b>`System.UsedSpacePercent`</b>,
///                  \anchor System_UsedSpacePercent
///                  _string_,
///     Total Usedspace Percent on the drive
///   }
///   \table_row3{   <b>`System.FreeSpacePercent`</b>,
///                  \anchor System_FreeSpacePercent
///                  _string_,
///     Total Freespace Percent on the drive
///   }
///   \table_row3{   <b>`System.CPUTemperature`</b>,
///                  \anchor System_CPUTemperature
///                  _string_,
///     Current CPU temperature
///   }
///   \table_row3{   <b>`System.CpuUsage`</b>,
///                  \anchor System_CpuUsage
///                  _string_,
///     Displays the cpu usage for each individual cpu core.
///   }
///   \table_row3{   <b>`System.GPUTemperature`</b>,
///                  \anchor System_GPUTemperature
///                  _string_,
///     Current GPU temperature
///   }
///   \table_row3{   <b>`System.FanSpeed`</b>,
///                  \anchor System_FanSpeed
///                  _string_,
///     Current fan speed
///   }
///   \table_row3{   <b>`System.BuildVersion`</b>,
///                  \anchor System_BuildVersion
///                  _string_,
///     Version of build
///   }
///   \table_row3{   <b>`System.BuildVersionShort`</b>,
///                  \anchor System_BuildVersionShort
///                  _string_,
///     Shorter string with version of build
///   }
///   \table_row3{   <b>`System.BuildDate`</b>,
///                  \anchor System_BuildDate
///                  _string_,
///     Date of build
///   }
///   \table_row3{   <b>`System.FriendlyName`</b>,
///                  \anchor System_FriendlyName
///                  _string_,
///     Returns the Kodi instance name. It will auto append (%hostname%) in case
///     the device name was not changed. eg. "Kodi (htpc)"
///   }
///   \table_row3{   <b>`System.FPS`</b>,
///                  \anchor System_FPS
///                  _string_,
///     Current rendering speed (frames per second)
///   }
///   \table_row3{   <b>`System.FreeMemory`</b>,
///                  \anchor System_FreeMemory
///                  _string_,
///     Amount of free memory in Mb
///   }
///   \table_row3{   <b>`System.ScreenMode`</b>,
///                  \anchor System_ScreenMode
///                  _string_,
///     Screenmode (eg windowed / fullscreen)
///   }
///   \table_row3{   <b>`System.ScreenWidth`</b>,
///                  \anchor System_ScreenWidth
///                  _string_,
///     Width of screen in pixels
///   }
///   \table_row3{   <b>`System.ScreenHeight`</b>,
///                  \anchor System_ScreenHeight
///                  _string_,
///     Height of screen in pixels
///   }
///   \table_row3{   <b>`System.StartupWindow`</b>,
///                  \anchor System_StartupWindow
///                  _string_,
///     The Window Kodi will load on startup
///   }
///   \table_row3{   <b>`System.CurrentWindow`</b>,
///                  \anchor System_CurrentWindow
///                  _string_,
///     Current Window we are in
///   }
///   \table_row3{   <b>`System.CurrentControl`</b>,
///                  \anchor System_CurrentControl
///                  _string_,
///     Current focused control
///   }
///   \table_row3{   <b>`System.CurrentControlId`</b>,
///                  \anchor System_CurrentControlId
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`System.DVDLabel`</b>,
///                  \anchor System_DVDLabel
///                  _string_,
///     Label of the disk in the DVD-ROM drive
///   }
///   \table_row3{   <b>`System.KernelVersion`</b>,
///                  \anchor System_KernelVersion
///                  _string_,
///     System kernel version
///   }
///   \table_row3{   <b>`System.OSVersionInfo`</b>,
///                  \anchor System_OSVersionInfo
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`System.Uptime`</b>,
///                  \anchor System_Uptime
///                  _string_,
///     System current uptime
///   }
///   \table_row3{   <b>`System.TotalUptime`</b>,
///                  \anchor System_TotalUptime
///                  _string_,
///     System total uptime
///   }
///   \table_row3{   <b>`System.CpuFrequency`</b>,
///                  \anchor System_CpuFrequency
///                  _string_,
///     System cpu frequency
///   }
///   \table_row3{   <b>`System.ScreenResolution`</b>,
///                  \anchor System_ScreenResolution
///                  _string_,
///     Screen resolution
///   }
///   \table_row3{   <b>`System.VideoEncoderInfo`</b>,
///                  \anchor System_VideoEncoderInfo
///                  _string_,
///     Video encoder info
///   }
///   \table_row3{   <b>`System.InternetState`</b>,
///                  \anchor System_InternetState
///                  _string_,
///     Will return the internet state\, connected or not connected and for
///     Conditional use: Connected->TRUE\, not Connected->FALSE\, do not use
///     to check status in a pythonscript since it is threaded.
///   }
///   \table_row3{   <b>`System.Language`</b>,
///                  \anchor System_Language
///                  _string_,
///     Shows the current language
///   }
///   \table_row3{   <b>`System.ProfileName`</b>,
///                  \anchor System_ProfileName
///                  _string_,
///     Shows the User name of the currently logged in Kodi user
///   }
///   \table_row3{   <b>`System.ProfileThumb`</b>,
///                  \anchor System_ProfileThumb
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`System.ProfileCount`</b>,
///                  \anchor System_ProfileCount
///                  _string_,
///     Shows the number of defined profiles
///   }
///   \table_row3{   <b>`System.ProfileAutoLogin`</b>,
///                  \anchor System_ProfileAutoLogin
///                  _string_,
///     The profile Kodi will auto login to
///   }
///   \table_row3{   <b>`System.StereoscopicMode`</b>,
///                  \anchor System_StereoscopicMode
///                  _string_,
///     The prefered stereoscopic mode (settings > video > playback)
///   }
///   \table_row3{   <b>`System.TemperatureUnits`</b>,
///                  \anchor System_TemperatureUnits
///                  _string_,
///     Shows Celsius or Fahrenheit symbol
///   }
///   \table_row3{   <b>`System.Progressbar`</b>,
///                  \anchor System_Progressbar
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`System.GetBool(boolean)`</b>,
///                  \anchor System_GetBool
///                  _string_,
///     Returns the value of any standard system boolean setting. Will not work
///     with settings in advancedsettings.xml
///   }
///   \table_row3{   <b>`System.AddonTitle(id)`</b>,
///                  \anchor System_AddonTitle
///                  _string_,
///     Returns the title of the addon with the given id
///   }
///   \table_row3{   <b>`System.AddonVersion(id)`</b>,
///                  \anchor System_AddonVersion
///                  _string_,
///     Returns the version of the addon with the given id
///   }
/// \table_end
/// @}
const infomap system_labels[] =  {{ "hasnetwork",       SYSTEM_ETHERNET_LINK_ACTIVE },
                                  { "hasmediadvd",      SYSTEM_MEDIA_DVD },
                                  { "dvdready",         SYSTEM_DVDREADY },
                                  { "trayopen",         SYSTEM_TRAYOPEN },
                                  { "haslocks",         SYSTEM_HASLOCKS },
                                  { "hashiddeninput",   SYSTEM_HAS_INPUT_HIDDEN },
                                  { "hasloginscreen",   SYSTEM_HAS_LOGINSCREEN },
                                  { "hasmodaldialog",   SYSTEM_HAS_MODAL_DIALOG },
                                  { "ismaster",         SYSTEM_ISMASTER },
                                  { "isfullscreen",     SYSTEM_ISFULLSCREEN },
                                  { "isstandalone",     SYSTEM_ISSTANDALONE },
                                  { "loggedon",         SYSTEM_LOGGEDON },
                                  { "showexitbutton",   SYSTEM_SHOW_EXIT_BUTTON },
                                  { "canpowerdown",     SYSTEM_CAN_POWERDOWN },
                                  { "cansuspend",       SYSTEM_CAN_SUSPEND },
                                  { "canhibernate",     SYSTEM_CAN_HIBERNATE },
                                  { "canreboot",        SYSTEM_CAN_REBOOT },
                                  { "screensaveractive",SYSTEM_SCREENSAVER_ACTIVE },
                                  { "dpmsactive",       SYSTEM_DPMS_ACTIVE },
                                  { "cputemperature",   SYSTEM_CPU_TEMPERATURE },     // labels from here
                                  { "cpuusage",         SYSTEM_CPU_USAGE },
                                  { "gputemperature",   SYSTEM_GPU_TEMPERATURE },
                                  { "fanspeed",         SYSTEM_FAN_SPEED },
                                  { "freespace",        SYSTEM_FREE_SPACE },
                                  { "usedspace",        SYSTEM_USED_SPACE },
                                  { "totalspace",       SYSTEM_TOTAL_SPACE },
                                  { "usedspacepercent", SYSTEM_USED_SPACE_PERCENT },
                                  { "freespacepercent", SYSTEM_FREE_SPACE_PERCENT },
                                  { "buildversion",     SYSTEM_BUILD_VERSION },
                                  { "buildversionshort",SYSTEM_BUILD_VERSION_SHORT },
                                  { "builddate",        SYSTEM_BUILD_DATE },
                                  { "fps",              SYSTEM_FPS },
                                  { "freememory",       SYSTEM_FREE_MEMORY },
                                  { "language",         SYSTEM_LANGUAGE },
                                  { "temperatureunits", SYSTEM_TEMPERATURE_UNITS },
                                  { "screenmode",       SYSTEM_SCREEN_MODE },
                                  { "screenwidth",      SYSTEM_SCREEN_WIDTH },
                                  { "screenheight",     SYSTEM_SCREEN_HEIGHT },
                                  { "currentwindow",    SYSTEM_CURRENT_WINDOW },
                                  { "currentcontrol",   SYSTEM_CURRENT_CONTROL },
                                  { "currentcontrolid", SYSTEM_CURRENT_CONTROL_ID },
                                  { "dvdlabel",         SYSTEM_DVD_LABEL },
                                  { "internetstate",    SYSTEM_INTERNET_STATE },
                                  { "osversioninfo",    SYSTEM_OS_VERSION_INFO },
                                  { "kernelversion",    SYSTEM_OS_VERSION_INFO }, // old, not correct name
                                  { "uptime",           SYSTEM_UPTIME },
                                  { "totaluptime",      SYSTEM_TOTALUPTIME },
                                  { "cpufrequency",     SYSTEM_CPUFREQUENCY },
                                  { "screenresolution", SYSTEM_SCREEN_RESOLUTION },
                                  { "videoencoderinfo", SYSTEM_VIDEO_ENCODER_INFO },
                                  { "profilename",      SYSTEM_PROFILENAME },
                                  { "profilethumb",     SYSTEM_PROFILETHUMB },
                                  { "profilecount",     SYSTEM_PROFILECOUNT },
                                  { "profileautologin", SYSTEM_PROFILEAUTOLOGIN },
                                  { "progressbar",      SYSTEM_PROGRESS_BAR },
                                  { "batterylevel",     SYSTEM_BATTERY_LEVEL },
                                  { "friendlyname",     SYSTEM_FRIENDLY_NAME },
                                  { "alarmpos",         SYSTEM_ALARM_POS },
                                  { "isinhibit",        SYSTEM_ISINHIBIT },
                                  { "hasshutdown",      SYSTEM_HAS_SHUTDOWN },
                                  { "haspvr",           SYSTEM_HAS_PVR },
                                  { "startupwindow",    SYSTEM_STARTUP_WINDOW },
                                  { "stereoscopicmode", SYSTEM_STEREOSCOPIC_MODE },
                                  { "hasadsp",          SYSTEM_HAS_ADSP },
                                  { "hascms",           SYSTEM_HAS_CMS }};

/// \page modules__General__List_of_gui_access
/// @{
/// \table_start
///   \table_row3{   <b>`System.HasAddon(id)`</b>,
///                  \anchor System_HasAddon
///                  _boolean_,
///     Returns true if the specified addon is installed on the system.
///   }
///   \table_row3{   <b>`System.HasCoreId(id)`</b>,
///                  \anchor System_HasCoreId
///                  _boolean_,
///     Todo
///   }
///   \table_row3{   <b>`System.HasAlarm(alarm)`</b>,
///                  \anchor System_HasAlarm
///                  _boolean_,
///     Returns true if the system has the ?alarm? alarm set.
///   }
///   \table_row3{   <b>`System.CoreUsage(id)`</b>,
///                  \anchor System_CoreUsage
///                  _string_,
///     Displays the usage of the cpu core with the given 'id'
///   }
///   \table_row3{   <b>`System.Setting(hidewatched)`</b>,
///                  \anchor System_Setting
///                  _boolean_,
///     Returns true if 'hide watched items' is selected.
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}
const infomap system_param[] =   {{ "hasalarm",         SYSTEM_HAS_ALARM },
                                  { "hascoreid",        SYSTEM_HAS_CORE_ID },
                                  { "setting",          SYSTEM_SETTING },
                                  { "hasaddon",         SYSTEM_HAS_ADDON },
                                  { "coreusage",        SYSTEM_GET_CORE_USAGE }};

/// \page modules__General__List_of_gui_access
/// \section modules__General__List_of_gui_access_Network Network
/// @{
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Network.IsDHCP`</b>,
///                  \anchor Network_IsDHCP
///                  _boolean_,
///     Network type is DHCP or FIXED
///   }
///   \table_row3{   <b>`Network.IPAddress`</b>,
///                  \anchor Network_IPAddress
///                  _string_,
///     The system's IP Address (formatted as IP: <ipaddress>)
///   }
///   \table_row3{   <b>`Network.LinkState`</b>,
///                  \anchor Network_LinkState
///                  _string_,
///     Network linkstate e.g. 10mbit/100mbit etc.
///   }
///   \table_row3{   <b>`Network.MacAddress`</b>,
///                  \anchor Network_MacAddress
///                  _string_,
///     The system's mac address
///   }
///   \table_row3{   <b>`Network.SubnetMask`</b>,
///                  \anchor Network_SubnetMask
///                  _string_,
///     Network subnet mask
///   }
///   \table_row3{   <b>`Network.GatewayAddress`</b>,
///                  \anchor Network_GatewayAddress
///                  _string_,
///     Network gateway address
///   }
///   \table_row3{   <b>`Network.DNS1Address`</b>,
///                  \anchor Network_DNS1Address
///                  _string_,
///     Network dns 1 address
///   }
///   \table_row3{   <b>`Network.DNS2Address`</b>,
///                  \anchor Network_DNS2Address
///                  _string_,
///     Network dns 2 address
///   }
///   \table_row3{   <b>`Network.DHCPAddress`</b>,
///                  \anchor Network_DHCPAddress
///                  _string_,
///     DHCP ip address
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}
const infomap network_labels[] = {{ "isdhcp",            NETWORK_IS_DHCP },
                                  { "ipaddress",         NETWORK_IP_ADDRESS }, //labels from here
                                  { "linkstate",         NETWORK_LINK_STATE },
                                  { "macaddress",        NETWORK_MAC_ADDRESS },
                                  { "subnetmask",        NETWORK_SUBNET_MASK },
                                  { "gatewayaddress",    NETWORK_GATEWAY_ADDRESS },
                                  { "dns1address",       NETWORK_DNS1_ADDRESS },
                                  { "dns2address",       NETWORK_DNS2_ADDRESS },
                                  { "dhcpaddress",       NETWORK_DHCP_ADDRESS }};

/// \page modules__General__List_of_gui_access
/// \section modules__General__List_of_gui_access_musicpartymode Music party mode
/// @{
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`MusicPartyMode.Enabled`</b>,
///                  \anchor MusicPartyMode_Enabled
///                  _boolean_,
///     Returns true if Party Mode is enabled
///   }
///   \table_row3{   <b>`MusicPartyMode.SongsPlayed`</b>,
///                  \anchor MusicPartyMode_SongsPlayed
///                  _string_,
///     Number of songs played during Party Mode
///   }
///   \table_row3{   <b>`MusicPartyMode.MatchingSongs`</b>,
///                  \anchor MusicPartyMode_MatchingSongs
///                  _string_,
///     Number of songs available to Party Mode
///   }
///   \table_row3{   <b>`MusicPartyMode.MatchingSongsPicked`</b>,
///                  \anchor MusicPartyMode_MatchingSongsPicked
///                  _string_,
///     Number of songs picked already for Party Mode
///   }
///   \table_row3{   <b>`MusicPartyMode.MatchingSongsLeft`</b>,
///                  \anchor MusicPartyMode_MatchingSongsLeft
///                  _string_,
///     Number of songs left to be picked from for Party Mode
///   }
///   \table_row3{   <b>`MusicPartyMode.RelaxedSongsPicked`</b>,
///                  \anchor MusicPartyMode_RelaxedSongsPicked
///                  _string_,
///     Not currently used
///   }
///   \table_row3{   <b>`MusicPartyMode.RandomSongsPicked`</b>,
///                  \anchor MusicPartyMode_RandomSongsPicked
///                  _string_,
///     Number of unique random songs picked during Party Mode
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}
const infomap musicpartymode[] = {{ "enabled",           MUSICPM_ENABLED },
                                  { "songsplayed",       MUSICPM_SONGSPLAYED },
                                  { "matchingsongs",     MUSICPM_MATCHINGSONGS },
                                  { "matchingsongspicked", MUSICPM_MATCHINGSONGSPICKED },
                                  { "matchingsongsleft", MUSICPM_MATCHINGSONGSLEFT },
                                  { "relaxedsongspicked",MUSICPM_RELAXEDSONGSPICKED },
                                  { "randomsongspicked", MUSICPM_RANDOMSONGSPICKED }};

/// \page modules__General__List_of_gui_access
/// \section modules__General__List_of_gui_access_MusicPlayer Music player
/// @{
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`MusicPlayer.Offset(number).Exists`</b>,
///                  \anchor MusicPlayer_Offset
///                  _boolean_,
///     Returns true if the music players playlist has a song queued in
///     position (number).
///   }
///   \table_row3{   <b>`MusicPlayer.Title`</b>,
///                  \anchor MusicPlayer_Title
///                  _string_,
///     Title of the currently playing song\, also available are
///     "MusicPlayer.offset(number).Title" offset is relative to the current
///     playing item and "MusicPlayer.Position(number).Title" position is relative
///     to the start of the playlist
///   }
///   \table_row3{   <b>`MusicPlayer.Album`</b>,
///                  \anchor MusicPlayer_Album
///                  _string_,
///     Album from which the current song is from\, also available are
///     "MusicPlayer.offset(number).Album" offset is relative to the current
///     playing item and "MusicPlayer.Position(number).Album" position is relative
///     to the start of the playlist
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Album_Mood)`</b>,
///                  \anchor MusicPlayer_Property_Album_Mood
///                  _string_,
///     Shows the moods of the currently playing Album
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Album_Style)`</b>,
///                  \anchor MusicPlayer_Property_Album_Style
///                  _string_,
///     Shows the styles of the currently playing Album
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Album_Theme)`</b>,
///                  \anchor MusicPlayer_Property_Album_Theme
///                  _string_,
///     Shows the themes of the currently playing Album
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Album_Type)`</b>,
///                  \anchor MusicPlayer_Property_Album_Type
///                  _string_,
///     Shows the Album Type (e.g. compilation\, enhanced\, explicit lyrics) of the
///     currently playing Album
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Album_Label)`</b>,
///                  \anchor MusicPlayer_Property_Album_Label
///                  _string_,
///     Shows the record label of the currently playing Album
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Album_Description)`</b>,
///                  \anchor MusicPlayer_Property_Album_Description
///                  _string_,
///     Shows a review of the currently playing Album
///   }
///   \table_row3{   <b>`MusicPlayer.Artist`</b>,
///                  \anchor MusicPlayer_Artist
///                  _string_,
///     Artist(s) of current song\, also available are
///     "MusicPlayer.offset(number).Artist" offset is relative to the current
///     playing item and "MusicPlayer.Position(number).Artist" position is
///     relative to the start of the playlist
///   }
///   \table_row3{   <b>`MusicPlayer.AlbumArtist`</b>,
///                  \anchor MusicPlayer_AlbumArtist
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`MusicPlayer.Cover`</b>,
///                  \anchor MusicPlayer_Cover
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Born)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Born
///                  _string_,
///     Date of Birth of the currently playing Artist
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Died)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Died
///                  _string_,
///     Date of Death of the currently playing Artist
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Formed)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Formed
///                  _string_,
///     Formation date of the currently playing Artist/Band
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Disbanded)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Disbanded
///                  _string_,
///     Disbanding date of the currently playing Artist/Band
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_YearsActive)`</b>,
///                  \anchor MusicPlayer_Property_Artist_YearsActive
///                  _string_,
///     Years the currently Playing artist has been active
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Instrument)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Instrument
///                  _string_,
///     Instruments played by the currently playing artist
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Description)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Description
///                  _string_,
///     Shows a biography of the currently playing artist
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Mood)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Mood
///                  _string_,
///     Shows the moods of the currently playing artist
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Style)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Style
///                  _string_,
///     Shows the styles of the currently playing artist
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Genre)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Genre
///                  _string_,
///     Shows the genre of the currently playing artist
///   }
///   \table_row3{   <b>`MusicPlayer.Genre`</b>,
///                  \anchor MusicPlayer_Genre
///                  _string_,
///     Genre(s) of current song\, also available are
///     "MusicPlayer.offset(number).Genre" offset is relative to the current
///     playing item and "MusicPlayer.Position(number).Genre" position is
///     relative to the start of the playlist
///   }
///   \table_row3{   <b>`MusicPlayer.Lyrics`</b>,
///                  \anchor MusicPlayer_Lyrics
///                  _string_,
///     Lyrics of current song stored in ID tag info
///   }
///   \table_row3{   <b>`MusicPlayer.Year`</b>,
///                  \anchor MusicPlayer_Year
///                  _string_,
///     Year of release of current song\, also available are
///     "MusicPlayer.offset(number).Year" offset is relative to the current
///     playing item and "MusicPlayer.Position(number).Year" position is
///     relative to the start of the playlist
///   }
///   \table_row3{   <b>`MusicPlayer.Rating`</b>,
///                  \anchor MusicPlayer_Rating
///                  _string_,
///     Numeric Rating of current song\, also available are
///     "MusicPlayer.offset(number).Rating" offset is relative to the current
///     playing item and "MusicPlayer.Position(number).Rating" position is
///     relative to the start of the playlist
///   }
///   \table_row3{   <b>`MusicPlayer.RatingAndVotes`</b>,
///                  \anchor MusicPlayer_RatingAndVotes
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`MusicPlayer.UserRating`</b>,
///                  \anchor MusicPlayer_UserRating
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`MusicPlayer.Votes`</b>,
///                  \anchor MusicPlayer_Votes
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`MusicPlayer.DiscNumber`</b>,
///                  \anchor MusicPlayer_DiscNumber
///                  _string_,
///     Disc Number of current song stored in ID tag info\, also available are
///     "MusicPlayer.offset(number).DiscNumber" offset is relative to the
///     current playing item and "MusicPlayer.Position(number).DiscNumber"
///     position is relative to the start of the playlist
///   }
///   \table_row3{   <b>`MusicPlayer.Comment`</b>,
///                  \anchor MusicPlayer_Comment
///                  _string_,
///     Comment of current song stored in ID tag info\, also available are
///     "MusicPlayer.offset(number).Comment" offset is relative to the current
///     playing item and "MusicPlayer.Position(number).Comment" position is
///     relative to the start of the playlist
///   }
///   \table_row3{   <b>`MusicPlayer.Mood`</b>,
///                  \anchor MusicPlayer_Mood
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`MusicPlayer.PlaylistPlaying`</b>,
///                  \anchor MusicPlayer_PlaylistPlaying
///                  _boolean_,
///     Todo
///   }
///   \table_row3{   <b>`MusicPlayer.Exists`</b>,
///                  \anchor MusicPlayer_Exists
///                  _boolean_,
///     Todo
///   }
///   \table_row3{   <b>`MusicPlayer.HasPrevious`</b>,
///                  \anchor MusicPlayer_HasPrevious
///                  _boolean_,
///     Returns true if the music player has a a Previous Song in the Playlist .
///   }
///   \table_row3{   <b>`MusicPlayer.HasNext`</b>,
///                  \anchor MusicPlayer_HasNext
///                  _boolean_,
///     Returns true if the music player has a next song queued in the Playlist.
///   }
///   \table_row3{   <b>`MusicPlayer.PlayCount`</b>,
///                  \anchor MusicPlayer_PlayCount
///                  _integer_,
///     Todo
///   }
///   \table_row3{   <b>`MusicPlayer.LastPlayed`</b>,
///                  \anchor MusicPlayer_LastPlayed
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`MusicPlayer.TrackNumber`</b>,
///                  \anchor MusicPlayer_TrackNumber
///                  _string_,
///     Track number of current song\, also available are
///     "MusicPlayer.offset(number).TrackNumber" offset is relative to the
///     current playing item and "MusicPlayer.Position(number).TrackNumber"
///     position is relative to the start of the playlist
///   }
///   \table_row3{   <b>`MusicPlayer.Duration`</b>,
///                  \anchor MusicPlayer_Duration
///                  _string_,
///     Duration of current song\, also available are
///     "MusicPlayer.offset(number).Duration" offset is relative to the
///     current playing item and "MusicPlayer.Position(number).Duration"
///     position is relative to the start of the playlist
///   }
///   \table_row3{   <b>`MusicPlayer.BitRate`</b>,
///                  \anchor MusicPlayer_BitRate
///                  _string_,
///     Bitrate of current song
///   }
///   \table_row3{   <b>`MusicPlayer.Channels`</b>,
///                  \anchor MusicPlayer_Channels
///                  _string_,
///     Number of channels of current song
///   }
///   \table_row3{   <b>`MusicPlayer.BitsPerSample`</b>,
///                  \anchor MusicPlayer_BitsPerSample
///                  _string_,
///     Number of bits per sample of current song
///   }
///   \table_row3{   <b>`MusicPlayer.SampleRate`</b>,
///                  \anchor MusicPlayer_SampleRate
///                  _string_,
///     Samplerate of current song
///   }
///   \table_row3{   <b>`MusicPlayer.Codec`</b>,
///                  \anchor MusicPlayer_Codec
///                  _string_,
///     Codec of current song
///   }
///   \table_row3{   <b>`MusicPlayer.PlaylistPosition`</b>,
///                  \anchor MusicPlayer_PlaylistPosition
///                  _string_,
///     Position of the current song in the current music playlist
///   }
///   \table_row3{   <b>`MusicPlayer.PlaylistLength`</b>,
///                  \anchor MusicPlayer_PlaylistLength
///                  _string_,
///     Total size of the current music playlist
///   }
///   \table_row3{   <b>`MusicPlayer.ChannelName`</b>,g
///                  \anchor MusicPlayer_ChannelName
///                  _string_,
///     Channel name of the radio programme that's currently playing (PVR).
///   }
///   \table_row3{   <b>`MusicPlayer.ChannelNumber`</b>,
///                  \anchor MusicPlayer_ChannelNumber
///                  _string_,
///     Channel number of the radio programme that's currently playing (PVR).
///   }
///   \table_row3{   <b>`MusicPlayer.SubChannelNumber`</b>,
///                  \anchor MusicPlayer_SubChannelNumber
///                  _string_,
///     Subchannel number of the radio channel that's currently playing (PVR).
///   }
///   \table_row3{   <b>`MusicPlayer.ChannelNumberLabel`</b>,
///                  \anchor MusicPlayer_ChannelNumberLabel
///                  _string_,
///     Channel and subchannel number of the radio channel that's currently
///     playing (PVR).
///   }
///   \table_row3{   <b>`MusicPlayer.ChannelGroup`</b>,
///                  \anchor MusicPlayer_ChannelGroup
///                  _string_,
///     Channel group of of the radio programme that's currently playing (PVR).
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}
const infomap musicplayer[] =    {{ "title",            MUSICPLAYER_TITLE },
                                  { "album",            MUSICPLAYER_ALBUM },
                                  { "artist",           MUSICPLAYER_ARTIST },
                                  { "albumartist",      MUSICPLAYER_ALBUM_ARTIST },
                                  { "year",             MUSICPLAYER_YEAR },
                                  { "genre",            MUSICPLAYER_GENRE },
                                  { "duration",         MUSICPLAYER_DURATION },
                                  { "tracknumber",      MUSICPLAYER_TRACK_NUMBER },
                                  { "cover",            MUSICPLAYER_COVER },
                                  { "bitrate",          MUSICPLAYER_BITRATE },
                                  { "playlistlength",   MUSICPLAYER_PLAYLISTLEN },
                                  { "playlistposition", MUSICPLAYER_PLAYLISTPOS },
                                  { "channels",         MUSICPLAYER_CHANNELS },
                                  { "bitspersample",    MUSICPLAYER_BITSPERSAMPLE },
                                  { "samplerate",       MUSICPLAYER_SAMPLERATE },
                                  { "codec",            MUSICPLAYER_CODEC },
                                  { "discnumber",       MUSICPLAYER_DISC_NUMBER },
                                  { "rating",           MUSICPLAYER_RATING },
                                  { "ratingandvotes",   MUSICPLAYER_RATING_AND_VOTES },
                                  { "userrating",       MUSICPLAYER_USER_RATING },
                                  { "votes",            MUSICPLAYER_VOTES },
                                  { "comment",          MUSICPLAYER_COMMENT },
                                  { "mood",             MUSICPLAYER_MOOD },
                                  { "lyrics",           MUSICPLAYER_LYRICS },
                                  { "playlistplaying",  MUSICPLAYER_PLAYLISTPLAYING },
                                  { "exists",           MUSICPLAYER_EXISTS },
                                  { "hasprevious",      MUSICPLAYER_HASPREVIOUS },
                                  { "hasnext",          MUSICPLAYER_HASNEXT },
                                  { "playcount",        MUSICPLAYER_PLAYCOUNT },
                                  { "lastplayed",       MUSICPLAYER_LASTPLAYED },
                                  { "channelname",      MUSICPLAYER_CHANNEL_NAME },
                                  { "channelnumber",    MUSICPLAYER_CHANNEL_NUMBER },
                                  { "subchannelnumber", MUSICPLAYER_SUB_CHANNEL_NUMBER },
                                  { "channelnumberlabel", MUSICPLAYER_CHANNEL_NUMBER_LBL },
                                  { "channelgroup",     MUSICPLAYER_CHANNEL_GROUP }
};

/// \page modules__General__List_of_gui_access
/// \section modules__General__List_of_gui_access_Videoplayer Video player
/// @{
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`VideoPlayer.UsingOverlays`</b>,
///                  \anchor VideoPlayer_UsingOverlays
///                  _boolean_,
///     Returns true if the video player is using the hardware overlays render
///     method. Useful\, as with hardware overlays you have no alpha blending to
///     the video image\, so shadows etc. need redoing\, or disabling.
///   }
///   \table_row3{   <b>`VideoPlayer.IsFullscreen`</b>,
///                  \anchor VideoPlayer_IsFullscreen
///                  _boolean_,
///     Returns true if the video player is in fullscreen mode.
///   }
///   \table_row3{   <b>`VideoPlayer.HasMenu`</b>,
///                  \anchor VideoPlayer_HasMenu
///                  _boolean_,
///     Returns true if the video player has a menu (ie is playing a DVD)
///   }
///   \table_row3{   <b>`VideoPlayer.HasInfo`</b>,
///                  \anchor VideoPlayer_HasInfo
///                  _boolean_,
///     Returns true if the current playing video has information from the
///     library or from a plugin (eg director/plot etc.)
///   }
///   \table_row3{   <b>`VideoPlayer.Content(parameter)`</b>,
///                  \anchor VideoPlayer_Content
///                  _boolean_,
///     Returns true if the current Video you are playing is contained in
///     corresponding Video Library sections.\n
///     The following values are accepted :
///     - files
///     - movies
///     - episodes
///     - musicvideos
///     - livetv
///   }
///   \table_row3{   <b>`VideoPlayer.HasSubtitles`</b>,
///                  \anchor VideoPlayer_HasSubtitles
///                  _boolean_,
///     Returns true if there are subtitles available for video.
///   }
///   \table_row3{   <b>`VideoPlayer.HasTeletext`</b>,
///                  \anchor VideoPlayer_HasTeletext
///                  _boolean_,
///     Returns true if teletext is usable on played TV channel
///   }
///   \table_row3{   <b>`VideoPlayer.IsStereoscopic`</b>,
///                  \anchor VideoPlayer_IsStereoscopic
///                  _boolean_,
///     Returns true when the currently playing video is a 3D (stereoscopic)
///     video
///   }
///   \table_row3{   <b>`VideoPlayer.SubtitlesEnabled`</b>,
///                  \anchor VideoPlayer_SubtitlesEnabled
///                  _boolean_,
///     Returns true if subtitles are turned on for video.
///   }
///   \table_row3{   <b>`VideoPlayer.HasEpg`</b>,
///                  \anchor VideoPlayer_HasEpg
///                  _boolean_,
///     Returns true when epg information is available for the currently playing
///     programme (PVR).
///   }
///   \table_row3{   <b>`VideoPlayer.CanResumeLiveTV`</b>,
///                  \anchor VideoPlayer_CanResumeLiveTV
///                  _boolean_,
///     Todo
///   }
///   \table_row3{   <b>`VideoPlayer.Title`</b>,
///                  \anchor VideoPlayer_Title
///                  _string_,
///     Title of currently playing video. If it's in the database it will return
///     the database title\, else the filename
///   }
///   \table_row3{   <b>`VideoPlayer.OriginalTitle`</b>,
///                  \anchor VideoPlayer_OriginalTitle
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`VideoPlayer.TVShowTitle`</b>,
///                  \anchor VideoPlayer_TVShowTitle
///                  _string_,
///     Title of currently playing episode's tvshow name
///   }
///   \table_row3{   <b>`VideoPlayer.Season`</b>,
///                  \anchor VideoPlayer_Season
///                  _string_,
///     Season number of the currently playing episode\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.Episode`</b>,
///                  \anchor VideoPlayer_Episode
///                  _string_,
///     Episode number of the currently playing episode
///   }
///   \table_row3{   <b>`VideoPlayer.Genre`</b>,
///                  \anchor VideoPlayer_Genre
///                  _string_,
///     Genre(s) of current movie\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.Director`</b>,
///                  \anchor VideoPlayer_Director
///                  _string_,
///     Director of current movie\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.Country`</b>,
///                  \anchor VideoPlayer_Country
///                  _string_,
///     Production country of current movie\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.Year`</b>,
///                  \anchor VideoPlayer_Year
///                  _string_,
///     Year of release of current movie\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.Cover`</b>,
///                  \anchor VideoPlayer_Cover
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`VideoPlayer.Rating`</b>,
///                  \anchor VideoPlayer_Rating
///                  _string_,
///     IMDb user rating of current movie\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.UserRating`</b>,
///                  \anchor VideoPlayer_UserRating
///                  _string_,
///     Shows the user rating of the currently playing item
///   }
///   \table_row3{   <b>`VideoPlayer.Votes`</b>,
///                  \anchor VideoPlayer_Votes
///                  _string_,
///     IMDb votes of current movie\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.RatingAndVotes`</b>,
///                  \anchor VideoPlayer_RatingAndVotes
///                  _string_,
///     IMDb user rating and votes of current movie\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.mpaa`</b>,
///                  \anchor VideoPlayer_mpaa
///                  _string_,
///     MPAA rating of current movie\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.IMDBNumber`</b>,
///                  \anchor VideoPlayer_IMDBNumber
///                  _string_,
///     The IMDB iD of the current video\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.Top250`</b>,
///                  \anchor VideoPlayer_Top250
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`VideoPlayer.EpisodeName`</b>,
///                  \anchor VideoPlayer_EpisodeName
///                  _string_,
///     (PVR only) The name of the episode if the playing video is a TV Show\,
///     if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.PlaylistPosition`</b>,
///                  \anchor VideoPlayer_PlaylistPosition
///                  _string_,
///     Position of the current song in the current video playlist
///   }
///   \table_row3{   <b>`VideoPlayer.PlaylistLength`</b>,
///                  \anchor VideoPlayer_PlaylistLength
///                  _string_,
///     Total size of the current video playlist
///   }
///   \table_row3{   <b>`VideoPlayer.Cast`</b>,
///                  \anchor VideoPlayer_Cast
///                  _string_,
///     A concatenated string of cast members of the current movie\, if it's in
///     the database
///   }
///   \table_row3{   <b>`VideoPlayer.CastAndRole`</b>,
///                  \anchor VideoPlayer_CastAndRole
///                  _string_,
///     A concatenated string of cast members and roles of the current movie\,
///     if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.Album`</b>,
///                  \anchor VideoPlayer_Album
///                  _string_,
///     Album from which the current Music Video is from\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.Artist`</b>,
///                  \anchor VideoPlayer_Artist
///                  _string_,
///     Artist(s) of current Music Video\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.Studio`</b>,
///                  \anchor VideoPlayer_Studio
///                  _string_,
///     Studio of current Music Video\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.Writer`</b>,
///                  \anchor VideoPlayer_Writer
///                  _string_,
///     Name of Writer of current playing Video\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.Tagline`</b>,
///                  \anchor VideoPlayer_Tagline
///                  _string_,
///     Small Summary of current playing Video\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.PlotOutline`</b>,
///                  \anchor VideoPlayer_PlotOutline
///                  _string_,
///     Small Summary of current playing Video\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.Plot`</b>,
///                  \anchor VideoPlayer_Plot
///                  _string_,
///     Complete Text Summary of current playing Video\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.Premiered`</b>,
///                  \anchor VideoPlayer_Premiered
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`VideoPlayer.Trailer`</b>,
///                  \anchor VideoPlayer_Trailer
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`VideoPlayer.LastPlayed`</b>,
///                  \anchor VideoPlayer_LastPlayed
///                  _string_,
///     Last play date of current playing Video\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.PlayCount`</b>,
///                  \anchor VideoPlayer_PlayCount
///                  _string_,
///     Playcount of current playing Video\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.VideoCodec`</b>,
///                  \anchor VideoPlayer_VideoCodec
///                  _string_,
///     Shows the video codec of the currently playing video (common values: see
///     \ref ListItem_VideoCodec "ListItem.VideoCodec")
///   }
///   \table_row3{   <b>`VideoPlayer.VideoResolution`</b>,
///                  \anchor VideoPlayer_VideoResolution
///                  _string_,
///     Shows the video resolution of the currently playing video (possible
///     values: see \ref ListItem_VideoResolution "ListItem.VideoResolution")
///   }
///   \table_row3{   <b>`VideoPlayer.VideoAspect`</b>,
///                  \anchor VideoPlayer_VideoAspect
///                  _string_,
///     Shows the aspect ratio of the currently playing video (possible values:
///     see \ref ListItem_VideoAspect "ListItem.VideoAspect")
///   }
///   \table_row3{   <b>`VideoPlayer.AudioCodec`</b>,
///                  \anchor VideoPlayer_AudioCodec
///                  _string_,
///     Shows the audio codec of the currently playing video\, optionally 'n'
///     defines the number of the audiostream (common values: see
///     \ref ListItem_AudioCodec "ListItem.AudioCodec")
///   }
///   \table_row3{   <b>`VideoPlayer.AudioChannels`</b>,
///                  \anchor VideoPlayer_AudioChannels
///                  _string_,
///     Shows the number of audio channels of the currently playing video
///     (possible values: see \ref ListItem_AudioChannels "ListItem.AudioChannels")
///   }
///   \table_row3{   <b>`VideoPlayer.AudioLanguage`</b>,
///                  \anchor VideoPlayer_AudioLanguage
///                  _string_,
///     Shows the language of the audio of the currently playing video(possible
///     values: see \ref ListItem_AudioLanguage "ListItem.AudioLanguage")
///   }
///   \table_row3{   <b>`VideoPlayer.SubtitlesLanguage`</b>,
///                  \anchor VideoPlayer_SubtitlesLanguage
///                  _string_,
///     Shows the language of the subtitle of the currently playing video
///     (possible values: see \ref ListItem_SubtitleLanguage "ListItem.SubtitleLanguage")
///   }
///   \table_row3{   <b>`VideoPlayer.StereoscopicMode`</b>,
///                  \anchor VideoPlayer_StereoscopicMode
///                  _string_,
///     Shows the stereoscopic mode of the currently playing video (possible
///     values: see \ref ListItem_StereoscopicMode "ListItem.StereoscopicMode")
///   }
///   \table_row3{   <b>`VideoPlayer.EndTime`</b>,
///                  \anchor VideoPlayer_EndTime
///                  _string_,
///     End date of the currently playing programme (PVR).
///   }
///   \table_row3{   <b>`VideoPlayer.NextTitle`</b>,
///                  \anchor VideoPlayer_NextTitle
///                  _string_,
///     Title of the programme that will be played next (PVR).
///   }
///   \table_row3{   <b>`VideoPlayer.NextGenre`</b>,
///                  \anchor VideoPlayer_NextGenre
///                  _string_,
///     Genre of the programme that will be played next (PVR).
///   }
///   \table_row3{   <b>`VideoPlayer.NextPlot`</b>,
///                  \anchor VideoPlayer_NextPlot
///                  _string_,
///     Plot of the programme that will be played next (PVR).
///   }
///   \table_row3{   <b>`VideoPlayer.NextPlotOutline`</b>,
///                  \anchor VideoPlayer_NextPlotOutline
///                  _string_,
///     Plot outline of the programme that will be played next (PVR).
///   }
///   \table_row3{   <b>`VideoPlayer.NextStartTime`</b>,
///                  \anchor VideoPlayer_NextStartTime
///                  _string_,
///     Start time of the programme that will be played next (PVR).
///   }
///   \table_row3{   <b>`VideoPlayer.NextEndTime`</b>,
///                  \anchor VideoPlayer_NextEndTime
///                  _string_,
///     End time of the programme that will be played next (PVR).
///   }
///   \table_row3{   <b>`VideoPlayer.NextDuration`</b>,
///                  \anchor VideoPlayer_NextDuration
///                  _string_,
///     Duration of the programme that will be played next (PVR).
///   }
///   \table_row3{   <b>`VideoPlayer.ChannelName`</b>,
///                  \anchor VideoPlayer_ChannelName
///                  _string_,
///     Name of the curently tuned channel (PVR).
///   }
///   \table_row3{   <b>`VideoPlayer.ChannelNumber`</b>,
///                  \anchor VideoPlayer_ChannelNumber
///                  _string_,
///     Number of the curently tuned channel (PVR).
///   }
///   \table_row3{   <b>`VideoPlayer.SubChannelNumber`</b>,
///                  \anchor VideoPlayer_SubChannelNumber
///                  _string_,
///     Subchannel number of the tv channel that's currently playing (PVR).
///   }
///   \table_row3{   <b>`VideoPlayer.ChannelNumberLabel`</b>,
///                  \anchor VideoPlayer_ChannelNumberLabel
///                  _string_,
///     Channel and subchannel number of the tv channel that's currently playing (PVR).
///   }
///   \table_row3{   <b>`VideoPlayer.ChannelGroup`</b>,
///                  \anchor VideoPlayer_ChannelGroup
///                  _string_,
///     Group of the curently tuned channel (PVR).
///   }
///   \table_row3{   <b>`VideoPlayer.ParentalRating`</b>,
///                  \anchor VideoPlayer_ParentalRating
///                  _string_,
///     Parental rating of the currently playing programme (PVR).
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}
const infomap videoplayer[] =    {{ "title",            VIDEOPLAYER_TITLE },
                                  { "genre",            VIDEOPLAYER_GENRE },
                                  { "country",          VIDEOPLAYER_COUNTRY },
                                  { "originaltitle",    VIDEOPLAYER_ORIGINALTITLE },
                                  { "director",         VIDEOPLAYER_DIRECTOR },
                                  { "year",             VIDEOPLAYER_YEAR },
                                  { "cover",            VIDEOPLAYER_COVER },
                                  { "usingoverlays",    VIDEOPLAYER_USING_OVERLAYS },
                                  { "isfullscreen",     VIDEOPLAYER_ISFULLSCREEN },
                                  { "hasmenu",          VIDEOPLAYER_HASMENU },
                                  { "playlistlength",   VIDEOPLAYER_PLAYLISTLEN },
                                  { "playlistposition", VIDEOPLAYER_PLAYLISTPOS },
                                  { "plot",             VIDEOPLAYER_PLOT },
                                  { "plotoutline",      VIDEOPLAYER_PLOT_OUTLINE },
                                  { "episode",          VIDEOPLAYER_EPISODE },
                                  { "season",           VIDEOPLAYER_SEASON },
                                  { "rating",           VIDEOPLAYER_RATING },
                                  { "ratingandvotes",   VIDEOPLAYER_RATING_AND_VOTES },
                                  { "userrating",       VIDEOPLAYER_USER_RATING },
                                  { "votes",            VIDEOPLAYER_VOTES },
                                  { "tvshowtitle",      VIDEOPLAYER_TVSHOW },
                                  { "premiered",        VIDEOPLAYER_PREMIERED },
                                  { "studio",           VIDEOPLAYER_STUDIO },
                                  { "mpaa",             VIDEOPLAYER_MPAA },
                                  { "top250",           VIDEOPLAYER_TOP250 },
                                  { "cast",             VIDEOPLAYER_CAST },
                                  { "castandrole",      VIDEOPLAYER_CAST_AND_ROLE },
                                  { "artist",           VIDEOPLAYER_ARTIST },
                                  { "album",            VIDEOPLAYER_ALBUM },
                                  { "writer",           VIDEOPLAYER_WRITER },
                                  { "tagline",          VIDEOPLAYER_TAGLINE },
                                  { "hasinfo",          VIDEOPLAYER_HAS_INFO },
                                  { "trailer",          VIDEOPLAYER_TRAILER },
                                  { "videocodec",       VIDEOPLAYER_VIDEO_CODEC },
                                  { "videoresolution",  VIDEOPLAYER_VIDEO_RESOLUTION },
                                  { "videoaspect",      VIDEOPLAYER_VIDEO_ASPECT },
                                  { "audiocodec",       VIDEOPLAYER_AUDIO_CODEC },
                                  { "audiochannels",    VIDEOPLAYER_AUDIO_CHANNELS },
                                  { "audiolanguage",    VIDEOPLAYER_AUDIO_LANG },
                                  { "hasteletext",      VIDEOPLAYER_HASTELETEXT },
                                  { "lastplayed",       VIDEOPLAYER_LASTPLAYED },
                                  { "playcount",        VIDEOPLAYER_PLAYCOUNT },
                                  { "hassubtitles",     VIDEOPLAYER_HASSUBTITLES },
                                  { "subtitlesenabled", VIDEOPLAYER_SUBTITLESENABLED },
                                  { "subtitleslanguage",VIDEOPLAYER_SUBTITLES_LANG },
                                  { "endtime",          VIDEOPLAYER_ENDTIME },
                                  { "nexttitle",        VIDEOPLAYER_NEXT_TITLE },
                                  { "nextgenre",        VIDEOPLAYER_NEXT_GENRE },
                                  { "nextplot",         VIDEOPLAYER_NEXT_PLOT },
                                  { "nextplotoutline",  VIDEOPLAYER_NEXT_PLOT_OUTLINE },
                                  { "nextstarttime",    VIDEOPLAYER_NEXT_STARTTIME },
                                  { "nextendtime",      VIDEOPLAYER_NEXT_ENDTIME },
                                  { "nextduration",     VIDEOPLAYER_NEXT_DURATION },
                                  { "channelname",      VIDEOPLAYER_CHANNEL_NAME },
                                  { "channelnumber",    VIDEOPLAYER_CHANNEL_NUMBER },
                                  { "subchannelnumber", VIDEOPLAYER_SUB_CHANNEL_NUMBER },
                                  { "channelnumberlabel", VIDEOPLAYER_CHANNEL_NUMBER_LBL },
                                  { "channelgroup",     VIDEOPLAYER_CHANNEL_GROUP },
                                  { "hasepg",           VIDEOPLAYER_HAS_EPG },
                                  { "parentalrating",   VIDEOPLAYER_PARENTAL_RATING },
                                  { "isstereoscopic",   VIDEOPLAYER_IS_STEREOSCOPIC },
                                  { "stereoscopicmode", VIDEOPLAYER_STEREOSCOPIC_MODE },
                                  { "canresumelivetv",  VIDEOPLAYER_CAN_RESUME_LIVE_TV },
                                  { "imdbnumber",       VIDEOPLAYER_IMDBNUMBER },
                                  { "episodename",      VIDEOPLAYER_EPISODENAME }
};

const infomap player_process[] =
{
  { "videodecoder", PLAYER_PROCESS_VIDEODECODER },
  { "deintmethod", PLAYER_PROCESS_DEINTMETHOD },
  { "pixformat", PLAYER_PROCESS_PIXELFORMAT },
  { "videowidth", PLAYER_PROCESS_VIDEOWIDTH },
  { "videoheight", PLAYER_PROCESS_VIDEOHEIGHT },
  { "videofps", PLAYER_PROCESS_VIDEOFPS },
  { "videodar", PLAYER_PROCESS_VIDEODAR },
  { "videohwdecoder", PLAYER_PROCESS_VIDEOHWDECODER },
  { "audiodecoder", PLAYER_PROCESS_AUDIODECODER },
  { "audiochannels", PLAYER_PROCESS_AUDIOCHANNELS },
  { "audiosamplerate", PLAYER_PROCESS_AUDIOSAMPLERATE },
  { "audiobitspersample", PLAYER_PROCESS_AUDIOBITSPERSAMPLE }
};

/// \page modules__General__List_of_gui_access
/// \section modules__General__List_of_gui_access_Container Container
/// @{
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Container(id).HasFiles`</b>,
///                  \anchor Container_HasFiles
///                  _boolean_,
///     Returns true if the container contains files (or current container if
///     id is omitted).
///   }
///   \table_row3{   <b>`Container(id).HasFolders`</b>,
///                  \anchor Container_HasFolders
///                  _boolean_,
///     Returns true if the container contains folders (or current container if
///     id is omitted).
///   }
///   \table_row3{   <b>`Container(id).IsStacked`</b>,
///                  \anchor Container_IsStacked
///                  _boolean_,
///     Returns true if the container is currently in stacked mode (or current
///     container if id is omitted).
///   }
///   \table_row3{   <b>`Container.FolderPath`</b>,
///                  \anchor Container_FolderPath
///                  _string_,
///     Shows complete path of currently displayed folder
///   }
///   \table_row3{   <b>`Container.FolderName`</b>,
///                  \anchor Container_FolderName
///                  _string_,
///     Shows top most folder in currently displayed folder
///   }
///   \table_row3{   <b>`Container.PluginName`</b>,
///                  \anchor Container_PluginName
///                  _string_,
///     Returns the current plugins base folder name
///   }
///   \table_row3{   <b>`Container.Viewmode`</b>,
///                  \anchor Container_Viewmode
///                  _string_,
///     Returns the current viewmode (list\, icons etc.)
///   }
///   \table_row3{   <b>`Container.ViewCount`</b>,
///                  \anchor Container_ViewCount
///                  _integer_,
///     Todo
///   }
///   \table_row3{   <b>`Container(id).Totaltime`</b>,
///                  \anchor Container_Totaltime
///                  _string_,
///     Returns the total time of all items in the current container
///   }
///   \table_row3{   <b>`Container(id).TotalWatched`</b>,
///                  \anchor Container_TotalWatched
///                  _string_,
///     Returns the number of watched items in the current container
///   }
///   \table_row3{   <b>`Container(id).TotalUnWatched`</b>,
///                  \anchor Container_TotalUnWatched
///                  _string_,
///     Returns the number of unwatched items in the current container
///   }
///   \table_row3{   <b>`Container.HasThumb`</b>,
///                  \anchor Container_HasThumb
///                  _string_,
///     Returns true if the current container you are in has a thumb assigned
///     to it
///   }
///   \table_row3{   <b>`Container.SortMethod`</b>,
///                  \anchor Container_SortMethod
///                  _string_,
///     Returns the current sort method (name\, year\, rating\, etc.)
///   }
///   \table_row3{   <b>`Container.SortOrder`</b>,
///                  \anchor Container_SortOrder
///                  _string_,
///     Returns the current sort order (Ascending/Descending)
///   }
///   \table_row3{   <b>`Container.ShowPlot`</b>,
///                  \anchor Container_ShowPlot
///                  _string_,
///     Returns the TV Show plot of the current container and can be used at
///     season and episode level
///   }
///   \table_row3{   <b>`Container.ShowTitle`</b>,
///                  \anchor Container_ShowTitle
///                  _string_,
///     Returns the TV Show title of the current container and can be used at
///     season and episode level
///   }
/// \table_end
/// @}
const infomap mediacontainer[] = {{ "hasfiles",         CONTAINER_HASFILES },
                                  { "hasfolders",       CONTAINER_HASFOLDERS },
                                  { "isstacked",        CONTAINER_STACKED },
                                  { "folderpath",       CONTAINER_FOLDERPATH },
                                  { "foldername",       CONTAINER_FOLDERNAME },
                                  { "pluginname",       CONTAINER_PLUGINNAME },
                                  { "plugincategory",   CONTAINER_PLUGINCATEGORY },
                                  { "viewmode",         CONTAINER_VIEWMODE },
                                  { "viewcount",        CONTAINER_VIEWCOUNT },
                                  { "totaltime",        CONTAINER_TOTALTIME },
                                  { "totalwatched",     CONTAINER_TOTALWATCHED },
                                  { "totalunwatched",   CONTAINER_TOTALUNWATCHED },
                                  { "hasthumb",         CONTAINER_HAS_THUMB },
                                  { "sortmethod",       CONTAINER_SORT_METHOD },
                                  { "sortorder",        CONTAINER_SORT_ORDER },
                                  { "showplot",         CONTAINER_SHOWPLOT },
                                  { "showtitle",        CONTAINER_SHOWTITLE }};

/// \page modules__General__List_of_gui_access
/// @{
/// \table_start
///   \table_row3{   <b>`Container(id).OnNext`</b>,
///                  \anchor Container_OnNext
///                  _boolean_,
///     Returns true if the container with id (or current container if id is
///     omitted) is moving to the next item. Allows views to be
///     custom-designed (such as 3D coverviews etc.)
///   }
///   \table_row3{   <b>`Container(id).OnScrollNext`</b>,
///                  \anchor Container_OnScrollNext
///                  _boolean_,
///     Returns true if the container with id (or current container if id is
///     omitted) is scrolling to the next item. Differs from OnNext in that
///     OnNext triggers on movement even if there is no scroll involved.
///   }
///   \table_row3{   <b>`Container(id).OnPrevious`</b>,
///                  \anchor Container_OnPrevious
///                  _boolean_,
///     Returns true if the container with id (or current container if id is
///     omitted) is moving to the previous item. Allows views to be
///     custom-designed (such as 3D coverviews etc.)
///   }
///   \table_row3{   <b>`Container(id).OnScrollPrevious`</b>,
///                  \anchor Container_OnScrollPrevious
///                  _boolean_,
///     Returns true if the container with id (or current container if id is
///     omitted) is scrolling to the previous item. Differs from OnPrevious in
///     that OnPrevious triggers on movement even if there is no scroll involved.
///   }
///   \table_row3{   <b>`Container(id).NumPages`</b>,
///                  \anchor Container_NumPages
///                  _boolean_,
///     Number of pages in the container with given id. If no id is specified it
///     grabs the current container.
///   }
///   \table_row3{   <b>`Container(id).NumItems`</b>,
///                  \anchor Container_NumItems
///                  _boolean_,
///     Number of items in the container or grouplist with given id. If no id is
///     specified it grabs the current container.
///   }
///   \table_row3{   <b>`Container(id).CurrentPage`</b>,
///                  \anchor Container_CurrentPage
///                  _boolean_,
///     Current page in the container with given id. If no id is specified it
///     grabs the current container.
///   }
///   \table_row3{   <b>`Container(id).Scrolling`</b>,
///                  \anchor Container_Scrolling
///                  _boolean_,
///     Returns true if the user is currently scrolling through the container
///     with id (or current container if id is omitted). Note that this is
///     slightly delayed from the actual scroll start. Use
///     Container(id).OnScrollNext/OnScrollPrevious to trigger animations
///     immediately on scroll.
///   }
///   \table_row3{   <b>`Container(id).HasNext`</b>,
///                  \anchor Container_HasNext
///                  _boolean_,
///     Returns true if the container or textbox with id (id) has a next page.
///   }
///   \table_row3{   <b>`Container.HasParent`</b>,
///                  \anchor Container_HasParent
///                  _boolean_,
///     Return true when the container contains a parent ('..') item.
///   }
///   \table_row3{   <b>`Container(id).HasPrevious`</b>,
///                  \anchor Container_HasPrevious
///                  _boolean_,
///     Returns true if the container or textbox with id (id) has a previous page.
///   }
///   \table_row3{   <b>`Container.CanFilter`</b>,
///                  \anchor Container_CanFilter
///                  _boolean_,
///     Returns true when the current container can be filtered.
///   }
///   \table_row3{   <b>`Container.CanFilterAdvanced`</b>,
///                  \anchor Container_CanFilterAdvanced
///                  _boolean_,
///     Returns true when advanced filtering can be applied to the current container.
///   }
///   \table_row3{   <b>`Container.Filtered`</b>,
///                  \anchor Container_Filtered
///                  _boolean_,
///     Returns true when a mediafilter is applied to the current container.
///   }
///   \table_row3{   <b>`Container(id).IsUpdating`</b>,
///                  \anchor Container_IsUpdating
///                  _boolean_,
///     Returns true if the container with dynamic list content is currently updating.
///   }
/// \table_end
/// @}
const infomap container_bools[] ={{ "onnext",           CONTAINER_MOVE_NEXT },
                                  { "onprevious",       CONTAINER_MOVE_PREVIOUS },
                                  { "onscrollnext",     CONTAINER_SCROLL_NEXT },
                                  { "onscrollprevious", CONTAINER_SCROLL_PREVIOUS },
                                  { "numpages",         CONTAINER_NUM_PAGES },
                                  { "numitems",         CONTAINER_NUM_ITEMS },
                                  { "currentpage",      CONTAINER_CURRENT_PAGE },
                                  { "scrolling",        CONTAINER_SCROLLING },
                                  { "hasnext",          CONTAINER_HAS_NEXT },
                                  { "hasparent",        CONTAINER_HAS_PARENT_ITEM },
                                  { "hasprevious",      CONTAINER_HAS_PREVIOUS },
                                  { "canfilter",        CONTAINER_CAN_FILTER },
                                  { "canfilteradvanced",CONTAINER_CAN_FILTERADVANCED },
                                  { "filtered",         CONTAINER_FILTERED },
                                  { "isupdating",       CONTAINER_ISUPDATING }};

/// \page modules__General__List_of_gui_access
/// @{
/// \table_start
///   \table_row3{   <b>`Container(id).Row`</b>,
///                  \anchor Container_Row
///                  _integer_,
///     Returns the row number of the focused position in a panel container.
///   }
///   \table_row3{   <b>`Container(id).Column`</b>,
///                  \anchor Container_Column
///                  _integer_,
///     Returns the column number of the focused position in a panel container.
///   }
///   \table_row3{   <b>`Container(id).Position`</b>,
///                  \anchor Container_Position
///                  _integer_,
///     Returns the current focused position of container / grouplist (id) as a
///     numeric label.
///   }
///   \table_row3{   <b>`Container(id).CurrentItem`</b>,
///                  \anchor Container_CurrentItem
///                  _integer_,
///     Current item in the container or grouplist with given id. If no id is
///     specified it grabs the current container.
///   }
///   \table_row3{   <b>`Container(id).SubItem`</b>,
///                  \anchor Container_SubItem
///                  _integer_,
///     Todo
///   }
///   \table_row3{   <b>`Container(id).HasFocus(item_number)`</b>,
///                  \anchor Container_HasFocus
///                  _boolean_,
///     Returns true if the container with id (or current container if id is
///     omitted) has static content and is focused on the item with id
///     item_number.
///   }
/// \table_end
/// @}
const infomap container_ints[] = {{ "row",              CONTAINER_ROW },
                                  { "column",           CONTAINER_COLUMN },
                                  { "position",         CONTAINER_POSITION },
                                  { "currentitem",      CONTAINER_CURRENT_ITEM },
                                  { "subitem",          CONTAINER_SUBITEM },
                                  { "hasfocus",         CONTAINER_HAS_FOCUS }};

/// \page modules__General__List_of_gui_access
/// @{
/// \table_start
///   \table_row3{   <b>`Container.Property(addoncategory)`</b>,
///                  \anchor Container_Property_addoncategory
///                  _string_,
///     Returns the current add-on category
///   }
///   \table_row3{   <b>`Container.Property(reponame)`</b>,
///                  \anchor Container_Property_reponame
///                  _string_,
///     Returns the current add-on repository name
///   }
///   \table_row3{   <b>`Container.Content(parameter)`</b>,
///                  \anchor Container_Content
///                  _string_,
///     Returns true if the current container you are in contains the following:
///     files\, songs\, artists\, albums\, movies\, tvshows\,
///     seasons\, episodes\, musicvideos\, genres\, years\,
///     actors\, playlists\, plugins\, studios\, directors\,
///     sets\, tags (Note: these currently only work in the Video and Music
///     Library or unless a Plugin has set the value) also available are
///     Addons true when a list of add-ons is shown LiveTV true when a
///     htsp (tvheadend) directory is shown
///   }
///   \table_row3{   <b>`Container.Art`</b>,
///                  \anchor Container_Art
///                  _string_,
///     Todo
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}
const infomap container_str[]  = {{ "property",         CONTAINER_PROPERTY },
                                  { "content",          CONTAINER_CONTENT },
                                  { "art",              CONTAINER_ART }};

/// \page modules__General__List_of_gui_access
/// \section modules__General__List_of_gui_access_ListItem ListItem
/// @{
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`ListItem.Thumb`</b>,
///                  \anchor ListItem_Thumb
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`ListItem.Icon`</b>,
///                  \anchor ListItem_Icon
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`ListItem.ActualIcon`</b>,
///                  \anchor ListItem_ActualIcon
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`ListItem.Overlay`</b>,
///                  \anchor ListItem_Overlay
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`ListItem.IsFolder`</b>,
///                  \anchor ListItem_IsFolder
///                  _boolean_,
///     Returns whether the current ListItem is a folder
///   }
///   \table_row3{   <b>`ListItem.IsPlaying`</b>,
///                  \anchor ListItem_IsPlaying
///                  _boolean_,
///     Returns whether the current ListItem.* info labels and images are
///     currently Playing media
///   }
///   \table_row3{   <b>`ListItem.IsResumable`</b>,
///                  \anchor ListItem_IsResumable
///                  _boolean_,
///     Returns true when the current ListItem has been partially played
///   }
///   \table_row3{   <b>`ListItem.IsCollection`</b>,
///                  \anchor ListItem_IsCollection
///                  _boolean_,
///     Returns true when the current ListItem is a movie set
///   }
///   \table_row3{   <b>`ListItem.IsSelected`</b>,
///                  \anchor ListItem_IsSelected
///                  _boolean_,
///     Returns whether the current ListItem is selected (f.e. currently playing
///     in playlist window)
///   }
///   \table_row3{   <b>`ListItem.HasEpg`</b>,
///                  \anchor ListItem_HasEpg
///                  _boolean_,
///     Returns true when the selected programme has epg info (PVR)
///   }
///   \table_row3{   <b>`ListItem.HasTimer`</b>,
///                  \anchor ListItem_HasTimer
///                  _boolean_,
///     Returns true when a recording timer has been set for the selected
///     programme (PVR)
///   }
///   \table_row3{   <b>`ListItem.IsRecording`</b>,
///                  \anchor ListItem_IsRecording
///                  _boolean_,
///     Returns true when the selected programme is being recorded (PVR)
///   }
///   \table_row3{   <b>`ListItem.IsEncrypted`</b>,
///                  \anchor ListItem_IsEncrypted
///                  _boolean_,
///     Returns true when the selected programme is encrypted (PVR)
///   }
///   \table_row3{   <b>`ListItem.IsStereoscopic`</b>,
///                  \anchor ListItem_IsStereoscopic
///                  _boolean_,
///     Returns true when the selected video is a 3D (stereoscopic) video
///   }
///   \table_row3{   <b>`ListItem.Property(IsSpecial)`</b>,
///                  \anchor ListItem_Property_IsSpecial
///                  _boolean_,
///     Returns whether the current Season/Episode is a Special
///   }
///   \table_row3{   <b>`ListItem.Property(DateLabel)`</b>,
///                  \anchor ListItem_Property_DateLabel
///                  _string_,
///     Can be used in the rulerlayout of the epggrid control. Will return true
///     if the item is a date label\, returns false if the item is a time label.
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Enabled)`</b>,
///                  \anchor ListItem_Property_AddonEnabled
///                  _boolean_,
///     Returns true when the selected addon is enabled (for use in the addon
///     info dialog only).
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Installed)`</b>,
///                  \anchor ListItem_Property_AddonInstalled
///                  _boolean_,
///     Returns true when the selected addon is installed (for use in the addon
///     info dialog only).
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.UpdateAvail)`</b>,
///                  \anchor ListItem_Property_AddonUpdateAvail
///                  _boolean_,
///     Returns true when there's an update available for the selected addon.
///   }
///   \table_row3{   <b>`ListItem.Label`</b>,
///                  \anchor ListItem_Label
///                  _string_,
///     Shows the left label of the currently selected item in a container
///   }
///   \table_row3{   <b>`ListItem.Label2`</b>,
///                  \anchor ListItem_Label2
///                  _string_,
///     Shows the right label of the currently selected item in a container
///   }
///   \table_row3{   <b>`ListItem.Title`</b>,
///                  \anchor ListItem_Title
///                  _string_,
///     Shows the title of the currently selected song or movie in a container
///   }
///   \table_row3{   <b>`ListItem.OriginalTitle`</b>,
///                  \anchor ListItem_OriginalTitle
///                  _string_,
///     Shows the original title of the currently selected movie in a container
///   }
///   \table_row3{   <b>`ListItem.SortLetter`</b>,
///                  \anchor ListItem_SortLetter
///                  _string_,
///     Shows the first letter of the current file in a container
///   }
///   \table_row3{   <b>`ListItem.TrackNumber`</b>,
///                  \anchor ListItem_TrackNumber
///                  _string_,
///     Shows the track number of the currently selected song in a container
///   }
///   \table_row3{   <b>`ListItem.Artist`</b>,
///                  \anchor ListItem_Artist
///                  _string_,
///     Shows the artist of the currently selected song in a container
///   }
///   \table_row3{   <b>`ListItem.AlbumArtist`</b>,
///                  \anchor ListItem_AlbumArtist
///                  _string_,
///     Shows the artist of the currently selected album in a list
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Born)`</b>,
///                  \anchor ListItem_Property_Artist_Born
///                  _string_,
///     Date of Birth of the currently selected Artist
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Died)`</b>,
///                  \anchor ListItem_Property_Artist_Died
///                  _string_,
///     Date of Death of the currently selected Artist
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Formed)`</b>,
///                  \anchor ListItem_Property_Artist_Formed
///                  _string_,
///     Formation date of the currently selected Band
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Disbanded)`</b>,
///                  \anchor ListItem_Property_Artist_Disbanded
///                  _string_,
///     Disbanding date of the currently selected Band
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_YearsActive)`</b>,
///                  \anchor ListItem_Property_Artist_YearsActive
///                  _string_,
///     Years the currently selected artist has been active
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Instrument)`</b>,
///                  \anchor ListItem_Property_Artist_Instrument
///                  _string_,
///     Instruments played by the currently selected artist
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Description)`</b>,
///                  \anchor ListItem_Property_Artist_Description
///                  _string_,
///     Shows a biography of the currently selected artist
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Mood)`</b>,
///                  \anchor ListItem_Property_Artist_Mood
///                  _string_,
///     Shows the moods of the currently selected artist
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Style)`</b>,
///                  \anchor ListItem_Property_Artist_Style
///                  _string_,
///     Shows the styles of the currently selected artist
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Genre)`</b>,
///                  \anchor ListItem_Property_Artist_Genre
///                  _string_,
///     Shows the genre of the currently selected artist
///   }
///   \table_row3{   <b>`ListItem.Album`</b>,
///                  \anchor ListItem_Album
///                  _string_,
///     Shows the album of the currently selected song in a container
///   }
///   \table_row3{   <b>`ListItem.Property(Album_Mood)`</b>,
///                  \anchor ListItem_Property_Album_Mood
///                  _string_,
///     Shows the moods of the currently selected Album
///   }
///   \table_row3{   <b>`ListItem.Property(Album_Style)`</b>,
///                  \anchor ListItem_Property_Album_Style
///                  _string_,
///     Shows the styles of the currently selected Album
///   }
///   \table_row3{   <b>`ListItem.Property(Album_Theme)`</b>,
///                  \anchor ListItem_Property_Album_Theme
///                  _string_,
///     Shows the themes of the currently selected Album
///   }
///   \table_row3{   <b>`ListItem.Property(Album_Type)`</b>,
///                  \anchor ListItem_Property_Album_Type
///                  _string_,
///     Shows the Album Type (e.g. compilation\, enhanced\, explicit lyrics) of
///     the currently selected Album
///   }
///   \table_row3{   <b>`ListItem.Property(Album_Label)`</b>,
///                  \anchor ListItem_Property_Album_Label
///                  _string_,
///     Shows the record label of the currently selected Album
///   }
///   \table_row3{   <b>`ListItem.Property(Album_Description)`</b>,
///                  \anchor ListItem_Property_Album_Description
///                  _string_,
///     Shows a review of the currently selected Album
///   }
///   \table_row3{   <b>`ListItem.DiscNumber`</b>,
///                  \anchor ListItem_DiscNumber
///                  _string_,
///     Shows the disc number of the currently selected song in a container
///   }
///   \table_row3{   <b>`ListItem.Year`</b>,
///                  \anchor ListItem_Year
///                  _string_,
///     Shows the year of the currently selected song\, album or movie in a
///     container
///   }
///   \table_row3{   <b>`ListItem.Premiered`</b>,
///                  \anchor ListItem_Premiered
///                  _string_,
///     Shows the release/aired date of the currently selected episode\, show\,
///     movie or EPG item in a container
///   }
///   \table_row3{   <b>`ListItem.Genre`</b>,
///                  \anchor ListItem_Genre
///                  _string_,
///     Shows the genre of the currently selected song\, album or movie in a
///     container
///   }
///   \table_row3{   <b>`ListItem.Contributor`</b>,
///                  \anchor ListItem_Contributor
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`ListItem.ContributorAndRole`</b>,
///                  \anchor ListItem_ContributorAndRole
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`ListItem.Director`</b>,
///                  \anchor ListItem_Director
///                  _string_,
///     Shows the director of the currently selected movie in a container
///   }
///   \table_row3{   <b>`ListItem.Country`</b>,
///                  \anchor ListItem_Country
///                  _string_,
///     Shows the production country of the currently selected movie in a
///     container
///   }
///   \table_row3{   <b>`ListItem.Episode`</b>,
///                  \anchor ListItem_Episode
///                  _string_,
///     Shows the episode number value for the currently selected episode. It
///     also shows the number of total\, watched or unwatched episodes for the
///     currently selected tvshow or season\, based on the the current watched
///     filter.
///   }
///   \table_row3{   <b>`ListItem.Season`</b>,
///                  \anchor ListItem_Season
///                  _string_,
///     Shows the season value for the currently selected tvshow
///   }
///   \table_row3{   <b>`ListItem.TVShowTitle`</b>,
///                  \anchor ListItem_TVShowTitle
///                  _string_,
///     Shows the name value for the currently selected tvshow in the season and
///     episode depth of the video library
///   }
///   \table_row3{   <b>`ListItem.Property(TotalSeasons)`</b>,
///                  \anchor ListItem_Property_TotalSeasons
///                  _string_,
///     Shows the total number of seasons for the currently selected tvshow
///   }
///   \table_row3{   <b>`ListItem.Property(TotalEpisodes)`</b>,
///                  \anchor ListItem_Property_TotalEpisodes
///                  _string_,
///     Shows the total number of episodes for the currently selected tvshow or
///     season
///   }
///   \table_row3{   <b>`ListItem.Property(WatchedEpisodes)`</b>,
///                  \anchor ListItem_Property_WatchedEpisodes
///                  _string_,
///     Shows the number of watched episodes for the currently selected tvshow
///     or season
///   }
///   \table_row3{   <b>`ListItem.Property(UnWatchedEpisodes)`</b>,
///                  \anchor ListItem_Property_UnWatchedEpisodes
///                  _string_,
///     Shows the number of unwatched episodes for the currently selected tvshow
///     or season
///   }
///   \table_row3{   <b>`ListItem.Property(NumEpisodes)`</b>,
///                  \anchor ListItem_Property_NumEpisodes
///                  _string_,
///     Shows the number of total\, watched or unwatched episodes for the
///     currently selected tvshow or season\, based on the the current watched filter.
///   }
///   \table_row3{   <b>`ListItem.PictureAperture`</b>,
///                  \anchor ListItem_PictureAperture
///                  _string_,
///     Shows the F-stop used to take the selected picture. This is the value of the
///     EXIF FNumber tag (hex code 0x829D).
///   }
///   \table_row3{   <b>`ListItem.PictureAuthor`</b>,
///                  \anchor ListItem_PictureAuthor
///                  _string_,
///     Shows the name of the person involved in writing about the selected picture.
///     This is the value of the IPTC Writer tag (hex code 0x7A).
///   }
///   \table_row3{   <b>`ListItem.PictureByline`</b>,
///                  \anchor ListItem_PictureByline
///                  _string_,
///     Shows the name of the person who created the selected picture. This is
///     the value of the IPTC Byline tag (hex code 0x50).
///   }
///   \table_row3{   <b>`ListItem.PictureBylineTitle`</b>,
///                  \anchor ListItem_PictureBylineTitle
///                  _string_,
///     Shows the title of the person who created the selected picture. This is
///     the value of the IPTC BylineTitle tag (hex code 0x55).
///   }
///   \table_row3{   <b>`ListItem.PictureCamMake`</b>,
///                  \anchor ListItem_PictureCamMake
///                  _string_,
///     Shows the manufacturer of the camera used to take the selected picture.
///     This is the value of the EXIF Make tag (hex code 0x010F).
///   }
///   \table_row3{   <b>`ListItem.PictureCamModel`</b>,
///                  \anchor ListItem_PictureCamModel
///                  _string_,
///     Shows the manufacturer's model name or number of the camera used to take
///     the selected picture. This is the value of the EXIF Model tag (hex code
///     0x0110).
///   }
///   \table_row3{   <b>`ListItem.PictureCaption`</b>,
///                  \anchor ListItem_PictureCaption
///                  _string_,
///     Shows a description of the selected picture. This is the value of the IPTC
///     Caption tag (hex code 0x78).
///   }
///   \table_row3{   <b>`ListItem.PictureCategory`</b>,
///                  \anchor ListItem_PictureCategory
///                  _string_,
///     Shows the subject of the selected picture as a category code. This is the
///     value of the IPTC Category tag (hex code 0x0F).
///   }
///   \table_row3{   <b>`ListItem.PictureCCDWidth`</b>,
///                  \anchor ListItem_PictureCCDWidth
///                  _string_,
///     Shows the width of the CCD in the camera used to take the selected
///     picture. This is calculated from three EXIF tags (0xA002 * 0xA210
///     / 0xA20e).
///   }
///   \table_row3{   <b>`ListItem.PictureCity`</b>,
///                  \anchor ListItem_PictureCity
///                  _string_,
///     Shows the city where the selected picture was taken. This is the value of
///     the IPTC City tag (hex code 0x5A).
///   }
///   \table_row3{   <b>`ListItem.PictureColour`</b>,
///                  \anchor ListItem_PictureColour
///                  _string_,
///     Shows whether the selected picture is "Colour" or "Black and White".
///   }
///   \table_row3{   <b>`ListItem.PictureComment`</b>,
///                  \anchor ListItem_PictureComment
///                  _string_,
///     Shows a description of the selected picture. This is the value of the
///     EXIF User Comment tag (hex code 0x9286). This is the same value as
///     \ref Slideshow_SlideComment "Slideshow.SlideComment".
///   }
///   \table_row3{   <b>`ListItem.PictureCopyrightNotice`</b>,
///                  \anchor ListItem_PictureCopyrightNotice
///                  _string_,
///     Shows the copyright notice of the selected picture. This is the value of
///     the IPTC Copyright tag (hex code 0x74).
///   }
///   \table_row3{   <b>`ListItem.PictureCountry`</b>,
///                  \anchor ListItem_PictureCountry
///                  _string_,
///     Shows the full name of the country where the selected picture was taken.
///     This is the value of the IPTC CountryName tag (hex code 0x65).
///   }
///   \table_row3{   <b>`ListItem.PictureCountryCode`</b>,
///                  \anchor ListItem_PictureCountryCode
///                  _string_,
///     Shows the country code of the country where the selected picture was
///     taken. This is the value of the IPTC CountryCode tag (hex code 0x64).
///   }
///   \table_row3{   <b>`ListItem.PictureCredit`</b>,
///                  \anchor ListItem_PictureCredit
///                  _string_,
///     Shows who provided the selected picture. This is the value of the IPTC
///     Credit tag (hex code 0x6E).
///   }
///   \table_row3{   <b>`ListItem.PictureDate`</b>,
///                  \anchor ListItem_PictureDate
///                  _string_,
///     Shows the localized date of the selected picture. The short form of the
///     date is used. The value of the EXIF DateTimeOriginal tag (hex code 0x9003)
///     is preferred. If the DateTimeOriginal tag is not found\, the value of
///     DateTimeDigitized (hex code 0x9004) or of DateTime (hex code 0x0132) might
///     be used.
///   }
///   \table_row3{   <b>`ListItem.PictureDatetime`</b>,
///                  \anchor ListItem_PictureDatetime
///                  _string_,
///     Shows the date/timestamp of the selected picture. The localized short form
///     of the date and time is used. The value of the EXIF DateTimeOriginal tag
///     (hex code 0x9003) is preferred. If the DateTimeOriginal tag is not found\,
///     the value of DateTimeDigitized (hex code 0x9004) or of DateTime (hex code
///     0x0132) might be used.
///   }
///   \table_row3{   <b>`ListItem.PictureDesc`</b>,
///                  \anchor ListItem_PictureDesc
///                  _string_,
///     Shows a short description of the selected picture. The SlideComment\,
///     EXIFComment\, or Caption values might contain a longer description. This
///     is the value of the EXIF ImageDescription tag (hex code 0x010E).
///   }
///   \table_row3{   <b>`ListItem.PictureDigitalZoom`</b>,
///                  \anchor ListItem_PictureDigitalZoom
///                  _string_,
///     Shows the digital zoom ratio when the selected picture was taken. This
///     is the value of the EXIF DigitalZoomRatio tag (hex code 0xA404).
///   }
///   \table_row3{   <b>`ListItem.PictureExpMode`</b>,
///                  \anchor ListItem_PictureExpMode
///                  _string_,
///     Shows the exposure mode of the selected picture. The possible values are
///     "Automatic"\, "Manual"\, and "Auto bracketing". This is the value of the
///     EXIF ExposureMode tag (hex code 0xA402).
///   }
///   \table_row3{   <b>`ListItem.PictureExposure`</b>,
///                  \anchor ListItem_PictureExposure
///                  _string_,
///     Shows the class of the program used by the camera to set exposure when
///     the selected picture was taken. Values include "Manual"\, "Program
///     (Auto)"\, "Aperture priority (Semi-Auto)"\, "Shutter priority (semi-auto)"\,
///     etc. This is the value of the EXIF ExposureProgram tag (hex code 0x8822).
///   }
///   \table_row3{   <b>`ListItem.PictureExposureBias`</b>,
///                  \anchor ListItem_PictureExposureBias
///                  _string_,
///     Shows the exposure bias of the selected picture. Typically this is a
///     number between -99.99 and 99.99. This is the value of the EXIF
///     ExposureBiasValue tag (hex code 0x9204).
///   }
///   \table_row3{   <b>`ListItem.PictureExpTime`</b>,
///                  \anchor ListItem_PictureExpTime
///                  _string_,
///     Shows the exposure time of the selected picture\, in seconds. This is the
///     value of the EXIF ExposureTime tag (hex code 0x829A). If the ExposureTime
///     tag is not found\, the ShutterSpeedValue tag (hex code 0x9201) might be
///     used.
///   }
///   \table_row3{   <b>`ListItem.PictureFlashUsed`</b>,
///                  \anchor ListItem_PictureFlashUsed
///                  _string_,
///     Shows the status of flash when the selected picture was taken. The value
///     will be either "Yes" or "No"\, and might include additional information.
///     This is the value of the EXIF Flash tag (hex code 0x9209).
///   }
///   \table_row3{   <b>`ListItem.PictureFocalLen`</b>,
///                  \anchor ListItem_PictureFocalLen
///                  _string_,
///     Shows the lens focal length of the selected picture
///   }
///   \table_row3{   <b>`ListItem.PictureFocusDist`</b>,
///                  \anchor ListItem_PictureFocusDist
///                  _string_,
///     Shows the focal length of the lens\, in mm. This is the value of the EXIF
///     FocalLength tag (hex code 0x920A).
///   }
///   \table_row3{   <b>`ListItem.PictureGPSLat`</b>,
///                  \anchor ListItem_PictureGPSLat
///                  _string_,
///     Shows the latitude where the selected picture was taken (degrees\,
///     minutes\, seconds North or South). This is the value of the EXIF
///     GPSInfo.GPSLatitude and GPSInfo.GPSLatitudeRef tags.
///   }
///   \table_row3{   <b>`ListItem.PictureGPSLon`</b>,
///                  \anchor ListItem_PictureGPSLon
///                  _string_,
///     Shows the longitude where the selected picture was taken (degrees\,
///     minutes\, seconds East or West). This is the value of the EXIF
///     GPSInfo.GPSLongitude and GPSInfo.GPSLongitudeRef tags.
///   }
///   \table_row3{   <b>`ListItem.PictureGPSAlt`</b>,
///                  \anchor ListItem_PictureGPSAlt
///                  _string_,
///     Shows the altitude in meters where the selected picture was taken. This
///     is the value of the EXIF GPSInfo.GPSAltitude tag.
///   }
///   \table_row3{   <b>`ListItem.PictureHeadline`</b>,
///                  \anchor ListItem_PictureHeadline
///                  _string_,
///     Shows a synopsis of the contents of the selected picture. This is the
///     value of the IPTC Headline tag (hex code 0x69).
///   }
///   \table_row3{   <b>`ListItem.PictureImageType`</b>,
///                  \anchor ListItem_PictureImageType
///                  _string_,
///     Shows the color components of the selected picture. This is the value of
///     the IPTC ImageType tag (hex code 0x82).
///   }
///   \table_row3{   <b>`ListItem.PictureIPTCDate`</b>,
///                  \anchor ListItem_PictureIPTCDate
///                  _string_,
///     Shows the date when the intellectual content of the selected picture was
///     created\, rather than when the picture was created. This is the value of
///     the IPTC DateCreated tag (hex code 0x37).
///   }
///   \table_row3{   <b>`ListItem.PictureIPTCTime`</b>,
///                  \anchor ListItem_PictureIPTCTime
///                  _string_,
///     Shows the time when the intellectual content of the selected picture was
///     created\, rather than when the picture was created. This is the value of
///     the IPTC TimeCreated tag (hex code 0x3C).
///   }
///   \table_row3{   <b>`ListItem.PictureISO`</b>,
///                  \anchor ListItem_PictureISO
///                  _string_,
///     Shows the ISO speed of the camera when the selected picture was taken.
///     This is the value of the EXIF ISOSpeedRatings tag (hex code 0x8827).
///   }
///   \table_row3{   <b>`ListItem.PictureKeywords`</b>,
///                  \anchor ListItem_PictureKeywords
///                  _string_,
///     Shows keywords assigned to the selected picture. This is the value of
///     the IPTC Keywords tag (hex code 0x19).
///   }
///   \table_row3{   <b>`ListItem.PictureLightSource`</b>,
///                  \anchor ListItem_PictureLightSource
///                  _string_,
///     Shows the kind of light source when the picture was taken. Possible
///     values include "Daylight"\, "Fluorescent"\, "Incandescent"\, etc. This is
///     the value of the EXIF LightSource tag (hex code 0x9208).
///   }
///   \table_row3{   <b>`ListItem.PictureLongDate`</b>,
///                  \anchor ListItem_PictureLongDate
///                  _string_,
///     Shows only the localized date of the selected picture. The long form of
///     the date is used. The value of the EXIF DateTimeOriginal tag (hex code
///     0x9003) is preferred. If the DateTimeOriginal tag is not found\, the
///     value of DateTimeDigitized (hex code 0x9004) or of DateTime (hex code
///     0x0132) might be used.
///   }
///   \table_row3{   <b>`ListItem.PictureLongDatetime`</b>,
///                  \anchor ListItem_PictureLongDatetime
///                  _string_,
///     Shows the date/timestamp of the selected picture. The localized long
///     form of the date and time is used. The value of the EXIF DateTimeOriginal
///     tag (hex code 0x9003) is preferred. if the DateTimeOriginal tag is not
///     found\, the value of DateTimeDigitized (hex code 0x9004) or of DateTime
///     (hex code 0x0132) might be used.
///   }
///   \table_row3{   <b>`ListItem.PictureMeteringMode`</b>,
///                  \anchor ListItem_PictureMeteringMode
///                  _string_,
///     Shows the metering mode used when the selected picture was taken. The
///     possible values are "Center weight"\, "Spot"\, or "Matrix". This is the
///     value of the EXIF MeteringMode tag (hex code 0x9207).
///   }
///   \table_row3{   <b>`ListItem.PictureObjectName`</b>,
///                  \anchor ListItem_PictureObjectName
///                  _string_,
///     Shows a shorthand reference for the selected picture. This is the value
///     of the IPTC ObjectName tag (hex code 0x05).
///   }
///   \table_row3{   <b>`ListItem.PictureOrientation`</b>,
///                  \anchor ListItem_PictureOrientation
///                  _string_,
///     Shows the orientation of the selected picture. Possible values are "Top
///     Left"\, "Top Right"\, "Left Top"\, "Right Bottom"\, etc. This is the value
///     of the EXIF Orientation tag (hex code 0x0112).
///   }
///   \table_row3{   <b>`ListItem.PicturePath`</b>,
///                  \anchor ListItem_PicturePath
///                  _string_,
///     Shows the filename and path of the selected picture
///   }
///   \table_row3{   <b>`ListItem.PictureProcess`</b>,
///                  \anchor ListItem_PictureProcess
///                  _string_,
///     Shows the process used to compress the selected picture
///   }
///   \table_row3{   <b>`ListItem.PictureReferenceService`</b>,
///                  \anchor ListItem_PictureReferenceService
///                  _string_,
///     Shows the Service Identifier of a prior envelope to which the selected
///     picture refers. This is the value of the IPTC ReferenceService tag
///     (hex code 0x2D).
///   }
///   \table_row3{   <b>`ListItem.PictureResolution`</b>,
///                  \anchor ListItem_PictureResolution
///                  _string_,
///     Shows the dimensions of the selected picture
///   }
///   \table_row3{   <b>`ListItem.PictureSource`</b>,
///                  \anchor ListItem_PictureSource
///                  _string_,
///     Shows the original owner of the selected picture. This is the value of
///     the IPTC Source tag (hex code 0x73).
///   }
///   \table_row3{   <b>`ListItem.PictureSpecialInstructions`</b>,
///                  \anchor ListItem_PictureSpecialInstructions
///                  _string_,
///     Shows other editorial instructions concerning the use of the selected
///     picture. This is the value of the IPTC SpecialInstructions tag (hex
///     code 0x28).
///   }
///   \table_row3{   <b>`ListItem.PictureState`</b>,
///                  \anchor ListItem_PictureState
///                  _string_,
///     Shows the State/Province where the selected picture was taken. This is
///     the value of the IPTC ProvinceState tag (hex code 0x5F).
///   }
///   \table_row3{   <b>`ListItem.PictureSublocation`</b>,
///                  \anchor ListItem_PictureSublocation
///                  _string_,
///     Shows the location within a city where the selected picture was taken -
///     might indicate the nearest landmark. This is the value of the IPTC
///     SubLocation tag (hex code 0x5C).
///   }
///   \table_row3{   <b>`ListItem.PictureSupplementalCategories`</b>,
///                  \anchor ListItem_PictureSupplementalCategories
///                  _string_,
///     Shows supplemental category codes to further refine the subject of the
///     selected picture. This is the value of the IPTC SuppCategory tag (hex
///     code 0x14).
///   }
///   \table_row3{   <b>`ListItem.PictureTransmissionReference`</b>,
///                  \anchor ListItem_PictureTransmissionReference
///                  _string_,
///     Shows a code representing the location of original transmission of the
///     selected picture. This is the value of the IPTC TransmissionReference
///     tag (hex code 0x67).
///   }
///   \table_row3{   <b>`ListItem.PictureUrgency`</b>,
///                  \anchor ListItem_PictureUrgency
///                  _string_,
///     Shows the urgency of the selected picture. Values are 1-9. The "1" is
///     most urgent. Some image management programs use urgency to indicate
///     picture rating\, where urgency "1" is 5 stars and urgency "5" is 1 star.
///     Urgencies 6-9 are not used for rating. This is the value of the IPTC
///     Urgency tag (hex code 0x0A).
///   }
///   \table_row3{   <b>`ListItem.PictureWhiteBalance`</b>,
///                  \anchor ListItem_PictureWhiteBalance
///                  _string_,
///     Shows the white balance mode set when the selected picture was taken.
///     The possible values are "Manual" and "Auto". This is the value of the
///     EXIF WhiteBalance tag (hex code 0xA403).
///   }
///   \table_row3{   <b>`ListItem.FileName`</b>,
///                  \anchor ListItem_FileName
///                  _string_,
///     Shows the filename of the currently selected song or movie in a container
///   }
///   \table_row3{   <b>`ListItem.Path`</b>,
///                  \anchor ListItem_Path
///                  _string_,
///     Shows the complete path of the currently selected song or movie in a
///     container
///   }
///   \table_row3{   <b>`ListItem.FolderName`</b>,
///                  \anchor ListItem_FolderName
///                  _string_,
///     Shows top most folder of the path of the currently selected song or
///     movie in a container
///   }
///   \table_row3{   <b>`ListItem.FolderPath`</b>,
///                  \anchor ListItem_FolderPath
///                  _string_,
///     Shows the complete path of the currently selected song or movie in a
///     container (without user details).
///   }
///   \table_row3{   <b>`ListItem.FileNameAndPath`</b>,
///                  \anchor ListItem_FileNameAndPath
///                  _string_,
///     Shows the full path with filename of the currently selected song or
///     movie in a container
///   }
///   \table_row3{   <b>`ListItem.FileExtension`</b>,
///                  \anchor ListItem_FileExtension
///                  _string_,
///     Shows the file extension (without leading dot) of the currently selected
///     item in a container
///   }
///   \table_row3{   <b>`ListItem.Date`</b>,
///                  \anchor ListItem_Date
///                  _string_,
///     Shows the file date of the currently selected song or movie in a
///     container / Aired date of an episode / Day\, start time and end time of
///     current selected TV programme (PVR)
///   }
///   \table_row3{   <b>`ListItem.DateAdded`</b>,
///                  \anchor ListItem_DateAdded
///                  _string_,
///     Shows the date the currently selected item was added to the
///     library / Date and time of an event in the EventLog window.
///   }
///   \table_row3{   <b>`ListItem.Size`</b>,
///                  \anchor ListItem_Size
///                  _string_,
///     Shows the file size of the currently selected song or movie in a
///     container
///   }
///   \table_row3{   <b>`ListItem.Rating`</b>,
///                  \anchor ListItem_Rating
///                  _string_,
///     Shows the IMDB rating of the currently selected movie in a container
///   }
///   \table_row3{   <b>`ListItem.Set`</b>,
///                  \anchor ListItem_Set
///                  _string_,
///     Shows the name of the set the movie is part of
///   }
///   \table_row3{   <b>`ListItem.SetId`</b>,
///                  \anchor ListItem_SetId
///                  _string_,
///     Shows the id of the set the movie is part of
///   }
///   \table_row3{   <b>`ListItem.UserRating`</b>,
///                  \anchor ListItem_UserRating
///                  _string_,
///     Shows the user rating of the currently selected item in a container
///   }
///   \table_row3{   <b>`ListItem.Votes`</b>,
///                  \anchor ListItem_Votes
///                  _string_,
///     Shows the IMDB votes of the currently selected movie in a container
///   }
///   \table_row3{   <b>`ListItem.RatingAndVotes`</b>,
///                  \anchor ListItem_RatingAndVotes
///                  _string_,
///     Shows the IMDB rating and votes of the currently selected movie in a
///     container
///   }
///   \table_row3{   <b>`ListItem.Mood`</b>,
///                  \anchor ListItem_Mood
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`ListItem.Mpaa`</b>,
///                  \anchor ListItem_Mpaa
///                  _string_,
///     Show the MPAA rating of the currently selected movie in a container
///   }
///   \table_row3{   <b>`ListItem.ProgramCount`</b>,
///                  \anchor ListItem_ProgramCount
///                  _string_,
///     Shows the number of times an xbe has been run from "my programs"
///   }
///   \table_row3{   <b>`ListItem.Duration`</b>,
///                  \anchor ListItem_Duration
///                  _string_,
///     Shows the song or movie duration of the currently selected movie in a
///     container
///   }
///   \table_row3{   <b>`ListItem.DBTYPE`</b>,
///                  \anchor ListItem_DBTYPE
///                  _string_,
///     Shows the database type of the ListItem.DBID for videos (movie\, set\,
///     genre\, actor\, tvshow\, season\, episode). It does not return any value
///     for the music library. Beware with season\, the "*all seasons" entry does
///     give a DBTYPE "season" and a DBID\, but you can't get the details of that
///     entry since it's a virtual entry in the Video Library.
///   }
///   \table_row3{   <b>`ListItem.DBID`</b>,
///                  \anchor ListItem_DBID
///                  _string_,
///     Shows the database id of the currently selected listitem in a container
///   }
///   \table_row3{   <b>`ListItem.Cast`</b>,
///                  \anchor ListItem_Cast
///                  _string_,
///     Shows a concatenated string of cast members of the currently selected
///     movie\, for use in dialogvideoinfo.xml
///   }
///   \table_row3{   <b>`ListItem.CastAndRole`</b>,
///                  \anchor ListItem_CastAndRole
///                  _string_,
///     Shows a concatenated string of cast members and roles of the currently
///     selected movie\, for use in dialogvideoinfo.xml
///   }
///   \table_row3{   <b>`ListItem.Studio`</b>,
///                  \anchor ListItem_Studio
///                  _string_,
///     Studio of current selected Music Video in a container
///   }
///   \table_row3{   <b>`ListItem.Top250`</b>,
///                  \anchor ListItem_Top250
///                  _string_,
///     Shows the IMDb top250 position of the currently selected listitem in a
///     container.
///   }
///   \table_row3{   <b>`ListItem.Trailer`</b>,
///                  \anchor ListItem_Trailer
///                  _string_,
///     Shows the full trailer path with filename of the currently selected
///     movie in a container
///   }
///   \table_row3{   <b>`ListItem.Writer`</b>,
///                  \anchor ListItem_Writer
///                  _string_,
///     Name of Writer of current Video in a container
///   }
///   \table_row3{   <b>`ListItem.Tag`</b>,
///                  \anchor ListItem_Tag
///                  _string_,
///     Summary of current Video in a container
///   }
///   \table_row3{   <b>`ListItem.Tagline`</b>,
///                  \anchor ListItem_Tagline
///                  _string_,
///     Small Summary of current Video in a container
///   }
///   \table_row3{   <b>`ListItem.PlotOutline`</b>,
///                  \anchor ListItem_PlotOutline
///                  _string_,
///     Small Summary of current Video in a container
///   }
///   \table_row3{   <b>`ListItem.Plot`</b>,
///                  \anchor ListItem_Plot
///                  _string_,
///     Complete Text Summary of Video in a container
///   }
///   \table_row3{   <b>`ListItem.IMDBNumber`</b>,
///                  \anchor ListItem_IMDBNumber
///                  _string_,
///     The IMDB iD of the selected Video in a container
///   }
///   \table_row3{   <b>`ListItem.EpisodeName`</b>,
///                  \anchor ListItem_EpisodeName
///                  _string_,
///     (PVR only) The name of the episode if the selected EPG item is a TV Show
///   }
///   \table_row3{   <b>`ListItem.PercentPlayed`</b>,
///                  \anchor ListItem_PercentPlayed
///                  _string_,
///     Returns percentage value [0-100] of how far the selected video has been
///     played
///   }
///   \table_row3{   <b>`ListItem.LastPlayed`</b>,
///                  \anchor ListItem_LastPlayed
///                  _string_,
///     Last play date of Video in a container
///   }
///   \table_row3{   <b>`ListItem.PlayCount`</b>,
///                  \anchor ListItem_PlayCount
///                  _string_,
///     Playcount of Video in a container
///   }
///   \table_row3{   <b>`ListItem.ChannelNumber`</b>,
///                  \anchor ListItem_ChannelNumber
///                  _string_,
///     Number of current selected TV channel in a container
///   }
///   \table_row3{   <b>`ListItem.ChannelName`</b>,
///                  \anchor ListItem_ChannelName
///                  _string_,
///     Name of current selected TV channel in a container
///   }
///   \table_row3{   <b>`ListItem.VideoCodec`</b>,
///                  \anchor ListItem_VideoCodec
///                  _string_,
///     Shows the video codec of the currently selected video (common values:
///     3iv2\, avc1\, div2\, div3\, divx\, divx 4\, dx50\, flv\, h264\, microsoft\, mp42\,
///     mp43\, mp4v\, mpeg1video\, mpeg2video\, mpg4\, rv40\, svq1\, svq3\,
///     theora\, vp6f\, wmv2\, wmv3\, wvc1\, xvid)
///   }
///   \table_row3{   <b>`ListItem.VideoResolution`</b>,
///                  \anchor ListItem_VideoResolution
///                  _string_,
///     Shows the resolution of the currently selected video (possible values:
///     480\, 576\, 540\, 720\, 1080\, 4K). Note that 540 usually means a widescreen
///     format (around 960x540) while 576 means PAL resolutions (normally
///     720x576)\, therefore 540 is actually better resolution than 576.
///   }
///   \table_row3{   <b>`ListItem.VideoAspect`</b>,
///                  \anchor ListItem_VideoAspect
///                  _string_,
///     Shows the aspect ratio of the currently selected video (possible values:
///     1.33\, 1.37\, 1.66\, 1.78\, 1.85\, 2.20\, 2.35\, 2.40\, 2.55\, 2.76)
///   }
///   \table_row3{   <b>`ListItem.AudioCodec`</b>,
///                  \anchor ListItem_AudioCodec
///                  _string_,
///     Shows the audio codec of the currently selected video (common values:
///     aac\, ac3\, cook\, dca\, dtshd_hra\, dtshd_ma\, eac3\, mp1\, mp2\, mp3\, pcm_s16be\, pcm_s16le\, pcm_u8\, truehd\, vorbis\, wmapro\, wmav2)
///   }
///   \table_row3{   <b>`ListItem.AudioChannels`</b>,
///                  \anchor ListItem_AudioChannels
///                  _string_,
///     Shows the number of audio channels of the currently selected video
///     (possible values: 1\, 2\, 4\, 5\, 6\, 8\, 10)
///   }
///   \table_row3{   <b>`ListItem.AudioLanguage`</b>,
///                  \anchor ListItem_AudioLanguage
///                  _string_,
///     Shows the audio language of the currently selected video (returns an
///     ISO 639-2 three character code\, e.g. eng\, epo\, deu)
///   }
///   \table_row3{   <b>`ListItem.SubtitleLanguage`</b>,
///                  \anchor ListItem_SubtitleLanguage
///                  _string_,
///     Shows the subtitle language of the currently selected video (returns an
///     ISO 639-2 three character code\, e.g. eng\, epo\, deu)
///   }
///   \table_row3{   <b>`ListItem.Property(AudioCodec.[n])`</b>,
///                  \anchor ListItem_Property_AudioCodec
///                  _string_,
///     Shows the audio codec of the currently selected video\, 'n' defines the
///     number of the audiostream (values: see \ref ListItem_AudioCodec "ListItem.AudioCodec")
///   }
///   \table_row3{   <b>`ListItem.Property(AudioChannels.[n])`</b>,
///                  \anchor ListItem_Property_AudioChannels
///                  _string_,
///     Shows the number of audio channels of the currently selected video\, 'n'
///     defines the number of the audiostream (values: see
///     \ref ListItem_AudioChannels "ListItem.AudioChannels")
///   }
///   \table_row3{   <b>`ListItem.Property(AudioLanguage.[n])`</b>,
///                  \anchor ListItem_Property_AudioLanguage
///                  _string_,
///     Shows the audio language of the currently selected video\, 'n' defines
///     the number of the audiostream (values: see \ref ListItem_AudioLanguage "ListItem.AudioLanguage")
///   }
///   \table_row3{   <b>`ListItem.Property(SubtitleLanguage.[n])`</b>,
///                  \anchor ListItem_Property_SubtitleLanguage
///                  _string_,
///     Shows the subtitle language of the currently selected video\, 'n' defines
///     the number of the subtitle (values: see \ref ListItem_SubtitleLanguage "ListItem.SubtitleLanguage")
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Name)`</b>,
///                  \anchor ListItem_Property_AddonName
///                  _string_,
///     Shows the name of the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Version)`</b>,
///                  \anchor ListItem_Property_AddonVersion
///                  _string_,
///     Shows the version of the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Summary)`</b>,
///                  \anchor ListItem_Property_AddonSummary
///                  _string_,
///     Shows a short description of the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Description)`</b>,
///                  \anchor ListItem_Property_AddonDescription
///                  _string_,
///     Shows the full description of the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Type)`</b>,
///                  \anchor ListItem_Property_AddonType
///                  _string_,
///     Shows the type (screensaver\, script\, skin\, etc...) of the currently
///     selected addon
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Creator)`</b>,
///                  \anchor ListItem_Property_AddonCreator
///                  _string_,
///     Shows the name of the author the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Disclaimer)`</b>,
///                  \anchor ListItem_Property_AddonDisclaimer
///                  _string_,
///     Shows the disclaimer of the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Changelog)`</b>,
///                  \anchor ListItem_Property_AddonChangelog
///                  _string_,
///     Shows the changelog of the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.ID)`</b>,
///                  \anchor ListItem_Property_AddonID
///                  _string_,
///     Shows the identifier of the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Status)`</b>,
///                  \anchor ListItem_Property_AddonStatus
///                  _string_,
///     Shows the status of the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Broken)`</b>,
///                  \anchor ListItem_Property_AddonBroken
///                  _string_,
///     Shows a message when the addon is marked as broken in the repo
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Path)`</b>,
///                  \anchor ListItem_Property_AddonPath
///                  _string_,
///     Shows the path of the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.StartTime`</b>,
///                  \anchor ListItem_StartTime
///                  _string_,
///     Start time of current selected TV programme in a container
///   }
///   \table_row3{   <b>`ListItem.EndTime`</b>,
///                  \anchor ListItem_EndTime
///                  _string_,
///     End time of current selected TV programme in a container
///   }
///   \table_row3{   <b>`ListItem.StartDate`</b>,
///                  \anchor ListItem_StartDate
///                  _string_,
///     Start date of current selected TV programme in a container
///   }
///   \table_row3{   <b>`ListItem.EndDate`</b>,
///                  \anchor ListItem_EndDate
///                  _string_,
///     End date of current selected TV programme in a container
///   }
///   \table_row3{   <b>`ListItem.NextTitle`</b>,
///                  \anchor ListItem_NextTitle
///                  _string_,
///     Title of the next item (PVR).
///   }
///   \table_row3{   <b>`ListItem.NextGenre`</b>,
///                  \anchor ListItem_NextGenre
///                  _string_,
///     Genre of the next item (PVR).
///   }
///   \table_row3{   <b>`ListItem.NextPlot`</b>,
///                  \anchor ListItem_NextPlot
///                  _string_,
///     Plot of the next item (PVR).
///   }
///   \table_row3{   <b>`ListItem.NextPlotOutline`</b>,
///                  \anchor ListItem_NextPlotOutline
///                  _string_,
///     Plot outline of the next item (PVR).
///   }
///   \table_row3{   <b>`ListItem.NextStartTime`</b>,
///                  \anchor ListItem_NextStartTime
///                  _string_,
///     Start time of the next item (PVR).
///   }
///   \table_row3{   <b>`ListItem.NextEndTime`</b>,
///                  \anchor ListItem_NextEndTime
///                  _string_,
///     End of the next item (PVR).
///   }
///   \table_row3{   <b>`ListItem.NextStartDate`</b>,
///                  \anchor ListItem_NextStartDate
///                  _string_,
///     Start date of the next item (PVR).
///   }
///   \table_row3{   <b>`ListItem.NextEndDate`</b>,
///                  \anchor ListItem_NextEndDate
///                  _string_,
///     End date of the next item (PVR).
///   }
///   \table_row3{   <b>`ListItem.ChannelGroup`</b>,
///                  \anchor ListItem_ChannelGroup
///                  _string_,
///     Channel group of the selected item (PVR).
///   }
///   \table_row3{   <b>`ListItem.SubChannelNumber`</b>,
///                  \anchor ListItem_SubChannelNumber
///                  _string_,
///     Subchannel number of the currently selected channel that's currently
///     playing (PVR).
///   }
///   \table_row3{   <b>`ListItem.ChannelNumberLabel`</b>,
///                  \anchor ListItem_ChannelNumberLabel
///                  _string_,
///     Channel and subchannel number of the currently selected channel that's
///     currently playing (PVR).
///   }
///   \table_row3{   <b>`ListItem.Progress`</b>,
///                  \anchor ListItem_Progress
///                  _string_,
///     Part of the programme that's been played (PVR).
///   }
///   \table_row3{   <b>`ListItem.StereoscopicMode`</b>,
///                  \anchor ListItem_StereoscopicMode
///                  _string_,
///     Returns the stereomode of the selected video (i.e. mono\,
///     split_vertical\, split_horizontal\, row_interleaved\,
///     anaglyph_cyan_red\, anaglyph_green_magenta)
///   }
///   \table_row3{   <b>`ListItem.HasTimerSchedule`</b>,
///                  \anchor ListItem_HasTimerSchedule
///                  _boolean_,
///     Whether the item was scheduled by a timer rule (PVR). (v16 addition)
///   }
///   \table_row3{   <b>`ListItem.HasRecording`</b>,
///                  \anchor ListItem_HasRecording
///                  _boolean_,
///     Todo
///   }
///   \table_row3{   <b>`ListItem.TimerHasError`</b>,
///                  \anchor ListItem_TimerHasError
///                  _boolean_,
///     Whether the item has a timer and it won't be recorded because of an error (PVR). (v17 addition)
///   }
///   \table_row3{   <b>`ListItem.TimerHasConflict`</b>,
///                  \anchor ListItem_TimerHasConflict
///                  _boolean_,
///     Whether the item has a timer and it won't be recorded because of a conflict (PVR). (v17 addition)
///   }
///   \table_row3{   <b>`ListItem.TimerIsActive`</b>,
///                  \anchor ListItem_TimerIsActive
///                  _boolean_,
///     Whether the item has a timer that will be recorded\, i.e. the timer is enabled (PVR). (v17 addition)
///   }
///   \table_row3{   <b>`ListItem.Comment`</b>,
///                  \anchor ListItem_Comment
///                  _string_,
///     Comment assigned to the item (PVR/MUSIC).
///   }
///   \table_row3{   <b>`ListItem.TimerType`</b>,
///                  \anchor ListItem_TimerType
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`ListItem.EpgEventTitle`</b>,
///                  \anchor ListItem_EpgEventTitle
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`ListItem.InProgress`</b>,
///                  \anchor ListItem_InProgress
///                  _boolean_,
///     Todo
///   }
///   \table_row3{   <b>`ListItem.IsParentFolder`</b>,
///                  \anchor ListItem_IsParentFolder
///                  _boolean_,
///     Todo
///   }
///   \table_row3{   <b>`ListItem.AddonName`</b>,
///                  \anchor ListItem_AddonName
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`ListItem.AddonVersion`</b>,
///                  \anchor ListItem_AddonVersion
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`ListItem.AddonCreator`</b>,
///                  \anchor ListItem_AddonCreator
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`ListItem.AddonSummary`</b>,
///                  \anchor ListItem_AddonSummary
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`ListItem.AddonDescription`</b>,
///                  \anchor ListItem_AddonDescription
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`ListItem.AddonDisclaimer`</b>,
///                  \anchor ListItem_AddonDisclaimer
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`ListItem.AddonBroken`</b>,
///                  \anchor ListItem_AddonBroken
///                  _boolean_,
///     Todo
///   }
///   \table_row3{   <b>`ListItem.AddonType`</b>,
///                  \anchor ListItem_AddonType
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`ListItem.AddonInstallDate`</b>,
///                  \anchor ListItem_AddonInstallDate
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`ListItem.AddonLastUpdated`</b>,
///                  \anchor ListItem_AddonLastUpdated
///                  _string_,
///     Todo
///   }
///   \table_row3{   <b>`ListItem.AddonLastUsed`</b>,
///                  \anchor ListItem_AddonLastUsed
///                  _string_,
///     Todo
///   }
//    \table_row3{   <b>`ListItem.AddonOrigin`</b>,
///                  \anchor ListItem_AddonOrigin
///                  _string_,
///     Name of the repository the add-on originates from.
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}
const infomap listitem_labels[]= {{ "thumb",            LISTITEM_THUMB },
                                  { "icon",             LISTITEM_ICON },
                                  { "actualicon",       LISTITEM_ACTUAL_ICON },
                                  { "overlay",          LISTITEM_OVERLAY },
                                  { "label",            LISTITEM_LABEL },
                                  { "label2",           LISTITEM_LABEL2 },
                                  { "title",            LISTITEM_TITLE },
                                  { "tracknumber",      LISTITEM_TRACKNUMBER },
                                  { "artist",           LISTITEM_ARTIST },
                                  { "album",            LISTITEM_ALBUM },
                                  { "albumartist",      LISTITEM_ALBUM_ARTIST },
                                  { "year",             LISTITEM_YEAR },
                                  { "genre",            LISTITEM_GENRE },
                                  { "contributors",     LISTITEM_CONTRIBUTORS },
                                  { "contributorandrole", LISTITEM_CONTRIBUTOR_AND_ROLE },
                                  { "director",         LISTITEM_DIRECTOR },
                                  { "filename",         LISTITEM_FILENAME },
                                  { "filenameandpath",  LISTITEM_FILENAME_AND_PATH },
                                  { "fileextension",    LISTITEM_FILE_EXTENSION },
                                  { "date",             LISTITEM_DATE },
                                  { "datetime",         LISTITEM_DATETIME },
                                  { "size",             LISTITEM_SIZE },
                                  { "rating",           LISTITEM_RATING },
                                  { "ratingandvotes",   LISTITEM_RATING_AND_VOTES },
                                  { "userrating",       LISTITEM_USER_RATING },
                                  { "votes",            LISTITEM_VOTES },
                                  { "mood",             LISTITEM_MOOD },
                                  { "programcount",     LISTITEM_PROGRAM_COUNT },
                                  { "duration",         LISTITEM_DURATION },
                                  { "isselected",       LISTITEM_ISSELECTED },
                                  { "isplaying",        LISTITEM_ISPLAYING },
                                  { "plot",             LISTITEM_PLOT },
                                  { "plotoutline",      LISTITEM_PLOT_OUTLINE },
                                  { "episode",          LISTITEM_EPISODE },
                                  { "season",           LISTITEM_SEASON },
                                  { "tvshowtitle",      LISTITEM_TVSHOW },
                                  { "premiered",        LISTITEM_PREMIERED },
                                  { "comment",          LISTITEM_COMMENT },
                                  { "path",             LISTITEM_PATH },
                                  { "foldername",       LISTITEM_FOLDERNAME },
                                  { "folderpath",       LISTITEM_FOLDERPATH },
                                  { "picturepath",      LISTITEM_PICTURE_PATH },
                                  { "pictureresolution",LISTITEM_PICTURE_RESOLUTION },
                                  { "picturedatetime",  LISTITEM_PICTURE_DATETIME },
                                  { "picturedate",      LISTITEM_PICTURE_DATE },
                                  { "picturelongdatetime",LISTITEM_PICTURE_LONGDATETIME },
                                  { "picturelongdate",  LISTITEM_PICTURE_LONGDATE },
                                  { "picturecomment",   LISTITEM_PICTURE_COMMENT },
                                  { "picturecaption",   LISTITEM_PICTURE_CAPTION },
                                  { "picturedesc",      LISTITEM_PICTURE_DESC },
                                  { "picturekeywords",  LISTITEM_PICTURE_KEYWORDS },
                                  { "picturecammake",   LISTITEM_PICTURE_CAM_MAKE },
                                  { "picturecammodel",  LISTITEM_PICTURE_CAM_MODEL },
                                  { "pictureaperture",  LISTITEM_PICTURE_APERTURE },
                                  { "picturefocallen",  LISTITEM_PICTURE_FOCAL_LEN },
                                  { "picturefocusdist", LISTITEM_PICTURE_FOCUS_DIST },
                                  { "pictureexpmode",   LISTITEM_PICTURE_EXP_MODE },
                                  { "pictureexptime",   LISTITEM_PICTURE_EXP_TIME },
                                  { "pictureiso",       LISTITEM_PICTURE_ISO },
                                  { "pictureauthor",                 LISTITEM_PICTURE_AUTHOR },
                                  { "picturebyline",                 LISTITEM_PICTURE_BYLINE },
                                  { "picturebylinetitle",            LISTITEM_PICTURE_BYLINE_TITLE },
                                  { "picturecategory",               LISTITEM_PICTURE_CATEGORY },
                                  { "pictureccdwidth",               LISTITEM_PICTURE_CCD_WIDTH },
                                  { "picturecity",                   LISTITEM_PICTURE_CITY },
                                  { "pictureurgency",                LISTITEM_PICTURE_URGENCY },
                                  { "picturecopyrightnotice",        LISTITEM_PICTURE_COPYRIGHT_NOTICE },
                                  { "picturecountry",                LISTITEM_PICTURE_COUNTRY },
                                  { "picturecountrycode",            LISTITEM_PICTURE_COUNTRY_CODE },
                                  { "picturecredit",                 LISTITEM_PICTURE_CREDIT },
                                  { "pictureiptcdate",               LISTITEM_PICTURE_IPTCDATE },
                                  { "picturedigitalzoom",            LISTITEM_PICTURE_DIGITAL_ZOOM },
                                  { "pictureexposure",               LISTITEM_PICTURE_EXPOSURE },
                                  { "pictureexposurebias",           LISTITEM_PICTURE_EXPOSURE_BIAS },
                                  { "pictureflashused",              LISTITEM_PICTURE_FLASH_USED },
                                  { "pictureheadline",               LISTITEM_PICTURE_HEADLINE },
                                  { "picturecolour",                 LISTITEM_PICTURE_COLOUR },
                                  { "picturelightsource",            LISTITEM_PICTURE_LIGHT_SOURCE },
                                  { "picturemeteringmode",           LISTITEM_PICTURE_METERING_MODE },
                                  { "pictureobjectname",             LISTITEM_PICTURE_OBJECT_NAME },
                                  { "pictureorientation",            LISTITEM_PICTURE_ORIENTATION },
                                  { "pictureprocess",                LISTITEM_PICTURE_PROCESS },
                                  { "picturereferenceservice",       LISTITEM_PICTURE_REF_SERVICE },
                                  { "picturesource",                 LISTITEM_PICTURE_SOURCE },
                                  { "picturespecialinstructions",    LISTITEM_PICTURE_SPEC_INSTR },
                                  { "picturestate",                  LISTITEM_PICTURE_STATE },
                                  { "picturesupplementalcategories", LISTITEM_PICTURE_SUP_CATEGORIES },
                                  { "picturetransmissionreference",  LISTITEM_PICTURE_TX_REFERENCE },
                                  { "picturewhitebalance",           LISTITEM_PICTURE_WHITE_BALANCE },
                                  { "pictureimagetype",              LISTITEM_PICTURE_IMAGETYPE },
                                  { "picturesublocation",            LISTITEM_PICTURE_SUBLOCATION },
                                  { "pictureiptctime",               LISTITEM_PICTURE_TIMECREATED },
                                  { "picturegpslat",    LISTITEM_PICTURE_GPS_LAT },
                                  { "picturegpslon",    LISTITEM_PICTURE_GPS_LON },
                                  { "picturegpsalt",    LISTITEM_PICTURE_GPS_ALT },
                                  { "studio",           LISTITEM_STUDIO },
                                  { "country",          LISTITEM_COUNTRY },
                                  { "mpaa",             LISTITEM_MPAA },
                                  { "cast",             LISTITEM_CAST },
                                  { "castandrole",      LISTITEM_CAST_AND_ROLE },
                                  { "writer",           LISTITEM_WRITER },
                                  { "tagline",          LISTITEM_TAGLINE },
                                  { "status",           LISTITEM_STATUS },
                                  { "top250",           LISTITEM_TOP250 },
                                  { "trailer",          LISTITEM_TRAILER },
                                  { "sortletter",       LISTITEM_SORT_LETTER },
                                  { "tag",              LISTITEM_TAG },
                                  { "set",              LISTITEM_SET },
                                  { "setid",            LISTITEM_SETID },
                                  { "videocodec",       LISTITEM_VIDEO_CODEC },
                                  { "videoresolution",  LISTITEM_VIDEO_RESOLUTION },
                                  { "videoaspect",      LISTITEM_VIDEO_ASPECT },
                                  { "audiocodec",       LISTITEM_AUDIO_CODEC },
                                  { "audiochannels",    LISTITEM_AUDIO_CHANNELS },
                                  { "audiolanguage",    LISTITEM_AUDIO_LANGUAGE },
                                  { "subtitlelanguage", LISTITEM_SUBTITLE_LANGUAGE },
                                  { "isresumable",      LISTITEM_IS_RESUMABLE},
                                  { "percentplayed",    LISTITEM_PERCENT_PLAYED},
                                  { "isfolder",         LISTITEM_IS_FOLDER },
                                  { "isparentfolder",   LISTITEM_IS_PARENTFOLDER },
                                  { "iscollection",     LISTITEM_IS_COLLECTION },
                                  { "originaltitle",    LISTITEM_ORIGINALTITLE },
                                  { "lastplayed",       LISTITEM_LASTPLAYED },
                                  { "playcount",        LISTITEM_PLAYCOUNT },
                                  { "discnumber",       LISTITEM_DISC_NUMBER },
                                  { "starttime",        LISTITEM_STARTTIME },
                                  { "endtime",          LISTITEM_ENDTIME },
                                  { "endtimeresume",    LISTITEM_ENDTIME_RESUME },
                                  { "startdate",        LISTITEM_STARTDATE },
                                  { "enddate",          LISTITEM_ENDDATE },
                                  { "nexttitle",        LISTITEM_NEXT_TITLE },
                                  { "nextgenre",        LISTITEM_NEXT_GENRE },
                                  { "nextplot",         LISTITEM_NEXT_PLOT },
                                  { "nextplotoutline",  LISTITEM_NEXT_PLOT_OUTLINE },
                                  { "nextstarttime",    LISTITEM_NEXT_STARTTIME },
                                  { "nextendtime",      LISTITEM_NEXT_ENDTIME },
                                  { "nextstartdate",    LISTITEM_NEXT_STARTDATE },
                                  { "nextenddate",      LISTITEM_NEXT_ENDDATE },
                                  { "channelname",      LISTITEM_CHANNEL_NAME },
                                  { "channelnumber",    LISTITEM_CHANNEL_NUMBER },
                                  { "subchannelnumber", LISTITEM_SUB_CHANNEL_NUMBER },
                                  { "channelnumberlabel", LISTITEM_CHANNEL_NUMBER_LBL },
                                  { "channelgroup",     LISTITEM_CHANNEL_GROUP },
                                  { "hasepg",           LISTITEM_HAS_EPG },
                                  { "hastimer",         LISTITEM_HASTIMER },
                                  { "hastimerschedule", LISTITEM_HASTIMERSCHEDULE },
                                  { "hasrecording",     LISTITEM_HASRECORDING },
                                  { "isrecording",      LISTITEM_ISRECORDING },
                                  { "inprogress",       LISTITEM_INPROGRESS },
                                  { "isencrypted",      LISTITEM_ISENCRYPTED },
                                  { "progress",         LISTITEM_PROGRESS },
                                  { "dateadded",        LISTITEM_DATE_ADDED },
                                  { "dbtype",           LISTITEM_DBTYPE },
                                  { "dbid",             LISTITEM_DBID },
                                  { "stereoscopicmode", LISTITEM_STEREOSCOPIC_MODE },
                                  { "isstereoscopic",   LISTITEM_IS_STEREOSCOPIC },
                                  { "imdbnumber",       LISTITEM_IMDBNUMBER },
                                  { "episodename",      LISTITEM_EPISODENAME },
                                  { "timertype",        LISTITEM_TIMERTYPE },
                                  { "epgeventtitle",    LISTITEM_EPG_EVENT_TITLE },
                                  { "timerisactive",    LISTITEM_TIMERISACTIVE },
                                  { "timerhaserror",    LISTITEM_TIMERHASERROR },
                                  { "timerhasconflict", LISTITEM_TIMERHASCONFLICT },
                                  { "addonname",        LISTITEM_ADDON_NAME },
                                  { "addonversion",     LISTITEM_ADDON_VERSION },
                                  { "addoncreator",     LISTITEM_ADDON_CREATOR },
                                  { "addonsummary",     LISTITEM_ADDON_SUMMARY },
                                  { "addondescription", LISTITEM_ADDON_DESCRIPTION },
                                  { "addondisclaimer",  LISTITEM_ADDON_DISCLAIMER },
                                  { "addonnews",        LISTITEM_ADDON_NEWS },
                                  { "addonbroken",      LISTITEM_ADDON_BROKEN },
                                  { "addontype",        LISTITEM_ADDON_TYPE },
                                  { "addoninstalldate", LISTITEM_ADDON_INSTALL_DATE },
                                  { "addonlastupdated", LISTITEM_ADDON_LAST_UPDATED },
                                  { "addonlastused",    LISTITEM_ADDON_LAST_USED },
                                  { "addonorigin",      LISTITEM_ADDON_ORIGIN },
                                  { "addonsize",        LISTITEM_ADDON_SIZE },
};

/// \page modules__General__List_of_gui_access
/// \section modules__General__List_of_gui_access_Visualisation Visualisation
/// @{
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Visualisation.Enabled`</b>,
///                  \anchor Visualisation_Enabled
///                  _boolean_,
///     Returns true if any visualisation has been set in settings (so not None).
///   }
///   \table_row3{   <b>`Visualisation.HasPresets`</b>,
///                  \anchor Visualisation_HasPresets
///                  _boolean_,
///     Returns true if the visualisation has built in presets.
///   }
///   \table_row3{   <b>`Visualisation.Locked`</b>,
///                  \anchor Visualisation_Locked
///                  _boolean_,
///     Returns true if the current visualisation preset is locked (eg in Milkdrop.)
///   }
///   \table_row3{   <b>`Visualisation.Preset`</b>,
///                  \anchor Visualisation_Preset
///                  _string_,
///     Shows the current preset of the visualisation.
///   }
///   \table_row3{   <b>`Visualisation.Name`</b>,
///                  \anchor Visualisation_Name
///                  _string_,
///     Shows the name of the visualisation.
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}
const infomap visualisation[] =  {{ "locked",           VISUALISATION_LOCKED },
                                  { "preset",           VISUALISATION_PRESET },
                                  { "haspresets",       VISUALISATION_HAS_PRESETS },
                                  { "name",             VISUALISATION_NAME },
                                  { "enabled",          VISUALISATION_ENABLED }};

/// \page modules__General__List_of_gui_access
/// \section modules__General__List_of_gui_access_Fanart Fanart
/// @{
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Fanart.Color1`</b>,
///                  \anchor Fanart_Color1
///                  _string_,
///     Returns the first of three colors included in the currently selected
///     Fanart theme for the parent TV Show. Colors are arranged Lightest to
///     Darkest.
///   }
///   \table_row3{   <b>`Fanart.Color2`</b>,
///                  \anchor Fanart_Color2
///                  _string_,
///     Returns the second of three colors included in the currently selected
///     Fanart theme for the parent TV Show. Colors are arranged Lightest to
///     Darkest.
///   }
///   \table_row3{   <b>`Fanart.Color3`</b>,
///                  \anchor Fanart_Color3
///                  _string_,
///     Returns the third of three colors included in the currently selected
///     Fanart theme for the parent TV Show. Colors are arranged Lightest to
///     Darkest.
///   }
///   \table_row3{   <b>`Fanart.Image`</b>,
///                  \anchor Fanart_Image
///                  _string_,
///     todo
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}
const infomap fanart_labels[] =  {{ "color1",           FANART_COLOR1 },
                                  { "color2",           FANART_COLOR2 },
                                  { "color3",           FANART_COLOR3 },
                                  { "image",            FANART_IMAGE }};

/// \page modules__General__List_of_gui_access
/// \section modules__General__List_of_gui_access_Skin Skin
/// @{
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Skin.CurrentTheme`</b>,
///                  \anchor Skin_CurrentTheme
///                  _boolean_,
///     todo
///   }
///   \table_row3{   <b>`Skin.CurrentColourTheme`</b>,
///                  \anchor Skin_CurrentColourTheme
///                  _boolean_,
///     todo
///   }
///   \table_row3{   <b>`Skin.AspectRatio`</b>,
///                  \anchor Skin_AspectRatio
///                  _boolean_,
///     todo
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}
const infomap skin_labels[] =    {{ "currenttheme",     SKIN_THEME },
                                  { "currentcolourtheme",SKIN_COLOUR_THEME },
                                  {"aspectratio",       SKIN_ASPECT_RATIO}};

/// \page modules__General__List_of_gui_access
/// \section modules__General__List_of_gui_access_Window Window
/// @{
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Window.IsMedia`</b>,
///                  \anchor Window_IsMedia
///                  _boolean_,
///     Returns true if this window is a media window (programs\, music\, video\,
///     scripts\, pictures)
///   }
///   \table_row3{   <b>`Window.Is(window)`</b>,
///                  \anchor Window_Is
///                  _boolean_,
///     todo
///   }
///   \table_row3{   <b>`Window.IsActive(window)`</b>,
///                  \anchor Window_IsActive
///                  _boolean_,
///     Returns true if the window with id or title _window_ is active (excludes
///     fade out time on dialogs) \ref modules__General__Window_IDs "See here for a list of windows"
///   }
///   \table_row3{   <b>`Window.IsTopMost(window)`</b>,
///                  \anchor Window_IsTopMost
///                  _boolean_,
///     Returns true if the window with id or title _window_ is on top of the
///     window stack (excludes fade out time on dialogs)
///     \ref modules__General__Window_IDs "See here for a list of windows"
///   }
///   \table_row3{   <b>`Window.IsVisible(window)`</b>,
///                  \anchor Window_IsVisible
///                  _boolean_,
///     Returns true if the window is visible (includes fade out time on dialogs)
///   }
///   \table_row3{   <b>`Window.Previous(window)`</b>,
///                  \anchor Window_Previous
///                  _boolean_,
///     Returns true if the window with id or title _window_ is being moved from.
///     \ref modules__General__Window_IDs "See here for a list of windows". Only
///     valid while windows are changing.
///   }
///   \table_row3{   <b>`Window.Next(window)`</b>,
///                  \anchor Window_Next
///                  _boolean_,
///     Returns true if the window with id or title _window_ is being moved to.
///     \ref modules__General__Window_IDs "See here for a list of windows". Only
///     valid while windows are changing.
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}
const infomap window_bools[] =   {{ "ismedia",          WINDOW_IS_MEDIA },
                                  { "is",               WINDOW_IS },
                                  { "isactive",         WINDOW_IS_ACTIVE },
                                  { "istopmost",        WINDOW_IS_TOPMOST },
                                  { "isvisible",        WINDOW_IS_VISIBLE },
                                  { "previous",         WINDOW_PREVIOUS },
                                  { "next",             WINDOW_NEXT }};

/// \page modules__General__List_of_gui_access
/// \section modules__General__List_of_gui_access_Control Control
/// @{
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Control.HasFocus(id)`</b>,
///                  \anchor Control_HasFocus
///                  _boolean_,
///     Returns true if the currently focused control has id "id".
///   }
///   \table_row3{   <b>`Control.IsVisible(id)`</b>,
///                  \anchor Control_IsVisible
///                  _boolean_,
///     Returns true if the control with id "id" is visible.
///   }
///   \table_row3{   <b>`Control.IsEnabled(id)`</b>,
///                  \anchor Control_IsEnabled
///                  _boolean_,
///     Returns true if the control with id "id" is enabled.
///   }
///   \table_row3{   <b>`Control.GetLabel(id)[.index()]`</b>,
///                  \anchor Control_GetLabel
///                  _string_,
///     Returns the label value or texture name of the control with the given id.
///     Optionally you can specify index(1) to retrieve label2 from an Edit
///     control.
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}
const infomap control_labels[] = {{ "hasfocus",         CONTROL_HAS_FOCUS },
                                  { "isvisible",        CONTROL_IS_VISIBLE },
                                  { "isenabled",        CONTROL_IS_ENABLED },
                                  { "getlabel",         CONTROL_GET_LABEL }};

/// \page modules__General__List_of_gui_access
/// \section modules__General__List_of_gui_access_Playlist Playlist
/// @{
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Playlist.Length(media)`</b>,
///                  \anchor Playlist_Length
///                  _integer_,
///     Total size of the current playlist. optional parameter media is either
///     video or music.
///   }
///   \table_row3{   <b>`Playlist.Position(media)`</b>,
///                  \anchor Playlist_Position
///                  _integer_,
///     Position of the current item in the current playlist. optional parameter
///     media is either video or music.
///   }
///   \table_row3{   <b>`Playlist.Random`</b>,
///                  \anchor Playlist_Random
///                  _integer_,
///     Returns string ID's 590 (Randomize Play Enabled) or 591 (Disabled)
///   }
///   \table_row3{   <b>`Playlist.Repeat`</b>,
///                  \anchor Playlist_Repeat
///                  _integer_,
///     Returns string ID's 592 (Repeat One)\, 593 (Repeat All)\, or 594 (Repeat Off)
///   }
///   \table_row3{   <b>`Playlist.IsRandom`</b>,
///                  \anchor Playlist_IsRandom
///                  _boolean_,
///     Returns true if the player is in random mode.
///   }
///   \table_row3{   <b>`Playlist.IsRepeat`</b>,
///                  \anchor Playlist_IsRepeat
///                  _boolean_,
///     Returns true if the player is in repeat all mode.
///   }
///   \table_row3{   <b>`Playlist.IsRepeatOne`</b>,
///                  \anchor Playlist_IsRepeatOne
///                  _boolean_,
///     Returns true if the player is in repeat one mode.
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}
const infomap playlist[] =       {{ "length",           PLAYLIST_LENGTH },
                                  { "position",         PLAYLIST_POSITION },
                                  { "random",           PLAYLIST_RANDOM },
                                  { "repeat",           PLAYLIST_REPEAT },
                                  { "israndom",         PLAYLIST_ISRANDOM },
                                  { "isrepeat",         PLAYLIST_ISREPEAT },
                                  { "isrepeatone",      PLAYLIST_ISREPEATONE }};

/// \page modules__General__List_of_gui_access
/// \section modules__General__List_of_gui_access_Pvr Pvr
/// @{
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Pvr.IsRecording`</b>,
///                  \anchor Pvr_IsRecording
///                  _boolean_,
///     Returns true when the system is recording a tv programme.
///   }
///   \table_row3{   <b>`Pvr.HasTimer`</b>,
///                  \anchor Pvr_HasTimer
///                  _boolean_,
///     Returns true when a recording timer is active.
///   }
///   \table_row3{   <b>`Pvr.HasTVChannels`</b>,
///                  \anchor Pvr_HasTVChannels
///                  _boolean_,
///     Returns true if there are TV channels available
///   }
///   \table_row3{   <b>`Pvr.HasRadioChannels`</b>,
///                  \anchor Pvr_HasRadioChannels
///                  _boolean_,
///     Returns true if there are radio channels available
///   }
///   \table_row3{   <b>`Pvr.HasNonRecordingTimer`</b>,
///                  \anchor Pvr_HasNonRecordingTimer
///                  _boolean_,
///     Returns true if there are timers present who currently not do recording
///   }
///   \table_row3{   <b>`Pvr.NowRecordingTitle`</b>,
///                  \anchor Pvr_NowRecordingTitle
///                  _string_,
///     Title of the programme being recorded
///   }
///   \table_row3{   <b>`Pvr.NowRecordingDateTime`</b>,
///                  \anchor Pvr_NowRecordingDateTime
///                  _Date/Time string_,
///     Start date and time of the current recording
///   }
///   \table_row3{   <b>`Pvr.NowRecordingChannel`</b>,
///                  \anchor Pvr_NowRecordingChannel
///                  _string_,
///     Channel number that's being recorded
///   }
///   \table_row3{   <b>`Pvr.NowRecordingChannelIcon`</b>,
///                  \anchor Pvr_NowRecordingChannelIcon
///                  _path_,
///     Icon of the current recording channel
///   }
///   \table_row3{   <b>`Pvr.NextRecordingTitle`</b>,
///                  \anchor Pvr_NextRecordingTitle
///                  _string_,
///     Title of the next programme that will be recorded
///   }
///   \table_row3{   <b>`Pvr.NextRecordingDateTime`</b>,
///                  \anchor Pvr_NextRecordingDateTime
///                  _Date/Time string_,
///     Start date and time of the next recording
///   }
///   \table_row3{   <b>`Pvr.NextRecordingChannel`</b>,
///                  \anchor Pvr_NextRecordingChannel
///                  _string_,
///     Channel name of the next recording
///   }
///   \table_row3{   <b>`Pvr.NextRecordingChannelIcon`</b>,
///                  \anchor Pvr_NextRecordingChannelIcon
///                  _path_,
///     Icon of the next recording channel
///   }
///   \table_row3{   <b>`Pvr.BackendName`</b>,
///                  \anchor Pvr_BackendName
///                  _string_,
///     Name of the backend being used
///   }
///   \table_row3{   <b>`Pvr.BackendVersion`</b>,
///                  \anchor Pvr_BackendVersion
///                  _string_,
///     Version of the backend that's being used
///   }
///   \table_row3{   <b>`Pvr.BackendHost`</b>,
///                  \anchor Pvr_BackendHost
///                  _string_,
///     Backend hostname
///   }
///   \table_row3{   <b>`Pvr.BackendDiskSpace`</b>,
///                  \anchor Pvr_BackendDiskSpace
///                  _string_,
///     Available diskspace on the backend as string with size
///   }
///   \table_row3{   <b>`Pvr.BackendDiskSpaceProgr`</b>,
///                  \anchor Pvr_BackendDiskSpaceProgr
///                  _integer_,
///     Available diskspace on the backend as percent value
///   }
///   \table_row3{   <b>`Pvr.BackendChannels`</b>,
///                  \anchor Pvr_BackendChannels
///                  _string (integer)_,
///     Number of available channels the backend provides
///   }
///   \table_row3{   <b>`Pvr.BackendTimers`</b>,
///                  \anchor Pvr_BackendTimers
///                  _string (integer)_,
///     Number of timers set for the backend
///   }
///   \table_row3{   <b>`Pvr.BackendRecordings`</b>,
///                  \anchor Pvr_BackendRecordings
///                  _string (integer)_,
///     Number of recording available on the backend
///   }
///   \table_row3{   <b>`Pvr.BackendDeletedRecordings`</b>,
///                  \anchor Pvr_BackendDeletedRecordings
///                  _string (integer)_,
///     Number of deleted recording present on the backend
///   }
///   \table_row3{   <b>`Pvr.BackendNumber`</b>,
///                  \anchor Pvr_BackendNumber
///                  _string_,
///     Backend number
///   }
///   \table_row3{   <b>`Pvr.HasEpg`</b>,
///                  \anchor Pvr_HasEpg
///                  _boolean_,
///     Returns true when an epg is available.
///   }
///   \table_row3{   <b>`Pvr.HasTxt`</b>,
///                  \anchor Pvr_HasTxt
///                  _boolean_,
///     Returns true when teletext is available.
///   }
///   \table_row3{   <b>`Pvr.TotalDiscSpace`</b>,
///                  \anchor Pvr_TotalDiscSpace
///                  _string_,
///     Total diskspace available for recordings
///   }
///   \table_row3{   <b>`Pvr.NextTimer`</b>,
///                  \anchor Pvr_NextTimer
///                  _boolean_,
///     Next timer date
///   }
///   \table_row3{   <b>`Pvr.IsPlayingTv`</b>,
///                  \anchor Pvr_IsPlayingTv
///                  _boolean_,
///     Returns true when live tv is being watched.
///   }
///   \table_row3{   <b>`Pvr.IsPlayingRadio`</b>,
///                  \anchor Pvr_IsPlayingRadio
///                  _boolean_,
///     Returns true when live radio is being listened to.
///   }
///   \table_row3{   <b>`Pvr.IsPlayingRecording`</b>,
///                  \anchor Pvr_IsPlayingRecording
///                  _boolean_,
///     Returns true when a recording is being watched.
///   }
///   \table_row3{   <b>`Pvr.Duration`</b>,
///                  \anchor Pvr_Duration
///                  _time string_,
///     Returns the duration of the currently played title on TV
///   }
///   \table_row3{   <b>`Pvr.Time`</b>,
///                  \anchor Pvr_Time
///                  _time string_,
///     Returns the time position of the currently played title on TV
///   }
///   \table_row3{   <b>`Pvr.Progress`</b>,
///                  \anchor Pvr_Progress
///                  _integer_,
///     Returns the position of currently played title on TV as integer
///   }
///   \table_row3{   <b>`Pvr.ActStreamClient`</b>,
///                  \anchor Pvr_ActStreamClient
///                  _string_,
///     Stream client name
///   }
///   \table_row3{   <b>`Pvr.ActStreamDevice`</b>,
///                  \anchor Pvr_ActStreamDevice
///                  _string_,
///     Stream device name
///   }
///   \table_row3{   <b>`Pvr.ActStreamStatus`</b>,
///                  \anchor Pvr_ActStreamStatus
///                  _string_,
///     Status of the stream
///   }
///   \table_row3{   <b>`Pvr.ActStreamSignal`</b>,
///                  \anchor Pvr_ActStreamSignal
///                  _string_,
///     Signal quality of the stream
///   }
///   \table_row3{   <b>`Pvr.ActStreamSnr`</b>,
///                  \anchor Pvr_ActStreamSnr
///                  _string_,
///     Signal to noise ratio of the stream
///   }
///   \table_row3{   <b>`Pvr.ActStreamBer`</b>,
///                  \anchor Pvr_ActStreamBer
///                  _string_,
///     Bit error rate of the stream
///   }
///   \table_row3{   <b>`Pvr.ActStreamUnc`</b>,
///                  \anchor Pvr_ActStreamUnc
///                  _string_,
///     UNC value of the stream
///   }
///   \table_row3{   <b>`Pvr.ActStreamProgrSignal`</b>,
///                  \anchor Pvr_ActStreamProgrSignal
///                  _integer_,
///     Signal quality of the programme
///   }
///   \table_row3{   <b>`Pvr.ActStreamProgrSnr`</b>,
///                  \anchor Pvr_ActStreamProgrSnr
///                  _integer_,
///     Signal to noise ratio of the programme
///   }
///   \table_row3{   <b>`Pvr.ActStreamIsEncrypted`</b>,
///                  \anchor Pvr_ActStreamIsEncrypted
///                  _boolean_,
///     Returns true when channel is encrypted on source
///   }
///   \table_row3{   <b>`Pvr.ActStreamEncryptionName`</b>,
///                  \anchor Pvr_ActStreamEncryptionName
///                  _string_,
///     Encryption used on the stream
///   }
///   \table_row3{   <b>`Pvr.ActStreamServiceName`</b>,
///                  \anchor Pvr_ActStreamServiceName
///                  _string_,
///     Returns the service name of played channel if available
///   }
///   \table_row3{   <b>`Pvr.ActStreamMux`</b>,
///                  \anchor Pvr_ActStreamMux
///                  _string_,
///     Returns the multiplex type of played channel if available
///   }
///   \table_row3{   <b>`Pvr.ActStreamProviderName`</b>,
///                  \anchor Pvr_ActStreamProviderName
///                  _string_,
///     Returns the provider name of the played channel if available
///   }
///   \table_row3{   <b>`Pvr.IsTimeShift`</b>,
///                  \anchor Pvr_IsTimeShift
///                  _boolean_,
///     Returns true when for channel is timeshift available
///   }
///   \table_row3{   <b>`Pvr.TimeShiftStart`</b>,
///                  \anchor Pvr_TimeShiftStart
///                  _time string_,
///     Start position of the timeshift
///   }
///   \table_row3{   <b>`Pvr.TimeShiftEnd`</b>,
///                  \anchor Pvr_TimeShiftEnd
///                  _time string_,
///     End position of the timeshift
///   }
///   \table_row3{   <b>`Pvr.TimeShiftCur`</b>,
///                  \anchor Pvr_TimeShiftCur
///                  _time string_,
///     Current position of the timeshift
///   }
///   \table_row3{   <b>`Pvr.TimeShiftProgress`</b>,
///                  \anchor Pvr_TimeShiftProgress
///                  _integer_,
///     Returns the position of currently timeshifted title on TV as interger
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}
const infomap pvr[] =            {{ "isrecording",              PVR_IS_RECORDING },
                                  { "hastimer",                 PVR_HAS_TIMER },
                                  { "hastvchannels",            PVR_HAS_TV_CHANNELS },
                                  { "hasradiochannels",         PVR_HAS_RADIO_CHANNELS },
                                  { "hasnonrecordingtimer",     PVR_HAS_NONRECORDING_TIMER },
                                  { "nowrecordingtitle",        PVR_NOW_RECORDING_TITLE },
                                  { "nowrecordingdatetime",     PVR_NOW_RECORDING_DATETIME },
                                  { "nowrecordingchannel",      PVR_NOW_RECORDING_CHANNEL },
                                  { "nowrecordingchannelicon",  PVR_NOW_RECORDING_CHAN_ICO },
                                  { "nextrecordingtitle",       PVR_NEXT_RECORDING_TITLE },
                                  { "nextrecordingdatetime",    PVR_NEXT_RECORDING_DATETIME },
                                  { "nextrecordingchannel",     PVR_NEXT_RECORDING_CHANNEL },
                                  { "nextrecordingchannelicon", PVR_NEXT_RECORDING_CHAN_ICO },
                                  { "backendname",              PVR_BACKEND_NAME },
                                  { "backendversion",           PVR_BACKEND_VERSION },
                                  { "backendhost",              PVR_BACKEND_HOST },
                                  { "backenddiskspace",         PVR_BACKEND_DISKSPACE },
                                  { "backenddiskspaceprogr",    PVR_BACKEND_DISKSPACE_PROGR },
                                  { "backendchannels",          PVR_BACKEND_CHANNELS },
                                  { "backendtimers",            PVR_BACKEND_TIMERS },
                                  { "backendrecordings",        PVR_BACKEND_RECORDINGS },
                                  { "backenddeletedrecordings", PVR_BACKEND_DELETED_RECORDINGS },
                                  { "backendnumber",            PVR_BACKEND_NUMBER },
                                  { "hasepg",                   PVR_HAS_EPG },
                                  { "hastxt",                   PVR_HAS_TXT },
                                  { "totaldiscspace",           PVR_TOTAL_DISKSPACE },
                                  { "nexttimer",                PVR_NEXT_TIMER },
                                  { "isplayingtv",              PVR_IS_PLAYING_TV },
                                  { "isplayingradio",           PVR_IS_PLAYING_RADIO },
                                  { "isplayingrecording",       PVR_IS_PLAYING_RECORDING },
                                  { "duration",                 PVR_PLAYING_DURATION },
                                  { "time",                     PVR_PLAYING_TIME },
                                  { "progress",                 PVR_PLAYING_PROGRESS },
                                  { "actstreamclient",          PVR_ACTUAL_STREAM_CLIENT },
                                  { "actstreamdevice",          PVR_ACTUAL_STREAM_DEVICE },
                                  { "actstreamstatus",          PVR_ACTUAL_STREAM_STATUS },
                                  { "actstreamsignal",          PVR_ACTUAL_STREAM_SIG },
                                  { "actstreamsnr",             PVR_ACTUAL_STREAM_SNR },
                                  { "actstreamber",             PVR_ACTUAL_STREAM_BER },
                                  { "actstreamunc",             PVR_ACTUAL_STREAM_UNC },
                                  { "actstreamprogrsignal",     PVR_ACTUAL_STREAM_SIG_PROGR },
                                  { "actstreamprogrsnr",        PVR_ACTUAL_STREAM_SNR_PROGR },
                                  { "actstreamisencrypted",     PVR_ACTUAL_STREAM_ENCRYPTED },
                                  { "actstreamencryptionname",  PVR_ACTUAL_STREAM_CRYPTION },
                                  { "actstreamservicename",     PVR_ACTUAL_STREAM_SERVICE },
                                  { "actstreammux",             PVR_ACTUAL_STREAM_MUX },
                                  { "actstreamprovidername",    PVR_ACTUAL_STREAM_PROVIDER },
                                  { "istimeshift",              PVR_IS_TIMESHIFTING },
                                  { "timeshiftstart",           PVR_TIMESHIFT_START_TIME },
                                  { "timeshiftend",             PVR_TIMESHIFT_END_TIME },
                                  { "timeshiftcur",             PVR_TIMESHIFT_PLAY_TIME },
                                  { "timeshiftprogress",        PVR_TIMESHIFT_PROGRESS }};

/// \page modules__General__List_of_gui_access
/// \section modules__General__List_of_gui_access_ADSP ADSP
/// @{
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`ADSP.IsActive`</b>,
///                  \anchor ADSP_IsActive
///                  _boolean_,
///     Returns true if dsp system is enabled
///   }
///   \table_row3{   <b>`ADSP.HasModes`</b>,
///                  \anchor ADSP_HasModes
///                  _boolean_,
///     Returns true if one or more modes are present on any of the types
///   }
///   \table_row3{   <b>`ADSP.HasInputResample`</b>,
///                  \anchor ADSP_HasInputResample
///                  _boolean_,
///     Returns true if on stream is a input resample is active
///   }
///   \table_row3{   <b>`ADSP.HasPreProcess`</b>,
///                  \anchor ADSP_HasPreProcess
///                  _boolean_,
///     Returns true if on stream is a pre process mode active
///   }
///   \table_row3{   <b>`ADSP.HasMasterProcess`</b>,
///                  \anchor ADSP_HasMasterProcess
///                  _boolean_,
///     Returns true if on stream is a master process mode available
///   }
///   \table_row3{   <b>`ADSP.HasPostProcess`</b>,
///                  \anchor ADSP_HasPostProcess
///                  _boolean_,
///     Returns true if on stream is a post process
///   }
///   \table_row3{   <b>`ADSP.HasOutputResample`</b>,
///                  \anchor ADSP_HasOutputResample
///                  _boolean_,
///     Returns true if on stream is a output resample
///   }
///   \table_row3{   <b>`ADSP.MasterActive`</b>,
///                  \anchor ADSP_MasterActive
///                  _boolean_,
///     Returns true if on stream is a master mode selected and active
///   }
///   \table_row3{   <b>`ADSP.ActiveStreamType`</b>,
///                  \anchor ADSP_ActiveStreamType
///                  _string_,
///     From user wanted and selected stream type\, e.g. music or video
///   }
///   \table_row3{   <b>`ADSP.DetectedStreamType`</b>,
///                  \anchor ADSP_DetectedStreamType
///                  _string_,
///     From Kodi detected stream type
///   }
///   \table_row3{   <b>`ADSP.MasterName`</b>,
///                  \anchor ADSP_MasterName
///                  _string_,
///     Name of the curently selected and used master dsp mode
///   }
///   \table_row3{   <b>`ADSP.MasterInfo`</b>,
///                  \anchor ADSP_MasterInfo
///                  _string_,
///     Continues updated information label of master mode (if available)
///   }
///   \table_row3{   <b>`ADSP.MasterOwnIcon`</b>,
///                  \anchor ADSP_MasterOwnIcon
///                  _path_,
///     Icon to use for selected master mode
///   }
///   \table_row3{   <b>`ADSP.MasterOverrideIcon`</b>,
///                  \anchor ADSP_MasterOverrideIcon
///                  _path_,
///     Icon to overrite Kodi's codec icon with one of add-on\, e.g. Dolby
///     Digital EX on Dolby Digital
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}
const infomap adsp[] =           {{ "isactive",                 ADSP_IS_ACTIVE },
                                  { "hasmodes",                 ADSP_HAS_MODES },
                                  { "hasinputresample",         ADSP_HAS_INPUT_RESAMPLE },
                                  { "haspreprocess",            ADSP_HAS_PRE_PROCESS },
                                  { "hasmasterprocess",         ADSP_HAS_MASTER_PROCESS },
                                  { "haspostprocess",           ADSP_HAS_POST_PROCESS },
                                  { "hasoutputresample",        ADSP_HAS_OUTPUT_RESAMPLE },
                                  { "masteractive",             ADSP_MASTER_ACTIVE },
                                  { "activestreamtype",         ADSP_ACTIVE_STREAM_TYPE },
                                  { "detectedstreamtype",       ADSP_DETECTED_STREAM_TYPE },
                                  { "mastername",               ADSP_MASTER_NAME },
                                  { "masterinfo",               ADSP_MASTER_INFO },
                                  { "masterownicon",            ADSP_MASTER_OWN_ICON },
                                  { "masteroverrideicon",       ADSP_MASTER_OVERRIDE_ICON }};

/// \page modules__General__List_of_gui_access
/// \section modules__General__List_of_gui_access_RDS Radio RDS
/// \note Only be supported on PVR Radio where the related add-on client can
/// bring it.
/// @{
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`RDS.HasRds`</b>,
///                  \anchor RDS_HasRds
///                  _boolean_,
///     Returns true if RDS is present
///   }
///   \table_row3{   <b>`RDS.HasRadioText`</b>,
///                  \anchor RDS_HasRadioText
///                  _boolean_,
///     Returns true if RDS contains also Radiotext
///   }
///   \table_row3{   <b>`RDS.HasRadioTextPlus`</b>,
///                  \anchor RDS_HasRadioTextPlus
///                  _boolean_,
///     Returns true if RDS with Radiotext contains also the plus information
///   }
///   \table_row3{   <b>`RDS.HasHotline`</b>,
///                  \anchor RDS_HasHotline
///                  _boolean_,
///     Returns true if a hotline phone number is present\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.HasStudio`</b>,
///                  \anchor RDS_HasStudio
///                  _boolean_,
///     Returns true if a studio name is present\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.AudioLanguage`</b>,
///                  \anchor RDS_AudioLanguage
///                  _string_,
///     The from RDS reported audio language of channel
///   }
///   \table_row3{   <b>`RDS.ChannelCountry`</b>,
///                  \anchor RDS_ChannelCountry
///                  _string_,
///     Country where the radio channel is sended
///   }
///   \table_row3{   <b>`RDS.GetLine(number)`</b>,
///                  \anchor RDS_GetLine
///                  _string_,
///     Returns the last sended RDS text messages on givern number\, 0 is the
///     last and 4 rows are supported (0-3)
///   }
///   \table_row3{   <b>`RDS.Title`</b>,
///                  \anchor RDS_Title
///                  _string_,
///     Title of item; e.g. track title of an album\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.Artist`</b>,
///                  \anchor RDS_Artist
///                  _string_,
///     A person or band/collective generally considered responsible for the work\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.Band`</b>,
///                  \anchor RDS_Band
///                  _string_,
///     Band/orchestra/accompaniment/musician\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.Composer`</b>,
///                  \anchor RDS_Composer
///                  _string_,
///     Name of the original composer/author\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.Conductor`</b>,
///                  \anchor RDS_Conductor
///                  _string_,
///     The artist(s) who performed the work. In classical music this would be
///     the conductor\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.Album`</b>,
///                  \anchor RDS_Album
///                  _string_,
///     The collection name to which this track belongs\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.TrackNumber`</b>,
///                  \anchor RDS_TrackNumber
///                  _string_,
///     The track number of the item on the album on which it was originally
///     released.\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.RadioStyle`</b>,
///                  \anchor RDS_RadioStyle
///                  _string_,
///     The from radio channel used style of currently played part\, is always
///     updated on changed\, e.g "popmusic" to "news" or "weather"...
///     | RDS                     | RBDS                    |
///     |:------------------------|:------------------------|
///     | none                    | none                    |
///     | news                    | news                    |
///     | currentaffairs          | information             |
///     | information             | sport                   |
///     | sport                   | talk                    |
///     | education               | rockmusic               |
///     | drama                   | classicrockmusic        |
///     | cultures                | adulthits               |
///     | science                 | softrock                |
///     | variedspeech            | top40                   |
///     | popmusic                | countrymusic            |
///     | rockmusic               | oldiesmusic             |
///     | easylistening           | softmusic               |
///     | lightclassics           | nostalgia               |
///     | seriousclassics         | jazzmusic               |
///     | othermusic              | classical               |
///     | weather                 | randb                   |
///     | finance                 | softrandb               |
///     | childrensprogs          | language                |
///     | socialaffairs           | religiousmusic          |
///     | religion                | religioustalk           |
///     | phonein                 | personality             |
///     | travelandtouring        | public                  |
///     | leisureandhobby         | college                 |
///     | jazzmusic               | spanishtalk             |
///     | countrymusic            | spanishmusic            |
///     | nationalmusic           | hiphop                  |
///     | oldiesmusic             |                         |
///     | folkmusic               |                         |
///     | documentary             | weather                 |
///     | alarmtest               | alarmtest               |
///     | alarm-alarm             | alarm-alarm             |
///     @note "alarm-alarm" is normally not used from radio stations\, is thought
///     to inform about horrible messages who are needed asap to all people.
///   }
///   \table_row3{   <b>`RDS.Comment`</b>,
///                  \anchor RDS_Comment
///                  _string_,
///     Radio station comment string if available\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.InfoNews`</b>,
///                  \anchor RDS_InfoNews
///                  _string_,
///     Message / headline (if available)\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.InfoNewsLocal`</b>,
///                  \anchor RDS_InfoNewsLocal
///                  _string_,
///     Local information news sended from radio channel (if available)\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.InfoStock`</b>,
///                  \anchor RDS_InfoStock
///                  _string_,
///     Quote information; either as one part or as several distinct parts:
///     "name 99latest value 99change 99high 99low 99volume" (if available)\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.InfoStockSize`</b>,
///                  \anchor RDS_InfoStockSize
///                  _string_,
///     Number of rows present in stock information\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.InfoSport`</b>,
///                  \anchor RDS_InfoSport
///                  _string_,
///     Result of a game; either as one part or as several distinct parts:
///     "match 99result"\, e.g. "Bayern München : Borussia 995:5"  (if available)\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.InfoSportSize`</b>,
///                  \anchor RDS_InfoSportSize
///                  _string_,
///     Number of rows present in sport information\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.InfoLottery`</b>,
///                  \anchor RDS_InfoLottery
///                  _string_,
///     Raffle / lottery: "key word 99values" (if available)\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.InfoLotterySize`</b>,
///                  \anchor RDS_InfoLotterySize
///                  _string_,
///     Number of rows present in lottery information\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.InfoWeather`</b>,
///                  \anchor RDS_InfoWeather
///                  _string_,
///     Weather informations sended from radio channel (if available)\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.InfoWeatherSize`</b>,
///                  \anchor RDS_InfoWeatherSize
///                  _string_,
///     Number of rows present in weather information\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.InfoCinema`</b>,
///                  \anchor RDS_InfoCinema
///                  _string_,
///     Information about movies in cinema (if available)\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.InfoCinemaSize`</b>,
///                  \anchor RDS_InfoCinemaSize
///                  _string_,
///     Number of rows present in cinema information\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.InfoHoroscope`</b>,
///                  \anchor RDS_InfoHoroscope
///                  _string_,
///     Horoscope; either as one part or as two distinct parts:
///     "key word 99text"\, e.g. "sign of the zodiac 99blablabla" (if available)\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.InfoHoroscopeSize`</b>,
///                  \anchor RDS_InfoHoroscopeSize
///                  _string_,
///     Number of rows present in horoscope information\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.InfoOther`</b>,
///                  \anchor RDS_InfoOther
///                  _string_,
///     Other information\, not especially specified: "key word 99info" (if available)\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.InfoOtherSize`</b>,
///                  \anchor RDS_InfoOtherSize
///                  _string_,
///     Number of rows present with other informations\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.ProgStation`</b>,
///                  \anchor RDS_ProgStation
///                  _string_,
///     Name of the radio channel
///     @note becomes also be set from epg if from RDS not available
///   }
///   \table_row3{   <b>`RDS.ProgNow`</b>,
///                  \anchor RDS_ProgNow
///                  _string_,
///     Now played program name
///     @note becomes also be set from epg if from RDS not available
///   }
///   \table_row3{   <b>`RDS.ProgNext`</b>,
///                  \anchor RDS_ProgNext
///                  _string_,
///     Next played program name (if available)
///     @note becomes also be set from epg if from RDS not available
///   }
///   \table_row3{   <b>`RDS.ProgHost`</b>,
///                  \anchor RDS_ProgHost
///                  _string_,
///     Name of the host of the radio show
///   }
///   \table_row3{   <b>`RDS.ProgEditStaff`</b>,
///                  \anchor RDS_ProgEditStaff
///                  _string_,
///     Name of the editorial staff; e.g. name of editorial journalist\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.ProgHomepage`</b>,
///                  \anchor RDS_ProgHomepage
///                  _string_,
///     Link to radio station homepage\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.ProgStyle`</b>,
///                  \anchor RDS_ProgStyle
///                  _string_,
///     Human readable string about radiostyle defined from RDS or RBDS
///   }
///   \table_row3{   <b>`RDS.PhoneHotline`</b>,
///                  \anchor RDS_PhoneHotline
///                  _string_,
///     The telephone number of the radio station's hotline\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.PhoneStudio`</b>,
///                  \anchor RDS_PhoneStudio
///                  _string_,
///     The telephone number of the radio station's studio\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.SmsStudio`</b>,
///                  \anchor RDS_SmsStudio
///                  _string_,
///     The sms number of the radio stations studio (to send directly a sms to
///     the studio) (if available)\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.EmailHotline`</b>,
///                  \anchor RDS_EmailHotline
///                  _string_,
///     The email adress of the radio stations hotline (if available)\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.EmailStudio`</b>,
///                  \anchor RDS_EmailStudio
///                  _string_,
///     The email adress of the radio stations studio (if available)\n
///     (Only be available on RadiotextPlus)
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}
const infomap rds[] =            {{ "hasrds",                   RDS_HAS_RDS },
                                  { "hasradiotext",             RDS_HAS_RADIOTEXT },
                                  { "hasradiotextplus",         RDS_HAS_RADIOTEXT_PLUS },
                                  { "audiolanguage",            RDS_AUDIO_LANG },
                                  { "channelcountry",           RDS_CHANNEL_COUNTRY },
                                  { "title",                    RDS_TITLE },
                                  { "getline",                  RDS_GET_RADIOTEXT_LINE },
                                  { "artist",                   RDS_ARTIST },
                                  { "band",                     RDS_BAND },
                                  { "composer",                 RDS_COMPOSER },
                                  { "conductor",                RDS_CONDUCTOR },
                                  { "album",                    RDS_ALBUM },
                                  { "tracknumber",              RDS_ALBUM_TRACKNUMBER },
                                  { "radiostyle",               RDS_GET_RADIO_STYLE },
                                  { "comment",                  RDS_COMMENT },
                                  { "infonews",                 RDS_INFO_NEWS },
                                  { "infonewslocal",            RDS_INFO_NEWS_LOCAL },
                                  { "infostock",                RDS_INFO_STOCK },
                                  { "infostocksize",            RDS_INFO_STOCK_SIZE },
                                  { "infosport",                RDS_INFO_SPORT },
                                  { "infosportsize",            RDS_INFO_SPORT_SIZE },
                                  { "infolottery",              RDS_INFO_LOTTERY },
                                  { "infolotterysize",          RDS_INFO_LOTTERY_SIZE },
                                  { "infoweather",              RDS_INFO_WEATHER },
                                  { "infoweathersize",          RDS_INFO_WEATHER_SIZE },
                                  { "infocinema",               RDS_INFO_CINEMA },
                                  { "infocinemasize",           RDS_INFO_CINEMA_SIZE },
                                  { "infohoroscope",            RDS_INFO_HOROSCOPE },
                                  { "infohoroscopesize",        RDS_INFO_HOROSCOPE_SIZE },
                                  { "infoother",                RDS_INFO_OTHER },
                                  { "infoothersize",            RDS_INFO_OTHER_SIZE },
                                  { "progstation",              RDS_PROG_STATION },
                                  { "prognow",                  RDS_PROG_NOW },
                                  { "prognext",                 RDS_PROG_NEXT },
                                  { "proghost",                 RDS_PROG_HOST },
                                  { "progeditstaff",            RDS_PROG_EDIT_STAFF },
                                  { "proghomepage",             RDS_PROG_HOMEPAGE },
                                  { "progstyle",                RDS_PROG_STYLE },
                                  { "phonehotline",             RDS_PHONE_HOTLINE },
                                  { "phonestudio",              RDS_PHONE_STUDIO },
                                  { "smsstudio",                RDS_SMS_STUDIO },
                                  { "emailhotline",             RDS_EMAIL_HOTLINE },
                                  { "emailstudio",              RDS_EMAIL_STUDIO },
                                  { "hashotline",               RDS_HAS_HOTLINE_DATA },
                                  { "hasstudio",                RDS_HAS_STUDIO_DATA }};

/// \page modules__General__List_of_gui_access
/// \section modules__General__List_of_gui_access_slideshow Slideshow
/// @{
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`Slideshow.IsActive`</b>,
///                  \anchor Slideshow_IsActive
///                  _boolean_,
///     Returns true if the picture slideshow is running
///   }
///   \table_row3{   <b>`Slideshow.IsPaused`</b>,
///                  \anchor Slideshow_IsPaused
///                  _boolean_,
///     Returns true if the picture slideshow is paused
///   }
///   \table_row3{   <b>`Slideshow.IsRandom`</b>,
///                  \anchor Slideshow_IsRandom
///                  _boolean_,
///     Returns true if the picture slideshow is in random mode
///   }
///   \table_row3{   <b>`Slideshow.IsVideo`</b>,
///                  \anchor Slideshow_IsVideo
///                  _boolean_,
///     Returns true if the picture slideshow is playing a video
///   }
/// \table_end
/// @}
const infomap slideshow[] =      {{ "ispaused",         SLIDESHOW_ISPAUSED },
                                  { "isactive",         SLIDESHOW_ISACTIVE },
                                  { "isvideo",          SLIDESHOW_ISVIDEO },
                                  { "israndom",         SLIDESHOW_ISRANDOM }};

// Crazy part, to use tableofcontents must it be on end
/// \page modules__General__List_of_gui_access
/// \tableofcontents

const int picture_slide_map[]  = {/* LISTITEM_PICTURE_RESOLUTION => */ SLIDE_RESOLUTION,
                                  /* LISTITEM_PICTURE_LONGDATE   => */ SLIDE_EXIF_LONG_DATE,
                                  /* LISTITEM_PICTURE_LONGDATETIME => */ SLIDE_EXIF_LONG_DATE_TIME,
                                  /* LISTITEM_PICTURE_DATE       => */ SLIDE_EXIF_DATE,
                                  /* LISTITEM_PICTURE_DATETIME   => */ SLIDE_EXIF_DATE_TIME,
                                  /* LISTITEM_PICTURE_COMMENT    => */ SLIDE_COMMENT,
                                  /* LISTITEM_PICTURE_CAPTION    => */ SLIDE_IPTC_CAPTION,
                                  /* LISTITEM_PICTURE_DESC       => */ SLIDE_EXIF_DESCRIPTION,
                                  /* LISTITEM_PICTURE_KEYWORDS   => */ SLIDE_IPTC_KEYWORDS,
                                  /* LISTITEM_PICTURE_CAM_MAKE   => */ SLIDE_EXIF_CAMERA_MAKE,
                                  /* LISTITEM_PICTURE_CAM_MODEL  => */ SLIDE_EXIF_CAMERA_MODEL,
                                  /* LISTITEM_PICTURE_APERTURE   => */ SLIDE_EXIF_APERTURE,
                                  /* LISTITEM_PICTURE_FOCAL_LEN  => */ SLIDE_EXIF_FOCAL_LENGTH,
                                  /* LISTITEM_PICTURE_FOCUS_DIST => */ SLIDE_EXIF_FOCUS_DIST,
                                  /* LISTITEM_PICTURE_EXP_MODE   => */ SLIDE_EXIF_EXPOSURE_MODE,
                                  /* LISTITEM_PICTURE_EXP_TIME   => */ SLIDE_EXIF_EXPOSURE_TIME,
                                  /* LISTITEM_PICTURE_ISO        => */ SLIDE_EXIF_ISO_EQUIV,
                                  /* LISTITEM_PICTURE_AUTHOR           => */ SLIDE_IPTC_AUTHOR,
                                  /* LISTITEM_PICTURE_BYLINE           => */ SLIDE_IPTC_BYLINE,
                                  /* LISTITEM_PICTURE_BYLINE_TITLE     => */ SLIDE_IPTC_BYLINE_TITLE,
                                  /* LISTITEM_PICTURE_CATEGORY         => */ SLIDE_IPTC_CATEGORY,
                                  /* LISTITEM_PICTURE_CCD_WIDTH        => */ SLIDE_EXIF_CCD_WIDTH,
                                  /* LISTITEM_PICTURE_CITY             => */ SLIDE_IPTC_CITY,
                                  /* LISTITEM_PICTURE_URGENCY          => */ SLIDE_IPTC_URGENCY,
                                  /* LISTITEM_PICTURE_COPYRIGHT_NOTICE => */ SLIDE_IPTC_COPYRIGHT_NOTICE,
                                  /* LISTITEM_PICTURE_COUNTRY          => */ SLIDE_IPTC_COUNTRY,
                                  /* LISTITEM_PICTURE_COUNTRY_CODE     => */ SLIDE_IPTC_COUNTRY_CODE,
                                  /* LISTITEM_PICTURE_CREDIT           => */ SLIDE_IPTC_CREDIT,
                                  /* LISTITEM_PICTURE_IPTCDATE         => */ SLIDE_IPTC_DATE,
                                  /* LISTITEM_PICTURE_DIGITAL_ZOOM     => */ SLIDE_EXIF_DIGITAL_ZOOM,
                                  /* LISTITEM_PICTURE_EXPOSURE         => */ SLIDE_EXIF_EXPOSURE,
                                  /* LISTITEM_PICTURE_EXPOSURE_BIAS    => */ SLIDE_EXIF_EXPOSURE_BIAS,
                                  /* LISTITEM_PICTURE_FLASH_USED       => */ SLIDE_EXIF_FLASH_USED,
                                  /* LISTITEM_PICTURE_HEADLINE         => */ SLIDE_IPTC_HEADLINE,
                                  /* LISTITEM_PICTURE_COLOUR           => */ SLIDE_COLOUR,
                                  /* LISTITEM_PICTURE_LIGHT_SOURCE     => */ SLIDE_EXIF_LIGHT_SOURCE,
                                  /* LISTITEM_PICTURE_METERING_MODE    => */ SLIDE_EXIF_METERING_MODE,
                                  /* LISTITEM_PICTURE_OBJECT_NAME      => */ SLIDE_IPTC_OBJECT_NAME,
                                  /* LISTITEM_PICTURE_ORIENTATION      => */ SLIDE_EXIF_ORIENTATION,
                                  /* LISTITEM_PICTURE_PROCESS          => */ SLIDE_PROCESS,
                                  /* LISTITEM_PICTURE_REF_SERVICE      => */ SLIDE_IPTC_REF_SERVICE,
                                  /* LISTITEM_PICTURE_SOURCE           => */ SLIDE_IPTC_SOURCE,
                                  /* LISTITEM_PICTURE_SPEC_INSTR       => */ SLIDE_IPTC_SPEC_INSTR,
                                  /* LISTITEM_PICTURE_STATE            => */ SLIDE_IPTC_STATE,
                                  /* LISTITEM_PICTURE_SUP_CATEGORIES   => */ SLIDE_IPTC_SUP_CATEGORIES,
                                  /* LISTITEM_PICTURE_TX_REFERENCE     => */ SLIDE_IPTC_TX_REFERENCE,
                                  /* LISTITEM_PICTURE_WHITE_BALANCE    => */ SLIDE_EXIF_WHITE_BALANCE,
                                  /* LISTITEM_PICTURE_IMAGETYPE        => */ SLIDE_IPTC_IMAGETYPE,
                                  /* LISTITEM_PICTURE_SUBLOCATION      => */ SLIDE_IPTC_SUBLOCATION,
                                  /* LISTITEM_PICTURE_TIMECREATED      => */ SLIDE_IPTC_TIMECREATED,
                                  /* LISTITEM_PICTURE_GPS_LAT    => */ SLIDE_EXIF_GPS_LATITUDE,
                                  /* LISTITEM_PICTURE_GPS_LON    => */ SLIDE_EXIF_GPS_LONGITUDE,
                                  /* LISTITEM_PICTURE_GPS_ALT    => */ SLIDE_EXIF_GPS_ALTITUDE };

CGUIInfoManager::Property::Property(const std::string &property, const std::string &parameters)
: name(property)
{
  CUtil::SplitParams(parameters, params);
}

const std::string &CGUIInfoManager::Property::param(unsigned int n /* = 0 */) const
{
  if (n < params.size())
    return params[n];
  return StringUtils::Empty;
}

unsigned int CGUIInfoManager::Property::num_params() const
{
  return params.size();
}

void CGUIInfoManager::SplitInfoString(const std::string &infoString, std::vector<Property> &info)
{
  // our string is of the form:
  // category[(params)][.info(params).info2(params)] ...
  // so we need to split on . while taking into account of () pairs
  unsigned int parentheses = 0;
  std::string property;
  std::string param;
  for (size_t i = 0; i < infoString.size(); ++i)
  {
    if (infoString[i] == '(')
    {
      if (!parentheses++)
        continue;
    }
    else if (infoString[i] == ')')
    {
      if (!parentheses)
        CLog::Log(LOGERROR, "unmatched parentheses in %s", infoString.c_str());
      else if (!--parentheses)
        continue;
    }
    else if (infoString[i] == '.' && !parentheses)
    {
      if (!property.empty()) // add our property and parameters
      {
        StringUtils::ToLower(property);
        info.push_back(Property(property, param));
      }
      property.clear();
      param.clear();
      continue;
    }
    if (parentheses)
      param += infoString[i];
    else
      property += infoString[i];
  }
  if (parentheses)
    CLog::Log(LOGERROR, "unmatched parentheses in %s", infoString.c_str());
  if (!property.empty())
  {
    StringUtils::ToLower(property);
    info.push_back(Property(property, param));
  }
}

/// \brief Translates a string as given by the skin into an int that we use for more
/// efficient retrieval of data.
int CGUIInfoManager::TranslateSingleString(const std::string &strCondition)
{
  bool listItemDependent;
  return TranslateSingleString(strCondition, listItemDependent);
}

int CGUIInfoManager::TranslateSingleString(const std::string &strCondition, bool &listItemDependent)
{
  /* We need to disable caching in INFO::InfoBool::Get if either of the following are true:
   *  1. if condition is between LISTITEM_START and LISTITEM_END
   *  2. if condition is string or integer the corresponding label is between LISTITEM_START and LISTITEM_END
   *  This is achieved by setting the bool pointed at by listItemDependent, either here or in a recursive call
   */
  // trim whitespaces
  std::string strTest = strCondition;
  StringUtils::Trim(strTest);

  std::vector< Property> info;
  SplitInfoString(strTest, info);

  if (info.empty())
    return 0;

  const Property &cat = info[0];
  if (info.size() == 1)
  { // single category
    if (cat.name == "false" || cat.name == "no")
      return SYSTEM_ALWAYS_FALSE;
    else if (cat.name == "true" || cat.name == "yes")
      return SYSTEM_ALWAYS_TRUE;

    // deprecated begin
    // should be removed before L*** v18
    if (cat.name == "isempty" && cat.num_params() == 1)
      return AddMultiInfo(GUIInfo(STRING_IS_EMPTY, TranslateSingleString(cat.param(), listItemDependent)));
    else if (cat.name == "stringcompare" && cat.num_params() == 2)
    {
      int info = TranslateSingleString(cat.param(0), listItemDependent);
      // pipe our original string through the localize parsing then make it lowercase (picks up $LBRACKET etc.)
      std::string label = CGUIInfoLabel::GetLabel(cat.param(1));
      StringUtils::ToLower(label);
      // 'true', 'false', 'yes', 'no' are valid strings, do not resolve them to SYSTEM_ALWAYS_TRUE or SYSTEM_ALWAYS_FALSE
      if (label != "true" && label != "false" && label != "yes" && label != "no")
      {
        int info2 = TranslateSingleString(cat.param(1), listItemDependent);
        if (info2 > 0)
          return AddMultiInfo(GUIInfo(STRING_COMPARE, info, -info2));
      }
      int compareString = ConditionalStringParameter(label);
      return AddMultiInfo(GUIInfo(STRING_COMPARE, info, compareString));
    }
    else if (cat.name == "integergreaterthan" && cat.num_params() == 2)
    {
      int info = TranslateSingleString(cat.param(0), listItemDependent);
      int compareInt = atoi(cat.param(1).c_str());
      return AddMultiInfo(GUIInfo(INTEGER_GREATER_THAN, info, compareInt));
    }
    else if (cat.name == "substring" && cat.num_params() >= 2)
    {
      int info = TranslateSingleString(cat.param(0), listItemDependent);
      std::string label = CGUIInfoLabel::GetLabel(cat.param(1));
      StringUtils::ToLower(label);
      int compareString = ConditionalStringParameter(label);
      if (cat.num_params() > 2)
      {
        if (StringUtils::EqualsNoCase(cat.param(2), "left"))
          return AddMultiInfo(GUIInfo(STRING_STR_LEFT, info, compareString));
        else if (StringUtils::EqualsNoCase(cat.param(2), "right"))
          return AddMultiInfo(GUIInfo(STRING_STR_RIGHT, info, compareString));
      }
      return AddMultiInfo(GUIInfo(STRING_STR, info, compareString));
    }
    // deprecated end
  }
  else if (info.size() == 2)
  {
    const Property &prop = info[1];
    if (cat.name == "string")
    {
      if (prop.name == "isempty")
      {
        return AddMultiInfo(GUIInfo(STRING_IS_EMPTY, TranslateSingleString(prop.param(), listItemDependent)));
      }
      else if (prop.num_params() == 2)
      {
        for (size_t i = 0; i < sizeof(string_bools) / sizeof(infomap); i++)
        {
          if (prop.name == string_bools[i].str)
          {
            int data1 = TranslateSingleString(prop.param(0), listItemDependent);
            // pipe our original string through the localize parsing then make it lowercase (picks up $LBRACKET etc.)
            std::string label = CGUIInfoLabel::GetLabel(prop.param(1));
            StringUtils::ToLower(label);
            // 'true', 'false', 'yes', 'no' are valid strings, do not resolve them to SYSTEM_ALWAYS_TRUE or SYSTEM_ALWAYS_FALSE
            if (label != "true" && label != "false" && label != "yes" && label != "no")
            {
              int data2 = TranslateSingleString(prop.param(1), listItemDependent);
              if (data2 > 0)
                return AddMultiInfo(GUIInfo(string_bools[i].val, data1, -data2));
            }
            return AddMultiInfo(GUIInfo(string_bools[i].val, data1, ConditionalStringParameter(label)));
          }
        }
      }
    }
    if (cat.name == "integer")
    {
      for (size_t i = 0; i < sizeof(integer_bools) / sizeof(infomap); i++)
      {
        if (prop.name == integer_bools[i].str)
        {
          int data1 = TranslateSingleString(prop.param(0), listItemDependent);
          int data2 = atoi(prop.param(1).c_str());
          return AddMultiInfo(GUIInfo(integer_bools[i].val, data1, data2));
        }
      }
    }
    else if (cat.name == "player")
    {
      for (size_t i = 0; i < sizeof(player_labels) / sizeof(infomap); i++)
      {
        if (prop.name == player_labels[i].str)
          return player_labels[i].val;
      }
      for (size_t i = 0; i < sizeof(player_times) / sizeof(infomap); i++)
      {
        if (prop.name == player_times[i].str)
          return AddMultiInfo(GUIInfo(player_times[i].val, TranslateTimeFormat(prop.param())));
      }
      if (prop.name == "process" && prop.num_params())
      {
        for (size_t i = 0; i < sizeof(player_process) / sizeof(infomap); i++)
        {
          if (StringUtils::EqualsNoCase(prop.param(), player_process[i].str))
            return player_process[i].val;
        }
      }
      if (prop.num_params() == 1)
      {
        for (size_t i = 0; i < sizeof(player_param) / sizeof(infomap); i++)
        {
          if (prop.name == player_param[i].str)
            return AddMultiInfo(GUIInfo(player_param[i].val, ConditionalStringParameter(prop.param())));
        }
      }
    }
    else if (cat.name == "weather")
    {
      for (size_t i = 0; i < sizeof(weather) / sizeof(infomap); i++)
      {
        if (prop.name == weather[i].str)
          return weather[i].val;
      }
    }
    else if (cat.name == "network")
    {
      for (size_t i = 0; i < sizeof(network_labels) / sizeof(infomap); i++)
      {
        if (prop.name == network_labels[i].str)
          return network_labels[i].val;
      }
    }
    else if (cat.name == "musicpartymode")
    {
      for (size_t i = 0; i < sizeof(musicpartymode) / sizeof(infomap); i++)
      {
        if (prop.name == musicpartymode[i].str)
          return musicpartymode[i].val;
      }
    }
    else if (cat.name == "system")
    {
      for (size_t i = 0; i < sizeof(system_labels) / sizeof(infomap); i++)
      {
        if (prop.name == system_labels[i].str)
          return system_labels[i].val;
      }
      if (prop.num_params() == 1)
      {
        const std::string &param = prop.param();
        if (prop.name == "getbool")
        {
          std::string paramCopy = param;
          StringUtils::ToLower(paramCopy);
          return AddMultiInfo(GUIInfo(SYSTEM_GET_BOOL, ConditionalStringParameter(paramCopy, true)));
        }
        for (size_t i = 0; i < sizeof(system_param) / sizeof(infomap); i++)
        {
          if (prop.name == system_param[i].str)
            return AddMultiInfo(GUIInfo(system_param[i].val, ConditionalStringParameter(param)));
        }
        if (prop.name == "memory")
        {
          if (param == "free") return SYSTEM_FREE_MEMORY;
          else if (param == "free.percent") return SYSTEM_FREE_MEMORY_PERCENT;
          else if (param == "used") return SYSTEM_USED_MEMORY;
          else if (param == "used.percent") return SYSTEM_USED_MEMORY_PERCENT;
          else if (param == "total") return SYSTEM_TOTAL_MEMORY;
        }
        else if (prop.name == "addontitle")
        {
          int infoLabel = TranslateSingleString(param, listItemDependent);
          if (infoLabel > 0)
            return AddMultiInfo(GUIInfo(SYSTEM_ADDON_TITLE, infoLabel, 0));
          std::string label = CGUIInfoLabel::GetLabel(param);
          StringUtils::ToLower(label);
          return AddMultiInfo(GUIInfo(SYSTEM_ADDON_TITLE, ConditionalStringParameter(label), 1));
        }
        else if (prop.name == "addonicon")
        {
          int infoLabel = TranslateSingleString(param, listItemDependent);
          if (infoLabel > 0)
            return AddMultiInfo(GUIInfo(SYSTEM_ADDON_ICON, infoLabel, 0));
          std::string label = CGUIInfoLabel::GetLabel(param);
          StringUtils::ToLower(label);
          return AddMultiInfo(GUIInfo(SYSTEM_ADDON_ICON, ConditionalStringParameter(label), 1));
        }
        else if (prop.name == "addonversion")
        {
          int infoLabel = TranslateSingleString(param, listItemDependent);
          if (infoLabel > 0)
            return AddMultiInfo(GUIInfo(SYSTEM_ADDON_VERSION, infoLabel, 0));
          std::string label = CGUIInfoLabel::GetLabel(param);
          StringUtils::ToLower(label);
          return AddMultiInfo(GUIInfo(SYSTEM_ADDON_VERSION, ConditionalStringParameter(label), 1));
        }
        else if (prop.name == "idletime")
          return AddMultiInfo(GUIInfo(SYSTEM_IDLE_TIME, atoi(param.c_str())));
      }
      if (prop.name == "alarmlessorequal" && prop.num_params() == 2)
        return AddMultiInfo(GUIInfo(SYSTEM_ALARM_LESS_OR_EQUAL, ConditionalStringParameter(prop.param(0)), ConditionalStringParameter(prop.param(1))));
      else if (prop.name == "date")
      {
        if (prop.num_params() == 2)
          return AddMultiInfo(GUIInfo(SYSTEM_DATE, StringUtils::DateStringToYYYYMMDD(prop.param(0)) % 10000, StringUtils::DateStringToYYYYMMDD(prop.param(1)) % 10000));
        else if (prop.num_params() == 1)
        {
          int dateformat = StringUtils::DateStringToYYYYMMDD(prop.param(0));
          if (dateformat <= 0) // not concrete date
            return AddMultiInfo(GUIInfo(SYSTEM_DATE, ConditionalStringParameter(prop.param(0), true), -1));
          else
            return AddMultiInfo(GUIInfo(SYSTEM_DATE, dateformat % 10000));
        }
        return SYSTEM_DATE;
      }
      else if (prop.name == "time")
      {
        if (prop.num_params() == 0)
          return AddMultiInfo(GUIInfo(SYSTEM_TIME, TIME_FORMAT_GUESS));
        if (prop.num_params() == 1)
        {
          TIME_FORMAT timeFormat = TranslateTimeFormat(prop.param(0));
          if (timeFormat == TIME_FORMAT_GUESS)
            return AddMultiInfo(GUIInfo(SYSTEM_TIME, StringUtils::TimeStringToSeconds(prop.param(0))));
          return AddMultiInfo(GUIInfo(SYSTEM_TIME, timeFormat));
        }
        else
          return AddMultiInfo(GUIInfo(SYSTEM_TIME, StringUtils::TimeStringToSeconds(prop.param(0)), StringUtils::TimeStringToSeconds(prop.param(1))));
      }
    }
    else if (cat.name == "library")
    {
      if (prop.name == "isscanning") return LIBRARY_IS_SCANNING;
      else if (prop.name == "isscanningvideo") return LIBRARY_IS_SCANNING_VIDEO; //! @todo change to IsScanning(Video)
      else if (prop.name == "isscanningmusic") return LIBRARY_IS_SCANNING_MUSIC;
      else if (prop.name == "hascontent" && prop.num_params())
      {
        std::string cat = prop.param(0);
        StringUtils::ToLower(cat);
        if (cat == "music") return LIBRARY_HAS_MUSIC;
        else if (cat == "video") return LIBRARY_HAS_VIDEO;
        else if (cat == "movies") return LIBRARY_HAS_MOVIES;
        else if (cat == "tvshows") return LIBRARY_HAS_TVSHOWS;
        else if (cat == "musicvideos") return LIBRARY_HAS_MUSICVIDEOS;
        else if (cat == "moviesets") return LIBRARY_HAS_MOVIE_SETS;
        else if (cat == "singles") return LIBRARY_HAS_SINGLES;
        else if (cat == "compilations") return LIBRARY_HAS_COMPILATIONS;
        else if (cat == "role" && prop.num_params() > 1)
          return AddMultiInfo(GUIInfo(LIBRARY_HAS_ROLE, ConditionalStringParameter(prop.param(1)), 0));
      }
    }
    else if (cat.name == "musicplayer")
    {
      for (size_t i = 0; i < sizeof(player_times) / sizeof(infomap); i++) //! @todo remove these, they're repeats
      {
        if (prop.name == player_times[i].str)
          return AddMultiInfo(GUIInfo(player_times[i].val, TranslateTimeFormat(prop.param())));
      }
      if (prop.name == "content" && prop.num_params())
        return AddMultiInfo(GUIInfo(MUSICPLAYER_CONTENT, ConditionalStringParameter(prop.param()), 0));
      else if (prop.name == "property")
      {
        // properties are stored case sensitive in m_listItemProperties, but lookup is insensitive in CGUIListItem::GetProperty
        if (StringUtils::EqualsNoCase(prop.param(), "fanart_image"))
          return AddMultiInfo(GUIInfo(PLAYER_ITEM_ART, ConditionalStringParameter("fanart")));
        return AddListItemProp(prop.param(), MUSICPLAYER_PROPERTY_OFFSET);
      }
      return TranslateMusicPlayerString(prop.name);
    }
    else if (cat.name == "videoplayer")
    {
      for (size_t i = 0; i < sizeof(player_times) / sizeof(infomap); i++) //! @todo remove these, they're repeats
      {
        if (prop.name == player_times[i].str)
          return AddMultiInfo(GUIInfo(player_times[i].val, TranslateTimeFormat(prop.param())));
      }
      if (prop.name == "content" && prop.num_params())
      {
        return AddMultiInfo(GUIInfo(VIDEOPLAYER_CONTENT, ConditionalStringParameter(prop.param()), 0));
      }
      for (size_t i = 0; i < sizeof(videoplayer) / sizeof(infomap); i++)
      {
        if (prop.name == videoplayer[i].str)
          return videoplayer[i].val;
      }
    }
    else if (cat.name == "slideshow")
    {
      for (size_t i = 0; i < sizeof(slideshow) / sizeof(infomap); i++)
      {
        if (prop.name == slideshow[i].str)
          return slideshow[i].val;
      }
      return CPictureInfoTag::TranslateString(prop.name);
    }
    else if (cat.name == "container")
    {
      for (size_t i = 0; i < sizeof(mediacontainer) / sizeof(infomap); i++) // these ones don't have or need an id
      {
        if (prop.name == mediacontainer[i].str)
          return mediacontainer[i].val;
      }
      int id = atoi(cat.param().c_str());
      for (size_t i = 0; i < sizeof(container_bools) / sizeof(infomap); i++) // these ones can have an id (but don't need to?)
      {
        if (prop.name == container_bools[i].str)
          return id ? AddMultiInfo(GUIInfo(container_bools[i].val, id)) : container_bools[i].val;
      }
      for (size_t i = 0; i < sizeof(container_ints) / sizeof(infomap); i++) // these ones can have an int param on the property
      {
        if (prop.name == container_ints[i].str)
          return AddMultiInfo(GUIInfo(container_ints[i].val, id, atoi(prop.param().c_str())));
      }
      for (size_t i = 0; i < sizeof(container_str) / sizeof(infomap); i++) // these ones have a string param on the property
      {
        if (prop.name == container_str[i].str)
          return AddMultiInfo(GUIInfo(container_str[i].val, id, ConditionalStringParameter(prop.param())));
      }
      if (prop.name == "sortdirection")
      {
        SortOrder order = SortOrderNone;
        if (StringUtils::EqualsNoCase(prop.param(), "ascending"))
          order = SortOrderAscending;
        else if (StringUtils::EqualsNoCase(prop.param(), "descending"))
          order = SortOrderDescending;
        return AddMultiInfo(GUIInfo(CONTAINER_SORT_DIRECTION, order));
      }
    }
    else if (cat.name == "listitem" || cat.name == "listitemposition"
      || cat.name == "listitemnowrap" || cat.name == "listitemabsolute")
    {
      int offset = atoi(cat.param().c_str());
      int ret = TranslateListItem(prop);
      if (ret)
        listItemDependent = true;
      if (offset)
      {
        if (cat.name == "listitem")
          return AddMultiInfo(GUIInfo(ret, 0, offset, INFOFLAG_LISTITEM_WRAP));
        else if (cat.name == "listitemposition")
          return AddMultiInfo(GUIInfo(ret, 0, offset, INFOFLAG_LISTITEM_POSITION));
        else if (cat.name == "listitemabsolute")
          return AddMultiInfo(GUIInfo(ret, 0, offset, INFOFLAG_LISTITEM_ABSOLUTE));
        else if (cat.name == "listitemnowrap")
          return AddMultiInfo(GUIInfo(ret, 0, offset));
      }
      return ret;
    }
    else if (cat.name == "visualisation")
    {
      for (size_t i = 0; i < sizeof(visualisation) / sizeof(infomap); i++)
      {
        if (prop.name == visualisation[i].str)
          return visualisation[i].val;
      }
    }
    else if (cat.name == "fanart")
    {
      for (size_t i = 0; i < sizeof(fanart_labels) / sizeof(infomap); i++)
      {
        if (prop.name == fanart_labels[i].str)
          return fanart_labels[i].val;
      }
    }
    else if (cat.name == "skin")
    {
      for (size_t i = 0; i < sizeof(skin_labels) / sizeof(infomap); i++)
      {
        if (prop.name == skin_labels[i].str)
          return skin_labels[i].val;
      }
      if (prop.num_params())
      {
        if (prop.name == "string")
        {
          if (prop.num_params() == 2)
            return AddMultiInfo(GUIInfo(SKIN_STRING, CSkinSettings::GetInstance().TranslateString(prop.param(0)), ConditionalStringParameter(prop.param(1))));
          else
            return AddMultiInfo(GUIInfo(SKIN_STRING, CSkinSettings::GetInstance().TranslateString(prop.param(0))));
        }
        if (prop.name == "hassetting")
          return AddMultiInfo(GUIInfo(SKIN_BOOL, CSkinSettings::GetInstance().TranslateBool(prop.param(0))));
        else if (prop.name == "hastheme")
          return AddMultiInfo(GUIInfo(SKIN_HAS_THEME, ConditionalStringParameter(prop.param(0))));
      }
    }
    else if (cat.name == "window")
    {
      if (prop.name == "property" && prop.num_params() == 1)
      { //! @todo this doesn't support foo.xml
        int winID = cat.param().empty() ? 0 : CButtonTranslator::TranslateWindow(cat.param());
        if (winID != WINDOW_INVALID)
          return AddMultiInfo(GUIInfo(WINDOW_PROPERTY, winID, ConditionalStringParameter(prop.param())));
      }
      for (size_t i = 0; i < sizeof(window_bools) / sizeof(infomap); i++)
      {
        if (prop.name == window_bools[i].str)
        { //! @todo The parameter for these should really be on the first not the second property
          if (prop.param().find("xml") != std::string::npos)
            return AddMultiInfo(GUIInfo(window_bools[i].val, 0, ConditionalStringParameter(prop.param())));
          int winID = prop.param().empty() ? WINDOW_INVALID : CButtonTranslator::TranslateWindow(prop.param());
          return winID != WINDOW_INVALID ? AddMultiInfo(GUIInfo(window_bools[i].val, winID, 0)) : window_bools[i].val;
        }
      }
    }
    else if (cat.name == "control")
    {
      for (size_t i = 0; i < sizeof(control_labels) / sizeof(infomap); i++)
      {
        if (prop.name == control_labels[i].str)
        { //! @todo The parameter for these should really be on the first not the second property
          int controlID = atoi(prop.param().c_str());
          if (controlID)
            return AddMultiInfo(GUIInfo(control_labels[i].val, controlID, 0));
          return 0;
        }
      }
    }
    else if (cat.name == "controlgroup" && prop.name == "hasfocus")
    {
      int groupID = atoi(cat.param().c_str());
      if (groupID)
        return AddMultiInfo(GUIInfo(CONTROL_GROUP_HAS_FOCUS, groupID, atoi(prop.param(0).c_str())));
    }
    else if (cat.name == "playlist")
    {
      int ret = -1;
      for (size_t i = 0; i < sizeof(playlist) / sizeof(infomap); i++)
      {
        if (prop.name == playlist[i].str)
        {
          ret = playlist[i].val;
          break;
        }
      }
      if (ret >= 0)
      {
        if (prop.num_params() <= 0)
          return ret;
        else
        {
          int playlistid = PLAYLIST_NONE;
          if (StringUtils::EqualsNoCase(prop.param(), "video"))
            playlistid = PLAYLIST_VIDEO;
          else if (StringUtils::EqualsNoCase(prop.param(), "music"))
            playlistid = PLAYLIST_MUSIC;

          if (playlistid > PLAYLIST_NONE)
            return AddMultiInfo(GUIInfo(ret, playlistid));
        }
      }
    }
    else if (cat.name == "pvr")
    {
      for (size_t i = 0; i < sizeof(pvr) / sizeof(infomap); i++)
      {
        if (prop.name == pvr[i].str)
          return pvr[i].val;
      }
    }
    else if (cat.name == "adsp")
    {
      for (size_t i = 0; i < sizeof(adsp) / sizeof(infomap); i++)
      {
        if (prop.name == adsp[i].str)
          return adsp[i].val;
      }
    }
    else if (cat.name == "rds")
    {
      if (prop.name == "getline")
        return AddMultiInfo(GUIInfo(RDS_GET_RADIOTEXT_LINE, atoi(prop.param(0).c_str())));

      for (size_t i = 0; i < sizeof(rds) / sizeof(infomap); i++)
      {
        if (prop.name == rds[i].str)
          return rds[i].val;
      }
    }
  }
  else if (info.size() == 3 || info.size() == 4)
  {
    if (info[0].name == "system" && info[1].name == "platform")
    { //! @todo replace with a single system.platform
      std::string platform = info[2].name;
      if (platform == "linux")
      {
        if (info.size() == 4)
        {
          std::string device = info[3].name;
          if (device == "raspberrypi") return SYSTEM_PLATFORM_LINUX_RASPBERRY_PI;
        }
        else return SYSTEM_PLATFORM_LINUX;
      }
      else if (platform == "windows") return SYSTEM_PLATFORM_WINDOWS;
      else if (platform == "darwin")  return SYSTEM_PLATFORM_DARWIN;
      else if (platform == "osx")  return SYSTEM_PLATFORM_DARWIN_OSX;
      else if (platform == "ios")  return SYSTEM_PLATFORM_DARWIN_IOS;
      else if (platform == "android") return SYSTEM_PLATFORM_ANDROID;
    }
    if (info[0].name == "musicplayer")
    { //! @todo these two don't allow duration(foo) and also don't allow more than this number of levels...
      if (info[1].name == "position")
      {
        int position = atoi(info[1].param().c_str());
        int value = TranslateMusicPlayerString(info[2].name); // musicplayer.position(foo).bar
        return AddMultiInfo(GUIInfo(value, 0, position));
      }
      else if (info[1].name == "offset")
      {
        int position = atoi(info[1].param().c_str());
        int value = TranslateMusicPlayerString(info[2].name); // musicplayer.offset(foo).bar
        return AddMultiInfo(GUIInfo(value, 1, position));
      }
    }
    else if (info[0].name == "container")
    {
      int id = atoi(info[0].param().c_str());
      int offset = atoi(info[1].param().c_str());
      if (info[1].name == "listitemnowrap")
      {
        listItemDependent = true;
        return AddMultiInfo(GUIInfo(TranslateListItem(info[2]), id, offset));
      }
      else if (info[1].name == "listitemposition")
      {
        listItemDependent = true;
        return AddMultiInfo(GUIInfo(TranslateListItem(info[2]), id, offset, INFOFLAG_LISTITEM_POSITION));
      }
      else if (info[1].name == "listitem")
      {
        listItemDependent = true;
        return AddMultiInfo(GUIInfo(TranslateListItem(info[2]), id, offset, INFOFLAG_LISTITEM_WRAP));
      }
      else if (info[1].name == "listitemabsolute")
      {
        listItemDependent = true;
        return AddMultiInfo(GUIInfo(TranslateListItem(info[2]), id, offset, INFOFLAG_LISTITEM_ABSOLUTE));
      }
    }
    else if (info[0].name == "control")
    {
      const Property &prop = info[1];
      for (size_t i = 0; i < sizeof(control_labels) / sizeof(infomap); i++)
      {
        if (prop.name == control_labels[i].str)
        { //! @todo The parameter for these should really be on the first not the second property
          int controlID = atoi(prop.param().c_str());
          if (controlID)
            return AddMultiInfo(GUIInfo(control_labels[i].val, controlID, atoi(info[2].param(0).c_str())));
          return 0;
        }
      }
    }
  }

  return 0;
}

int CGUIInfoManager::TranslateListItem(const Property &info)
{
  if (info.num_params() == 1)
  {
    if (info.name == "property")
    {
      // properties are stored case sensitive in m_listItemProperties, but lookup is insensitive in CGUIListItem::GetProperty
      if (StringUtils::EqualsNoCase(info.param(), "fanart_image"))
        return AddListItemProp("fanart", LISTITEM_ART_OFFSET);
      return AddListItemProp(info.param());
    }
    if (info.name == "art")
      return AddListItemProp(info.param(), LISTITEM_ART_OFFSET);
    if (info.name == "ratings")
      return AddListItemProp(info.param(), LISTITEM_RATING_OFFSET);
    if (info.name == "votes")
      return AddListItemProp(info.param(), LISTITEM_VOTES_OFFSET);
    if (info.name == "ratingandvotes")
      return AddListItemProp(info.param(), LISTITEM_RATING_AND_VOTES_OFFSET);
  }

  for (size_t i = 0; i < sizeof(listitem_labels) / sizeof(infomap); i++) // these ones don't have or need an id
  {
    if (info.name == listitem_labels[i].str)
      return listitem_labels[i].val;
  }
  return 0;
}

int CGUIInfoManager::TranslateMusicPlayerString(const std::string &info) const
{
  for (size_t i = 0; i < sizeof(musicplayer) / sizeof(infomap); i++)
  {
    if (info == musicplayer[i].str)
      return musicplayer[i].val;
  }
  return 0;
}

TIME_FORMAT CGUIInfoManager::TranslateTimeFormat(const std::string &format)
{
  if (format.empty()) return TIME_FORMAT_GUESS;
  else if (StringUtils::EqualsNoCase(format, "hh")) return TIME_FORMAT_HH;
  else if (StringUtils::EqualsNoCase(format, "mm")) return TIME_FORMAT_MM;
  else if (StringUtils::EqualsNoCase(format, "ss")) return TIME_FORMAT_SS;
  else if (StringUtils::EqualsNoCase(format, "hh:mm")) return TIME_FORMAT_HH_MM;
  else if (StringUtils::EqualsNoCase(format, "mm:ss")) return TIME_FORMAT_MM_SS;
  else if (StringUtils::EqualsNoCase(format, "hh:mm:ss")) return TIME_FORMAT_HH_MM_SS;
  else if (StringUtils::EqualsNoCase(format, "hh:mm:ss xx")) return TIME_FORMAT_HH_MM_SS_XX;
  else if (StringUtils::EqualsNoCase(format, "h")) return TIME_FORMAT_H;
  else if (StringUtils::EqualsNoCase(format, "h:mm:ss")) return TIME_FORMAT_H_MM_SS;
  else if (StringUtils::EqualsNoCase(format, "h:mm:ss xx")) return TIME_FORMAT_H_MM_SS_XX;
  else if (StringUtils::EqualsNoCase(format, "xx")) return TIME_FORMAT_XX;
  return TIME_FORMAT_GUESS;
}

std::string CGUIInfoManager::GetLabel(int info, int contextWindow, std::string *fallback)
{
  if (info >= CONDITIONAL_LABEL_START && info <= CONDITIONAL_LABEL_END)
    return GetSkinVariableString(info, false);

  std::string strLabel;
  if (info >= MULTI_INFO_START && info <= MULTI_INFO_END)
    return GetMultiInfoLabel(m_multiInfo[info - MULTI_INFO_START], contextWindow);

  if (info >= SLIDE_INFO_START && info <= SLIDE_INFO_END)
    return GetPictureLabel(info);

  if (info >= LISTITEM_PROPERTY_START+MUSICPLAYER_PROPERTY_OFFSET &&
      info - (LISTITEM_PROPERTY_START+MUSICPLAYER_PROPERTY_OFFSET) < (int)m_listitemProperties.size())
  { // grab the property
    if (!m_currentFile)
      return "";

    std::string property = m_listitemProperties[info - LISTITEM_PROPERTY_START-MUSICPLAYER_PROPERTY_OFFSET];
    if (StringUtils::StartsWithNoCase(property, "Role.") && m_currentFile->HasMusicInfoTag())
    { // "Role.xxxx" properties are held in music tag
      property.erase(0, 5); //Remove Role.
      return m_currentFile->GetMusicInfoTag()->GetArtistStringForRole(property);
    }
    return m_currentFile->GetProperty(property).asString();
  }

  if (info >= LISTITEM_START && info <= LISTITEM_END)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_HAS_LIST_ITEMS); // true for has list items
    if (window)
    {
      CFileItemPtr item = window->GetCurrentListItem();
      strLabel = GetItemLabel(item.get(), info, fallback);
    }

    return strLabel;
  }

  switch (info)
  {
  case PVR_NEXT_RECORDING_CHANNEL:
  case PVR_NEXT_RECORDING_CHAN_ICO:
  case PVR_NEXT_RECORDING_DATETIME:
  case PVR_NEXT_RECORDING_TITLE:
  case PVR_NOW_RECORDING_CHANNEL:
  case PVR_NOW_RECORDING_CHAN_ICO:
  case PVR_NOW_RECORDING_DATETIME:
  case PVR_NOW_RECORDING_TITLE:
  case PVR_BACKEND_NAME:
  case PVR_BACKEND_VERSION:
  case PVR_BACKEND_HOST:
  case PVR_BACKEND_DISKSPACE:
  case PVR_BACKEND_CHANNELS:
  case PVR_BACKEND_TIMERS:
  case PVR_BACKEND_RECORDINGS:
  case PVR_BACKEND_DELETED_RECORDINGS:
  case PVR_BACKEND_NUMBER:
  case PVR_TOTAL_DISKSPACE:
  case PVR_NEXT_TIMER:
  case PVR_PLAYING_DURATION:
  case PVR_PLAYING_TIME:
  case PVR_PLAYING_PROGRESS:
  case PVR_ACTUAL_STREAM_CLIENT:
  case PVR_ACTUAL_STREAM_DEVICE:
  case PVR_ACTUAL_STREAM_STATUS:
  case PVR_ACTUAL_STREAM_SIG:
  case PVR_ACTUAL_STREAM_SNR:
  case PVR_ACTUAL_STREAM_SIG_PROGR:
  case PVR_ACTUAL_STREAM_SNR_PROGR:
  case PVR_ACTUAL_STREAM_BER:
  case PVR_ACTUAL_STREAM_UNC:
  case PVR_ACTUAL_STREAM_CRYPTION:
  case PVR_ACTUAL_STREAM_SERVICE:
  case PVR_ACTUAL_STREAM_MUX:
  case PVR_ACTUAL_STREAM_PROVIDER:
  case PVR_TIMESHIFT_START_TIME:
  case PVR_TIMESHIFT_END_TIME:
  case PVR_TIMESHIFT_PLAY_TIME:
    g_PVRManager.TranslateCharInfo(info, strLabel);
    break;
  case ADSP_ACTIVE_STREAM_TYPE:
  case ADSP_DETECTED_STREAM_TYPE:
  case ADSP_MASTER_NAME:
  case ADSP_MASTER_INFO:
  case ADSP_MASTER_OWN_ICON:
  case ADSP_MASTER_OVERRIDE_ICON:
    CServiceBroker::GetADSP().TranslateCharInfo(info, strLabel);
    break;
  case WEATHER_CONDITIONS:
    strLabel = g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_COND);
    StringUtils::Trim(strLabel);
    break;
  case WEATHER_TEMPERATURE:
    strLabel = StringUtils::Format("%s%s",
                                   g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_TEMP).c_str(),
                                   g_langInfo.GetTemperatureUnitString().c_str());
    break;
  case WEATHER_LOCATION:
    strLabel = g_weatherManager.GetInfo(WEATHER_LABEL_LOCATION);
    break;
  case WEATHER_FANART_CODE:
    strLabel = URIUtils::GetFileName(g_weatherManager.GetInfo(WEATHER_IMAGE_CURRENT_ICON));
    URIUtils::RemoveExtension(strLabel);
    break;
  case WEATHER_PLUGIN:
    strLabel = CSettings::GetInstance().GetString(CSettings::SETTING_WEATHER_ADDON);
    break;
  case SYSTEM_DATE:
    strLabel = GetDate();
    break;
  case SYSTEM_FPS:
    strLabel = StringUtils::Format("%02.2f", m_fps);
    break;
  case PLAYER_VOLUME:
    strLabel = StringUtils::Format("%2.1f dB", CAEUtil::PercentToGain(g_application.GetVolume(false)));
    break;
  case PLAYER_SUBTITLE_DELAY:
    strLabel = StringUtils::Format("%2.3f s", CMediaSettings::GetInstance().GetCurrentVideoSettings().m_SubtitleDelay);
    break;
  case PLAYER_AUDIO_DELAY:
    strLabel = StringUtils::Format("%2.3f s", CMediaSettings::GetInstance().GetCurrentVideoSettings().m_AudioDelay);
    break;
  case PLAYER_CHAPTER:
    if(g_application.m_pPlayer->IsPlaying())
      strLabel = StringUtils::Format("%02d", g_application.m_pPlayer->GetChapter());
    break;
  case PLAYER_CHAPTERCOUNT:
    if(g_application.m_pPlayer->IsPlaying())
      strLabel = StringUtils::Format("%02d", g_application.m_pPlayer->GetChapterCount());
    break;
  case PLAYER_CHAPTERNAME:
    if(g_application.m_pPlayer->IsPlaying())
      g_application.m_pPlayer->GetChapterName(strLabel);
    break;
  case PLAYER_CACHELEVEL:
    {
      int iLevel = 0;
      if(g_application.m_pPlayer->IsPlaying() && GetInt(iLevel, PLAYER_CACHELEVEL) && iLevel >= 0)
        strLabel = StringUtils::Format("%i", iLevel);
    }
    break;
  case PLAYER_TIME:
    if(g_application.m_pPlayer->IsPlaying())
      strLabel = GetCurrentPlayTime(TIME_FORMAT_HH_MM);
    break;
  case PLAYER_DURATION:
    if(g_application.m_pPlayer->IsPlaying())
      strLabel = GetDuration(TIME_FORMAT_HH_MM);
    break;
  case PLAYER_PATH:
  case PLAYER_FILENAME:
  case PLAYER_FILEPATH:
    if (m_currentFile)
    {
      if (m_currentFile->HasMusicInfoTag())
        strLabel = m_currentFile->GetMusicInfoTag()->GetURL();
      else if (m_currentFile->HasVideoInfoTag())
        strLabel = m_currentFile->GetVideoInfoTag()->m_strFileNameAndPath;
      if (strLabel.empty())
        strLabel = m_currentFile->GetPath();
    }
    if (info == PLAYER_PATH)
    {
      // do this twice since we want the path outside the archive if this
      // is to be of use.
      if (URIUtils::IsInArchive(strLabel))
        strLabel = URIUtils::GetParentPath(strLabel);
      strLabel = URIUtils::GetParentPath(strLabel);
    }
    else if (info == PLAYER_FILENAME)
      strLabel = URIUtils::GetFileName(strLabel);
    break;
  case PLAYER_TITLE:
    {
      if(m_currentFile)
      {
        if (m_currentFile->HasPVRRadioRDSInfoTag())
        {
          /*! Load the RDS Radiotext+ if present */
          if (!m_currentFile->GetPVRRadioRDSInfoTag()->GetTitle().empty())
            return m_currentFile->GetPVRRadioRDSInfoTag()->GetTitle();
          /*! If no plus present load the RDS Radiotext info line 0 if present */
          if (!g_application.m_pPlayer->GetRadioText(0).empty())
            return g_application.m_pPlayer->GetRadioText(0);
        }
        if (m_currentFile->HasPVRChannelInfoTag())
        {
          CEpgInfoTagPtr tag(m_currentFile->GetPVRChannelInfoTag()->GetEPGNow());
          return tag ?
                   tag->Title() :
                   CSettings::GetInstance().GetBool(CSettings::SETTING_EPG_HIDENOINFOAVAILABLE) ?
                            "" : g_localizeStrings.Get(19055); // no information available
        }
        if (m_currentFile->HasPVRRecordingInfoTag() && !m_currentFile->GetPVRRecordingInfoTag()->m_strTitle.empty())
          return m_currentFile->GetPVRRecordingInfoTag()->m_strTitle;
        if (m_currentFile->HasVideoInfoTag() && !m_currentFile->GetVideoInfoTag()->m_strTitle.empty())
          return m_currentFile->GetVideoInfoTag()->m_strTitle;
        if (m_currentFile->HasMusicInfoTag() && !m_currentFile->GetMusicInfoTag()->GetTitle().empty())
          return m_currentFile->GetMusicInfoTag()->GetTitle();
        // don't have the title, so use VideoPlayer, label, or drop down to title from path
        if (!g_application.m_pPlayer->GetPlayingTitle().empty())
          return g_application.m_pPlayer->GetPlayingTitle();
        if (!m_currentFile->GetLabel().empty())
          return m_currentFile->GetLabel();
        return CUtil::GetTitleFromPath(m_currentFile->GetPath());
      }
      else
      {
        if (!g_application.m_pPlayer->GetPlayingTitle().empty())
          return g_application.m_pPlayer->GetPlayingTitle();
      }
    }
    break;
  case PLAYER_PLAYSPEED:
      if(g_application.m_pPlayer->IsPlaying())
        strLabel = StringUtils::Format("%.2f", g_application.m_pPlayer->GetPlaySpeed());
      break;
  case MUSICPLAYER_TITLE:
  case MUSICPLAYER_ALBUM:
  case MUSICPLAYER_ARTIST:
  case MUSICPLAYER_ALBUM_ARTIST:
  case MUSICPLAYER_GENRE:
  case MUSICPLAYER_YEAR:
  case MUSICPLAYER_TRACK_NUMBER:
  case MUSICPLAYER_BITRATE:
  case MUSICPLAYER_PLAYLISTLEN:
  case MUSICPLAYER_PLAYLISTPOS:
  case MUSICPLAYER_CHANNELS:
  case MUSICPLAYER_BITSPERSAMPLE:
  case MUSICPLAYER_SAMPLERATE:
  case MUSICPLAYER_CODEC:
  case MUSICPLAYER_DISC_NUMBER:
  case MUSICPLAYER_RATING:
  case MUSICPLAYER_RATING_AND_VOTES:
  case MUSICPLAYER_USER_RATING:
  case MUSICPLAYER_COMMENT:
  case MUSICPLAYER_CONTRIBUTORS:
  case MUSICPLAYER_CONTRIBUTOR_AND_ROLE:
  case MUSICPLAYER_LYRICS:
  case MUSICPLAYER_CHANNEL_NAME:
  case MUSICPLAYER_CHANNEL_NUMBER:
  case MUSICPLAYER_SUB_CHANNEL_NUMBER:
  case MUSICPLAYER_CHANNEL_NUMBER_LBL:
  case MUSICPLAYER_CHANNEL_GROUP:
  case MUSICPLAYER_PLAYCOUNT:
  case MUSICPLAYER_LASTPLAYED:
    strLabel = GetMusicLabel(info);
  break;
  case VIDEOPLAYER_TITLE:
  case VIDEOPLAYER_ORIGINALTITLE:
  case VIDEOPLAYER_GENRE:
  case VIDEOPLAYER_DIRECTOR:
  case VIDEOPLAYER_YEAR:
  case VIDEOPLAYER_PLAYLISTLEN:
  case VIDEOPLAYER_PLAYLISTPOS:
  case VIDEOPLAYER_PLOT:
  case VIDEOPLAYER_PLOT_OUTLINE:
  case VIDEOPLAYER_EPISODE:
  case VIDEOPLAYER_SEASON:
  case VIDEOPLAYER_RATING:
  case VIDEOPLAYER_RATING_AND_VOTES:
  case VIDEOPLAYER_USER_RATING:
  case VIDEOPLAYER_TVSHOW:
  case VIDEOPLAYER_PREMIERED:
  case VIDEOPLAYER_STUDIO:
  case VIDEOPLAYER_COUNTRY:
  case VIDEOPLAYER_MPAA:
  case VIDEOPLAYER_TOP250:
  case VIDEOPLAYER_CAST:
  case VIDEOPLAYER_CAST_AND_ROLE:
  case VIDEOPLAYER_ARTIST:
  case VIDEOPLAYER_ALBUM:
  case VIDEOPLAYER_WRITER:
  case VIDEOPLAYER_TAGLINE:
  case VIDEOPLAYER_TRAILER:
  case VIDEOPLAYER_STARTTIME:
  case VIDEOPLAYER_ENDTIME:
  case VIDEOPLAYER_NEXT_TITLE:
  case VIDEOPLAYER_NEXT_GENRE:
  case VIDEOPLAYER_NEXT_PLOT:
  case VIDEOPLAYER_NEXT_PLOT_OUTLINE:
  case VIDEOPLAYER_NEXT_STARTTIME:
  case VIDEOPLAYER_NEXT_ENDTIME:
  case VIDEOPLAYER_NEXT_DURATION:
  case VIDEOPLAYER_CHANNEL_NAME:
  case VIDEOPLAYER_CHANNEL_NUMBER:
  case VIDEOPLAYER_SUB_CHANNEL_NUMBER:
  case VIDEOPLAYER_CHANNEL_NUMBER_LBL:
  case VIDEOPLAYER_CHANNEL_GROUP:
  case VIDEOPLAYER_PARENTAL_RATING:
  case VIDEOPLAYER_PLAYCOUNT:
  case VIDEOPLAYER_LASTPLAYED:
  case VIDEOPLAYER_IMDBNUMBER:
  case VIDEOPLAYER_EPISODENAME:
    strLabel = GetVideoLabel(info);
  break;
  case VIDEOPLAYER_VIDEO_CODEC:
    if(g_application.m_pPlayer->IsPlaying())
    {
      strLabel = m_videoInfo.videoCodecName;
    }
    break;
  case VIDEOPLAYER_VIDEO_RESOLUTION:
    if(g_application.m_pPlayer->IsPlaying())
    {
      return CStreamDetails::VideoDimsToResolutionDescription(m_videoInfo.width, m_videoInfo.height);
    }
    break;
  case VIDEOPLAYER_AUDIO_CODEC:
    if(g_application.m_pPlayer->IsPlaying())
    {
      strLabel = m_audioInfo.audioCodecName;
    }
    break;
  case VIDEOPLAYER_VIDEO_ASPECT:
    if (g_application.m_pPlayer->IsPlaying())
    {
      strLabel = CStreamDetails::VideoAspectToAspectDescription(m_videoInfo.videoAspectRatio);
    }
    break;
  case VIDEOPLAYER_AUDIO_CHANNELS:
    if(g_application.m_pPlayer->IsPlaying())
    {
      if (m_audioInfo.channels > 0)
        strLabel = StringUtils::Format("%i", m_audioInfo.channels);
    }
    break;
  case VIDEOPLAYER_AUDIO_LANG:
    if(g_application.m_pPlayer->IsPlaying())
    {
      strLabel = m_audioInfo.language;
    }
    break;
  case VIDEOPLAYER_STEREOSCOPIC_MODE:
    if(g_application.m_pPlayer->IsPlaying())
    {
      strLabel = m_videoInfo.stereoMode;
    }
    break;
  case VIDEOPLAYER_SUBTITLES_LANG:
    if(g_application.m_pPlayer && g_application.m_pPlayer->IsPlaying() && g_application.m_pPlayer->GetSubtitleVisible())
    {
      SPlayerSubtitleStreamInfo info;
      g_application.m_pPlayer->GetSubtitleStreamInfo(g_application.m_pPlayer->GetSubtitle(), info);
      strLabel = info.language;
    }
    break;
  case PLAYER_PROCESS_VIDEODECODER:
      strLabel = CServiceBroker::GetDataCacheCore().GetVideoDecoderName();
      break;
  case PLAYER_PROCESS_DEINTMETHOD:
      strLabel = CServiceBroker::GetDataCacheCore().GetVideoDeintMethod();
      break;
  case PLAYER_PROCESS_PIXELFORMAT:
      strLabel = CServiceBroker::GetDataCacheCore().GetVideoPixelFormat();
      break;
  case PLAYER_PROCESS_VIDEOFPS:
      strLabel = StringUtils::Format("%.3f", CServiceBroker::GetDataCacheCore().GetVideoFps());
      break;
  case PLAYER_PROCESS_VIDEODAR:
      strLabel = StringUtils::Format("%.2f", CServiceBroker::GetDataCacheCore().GetVideoDAR());
      break;
  case PLAYER_PROCESS_VIDEOWIDTH:
      strLabel = StringUtils::FormatNumber(CServiceBroker::GetDataCacheCore().GetVideoWidth());
      break;
  case PLAYER_PROCESS_VIDEOHEIGHT:
      strLabel = StringUtils::FormatNumber(CServiceBroker::GetDataCacheCore().GetVideoHeight());
      break;
  case PLAYER_PROCESS_AUDIODECODER:
      strLabel = CServiceBroker::GetDataCacheCore().GetAudioDecoderName();
      break;
  case PLAYER_PROCESS_AUDIOCHANNELS:
      strLabel = CServiceBroker::GetDataCacheCore().GetAudioChannels();
      break;
  case PLAYER_PROCESS_AUDIOSAMPLERATE:
      strLabel = StringUtils::FormatNumber(CServiceBroker::GetDataCacheCore().GetAudioSampleRate());
      break;
  case PLAYER_PROCESS_AUDIOBITSPERSAMPLE:
      strLabel = StringUtils::FormatNumber(CServiceBroker::GetDataCacheCore().GetAudioBitsPerSampe());
      break;
  case RDS_AUDIO_LANG:
  case RDS_CHANNEL_COUNTRY:
  case RDS_TITLE:
  case RDS_BAND:
  case RDS_COMPOSER:
  case RDS_CONDUCTOR:
  case RDS_ALBUM:
  case RDS_ALBUM_TRACKNUMBER:
  case RDS_GET_RADIO_STYLE:
  case RDS_COMMENT:
  case RDS_ARTIST:
  case RDS_INFO_NEWS:
  case RDS_INFO_NEWS_LOCAL:
  case RDS_INFO_STOCK:
  case RDS_INFO_STOCK_SIZE:
  case RDS_INFO_SPORT:
  case RDS_INFO_SPORT_SIZE:
  case RDS_INFO_LOTTERY:
  case RDS_INFO_LOTTERY_SIZE:
  case RDS_INFO_WEATHER:
  case RDS_INFO_WEATHER_SIZE:
  case RDS_INFO_CINEMA:
  case RDS_INFO_CINEMA_SIZE:
  case RDS_INFO_HOROSCOPE:
  case RDS_INFO_HOROSCOPE_SIZE:
  case RDS_INFO_OTHER:
  case RDS_INFO_OTHER_SIZE:
  case RDS_PROG_STATION:
  case RDS_PROG_NOW:
  case RDS_PROG_NEXT:
  case RDS_PROG_HOST:
  case RDS_PROG_EDIT_STAFF:
  case RDS_PROG_HOMEPAGE:
  case RDS_PROG_STYLE:
  case RDS_PHONE_HOTLINE:
  case RDS_PHONE_STUDIO:
  case RDS_SMS_STUDIO:
  case RDS_EMAIL_HOTLINE:
  case RDS_EMAIL_STUDIO:
    strLabel = GetRadioRDSLabel(info);
  break;
  case PLAYLIST_LENGTH:
  case PLAYLIST_POSITION:
  case PLAYLIST_RANDOM:
  case PLAYLIST_REPEAT:
    strLabel = GetPlaylistLabel(info);
  break;
  case MUSICPM_SONGSPLAYED:
  case MUSICPM_MATCHINGSONGS:
  case MUSICPM_MATCHINGSONGSPICKED:
  case MUSICPM_MATCHINGSONGSLEFT:
  case MUSICPM_RELAXEDSONGSPICKED:
  case MUSICPM_RANDOMSONGSPICKED:
    strLabel = GetMusicPartyModeLabel(info);
  break;

  case SYSTEM_FREE_SPACE:
  case SYSTEM_USED_SPACE:
  case SYSTEM_TOTAL_SPACE:
  case SYSTEM_FREE_SPACE_PERCENT:
  case SYSTEM_USED_SPACE_PERCENT:
    return g_sysinfo.GetHddSpaceInfo(info);
  break;

  case SYSTEM_CPU_TEMPERATURE:
  case SYSTEM_GPU_TEMPERATURE:
  case SYSTEM_FAN_SPEED:
  case SYSTEM_CPU_USAGE:
    return GetSystemHeatInfo(info);
    break;

  case SYSTEM_VIDEO_ENCODER_INFO:
  case NETWORK_MAC_ADDRESS:
  case SYSTEM_OS_VERSION_INFO:
  case SYSTEM_CPUFREQUENCY:
  case SYSTEM_INTERNET_STATE:
  case SYSTEM_UPTIME:
  case SYSTEM_TOTALUPTIME:
  case SYSTEM_BATTERY_LEVEL:
    return g_sysinfo.GetInfo(info);
    break;

  case SYSTEM_SCREEN_RESOLUTION:
    if(g_Windowing.IsFullScreen())
      strLabel = StringUtils::Format("%ix%i@%.2fHz - %s",
        CDisplaySettings::GetInstance().GetCurrentResolutionInfo().iScreenWidth,
        CDisplaySettings::GetInstance().GetCurrentResolutionInfo().iScreenHeight,
        CDisplaySettings::GetInstance().GetCurrentResolutionInfo().fRefreshRate,
        g_localizeStrings.Get(244).c_str());
    else
      strLabel = StringUtils::Format("%ix%i - %s",
        CDisplaySettings::GetInstance().GetCurrentResolutionInfo().iScreenWidth,
        CDisplaySettings::GetInstance().GetCurrentResolutionInfo().iScreenHeight,
        g_localizeStrings.Get(242).c_str());
    return strLabel;
    break;

  case CONTAINER_FOLDERPATH:
  case CONTAINER_FOLDERNAME:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        if (info==CONTAINER_FOLDERNAME)
          strLabel = ((CGUIMediaWindow*)window)->CurrentDirectory().GetLabel();
        else
          strLabel = CURL(((CGUIMediaWindow*)window)->CurrentDirectory().GetPath()).GetWithoutUserDetails();
      }
      break;
    }
  case CONTAINER_PLUGINNAME:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        CURL url(((CGUIMediaWindow*)window)->CurrentDirectory().GetPath());
        if (url.IsProtocol("plugin"))
          strLabel = URIUtils::GetFileName(url.GetHostName());
      }
      break;
    }
  case CONTAINER_VIEWCOUNT:
  case CONTAINER_VIEWMODE:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        const CGUIControl *control = window->GetControl(window->GetViewContainerID());
        if (control && control->IsContainer())
        {
          if (info == CONTAINER_VIEWMODE)
            strLabel = ((IGUIContainer *)control)->GetLabel();
          else if (info == CONTAINER_VIEWCOUNT)
            strLabel = StringUtils::Format("%i", window->GetViewCount());
        }
      }
      break;
    }
  case CONTAINER_SORT_METHOD:
  case CONTAINER_SORT_ORDER:
  {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        const CGUIViewState *viewState = ((CGUIMediaWindow*)window)->GetViewState();
        if (viewState)
        {
          if (info == CONTAINER_SORT_METHOD)
            strLabel = g_localizeStrings.Get(viewState->GetSortMethodLabel());
          else if (info == CONTAINER_SORT_ORDER)
            strLabel = g_localizeStrings.Get(viewState->GetSortOrderLabel());
        }
      }
    }
    break;
  case CONTAINER_NUM_PAGES:
  case CONTAINER_NUM_ITEMS:
  case CONTAINER_CURRENT_ITEM:
  case CONTAINER_CURRENT_PAGE:
    return GetMultiInfoLabel(GUIInfo(info), contextWindow);
    break;
  case CONTAINER_SHOWPLOT:
  case CONTAINER_SHOWTITLE:
  case CONTAINER_PLUGINCATEGORY:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        if (info == CONTAINER_SHOWPLOT)
          return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty("showplot").asString();
        else if (info == CONTAINER_SHOWTITLE)
          return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty("showtitle").asString();
        else if (info == CONTAINER_PLUGINCATEGORY)
          return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty("plugincategory").asString();
      }
    }
    break;
  case CONTAINER_TOTALTIME:
  case CONTAINER_TOTALWATCHED:
  case CONTAINER_TOTALUNWATCHED:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        const CFileItemList& items=((CGUIMediaWindow *)window)->CurrentDirectory();
        int count=0;
        for (int i=0;i<items.Size();++i)
        {
          // Iterate through container and count watched, unwatched and total duration.
          CFileItemPtr item=items.Get(i);
          if (info == CONTAINER_TOTALWATCHED && item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_playCount > 0)
            count += 1;
          else if (info == CONTAINER_TOTALUNWATCHED && item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_playCount == 0)
            count += 1;
          else if (info == CONTAINER_TOTALTIME && item->HasMusicInfoTag())
            count += item->GetMusicInfoTag()->GetDuration();
          else if (info == CONTAINER_TOTALTIME && item->HasVideoInfoTag())
            count += item->GetVideoInfoTag()->m_streamDetails.GetVideoDuration();
        }
        if (info == CONTAINER_TOTALTIME && count > 0)
          return StringUtils::SecondsToTimeString(count);
        else if (info == CONTAINER_TOTALWATCHED || info == CONTAINER_TOTALUNWATCHED)
          return StringUtils::Format("%i", count);
      }
    }
    break;
  case SYSTEM_BUILD_VERSION_SHORT:
    strLabel = CSysInfo::GetVersionShort();
    break;
  case SYSTEM_BUILD_VERSION:
    strLabel = CSysInfo::GetVersion();
    break;
  case SYSTEM_BUILD_DATE:
    strLabel = CSysInfo::GetBuildDate();
    break;
  case SYSTEM_FREE_MEMORY:
  case SYSTEM_FREE_MEMORY_PERCENT:
  case SYSTEM_USED_MEMORY:
  case SYSTEM_USED_MEMORY_PERCENT:
  case SYSTEM_TOTAL_MEMORY:
    {
      MEMORYSTATUSEX stat;
      stat.dwLength = sizeof(MEMORYSTATUSEX);
      GlobalMemoryStatusEx(&stat);
      int iMemPercentFree = 100 - ((int)( 100.0f* (stat.ullTotalPhys - stat.ullAvailPhys)/stat.ullTotalPhys + 0.5f ));
      int iMemPercentUsed = 100 - iMemPercentFree;

      if (info == SYSTEM_FREE_MEMORY)
        strLabel = StringUtils::Format("%uMB", (unsigned int)(stat.ullAvailPhys/MB));
      else if (info == SYSTEM_FREE_MEMORY_PERCENT)
        strLabel = StringUtils::Format("%i%%", iMemPercentFree);
      else if (info == SYSTEM_USED_MEMORY)
        strLabel = StringUtils::Format("%uMB", (unsigned int)((stat.ullTotalPhys - stat.ullAvailPhys)/MB));
      else if (info == SYSTEM_USED_MEMORY_PERCENT)
        strLabel = StringUtils::Format("%i%%", iMemPercentUsed);
      else if (info == SYSTEM_TOTAL_MEMORY)
        strLabel = StringUtils::Format("%uMB", (unsigned int)(stat.ullTotalPhys/MB));
    }
    break;
  case SYSTEM_SCREEN_MODE:
    strLabel = g_graphicsContext.GetResInfo().strMode;
    break;
  case SYSTEM_SCREEN_WIDTH:
    strLabel = StringUtils::Format("%i", g_graphicsContext.GetResInfo().iScreenWidth);
    break;
  case SYSTEM_SCREEN_HEIGHT:
    strLabel = StringUtils::Format("%i", g_graphicsContext.GetResInfo().iScreenHeight);
    break;
  case SYSTEM_CURRENT_WINDOW:
    return g_localizeStrings.Get(g_windowManager.GetFocusedWindow());
    break;
  case SYSTEM_STARTUP_WINDOW:
    strLabel = StringUtils::Format("%i", CSettings::GetInstance().GetInt(CSettings::SETTING_LOOKANDFEEL_STARTUPWINDOW));
    break;
  case SYSTEM_CURRENT_CONTROL:
  case SYSTEM_CURRENT_CONTROL_ID:
    {
      CGUIWindow *window = g_windowManager.GetWindow(g_windowManager.GetFocusedWindow());
      if (window)
      {
        CGUIControl *control = window->GetFocusedControl();
        if (control)
        {
          if (info == SYSTEM_CURRENT_CONTROL_ID)
            strLabel = StringUtils::Format("%i", control->GetID());
          else if (info == SYSTEM_CURRENT_CONTROL)
            strLabel = control->GetDescription();
        }
      }
    }
    break;
#ifdef HAS_DVD_DRIVE
  case SYSTEM_DVD_LABEL:
    strLabel = g_mediaManager.GetDiskLabel();
    break;
#endif
  case SYSTEM_ALARM_POS:
    if (g_alarmClock.GetRemaining("shutdowntimer") == 0.f)
      strLabel = "";
    else
    {
      double fTime = g_alarmClock.GetRemaining("shutdowntimer");
      if (fTime > 60.f)
        strLabel = StringUtils::Format(g_localizeStrings.Get(13213).c_str(), g_alarmClock.GetRemaining("shutdowntimer")/60.f);
      else
        strLabel = StringUtils::Format(g_localizeStrings.Get(13214).c_str(), g_alarmClock.GetRemaining("shutdowntimer"));
    }
    break;
  case SYSTEM_PROFILENAME:
    strLabel = CProfilesManager::GetInstance().GetCurrentProfile().getName();
    break;
  case SYSTEM_PROFILECOUNT:
    strLabel = StringUtils::Format("%" PRIuS, CProfilesManager::GetInstance().GetNumberOfProfiles());
    break;
  case SYSTEM_PROFILEAUTOLOGIN:
    {
      int profileId = CProfilesManager::GetInstance().GetAutoLoginProfileId();
      if ((profileId < 0) || (!CProfilesManager::GetInstance().GetProfileName(profileId, strLabel)))
        strLabel = g_localizeStrings.Get(37014); // Last used profile
    }
    break;
  case SYSTEM_LANGUAGE:
    strLabel = g_langInfo.GetEnglishLanguageName();
    break;
  case SYSTEM_TEMPERATURE_UNITS:
    strLabel = g_langInfo.GetTemperatureUnitString();
    break;
  case SYSTEM_PROGRESS_BAR:
    {
      int percent;
      if (GetInt(percent, SYSTEM_PROGRESS_BAR) && percent > 0)
        strLabel = StringUtils::Format("%i", percent);
    }
    break;
  case SYSTEM_FRIENDLY_NAME:
    strLabel = CSysInfo::GetDeviceName();
    break;
  case SYSTEM_STEREOSCOPIC_MODE:
    {
      int stereoMode = CSettings::GetInstance().GetInt(CSettings::SETTING_VIDEOSCREEN_STEREOSCOPICMODE);
      strLabel = StringUtils::Format("%i", stereoMode);
    }
    break;

  case SKIN_THEME:
    strLabel = CSettings::GetInstance().GetString(CSettings::SETTING_LOOKANDFEEL_SKINTHEME);
    break;
  case SKIN_COLOUR_THEME:
    strLabel = CSettings::GetInstance().GetString(CSettings::SETTING_LOOKANDFEEL_SKINCOLORS);
    break;
  case SKIN_ASPECT_RATIO:
    if (g_SkinInfo)
      strLabel = g_SkinInfo->GetCurrentAspect();
    break;
  case NETWORK_IP_ADDRESS:
    {
      CNetworkInterface* iface = g_application.getNetwork().GetFirstConnectedInterface();
      if (iface)
        return iface->GetCurrentIPAddress();
    }
    break;
  case NETWORK_SUBNET_MASK:
    {
      CNetworkInterface* iface = g_application.getNetwork().GetFirstConnectedInterface();
      if (iface)
        return iface->GetCurrentNetmask();
    }
    break;
  case NETWORK_GATEWAY_ADDRESS:
    {
      CNetworkInterface* iface = g_application.getNetwork().GetFirstConnectedInterface();
      if (iface)
        return iface->GetCurrentDefaultGateway();
    }
    break;
  case NETWORK_DNS1_ADDRESS:
    {
      std::vector<std::string> nss = g_application.getNetwork().GetNameServers();
      if (nss.size() >= 1)
        return nss[0];
    }
    break;
  case NETWORK_DNS2_ADDRESS:
    {
      std::vector<std::string> nss = g_application.getNetwork().GetNameServers();
      if (nss.size() >= 2)
        return nss[1];
    }
    break;
  case NETWORK_DHCP_ADDRESS:
    {
      std::string dhcpserver;
      return dhcpserver;
    }
    break;
  case NETWORK_LINK_STATE:
    {
      std::string linkStatus = g_localizeStrings.Get(151);
      linkStatus += " ";
      CNetworkInterface* iface = g_application.getNetwork().GetFirstConnectedInterface();
      if (iface && iface->IsConnected())
        linkStatus += g_localizeStrings.Get(15207);
      else
        linkStatus += g_localizeStrings.Get(15208);
      return linkStatus;
    }
    break;

  case VISUALISATION_PRESET:
    {
      CGUIMessage msg(GUI_MSG_GET_VISUALISATION, 0, 0);
      g_windowManager.SendMessage(msg);
      if (msg.GetPointer())
      {
        CVisualisation* viz = NULL;
        viz = (CVisualisation*)msg.GetPointer();
        if (viz)
        {
          strLabel = viz->GetPresetName();
          URIUtils::RemoveExtension(strLabel);
        }
      }
    }
    break;
  case VISUALISATION_NAME:
    {
      AddonPtr addon;
      strLabel = CSettings::GetInstance().GetString(CSettings::SETTING_MUSICPLAYER_VISUALISATION);
      if (CAddonMgr::GetInstance().GetAddon(strLabel,addon) && addon)
        strLabel = addon->Name();
    }
    break;
  case FANART_COLOR1:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
        return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty("fanart_color1").asString();
    }
    break;
  case FANART_COLOR2:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
        return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty("fanart_color2").asString();
    }
    break;
  case FANART_COLOR3:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
        return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty("fanart_color3").asString();
    }
    break;
  case FANART_IMAGE:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
        return ((CGUIMediaWindow *)window)->CurrentDirectory().GetArt("fanart");
    }
    break;
  case SYSTEM_RENDER_VENDOR:
    strLabel = g_Windowing.GetRenderVendor();
    break;
  case SYSTEM_RENDER_RENDERER:
    strLabel = g_Windowing.GetRenderRenderer();
    break;
  case SYSTEM_RENDER_VERSION:
    strLabel = g_Windowing.GetRenderVersionString();
    break;
  }

  return strLabel;
}

// tries to get a integer value for use in progressbars/sliders and such
bool CGUIInfoManager::GetInt(int &value, int info, int contextWindow, const CGUIListItem *item /* = NULL */) const
{
  if (info >= MULTI_INFO_START && info <= MULTI_INFO_END)
    return GetMultiInfoInt(value, m_multiInfo[info - MULTI_INFO_START], contextWindow);

  if (info >= LISTITEM_START && info <= LISTITEM_END)
  {
    if (item == NULL)
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_HAS_LIST_ITEMS); // true for has list items
      if (window)
        item = window->GetCurrentListItem().get();
    }

    return GetItemInt(value, item, info);
  }

  value = 0;
  switch( info )
  {
    case PLAYER_VOLUME:
      value = (int)g_application.GetVolume();
      return true;
    case PLAYER_SUBTITLE_DELAY:
      value = g_application.GetSubtitleDelay();
      return true;
    case PLAYER_AUDIO_DELAY:
      value = g_application.GetAudioDelay();
      return true;
    case PLAYER_PROGRESS:
    case PLAYER_PROGRESS_CACHE:
    case PLAYER_SEEKBAR:
    case PLAYER_CACHELEVEL:
    case PLAYER_CHAPTER:
    case PLAYER_CHAPTERCOUNT:
      {
        if( g_application.m_pPlayer->IsPlaying())
        {
          switch( info )
          {
          case PLAYER_PROGRESS:
            {
              const CEpgInfoTagPtr tag(GetEpgInfoTag());
              if (tag)
                value = MathUtils::round_int(tag->ProgressPercentage());
              else
                value = MathUtils::round_int(g_application.GetPercentage());
              break;
            }
          case PLAYER_PROGRESS_CACHE:
            value = MathUtils::round_int(g_application.GetCachePercentage());
            break;
          case PLAYER_SEEKBAR:
            value = MathUtils::round_int(GetSeekPercent());
            break;
          case PLAYER_CACHELEVEL:
            value = (int)(g_application.m_pPlayer->GetCacheLevel());
            break;
          case PLAYER_CHAPTER:
            value = g_application.m_pPlayer->GetChapter();
            break;
          case PLAYER_CHAPTERCOUNT:
            value = g_application.m_pPlayer->GetChapterCount();
            break;
          }
        }
      }
      return true;
    case SYSTEM_FREE_MEMORY:
    case SYSTEM_USED_MEMORY:
      {
        MEMORYSTATUSEX stat;
        stat.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&stat);
        int memPercentUsed = (int)( 100.0f* (stat.ullTotalPhys - stat.ullAvailPhys)/stat.ullTotalPhys + 0.5f );
        if (info == SYSTEM_FREE_MEMORY)
          value = 100 - memPercentUsed;
        else
          value = memPercentUsed;
        return true;
      }
    case SYSTEM_PROGRESS_BAR:
      {
        CGUIDialogProgress *bar = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
        if (bar && bar->IsDialogRunning())
          value = bar->GetPercentage();
        return true;
      }
    case SYSTEM_FREE_SPACE:
    case SYSTEM_USED_SPACE:
      {
        g_sysinfo.GetHddSpaceInfo(value, info, true);
        return true;
      }
    case SYSTEM_CPU_USAGE:
      value = g_cpuInfo.getUsedPercentage();
      return true;
    case PVR_PLAYING_PROGRESS:
    case PVR_ACTUAL_STREAM_SIG_PROGR:
    case PVR_ACTUAL_STREAM_SNR_PROGR:
    case PVR_BACKEND_DISKSPACE_PROGR:
    case PVR_TIMESHIFT_PROGRESS:
      value = g_PVRManager.TranslateIntInfo(info);
      return true;
    case SYSTEM_BATTERY_LEVEL:
      value = g_powerManager.BatteryLevel();
      return true;
  }
  return false;
}

// functor for comparison InfoPtr's
struct InfoBoolFinder
{
  InfoBoolFinder(const std::string &expression, int context) : m_bool(expression, context) {};
  bool operator() (const InfoPtr &right) const { return m_bool == *right; };
  InfoBool m_bool;
};

INFO::InfoPtr CGUIInfoManager::Register(const std::string &expression, int context)
{
  std::string condition(CGUIInfoLabel::ReplaceLocalize(expression));
  StringUtils::Trim(condition);

  if (condition.empty())
    return INFO::InfoPtr();

  CSingleLock lock(m_critInfo);
  // do we have the boolean expression already registered?
  std::vector<InfoPtr>::const_iterator i = std::find_if(m_bools.begin(), m_bools.end(), InfoBoolFinder(condition, context));
  if (i != m_bools.end())
    return *i;

  if (condition.find_first_of("|+[]!") != condition.npos)
    m_bools.push_back(std::make_shared<InfoExpression>(condition, context));
  else
    m_bools.push_back(std::make_shared<InfoSingle>(condition, context));

  return m_bools.back();
}

bool CGUIInfoManager::EvaluateBool(const std::string &expression, int contextWindow /* = 0 */, const CGUIListItemPtr &item /* = NULL */)
{
  bool result = false;
  INFO::InfoPtr info = Register(expression, contextWindow);
  if (info)
    result = info->Get(item.get());
  return result;
}

// checks the condition and returns it as necessary.  Currently used
// for toggle button controls and visibility of images.
bool CGUIInfoManager::GetBool(int condition1, int contextWindow, const CGUIListItem *item)
{
  bool bReturn = false;
  int condition = abs(condition1);

  if (condition >= LISTITEM_START && condition < LISTITEM_END)
  {
    if (item)
      bReturn = GetItemBool(item, condition);
    else
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_HAS_LIST_ITEMS); // true for has list items
      if (window)
      {
        CFileItemPtr item = window->GetCurrentListItem();
        bReturn = GetItemBool(item.get(), condition);
      }
    }
  }
  // Ethernet Link state checking
  // Will check if system has a Ethernet Link connection! [Cable in!]
  // This can used for the skinner to switch off Network or Inter required functions
  else if ( condition == SYSTEM_ALWAYS_TRUE)
    bReturn = true;
  else if (condition == SYSTEM_ALWAYS_FALSE)
    bReturn = false;
  else if (condition == SYSTEM_ETHERNET_LINK_ACTIVE)
    bReturn = true;
  else if (condition == WINDOW_IS_MEDIA)
  { // note: This doesn't return true for dialogs (content, favourites, login, videoinfo)
    CGUIWindow *pWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
    bReturn = (pWindow && pWindow->IsMediaWindow());
  }
  else if (condition == PLAYER_MUTED)
    bReturn = (g_application.IsMuted() || g_application.GetVolume(false) <= VOLUME_MINIMUM);
  else if (condition >= LIBRARY_HAS_MUSIC && condition <= LIBRARY_HAS_COMPILATIONS)
    bReturn = GetLibraryBool(condition);
  else if (condition == LIBRARY_IS_SCANNING)
  {
    if (g_application.IsMusicScanning() || g_application.IsVideoScanning())
      bReturn = true;
    else
      bReturn = false;
  }
  else if (condition == LIBRARY_IS_SCANNING_VIDEO)
  {
    bReturn = g_application.IsVideoScanning();
  }
  else if (condition == LIBRARY_IS_SCANNING_MUSIC)
  {
    bReturn = g_application.IsMusicScanning();
  }
  else if (condition == SYSTEM_PLATFORM_LINUX)
#if defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
    bReturn = true;
#else
    bReturn = false;
#endif
  else if (condition == SYSTEM_PLATFORM_WINDOWS)
#ifdef TARGET_WINDOWS
    bReturn = true;
#else
    bReturn = false;
#endif
  else if (condition == SYSTEM_PLATFORM_DARWIN)
#ifdef TARGET_DARWIN
    bReturn = true;
#else
    bReturn = false;
#endif
  else if (condition == SYSTEM_PLATFORM_DARWIN_OSX)
#ifdef TARGET_DARWIN_OSX
    bReturn = true;
#else
    bReturn = false;
#endif
  else if (condition == SYSTEM_PLATFORM_DARWIN_IOS)
#ifdef TARGET_DARWIN_IOS
    bReturn = true;
#else
    bReturn = false;
#endif
  else if (condition == SYSTEM_PLATFORM_ANDROID)
#if defined(TARGET_ANDROID)
    bReturn = true;
#else
    bReturn = false;
#endif
  else if (condition == SYSTEM_PLATFORM_LINUX_RASPBERRY_PI)
#if defined(TARGET_RASPBERRY_PI)
    bReturn = true;
#else
    bReturn = false;
#endif
  else if (condition == SYSTEM_MEDIA_DVD)
    bReturn = g_mediaManager.IsDiscInDrive();
#ifdef HAS_DVD_DRIVE
  else if (condition == SYSTEM_DVDREADY)
    bReturn = g_mediaManager.GetDriveStatus() != DRIVE_NOT_READY;
  else if (condition == SYSTEM_TRAYOPEN)
    bReturn = g_mediaManager.GetDriveStatus() == DRIVE_OPEN;
#endif
  else if (condition == SYSTEM_CAN_POWERDOWN)
    bReturn = g_powerManager.CanPowerdown();
  else if (condition == SYSTEM_CAN_SUSPEND)
    bReturn = g_powerManager.CanSuspend();
  else if (condition == SYSTEM_CAN_HIBERNATE)
    bReturn = g_powerManager.CanHibernate();
  else if (condition == SYSTEM_CAN_REBOOT)
    bReturn = g_powerManager.CanReboot();
  else if (condition == SYSTEM_SCREENSAVER_ACTIVE)
    bReturn = g_application.IsInScreenSaver();
  else if (condition == SYSTEM_DPMS_ACTIVE)
    bReturn = g_application.IsDPMSActive();

  else if (condition == PLAYER_SHOWINFO)
    bReturn = m_playerShowInfo;
  else if (condition == PLAYER_IS_CHANNEL_PREVIEW_ACTIVE)
    bReturn = IsPlayerChannelPreviewActive();
  else if (condition >= MULTI_INFO_START && condition <= MULTI_INFO_END)
  {
    return GetMultiInfoBool(m_multiInfo[condition - MULTI_INFO_START], contextWindow, item);
  }
  else if (condition == SYSTEM_HASLOCKS)
    bReturn = CProfilesManager::GetInstance().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE;
  else if (condition == SYSTEM_HAS_PVR)
    bReturn = true;
  else if (condition == SYSTEM_HAS_ADSP)
    bReturn = true;
  else if (condition == SYSTEM_HAS_CMS)
#ifdef HAS_GL
    bReturn = true;
#else
    bReturn = false;
#endif
  else if (condition == SYSTEM_ISMASTER)
    bReturn = CProfilesManager::GetInstance().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && g_passwordManager.bMasterUser;
  else if (condition == SYSTEM_ISFULLSCREEN)
    bReturn = g_Windowing.IsFullScreen();
  else if (condition == SYSTEM_ISSTANDALONE)
    bReturn = g_application.IsStandAlone();
  else if (condition == SYSTEM_ISINHIBIT)
    bReturn = g_application.IsIdleShutdownInhibited();
  else if (condition == SYSTEM_HAS_SHUTDOWN)
    bReturn = (CSettings::GetInstance().GetInt(CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNTIME) > 0);
  else if (condition == SYSTEM_LOGGEDON)
    bReturn = !(g_windowManager.GetActiveWindow() == WINDOW_LOGIN_SCREEN);
  else if (condition == SYSTEM_SHOW_EXIT_BUTTON)
    bReturn = g_advancedSettings.m_showExitButton;
  else if (condition == SYSTEM_HAS_LOGINSCREEN)
    bReturn = CProfilesManager::GetInstance().UsingLoginScreen();
  else if (condition == SYSTEM_HAS_MODAL_DIALOG)
    bReturn = g_windowManager.HasModalDialog();
  else if (condition == WEATHER_IS_FETCHED)
    bReturn = g_weatherManager.IsFetched();
  else if (condition >= PVR_CONDITIONS_START && condition <= PVR_CONDITIONS_END)
    bReturn = g_PVRManager.TranslateBoolInfo(condition);
  else if (condition >= ADSP_CONDITIONS_START && condition <= ADSP_CONDITIONS_END)
    bReturn = CServiceBroker::GetADSP().TranslateBoolInfo(condition);
  else if (condition == SYSTEM_INTERNET_STATE)
  {
    g_sysinfo.GetInfo(condition);
    bReturn = g_sysinfo.HasInternet();
  }
  else if (condition == SYSTEM_HAS_INPUT_HIDDEN)
  {
    CGUIDialogNumeric *pNumeric = (CGUIDialogNumeric *)g_windowManager.GetWindow(WINDOW_DIALOG_NUMERIC);
    CGUIDialogKeyboardGeneric *pKeyboard = (CGUIDialogKeyboardGeneric*)g_windowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);

    if (pNumeric && pNumeric->IsActive())
      bReturn = pNumeric->IsInputHidden();
    else if (pKeyboard && pKeyboard->IsActive())
      bReturn = pKeyboard->IsInputHidden();
  }
  else if (condition == CONTAINER_HASFILES || condition == CONTAINER_HASFOLDERS)
  {
    CGUIWindow *pWindow = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (pWindow)
    {
      const CFileItemList& items=((CGUIMediaWindow*)pWindow)->CurrentDirectory();
      for (int i=0;i<items.Size();++i)
      {
        CFileItemPtr item=items.Get(i);
        if (!item->m_bIsFolder && condition == CONTAINER_HASFILES)
        {
          bReturn=true;
          break;
        }
        else if (item->m_bIsFolder && !item->IsParentFolder() && condition == CONTAINER_HASFOLDERS)
        {
          bReturn=true;
          break;
        }
      }
    }
  }
  else if (condition == CONTAINER_STACKED)
  {
    CGUIWindow *pWindow = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (pWindow)
      bReturn = ((CGUIMediaWindow*)pWindow)->CurrentDirectory().GetProperty("isstacked").asBoolean();
  }
  else if (condition == CONTAINER_HAS_THUMB)
  {
    CGUIWindow *pWindow = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (pWindow)
      bReturn = ((CGUIMediaWindow*)pWindow)->CurrentDirectory().HasArt("thumb");
  }
  else if (condition == CONTAINER_HAS_NEXT || condition == CONTAINER_HAS_PREVIOUS ||
           condition == CONTAINER_SCROLLING || condition == CONTAINER_ISUPDATING ||
           condition == CONTAINER_HAS_PARENT_ITEM)
  {
    const CGUIControl *control = NULL;
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (window)
      control = window->GetControl(window->GetViewContainerID());

    if (control)
    {
      if (control->IsContainer())
        bReturn = control->GetCondition(condition, 0);
      else if (control->GetControlType() == CGUIControl::GUICONTROL_TEXTBOX)
        bReturn = ((CGUITextBox *)control)->GetCondition(condition, 0);
    }
  }
  else if (condition == CONTAINER_CAN_FILTER)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (window)
      bReturn = !((CGUIMediaWindow*)window)->CanFilterAdvanced();
  }
  else if (condition == CONTAINER_CAN_FILTERADVANCED)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (window)
      bReturn = ((CGUIMediaWindow*)window)->CanFilterAdvanced();
  }
  else if (condition == CONTAINER_FILTERED)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (window)
      bReturn = ((CGUIMediaWindow*)window)->IsFiltered();
  }
  else if (condition == VIDEOPLAYER_HAS_INFO)
    bReturn = ((m_currentFile->HasVideoInfoTag() && !m_currentFile->GetVideoInfoTag()->IsEmpty()) ||
               (m_currentFile->HasPVRChannelInfoTag()  && !m_currentFile->GetPVRChannelInfoTag()->IsEmpty()));
  else if (condition >= CONTAINER_SCROLL_PREVIOUS && condition <= CONTAINER_SCROLL_NEXT)
  {
    // no parameters, so we assume it's just requested for a media window.  It therefore
    // can only happen if the list has focus.
    CGUIWindow *pWindow = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (pWindow)
    {
      std::map<int,int>::const_iterator it = m_containerMoves.find(pWindow->GetViewContainerID());
      if (it != m_containerMoves.end())
      {
        if (condition > CONTAINER_STATIC) // moving up
          bReturn = it->second >= std::max(condition - CONTAINER_STATIC, 1);
        else
          bReturn = it->second <= std::min(condition - CONTAINER_STATIC, -1);
      }
    }
  }
  else if (condition == SLIDESHOW_ISPAUSED)
  {
    CGUIWindowSlideShow *slideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    bReturn = (slideShow && slideShow->IsPaused());
  }
  else if (condition == SLIDESHOW_ISRANDOM)
  {
    CGUIWindowSlideShow *slideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    bReturn = (slideShow && slideShow->IsShuffled());
  }
  else if (condition == SLIDESHOW_ISACTIVE)
  {
    CGUIWindowSlideShow *slideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    bReturn = (slideShow && slideShow->InSlideShow());
  }
  else if (condition == SLIDESHOW_ISVIDEO)
  {
    CGUIWindowSlideShow *slideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    bReturn = (slideShow && slideShow->GetCurrentSlide() && slideShow->GetCurrentSlide()->IsVideo());
  }
  else if (g_application.m_pPlayer->IsPlaying())
  {
    switch (condition)
    {
    case PLAYER_HAS_MEDIA:
      bReturn = true;
      break;
    case PLAYER_HAS_AUDIO:
      bReturn = g_application.m_pPlayer->IsPlayingAudio();
      break;
    case PLAYER_HAS_VIDEO:
      bReturn = g_application.m_pPlayer->IsPlayingVideo();
      break;
    case PLAYER_PLAYING:
      {
        float speed = g_application.m_pPlayer->GetPlaySpeed();
        bReturn = (speed >= 0.75 && speed <= 1.55);
      }
      break;
    case PLAYER_PAUSED:
      bReturn = g_application.m_pPlayer->IsPausedPlayback();
      break;
    case PLAYER_REWINDING:
      bReturn = g_application.m_pPlayer->GetPlaySpeed() < 0;
      break;
    case PLAYER_FORWARDING:
      bReturn = g_application.m_pPlayer->GetPlaySpeed() > 1.5;
      break;
    case PLAYER_REWINDING_2x:
      bReturn = g_application.m_pPlayer->GetPlaySpeed() == -2;
      break;
    case PLAYER_REWINDING_4x:
      bReturn = g_application.m_pPlayer->GetPlaySpeed() == -4;
      break;
    case PLAYER_REWINDING_8x:
      bReturn = g_application.m_pPlayer->GetPlaySpeed() == -8;
      break;
    case PLAYER_REWINDING_16x:
      bReturn = g_application.m_pPlayer->GetPlaySpeed() == -16;
      break;
    case PLAYER_REWINDING_32x:
      bReturn = g_application.m_pPlayer->GetPlaySpeed() == -32;
      break;
    case PLAYER_FORWARDING_2x:
      bReturn = g_application.m_pPlayer->GetPlaySpeed() == 2;
      break;
    case PLAYER_FORWARDING_4x:
      bReturn = g_application.m_pPlayer->GetPlaySpeed() == 4;
      break;
    case PLAYER_FORWARDING_8x:
      bReturn = g_application.m_pPlayer->GetPlaySpeed() == 8;
      break;
    case PLAYER_FORWARDING_16x:
      bReturn = g_application.m_pPlayer->GetPlaySpeed() == 16;
      break;
    case PLAYER_FORWARDING_32x:
      bReturn = g_application.m_pPlayer->GetPlaySpeed() == 32;
      break;
    case PLAYER_CAN_RECORD:
      bReturn = g_application.m_pPlayer->CanRecord();
      break;
    case PLAYER_CAN_PAUSE:
      bReturn = g_application.m_pPlayer->CanPause();
      break;
    case PLAYER_CAN_SEEK:
      bReturn = g_application.m_pPlayer->CanSeek();
      break;
    case PLAYER_SUPPORTS_TEMPO:
      bReturn = g_application.m_pPlayer->SupportsTempo();
      break;
    case PLAYER_IS_TEMPO:
      {
        float speed = g_application.m_pPlayer->GetPlaySpeed();
        bReturn = (speed >= 0.75 && speed <= 1.55 && speed != 1);
      }
      break;
    case PLAYER_RECORDING:
      bReturn = g_application.m_pPlayer->IsRecording();
    break;
    case PLAYER_DISPLAY_AFTER_SEEK:
      bReturn = GetDisplayAfterSeek();
    break;
    case PLAYER_CACHING:
      bReturn = g_application.m_pPlayer->IsCaching();
    break;
    case PLAYER_SEEKBAR:
      {
        CGUIDialog *seekBar = (CGUIDialog*)g_windowManager.GetWindow(WINDOW_DIALOG_SEEK_BAR);
        bReturn = seekBar ? seekBar->IsDialogRunning() : false;
      }
    break;
    case PLAYER_SEEKING:
      bReturn = CSeekHandler::GetInstance().InProgress();
    break;
    case PLAYER_SHOWTIME:
      bReturn = m_playerShowTime;
    break;
    case PLAYER_PASSTHROUGH:
      bReturn = g_application.m_pPlayer->IsPassthrough();
      break;
    case PLAYER_ISINTERNETSTREAM:
      bReturn = m_currentFile && URIUtils::IsInternetStream(m_currentFile->GetPath());
      break;
    case MUSICPM_ENABLED:
      bReturn = g_partyModeManager.IsEnabled();
    break;
    case MUSICPLAYER_HASPREVIOUS:
      {
        // requires current playlist be PLAYLIST_MUSIC
        bReturn = false;
        if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
          bReturn = (g_playlistPlayer.GetCurrentSong() > 0); // not first song
      }
      break;
    case MUSICPLAYER_HASNEXT:
      {
        // requires current playlist be PLAYLIST_MUSIC
        bReturn = false;
        if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
          bReturn = (g_playlistPlayer.GetCurrentSong() < (g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size() - 1)); // not last song
      }
      break;
    case MUSICPLAYER_PLAYLISTPLAYING:
      {
        bReturn = false;
        if (g_application.m_pPlayer->IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
          bReturn = true;
      }
      break;
    case VIDEOPLAYER_USING_OVERLAYS:
      bReturn = (CSettings::GetInstance().GetInt(CSettings::SETTING_VIDEOPLAYER_RENDERMETHOD) == RENDER_OVERLAYS);
    break;
    case VIDEOPLAYER_ISFULLSCREEN:
      bReturn = g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO;
    break;
    case VIDEOPLAYER_HASMENU:
      bReturn = g_application.m_pPlayer->HasMenu();
    break;
    case PLAYLIST_ISRANDOM:
      bReturn = g_playlistPlayer.IsShuffled(g_playlistPlayer.GetCurrentPlaylist());
    break;
    case PLAYLIST_ISREPEAT:
      bReturn = g_playlistPlayer.GetRepeat(g_playlistPlayer.GetCurrentPlaylist()) == PLAYLIST::REPEAT_ALL;
    break;
    case PLAYLIST_ISREPEATONE:
      bReturn = g_playlistPlayer.GetRepeat(g_playlistPlayer.GetCurrentPlaylist()) == PLAYLIST::REPEAT_ONE;
    break;
    case PLAYER_HASDURATION:
      bReturn = g_application.GetTotalTime() > 0;
      break;
    case VIDEOPLAYER_HASTELETEXT:
      if (g_application.m_pPlayer->GetTeletextCache())
        bReturn = true;
      break;
    case VIDEOPLAYER_HASSUBTITLES:
      bReturn = g_application.m_pPlayer->GetSubtitleCount() > 0;
      break;
    case VIDEOPLAYER_SUBTITLESENABLED:
      bReturn = g_application.m_pPlayer->GetSubtitleVisible();
      break;
    case VISUALISATION_LOCKED:
      {
        CGUIMessage msg(GUI_MSG_GET_VISUALISATION, 0, 0);
        g_windowManager.SendMessage(msg);
        if (msg.GetPointer())
        {
          CVisualisation *pVis = (CVisualisation *)msg.GetPointer();
          bReturn = pVis->IsLocked();
        }
      }
    break;
    case VISUALISATION_ENABLED:
      bReturn = !CSettings::GetInstance().GetString(CSettings::SETTING_MUSICPLAYER_VISUALISATION).empty();
    break;
    case VIDEOPLAYER_HAS_EPG:
      if (m_currentFile->HasPVRChannelInfoTag())
        bReturn = (m_currentFile->GetPVRChannelInfoTag()->GetEPGNow().get() != NULL);
    break;
    case VIDEOPLAYER_IS_STEREOSCOPIC:
      if(g_application.m_pPlayer->IsPlaying())
      {
        bReturn = !m_videoInfo.stereoMode.empty();
      }
      break;
    case VIDEOPLAYER_CAN_RESUME_LIVE_TV:
      if (m_currentFile->HasPVRRecordingInfoTag())
      {
        EPG::CEpgInfoTagPtr epgTag = EPG::CEpgContainer::GetInstance().GetTagById(m_currentFile->GetPVRRecordingInfoTag()->Channel(), m_currentFile->GetPVRRecordingInfoTag()->BroadcastUid());
        bReturn = (epgTag && epgTag->IsActive() && epgTag->ChannelTag());
      }
      break;
    case VISUALISATION_HAS_PRESETS:
    {
      CGUIMessage msg(GUI_MSG_GET_VISUALISATION, 0, 0);
      g_windowManager.SendMessage(msg);
      if (msg.GetPointer())
      {
        CVisualisation* viz = NULL;
        viz = (CVisualisation*)msg.GetPointer();
        bReturn = (viz && viz->HasPresets());
      }
    }
    break;
    case RDS_HAS_RDS:
      bReturn = g_application.m_pPlayer->IsPlayingRDS();
    break;
    case RDS_HAS_RADIOTEXT:
      if (m_currentFile->HasPVRRadioRDSInfoTag())
        bReturn = m_currentFile->GetPVRRadioRDSInfoTag()->IsPlayingRadiotext();
    break;
    case RDS_HAS_RADIOTEXT_PLUS:
      if (m_currentFile->HasPVRRadioRDSInfoTag())
        bReturn = m_currentFile->GetPVRRadioRDSInfoTag()->IsPlayingRadiotextPlus();
    break;
    case RDS_HAS_HOTLINE_DATA:
      if (m_currentFile->HasPVRRadioRDSInfoTag())
        bReturn = (!m_currentFile->GetPVRRadioRDSInfoTag()->GetEMailHotline().empty() ||
                   !m_currentFile->GetPVRRadioRDSInfoTag()->GetPhoneHotline().empty());
    break;
    case RDS_HAS_STUDIO_DATA:
      if (m_currentFile->HasPVRRadioRDSInfoTag())
        bReturn = (!m_currentFile->GetPVRRadioRDSInfoTag()->GetEMailStudio().empty() ||
                   !m_currentFile->GetPVRRadioRDSInfoTag()->GetSMSStudio().empty() ||
                   !m_currentFile->GetPVRRadioRDSInfoTag()->GetPhoneStudio().empty());
    break;
    case PLAYER_PROCESS_VIDEOHWDECODER:
        bReturn = CServiceBroker::GetDataCacheCore().IsVideoHwDecoder();
        break;
    default: // default, use integer value different from 0 as true
      {
        int val;
        bReturn = GetInt(val, condition) && val != 0;
      }
    }
  }
  if (condition1 < 0)
    bReturn = !bReturn;
  return bReturn;
}

/// \brief Examines the multi information sent and returns true or false accordingly.
bool CGUIInfoManager::GetMultiInfoBool(const GUIInfo &info, int contextWindow, const CGUIListItem *item)
{
  bool bReturn = false;
  int condition = abs(info.m_info);

  if (condition >= LISTITEM_START && condition <= LISTITEM_END)
  {
    if (!item)
    {
      CGUIWindow *window = NULL;
      int data1 = info.GetData1();
      if (!data1) // No container specified, so we lookup the current view container
      {
        window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_HAS_LIST_ITEMS);
        if (window && window->IsMediaWindow())
          data1 = ((CGUIMediaWindow*)(window))->GetViewContainerID();
      }

      if (!window) // If we don't have a window already (from lookup above), get one
        window = GetWindowWithCondition(contextWindow, 0);

      if (window)
      {
        const CGUIControl *control = window->GetControl(data1);
        if (control && control->IsContainer())
          item = ((IGUIContainer *)control)->GetListItem(info.GetData2(), info.GetInfoFlag()).get();
      }
    }
    if (item) // If we got a valid item, do the lookup
      bReturn = GetItemBool(item, condition); // Image prioritizes images over labels (in the case of music item ratings for instance)
  }
  else
  {
    switch (condition)
    {
      case SKIN_BOOL:
        {
          bReturn = CSkinSettings::GetInstance().GetBool(info.GetData1());
        }
        break;
      case SKIN_STRING:
        {
          if (info.GetData2())
            bReturn = StringUtils::EqualsNoCase(CSkinSettings::GetInstance().GetString(info.GetData1()), m_stringParameters[info.GetData2()]);
          else
            bReturn = !CSkinSettings::GetInstance().GetString(info.GetData1()).empty();
        }
        break;
      case SKIN_HAS_THEME:
        {
          std::string theme = CSettings::GetInstance().GetString(CSettings::SETTING_LOOKANDFEEL_SKINTHEME);
          URIUtils::RemoveExtension(theme);
          bReturn = StringUtils::EqualsNoCase(theme, m_stringParameters[info.GetData1()]);
        }
        break;
      case STRING_IS_EMPTY:
        // note: Get*Image() falls back to Get*Label(), so this should cover all of them
        if (item && item->IsFileItem() && info.GetData1() >= LISTITEM_START && info.GetData1() < LISTITEM_END)
          bReturn = GetItemImage((const CFileItem *)item, info.GetData1()).empty();
        else
          bReturn = GetImage(info.GetData1(), contextWindow).empty();
        break;
      case STRING_COMPARE: // STRING_COMPARE is deprecated - should be removed before L*** v18
      case STRING_IS_EQUAL:
        {
          std::string compare;
          if (info.GetData2() < 0) // info labels are stored with negative numbers
          {
            int info2 = -info.GetData2();
            if (item && item->IsFileItem() && info2 >= LISTITEM_START && info2 < LISTITEM_END)
              compare = GetItemImage((const CFileItem *)item, info2);
            else
              compare = GetImage(info2, contextWindow);
          }
          else if (info.GetData2() < (int)m_stringParameters.size())
          { // conditional string
            compare = m_stringParameters[info.GetData2()];
          }
          if (item && item->IsFileItem() && info.GetData1() >= LISTITEM_START && info.GetData1() < LISTITEM_END)
            bReturn = StringUtils::EqualsNoCase(GetItemImage((const CFileItem *)item, info.GetData1()), compare);
          else
            bReturn = StringUtils::EqualsNoCase(GetImage(info.GetData1(), contextWindow), compare);
        }
        break;
      case INTEGER_IS_EQUAL:
      case INTEGER_GREATER_THAN:
      case INTEGER_GREATER_OR_EQUAL:
      case INTEGER_LESS_THAN:
      case INTEGER_LESS_OR_EQUAL:
        {
          int integer;
          if (!GetInt(integer, info.GetData1(), contextWindow, item))
          {
            std::string value;
            if (item && item->IsFileItem() && info.GetData1() >= LISTITEM_START && info.GetData1() < LISTITEM_END)
              value = GetItemImage((const CFileItem *)item, info.GetData1());
            else
              value = GetImage(info.GetData1(), contextWindow);

            // Handle the case when a value contains time separator (:). This makes IntegerGreaterThan
            // useful for Player.Time* members without adding a separate set of members returning time in seconds
            if (value.find_first_of( ':' ) != value.npos)
              integer = StringUtils::TimeStringToSeconds(value);
            else
              integer = atoi(value.c_str());
          }

          // compare
          if (condition == INTEGER_IS_EQUAL)
            bReturn = integer == info.GetData2();
          else if (condition == INTEGER_GREATER_THAN)
            bReturn = integer > info.GetData2();
          else if (condition == INTEGER_GREATER_OR_EQUAL)
            bReturn = integer >= info.GetData2();
          else if (condition == INTEGER_LESS_THAN)
            bReturn = integer < info.GetData2();
          else if (condition == INTEGER_LESS_OR_EQUAL)
            bReturn = integer <= info.GetData2();
        }
        break;
      case STRING_STR:          // STRING_STR is deprecated - should be removed before L*** v18
      case STRING_STR_LEFT:     // STRING_STR_LEFT is deprecated - should be removed before L*** v18
      case STRING_STR_RIGHT:    // STRING_STR_RIGHT is deprecated - should be removed before L*** v18
      case STRING_STARTS_WITH:
      case STRING_ENDS_WITH:
      case STRING_CONTAINS:
        {
          std::string compare = m_stringParameters[info.GetData2()];
          // our compare string is already in lowercase, so lower case our label as well
          // as std::string::Find() is case sensitive
          std::string label;
          if (item && item->IsFileItem() && info.GetData1() >= LISTITEM_START && info.GetData1() < LISTITEM_END)
          {
            label = GetItemImage((const CFileItem *)item, info.GetData1());
            StringUtils::ToLower(label);
          }
          else
          {
            label = GetImage(info.GetData1(), contextWindow);
            StringUtils::ToLower(label);
          }
          if (condition == STRING_STR_LEFT || condition == STRING_STARTS_WITH)
            bReturn = StringUtils::StartsWith(label, compare);
          else if (condition == STRING_STR_RIGHT || condition == STRING_ENDS_WITH)
            bReturn = StringUtils::EndsWith(label, compare);
          else
            bReturn = label.find(compare) != std::string::npos;
        }
        break;
      case SYSTEM_ALARM_LESS_OR_EQUAL:
        {
          int time = lrint(g_alarmClock.GetRemaining(m_stringParameters[info.GetData1()]));
          int timeCompare = atoi(m_stringParameters[info.GetData2()].c_str());
          if (time > 0)
            bReturn = timeCompare >= time;
          else
            bReturn = false;
        }
        break;
      case SYSTEM_IDLE_TIME:
        bReturn = g_application.GlobalIdleTime() >= (int)info.GetData1();
        break;
      case CONTROL_GROUP_HAS_FOCUS:
        {
          CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
          if (window)
            bReturn = window->ControlGroupHasFocus(info.GetData1(), info.GetData2());
        }
        break;
      case CONTROL_IS_VISIBLE:
        {
          CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
          if (window)
          {
            // Note: This'll only work for unique id's
            const CGUIControl *control = window->GetControl(info.GetData1());
            if (control)
              bReturn = control->IsVisible();
          }
        }
        break;
      case CONTROL_IS_ENABLED:
        {
          CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
          if (window)
          {
            // Note: This'll only work for unique id's
            const CGUIControl *control = window->GetControl(info.GetData1());
            if (control)
              bReturn = !control->IsDisabled();
          }
        }
        break;
      case CONTROL_HAS_FOCUS:
        {
          CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
          if (window)
            bReturn = (window->GetFocusedControlID() == (int)info.GetData1());
        }
        break;
      case WINDOW_NEXT:
        if (info.GetData1())
          bReturn = ((int)info.GetData1() == m_nextWindowID);
        else
        {
          CGUIWindow *window = g_windowManager.GetWindow(m_nextWindowID);
          if (window && StringUtils::EqualsNoCase(URIUtils::GetFileName(window->GetProperty("xmlfile").asString()), m_stringParameters[info.GetData2()]))
            bReturn = true;
        }
        break;
      case WINDOW_PREVIOUS:
        if (info.GetData1())
          bReturn = ((int)info.GetData1() == m_prevWindowID);
        else
        {
          CGUIWindow *window = g_windowManager.GetWindow(m_prevWindowID);
          if (window && StringUtils::EqualsNoCase(URIUtils::GetFileName(window->GetProperty("xmlfile").asString()), m_stringParameters[info.GetData2()]))
            bReturn = true;
        }
        break;
      case WINDOW_IS:
        if (info.GetData1())
        {
          CGUIWindow *window = g_windowManager.GetWindow(contextWindow);
          bReturn = (window && window->GetID() == static_cast<int>(info.GetData1()));
        }
        else
          CLog::Log(LOGERROR, "The window id is required.");
        break;
      case WINDOW_IS_VISIBLE:
        if (info.GetData1())
          bReturn = g_windowManager.IsWindowVisible(info.GetData1());
        else
          bReturn = g_windowManager.IsWindowVisible(m_stringParameters[info.GetData2()]);
        break;
      case WINDOW_IS_TOPMOST:
        if (info.GetData1())
          bReturn = g_windowManager.IsWindowTopMost(info.GetData1());
        else
          bReturn = g_windowManager.IsWindowTopMost(m_stringParameters[info.GetData2()]);
        break;
      case WINDOW_IS_ACTIVE:
        if (info.GetData1())
          bReturn = g_windowManager.IsWindowActive(info.GetData1());
        else
          bReturn = g_windowManager.IsWindowActive(m_stringParameters[info.GetData2()]);
        break;
      case SYSTEM_HAS_ALARM:
        bReturn = g_alarmClock.HasAlarm(m_stringParameters[info.GetData1()]);
        break;
      case SYSTEM_GET_BOOL:
        bReturn = CSettings::GetInstance().GetBool(m_stringParameters[info.GetData1()]);
        break;
      case SYSTEM_HAS_CORE_ID:
        bReturn = g_cpuInfo.HasCoreId(info.GetData1());
        break;
      case SYSTEM_SETTING:
        {
          if ( StringUtils::EqualsNoCase(m_stringParameters[info.GetData1()], "hidewatched") )
          {
            CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
            if (window)
              bReturn = CMediaSettings::GetInstance().GetWatchedMode(((CGUIMediaWindow *)window)->CurrentDirectory().GetContent()) == WatchedModeUnwatched;
          }
        }
        break;
      case SYSTEM_HAS_ADDON:
      {
        AddonPtr addon;
        bReturn = CAddonMgr::GetInstance().GetAddon(m_stringParameters[info.GetData1()],addon) && addon;
        break;
      }
      case CONTAINER_SCROLL_PREVIOUS:
      case CONTAINER_MOVE_PREVIOUS:
      case CONTAINER_MOVE_NEXT:
      case CONTAINER_SCROLL_NEXT:
        {
          std::map<int,int>::const_iterator it = m_containerMoves.find(info.GetData1());
          if (it != m_containerMoves.end())
          {
            if (condition > CONTAINER_STATIC) // moving up
              bReturn = it->second >= std::max(condition - CONTAINER_STATIC, 1);
            else
              bReturn = it->second <= std::min(condition - CONTAINER_STATIC, -1);
          }
        }
        break;
      case CONTAINER_CONTENT:
        {
          std::string content;
          CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
          if (window)
          {
            if (window->GetID() == WINDOW_DIALOG_MUSIC_INFO)
              content = ((CGUIDialogMusicInfo *)window)->CurrentDirectory().GetContent();
            else if (window->GetID() == WINDOW_DIALOG_VIDEO_INFO)
              content = ((CGUIDialogVideoInfo *)window)->CurrentDirectory().GetContent();
          }
          if (content.empty())
          {
            window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
            if (window)
              content = ((CGUIMediaWindow *)window)->CurrentDirectory().GetContent();
          }
          bReturn = StringUtils::EqualsNoCase(m_stringParameters[info.GetData2()], content);
        }
        break;
      case CONTAINER_ROW:
      case CONTAINER_COLUMN:
      case CONTAINER_POSITION:
      case CONTAINER_HAS_NEXT:
      case CONTAINER_HAS_PREVIOUS:
      case CONTAINER_SCROLLING:
      case CONTAINER_SUBITEM:
      case CONTAINER_ISUPDATING:
      case CONTAINER_HAS_PARENT_ITEM:
        {
          const CGUIControl *control = NULL;
          if (info.GetData1())
          { // container specified
            CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
            if (window)
              control = window->GetControl(info.GetData1());
          }
          else
          { // no container specified - assume a mediawindow
            CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
            if (window)
              control = window->GetControl(window->GetViewContainerID());
          }
          if (control)
            bReturn = control->GetCondition(condition, info.GetData2());
        }
        break;
      case CONTAINER_HAS_FOCUS:
        { // grab our container
          CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
          if (window)
          {
            const CGUIControl *control = window->GetControl(info.GetData1());
            if (control && control->IsContainer())
            {
              CFileItemPtr item = std::static_pointer_cast<CFileItem>(((IGUIContainer *)control)->GetListItem(0));
              if (item && item->m_iprogramCount == info.GetData2())  // programcount used to store item id
                bReturn = true;
            }
          }
          break;
        }
      case MUSICPLAYER_CONTENT:
        {
          std::string strContent = "files";
          if (m_currentFile->HasPVRChannelInfoTag())
            strContent = "livetv";
          bReturn = StringUtils::EqualsNoCase(m_stringParameters[info.GetData1()], strContent);
          break;
        }
      case VIDEOPLAYER_CONTENT:
        {
          std::string strContent="files";
          if (m_currentFile->HasVideoInfoTag() && m_currentFile->GetVideoInfoTag()->m_type == MediaTypeMovie)
            strContent = "movies";
          if (m_currentFile->HasVideoInfoTag() && m_currentFile->GetVideoInfoTag()->m_type == MediaTypeEpisode)
            strContent = "episodes";
          if (m_currentFile->HasVideoInfoTag() && m_currentFile->GetVideoInfoTag()->m_type == MediaTypeMusicVideo)
            strContent = "musicvideos";
          if (m_currentFile->HasPVRChannelInfoTag())
            strContent = "livetv";
          bReturn = StringUtils::EqualsNoCase(m_stringParameters[info.GetData1()], strContent);
        }
        break;
      case CONTAINER_SORT_METHOD:
      {
        CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
        if (window)
        {
          const CGUIViewState *viewState = ((CGUIMediaWindow*)window)->GetViewState();
          if (viewState)
            bReturn = ((unsigned int)viewState->GetSortMethod().sortBy == info.GetData1());
        }
        break;
      }
      case CONTAINER_SORT_DIRECTION:
      {
        CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
        if (window)
        {
          const CGUIViewState *viewState = ((CGUIMediaWindow*)window)->GetViewState();
          if (viewState)
            bReturn = ((unsigned int)viewState->GetSortOrder() == info.GetData1());
        }
        break;
      }
      case SYSTEM_DATE:
        {
          if (info.GetData2() == -1) // info doesn't contain valid startDate
            return false;
          CDateTime date = CDateTime::GetCurrentDateTime();
          int currentDate = date.GetMonth()*100+date.GetDay();
          int startDate = info.GetData1();
          int stopDate = info.GetData2();

          if (stopDate < startDate)
            bReturn = currentDate >= startDate || currentDate < stopDate;
          else
            bReturn = currentDate >= startDate && currentDate < stopDate;
        }
        break;
      case SYSTEM_TIME:
        {
          CDateTime time=CDateTime::GetCurrentDateTime();
          int currentTime = time.GetMinuteOfDay();
          int startTime = info.GetData1();
          int stopTime = info.GetData2();

          if (stopTime < startTime)
            bReturn = currentTime >= startTime || currentTime < stopTime;
          else
            bReturn = currentTime >= startTime && currentTime < stopTime;
        }
        break;
      case MUSICPLAYER_EXISTS:
        {
          int index = info.GetData2();
          if (info.GetData1() == 1)
          { // relative index
            if (g_playlistPlayer.GetCurrentPlaylist() != PLAYLIST_MUSIC)
            {
              bReturn = false;
              break;
            }
            index += g_playlistPlayer.GetCurrentSong();
          }
          bReturn = (index >= 0 && index < g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size());
        }
        break;

      case PLAYLIST_ISRANDOM:
        {
          int playlistid = info.GetData1();
          if (playlistid > PLAYLIST_NONE)
            bReturn = g_playlistPlayer.IsShuffled(playlistid);
        }
        break;

      case PLAYLIST_ISREPEAT:
        {
          int playlistid = info.GetData1();
          if (playlistid > PLAYLIST_NONE)
            bReturn = g_playlistPlayer.GetRepeat(playlistid) == PLAYLIST::REPEAT_ALL;
        }
        break;

      case PLAYLIST_ISREPEATONE:
        {
          int playlistid = info.GetData1();
          if (playlistid > PLAYLIST_NONE)
            bReturn = g_playlistPlayer.GetRepeat(playlistid) == PLAYLIST::REPEAT_ONE;
        }
        break;
      case LIBRARY_HAS_ROLE:
      {
        std::string strRole = m_stringParameters[info.GetData1()];
        // Find value for role if already stored
        int artistcount = -1;
        for (const auto &role : m_libraryRoleCounts)
        {
          if (StringUtils::EqualsNoCase(strRole, role.first))
          {
            artistcount = role.second;
            break;
          }
        }
        // Otherwise get from DB and store
        if (artistcount < 0)
        {
          CMusicDatabase db;
          if (db.Open())
          {
            artistcount = db.GetArtistCountForRole(strRole);
            db.Close();
            m_libraryRoleCounts.push_back(std::make_pair(strRole, artistcount));
          }
        }
        bReturn = artistcount > 0;
      }
    }
  }
  return (info.m_info < 0) ? !bReturn : bReturn;
}

bool CGUIInfoManager::GetMultiInfoInt(int &value, const GUIInfo &info, int contextWindow) const
{
  if (info.m_info >= LISTITEM_START && info.m_info <= LISTITEM_END)
  {
    CFileItemPtr item;
    CGUIWindow *window = NULL;

    int data1 = info.GetData1();
    if (!data1) // No container specified, so we lookup the current view container
    {
      window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_HAS_LIST_ITEMS);
      if (window && window->IsMediaWindow())
        data1 = ((CGUIMediaWindow*)(window))->GetViewContainerID();
    }

    if (!window) // If we don't have a window already (from lookup above), get one
      window = GetWindowWithCondition(contextWindow, 0);

    if (window)
    {
      const CGUIControl *control = window->GetControl(data1);
      if (control && control->IsContainer())
        item = std::static_pointer_cast<CFileItem>(((IGUIContainer *)control)->GetListItem(info.GetData2(), info.GetInfoFlag()));
    }

    if (item) // If we got a valid item, do the lookup
      return GetItemInt(value, item.get(), info.m_info);
  }

  return 0;
}

/// \brief Examines the multi information sent and returns the string as appropriate
std::string CGUIInfoManager::GetMultiInfoLabel(const GUIInfo &info, int contextWindow, std::string *fallback)
{
  if (info.m_info == SKIN_STRING)
  {
    return CSkinSettings::GetInstance().GetString(info.GetData1());
  }
  else if (info.m_info == SKIN_BOOL)
  {
    bool bInfo = CSkinSettings::GetInstance().GetBool(info.GetData1());
    if (bInfo)
      return g_localizeStrings.Get(20122);
  }
  if (info.m_info >= LISTITEM_START && info.m_info <= LISTITEM_END)
  {
    CFileItemPtr item;
    CGUIWindow *window = NULL;

    int data1 = info.GetData1();
    if (!data1) // No container specified, so we lookup the current view container
    {
      window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_HAS_LIST_ITEMS);
      if (window && window->IsMediaWindow())
        data1 = ((CGUIMediaWindow*)(window))->GetViewContainerID();
    }

    if (!window) // If we don't have a window already (from lookup above), get one
      window = GetWindowWithCondition(contextWindow, 0);

    if (window)
    {
      const CGUIControl *control = window->GetControl(data1);
      if (control && control->IsContainer())
        item = std::static_pointer_cast<CFileItem>(((IGUIContainer *)control)->GetListItem(info.GetData2(), info.GetInfoFlag()));
    }

    if (item) // If we got a valid item, do the lookup
      return GetItemImage(item.get(), info.m_info, fallback); // Image prioritizes images over labels (in the case of music item ratings for instance)
  }
  else if (info.m_info == PLAYER_TIME)
  {
    return GetCurrentPlayTime((TIME_FORMAT)info.GetData1());
  }
  else if (info.m_info == PLAYER_TIME_REMAINING)
  {
    return GetCurrentPlayTimeRemaining((TIME_FORMAT)info.GetData1());
  }
  else if (info.m_info == PLAYER_FINISH_TIME)
  {
    CDateTime time;
    CEpgInfoTagPtr currentTag(GetEpgInfoTag());
    if (currentTag)
      time = currentTag->EndAsLocalTime();
    else
    {
      time = CDateTime::GetCurrentDateTime();
      time += CDateTimeSpan(0, 0, 0, GetPlayTimeRemaining());
    }
    return LocalizeTime(time, (TIME_FORMAT)info.GetData1());
  }
  else if (info.m_info == PLAYER_START_TIME)
  {
    CDateTime time;
    CEpgInfoTagPtr currentTag(GetEpgInfoTag());
    if (currentTag)
      time = currentTag->StartAsLocalTime();
    else
    {
      time = CDateTime::GetCurrentDateTime();
      time -= CDateTimeSpan(0, 0, 0, (int)GetPlayTime());
    }
    return LocalizeTime(time, (TIME_FORMAT)info.GetData1());
  }
  else if (info.m_info == PLAYER_TIME_SPEED)
  {
    std::string strTime;
    float speed = g_application.m_pPlayer->GetPlaySpeed();
    if (speed < 0.8 || speed > 1.5)
      strTime = StringUtils::Format("%s (%ix)", GetCurrentPlayTime((TIME_FORMAT)info.GetData1()).c_str(), (int)speed);
    else
      strTime = GetCurrentPlayTime();
    return strTime;
  }
  else if (info.m_info == PLAYER_DURATION)
  {
    return GetDuration((TIME_FORMAT)info.GetData1());
  }
  else if (info.m_info == PLAYER_SEEKTIME)
  {
    return GetCurrentSeekTime((TIME_FORMAT)info.GetData1());
  }
  else if (info.m_info == PLAYER_SEEKOFFSET)
  {
    std::string seekOffset = StringUtils::SecondsToTimeString(abs(m_seekOffset / 1000), (TIME_FORMAT)info.GetData1());
    if (m_seekOffset < 0)
      return "-" + seekOffset;
    if (m_seekOffset > 0)
      return "+" + seekOffset;
  }
  else if (info.m_info == PLAYER_SEEKSTEPSIZE)
  {
    int seekSize = CSeekHandler::GetInstance().GetSeekSize();
    std::string strSeekSize = StringUtils::SecondsToTimeString(abs(seekSize), (TIME_FORMAT)info.GetData1());
    if (seekSize < 0)
      return "-" + strSeekSize;
    if (seekSize > 0)
      return "+" + strSeekSize;
  }
  else if (info.m_info == PLAYER_ITEM_ART)
  {
    return m_currentFile->GetArt(m_stringParameters[info.GetData1()]);
  }
  else if (info.m_info == SYSTEM_TIME)
  {
    return GetTime((TIME_FORMAT)info.GetData1());
  }
  else if (info.m_info == SYSTEM_DATE)
  {
    CDateTime time=CDateTime::GetCurrentDateTime();
    return time.GetAsLocalizedDate(m_stringParameters[info.GetData1()]);
  }
  else if (info.m_info == CONTAINER_NUM_PAGES || info.m_info == CONTAINER_CURRENT_PAGE ||
           info.m_info == CONTAINER_NUM_ITEMS || info.m_info == CONTAINER_POSITION ||
           info.m_info == CONTAINER_ROW || info.m_info == CONTAINER_COLUMN ||
           info.m_info == CONTAINER_CURRENT_ITEM)
  {
    const CGUIControl *control = NULL;
    if (info.GetData1())
    { // container specified
      CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
      if (window)
        control = window->GetControl(info.GetData1());
    }
    else
    { // no container specified - assume a mediawindow
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
        control = window->GetControl(window->GetViewContainerID());
    }
    if (control)
    {
      if (control->IsContainer())
        return ((IGUIContainer *)control)->GetLabel(info.m_info);
      else if (control->GetControlType() == CGUIControl::GUICONTROL_GROUPLIST)
        return ((CGUIControlGroupList *)control)->GetLabel(info.m_info);
      else if (control->GetControlType() == CGUIControl::GUICONTROL_TEXTBOX)
        return ((CGUITextBox *)control)->GetLabel(info.m_info);
    }
  }
  else if (info.m_info == SYSTEM_GET_CORE_USAGE)
  {
    std::string strCpu = StringUtils::Format("%4.2f", g_cpuInfo.GetCoreInfo(atoi(m_stringParameters[info.GetData1()].c_str())).m_fPct);
    return strCpu;
  }
  else if (info.m_info >= MUSICPLAYER_TITLE && info.m_info <= MUSICPLAYER_ALBUM_ARTIST)
    return GetMusicPlaylistInfo(info);
  else if (info.m_info == CONTAINER_PROPERTY)
  {
    CGUIWindow *window = NULL;
    if (info.GetData1())
    { // container specified
      window = GetWindowWithCondition(contextWindow, 0);
    }
    else
    { // no container specified - assume a mediawindow
      window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    }
    if (window)
      return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty(m_stringParameters[info.GetData2()]).asString();
  }
  else if (info.m_info == CONTAINER_ART)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (window)
      return ((CGUIMediaWindow *)window)->CurrentDirectory().GetArt(m_stringParameters[info.GetData2()]);
  }
  else if (info.m_info == CONTAINER_CONTENT)
  {
    CGUIWindow *window = NULL;
    if (info.GetData1())
    { // container specified
      window = GetWindowWithCondition(contextWindow, 0);
    }
    else
    { // no container specified - assume a mediawindow
      window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    }
    if (window)
      return ((CGUIMediaWindow *)window)->CurrentDirectory().GetContent();
  }
  else if (info.m_info == CONTROL_GET_LABEL)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
    if (window)
    {
      const CGUIControl *control = window->GetControl(info.GetData1());
      if (control)
      {
        int data2 = info.GetData2();
        if (data2)
          return control->GetDescriptionByIndex(data2);
        else
          return control->GetDescription();
      }
    }
  }
  else if (info.m_info == WINDOW_PROPERTY)
  {
    CGUIWindow *window = NULL;
    if (info.GetData1())
    { // window specified
      window = g_windowManager.GetWindow(info.GetData1());//GetWindowWithCondition(contextWindow, 0);
    }
    else
    { // no window specified - assume active
      window = GetWindowWithCondition(contextWindow, 0);
    }

    if (window)
      return window->GetProperty(m_stringParameters[info.GetData2()]).asString();
  }
  else if (info.m_info == SYSTEM_ADDON_TITLE ||
           info.m_info == SYSTEM_ADDON_ICON ||
           info.m_info == SYSTEM_ADDON_VERSION)
  {
    // This logic does not check/care whether an addon has been disabled/marked as broken,
    // it simply retrieves it's name or icon that means if an addon is placed on the home screen it
    // will stay there even if it's disabled/marked as broken. This might need to be changed/fixed
    // in the future.
    AddonPtr addon;
    if (info.GetData2() == 0)
      CAddonMgr::GetInstance().GetAddon(const_cast<CGUIInfoManager*>(this)->GetLabel(info.GetData1(), contextWindow),addon,ADDON_UNKNOWN,false);
    else
      CAddonMgr::GetInstance().GetAddon(m_stringParameters[info.GetData1()],addon,ADDON_UNKNOWN,false);
    if (addon && info.m_info == SYSTEM_ADDON_TITLE)
      return addon->Name();
    if (addon && info.m_info == SYSTEM_ADDON_ICON)
      return addon->Icon();
    if (addon && info.m_info == SYSTEM_ADDON_VERSION)
      return addon->Version().asString();
  }
  else if (info.m_info == PLAYLIST_LENGTH ||
           info.m_info == PLAYLIST_POSITION ||
           info.m_info == PLAYLIST_RANDOM ||
           info.m_info == PLAYLIST_REPEAT)
  {
    int playlistid = info.GetData1();
    if (playlistid > PLAYLIST_NONE)
      return GetPlaylistLabel(info.m_info, playlistid);
  }
  else if (info.m_info == RDS_GET_RADIOTEXT_LINE)
  {
    return g_application.m_pPlayer->GetRadioText(info.GetData1());
  }

  return "";
}

/// \brief Obtains the filename of the image to show from whichever subsystem is needed
std::string CGUIInfoManager::GetImage(int info, int contextWindow, std::string *fallback)
{
  if (info >= CONDITIONAL_LABEL_START && info <= CONDITIONAL_LABEL_END)
    return GetSkinVariableString(info, true);

  if (info >= MULTI_INFO_START && info <= MULTI_INFO_END)
  {
    return GetMultiInfoLabel(m_multiInfo[info - MULTI_INFO_START], contextWindow, fallback);
  }
  else if (info == WEATHER_CONDITIONS)
    return g_weatherManager.GetInfo(WEATHER_IMAGE_CURRENT_ICON);
  else if (info == SYSTEM_PROFILETHUMB)
  {
    std::string thumb = CProfilesManager::GetInstance().GetCurrentProfile().getThumb();
    if (thumb.empty())
      thumb = "DefaultUser.png";
    return thumb;
  }
  else if (info == MUSICPLAYER_COVER)
  {
    if (!g_application.m_pPlayer->IsPlayingAudio()) return "";
    if (fallback)
      *fallback = "DefaultAlbumCover.png";
    return m_currentFile->HasArt("thumb") ? m_currentFile->GetArt("thumb") : "DefaultAlbumCover.png";
  }
  else if (info == VIDEOPLAYER_COVER)
  {
    if (!g_application.m_pPlayer->IsPlayingVideo()) return "";
    if (fallback)
      *fallback = "DefaultVideoCover.png";
    if(m_currentMovieThumb.empty())
      return m_currentFile->HasArt("thumb") ? m_currentFile->GetArt("thumb") : "DefaultVideoCover.png";
    else return m_currentMovieThumb;
  }
  else if (info == LISTITEM_THUMB || info == LISTITEM_ICON || info == LISTITEM_ACTUAL_ICON ||
          info == LISTITEM_OVERLAY)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_HAS_LIST_ITEMS);
    if (window)
    {
      CFileItemPtr item = window->GetCurrentListItem();
      if (item)
        return GetItemImage(item.get(), info, fallback);
    }
  }
  return GetLabel(info, contextWindow, fallback);
}

std::string CGUIInfoManager::GetDate(bool bNumbersOnly)
{
  CDateTime time=CDateTime::GetCurrentDateTime();
  return time.GetAsLocalizedDate(!bNumbersOnly);
}

std::string CGUIInfoManager::GetTime(TIME_FORMAT format) const
{
  CDateTime time=CDateTime::GetCurrentDateTime();
  return LocalizeTime(time, format);
}

std::string CGUIInfoManager::LocalizeTime(const CDateTime &time, TIME_FORMAT format) const
{
  const std::string timeFormat = g_langInfo.GetTimeFormat();
  bool use12hourclock = timeFormat.find('h') != std::string::npos;
  switch (format)
  {
  case TIME_FORMAT_GUESS:
    return time.GetAsLocalizedTime("", false);
  case TIME_FORMAT_SS:
    return time.GetAsLocalizedTime("ss", true);
  case TIME_FORMAT_MM:
    return time.GetAsLocalizedTime("mm", true);
  case TIME_FORMAT_MM_SS:
    return time.GetAsLocalizedTime("mm:ss", true);
  case TIME_FORMAT_HH:  // this forces it to a 12 hour clock
    return time.GetAsLocalizedTime(use12hourclock ? "h" : "HH", false);
  case TIME_FORMAT_HH_MM:
    return time.GetAsLocalizedTime(use12hourclock ? "h:mm" : "HH:mm", false);
  case TIME_FORMAT_HH_MM_XX:
      return time.GetAsLocalizedTime(use12hourclock ? "h:mm xx" : "HH:mm", false);
  case TIME_FORMAT_HH_MM_SS:
    return time.GetAsLocalizedTime(use12hourclock ? "hh:mm:ss" : "HH:mm:ss", true);
  case TIME_FORMAT_HH_MM_SS_XX:
    return time.GetAsLocalizedTime(use12hourclock ? "hh:mm:ss xx" : "HH:mm:ss", true);
  case TIME_FORMAT_H:
    return time.GetAsLocalizedTime("h", false);
  case TIME_FORMAT_H_MM_SS:
    return time.GetAsLocalizedTime("h:mm:ss", true);
  case TIME_FORMAT_H_MM_SS_XX:
    return time.GetAsLocalizedTime("h:mm:ss xx", true);
  case TIME_FORMAT_XX:
    return use12hourclock ? time.GetAsLocalizedTime("xx", false) : "";
  default:
    break;
  }
  return time.GetAsLocalizedTime("", false);
}

std::string CGUIInfoManager::GetDuration(TIME_FORMAT format) const
{
  if (g_application.m_pPlayer->IsPlayingAudio() && m_currentFile->HasMusicInfoTag())
  {
    const CMusicInfoTag& tag = *m_currentFile->GetMusicInfoTag();
    if (tag.GetDuration() > 0)
      return StringUtils::SecondsToTimeString(tag.GetDuration(), format);
  }
  if (g_application.m_pPlayer->IsPlayingVideo() && !m_currentMovieDuration.empty())
    return m_currentMovieDuration;
  unsigned int iTotal = MathUtils::round_int(g_application.GetTotalTime());
  if (iTotal > 0)
    return StringUtils::SecondsToTimeString(iTotal, format);
  return "";
}

std::string CGUIInfoManager::GetMusicPartyModeLabel(int item)
{
  // get song counts
  if (item >= MUSICPM_SONGSPLAYED && item <= MUSICPM_RANDOMSONGSPICKED)
  {
    int iSongs = -1;
    switch (item)
    {
    case MUSICPM_SONGSPLAYED:
      {
        iSongs = g_partyModeManager.GetSongsPlayed();
        break;
      }
    case MUSICPM_MATCHINGSONGS:
      {
        iSongs = g_partyModeManager.GetMatchingSongs();
        break;
      }
    case MUSICPM_MATCHINGSONGSPICKED:
      {
        iSongs = g_partyModeManager.GetMatchingSongsPicked();
        break;
      }
    case MUSICPM_MATCHINGSONGSLEFT:
      {
        iSongs = g_partyModeManager.GetMatchingSongsLeft();
        break;
      }
    case MUSICPM_RELAXEDSONGSPICKED:
      {
        iSongs = g_partyModeManager.GetRelaxedSongs();
        break;
      }
    case MUSICPM_RANDOMSONGSPICKED:
      {
        iSongs = g_partyModeManager.GetRandomSongs();
        break;
      }
    }
    if (iSongs < 0)
      return "";
    std::string strLabel = StringUtils::Format("%i", iSongs);
    return strLabel;
  }
  return "";
}

const std::string CGUIInfoManager::GetMusicPlaylistInfo(const GUIInfo& info)
{
  PLAYLIST::CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
  if (playlist.size() < 1)
    return "";
  int index = info.GetData2();
  if (info.GetData1() == 1)
  { // relative index (requires current playlist is PLAYLIST_MUSIC)
    if (g_playlistPlayer.GetCurrentPlaylist() != PLAYLIST_MUSIC)
      return "";
    index = g_playlistPlayer.GetNextSong(index);
  }
  if (index < 0 || index >= playlist.size())
    return "";
  CFileItemPtr playlistItem = playlist[index];
  if (!playlistItem->GetMusicInfoTag()->Loaded())
  {
    playlistItem->LoadMusicTag();
    playlistItem->GetMusicInfoTag()->SetLoaded();
  }
  // try to set a thumbnail
  if (!playlistItem->HasArt("thumb"))
  {
    CMusicThumbLoader loader;
    loader.LoadItem(playlistItem.get());
    // still no thumb? then just the set the default cover
    if (!playlistItem->HasArt("thumb"))
      playlistItem->SetArt("thumb", "DefaultAlbumCover.png");
  }
  if (info.m_info == MUSICPLAYER_PLAYLISTPOS)
  {
    std::string strPosition = StringUtils::Format("%i", index + 1);
    return strPosition;
  }
  else if (info.m_info == MUSICPLAYER_COVER)
    return playlistItem->GetArt("thumb");
  return GetMusicTagLabel(info.m_info, playlistItem.get());
}

std::string CGUIInfoManager::GetPlaylistLabel(int item, int playlistid /* = PLAYLIST_NONE */) const
{
  if (playlistid <= PLAYLIST_NONE && !g_application.m_pPlayer->IsPlaying())
    return "";

  int iPlaylist = playlistid == PLAYLIST_NONE ? g_playlistPlayer.GetCurrentPlaylist() : playlistid;
  switch (item)
  {
  case PLAYLIST_LENGTH:
    {
      return StringUtils::Format("%i", g_playlistPlayer.GetPlaylist(iPlaylist).size());
    }
  case PLAYLIST_POSITION:
    {
      return StringUtils::Format("%i", g_playlistPlayer.GetCurrentSong() + 1);
    }
  case PLAYLIST_RANDOM:
    {
      if (g_playlistPlayer.IsShuffled(iPlaylist))
        return g_localizeStrings.Get(590); // 590: Random
      else
        return g_localizeStrings.Get(591); // 591: Off
    }
  case PLAYLIST_REPEAT:
    {
      PLAYLIST::REPEAT_STATE state = g_playlistPlayer.GetRepeat(iPlaylist);
      if (state == PLAYLIST::REPEAT_ONE)
        return g_localizeStrings.Get(592); // 592: One
      else if (state == PLAYLIST::REPEAT_ALL)
        return g_localizeStrings.Get(593); // 593: All
      else
        return g_localizeStrings.Get(594); // 594: Off
    }
  }
  return "";
}

std::string CGUIInfoManager::GetRadioRDSLabel(int item)
{
  if (!g_application.m_pPlayer->IsPlaying() ||
      !m_currentFile->HasPVRChannelInfoTag() ||
      !m_currentFile->HasPVRRadioRDSInfoTag())
    return "";

  const PVR::CPVRRadioRDSInfoTag &tag = *m_currentFile->GetPVRRadioRDSInfoTag();
  switch (item)
  {
  case RDS_CHANNEL_COUNTRY:
    return tag.GetCountry();

  case RDS_AUDIO_LANG:
    {
      if (!tag.GetLanguage().empty())
        return tag.GetLanguage();

      SPlayerAudioStreamInfo info;
      g_application.m_pPlayer->GetAudioStreamInfo(g_application.m_pPlayer->GetAudioStream(), info);
      return info.language;
    }

  case RDS_TITLE:
    return tag.GetTitle();

  case RDS_ARTIST:
    return tag.GetArtist();

  case RDS_BAND:
    return tag.GetBand();

  case RDS_COMPOSER:
    return tag.GetComposer();

  case RDS_CONDUCTOR:
    return tag.GetConductor();

  case RDS_ALBUM:
    return tag.GetAlbum();

  case RDS_ALBUM_TRACKNUMBER:
    {
      if (tag.GetAlbumTrackNumber() > 0)
        return StringUtils::Format("%i", tag.GetAlbumTrackNumber());
      break;
    }
  case RDS_GET_RADIO_STYLE:
    return tag.GetRadioStyle();

  case RDS_COMMENT:
    return tag.GetComment();

  case RDS_INFO_NEWS:
    return tag.GetInfoNews();

  case RDS_INFO_NEWS_LOCAL:
    return tag.GetInfoNewsLocal();

  case RDS_INFO_STOCK:
    return tag.GetInfoStock();

  case RDS_INFO_STOCK_SIZE:
    return StringUtils::Format("%i", (int)tag.GetInfoStock().size());

  case RDS_INFO_SPORT:
    return tag.GetInfoSport();

  case RDS_INFO_SPORT_SIZE:
    return StringUtils::Format("%i", (int)tag.GetInfoSport().size());

  case RDS_INFO_LOTTERY:
    return tag.GetInfoLottery();

  case RDS_INFO_LOTTERY_SIZE:
    return StringUtils::Format("%i", (int)tag.GetInfoLottery().size());

  case RDS_INFO_WEATHER:
    return tag.GetInfoWeather();

  case RDS_INFO_WEATHER_SIZE:
    return StringUtils::Format("%i", (int)tag.GetInfoWeather().size());

  case RDS_INFO_HOROSCOPE:
    return tag.GetInfoHoroscope();

  case RDS_INFO_HOROSCOPE_SIZE:
    return StringUtils::Format("%i", (int)tag.GetInfoHoroscope().size());

  case RDS_INFO_CINEMA:
    return tag.GetInfoCinema();

  case RDS_INFO_CINEMA_SIZE:
    return StringUtils::Format("%i", (int)tag.GetInfoCinema().size());

  case RDS_INFO_OTHER:
    return tag.GetInfoOther();

  case RDS_INFO_OTHER_SIZE:
    return StringUtils::Format("%i", (int)tag.GetInfoOther().size());

  case RDS_PROG_STATION:
    {
      if (!tag.GetProgStation().empty())
        return tag.GetProgStation();
      const CPVRChannelPtr channeltag = m_currentFile->GetPVRChannelInfoTag();
      if (channeltag)
        return channeltag->ChannelName();
      break;
    }

  case RDS_PROG_NOW:
    {
      if (!tag.GetProgNow().empty())
        return tag.GetProgNow();

      CEpgInfoTagPtr epgNow(m_currentFile->GetPVRChannelInfoTag()->GetEPGNow());
      return epgNow ?
                epgNow->Title() :
                CSettings::GetInstance().GetBool("epg.hidenoinfoavailable") ? "" : g_localizeStrings.Get(19055); // no information available
      break;
    }

  case RDS_PROG_NEXT:
    {
      if (!tag.GetProgNext().empty())
        return tag.GetProgNext();

      CEpgInfoTagPtr epgNext(m_currentFile->GetPVRChannelInfoTag()->GetEPGNext());
      return epgNext ?
                epgNext->Title() :
                CSettings::GetInstance().GetBool("epg.hidenoinfoavailable") ? "" : g_localizeStrings.Get(19055); // no information available
      break;
    }

  case RDS_PROG_HOST:
    return tag.GetProgHost();

  case RDS_PROG_EDIT_STAFF:
    return tag.GetEditorialStaff();

  case RDS_PROG_HOMEPAGE:
    return tag.GetProgWebsite();

  case RDS_PROG_STYLE:
    return tag.GetProgStyle();

  case RDS_PHONE_HOTLINE:
    return tag.GetPhoneHotline();

  case RDS_PHONE_STUDIO:
    return tag.GetPhoneStudio();

  case RDS_SMS_STUDIO:
    return tag.GetSMSStudio();

  case RDS_EMAIL_HOTLINE:
    return tag.GetEMailHotline();

  case RDS_EMAIL_STUDIO:
    return tag.GetEMailStudio();

  default:
    break;
  }
  return "";
}

std::string CGUIInfoManager::GetMusicLabel(int item)
{
  if (!g_application.m_pPlayer->IsPlaying() || !m_currentFile->HasMusicInfoTag()) return "";

  switch (item)
  {
  case MUSICPLAYER_PLAYLISTLEN:
    {
      if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
        return GetPlaylistLabel(PLAYLIST_LENGTH);
    }
    break;
  case MUSICPLAYER_PLAYLISTPOS:
    {
      if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
        return GetPlaylistLabel(PLAYLIST_POSITION);
    }
    break;
  case MUSICPLAYER_BITRATE:
    {
      std::string strBitrate = "";
      if (m_audioInfo.bitrate > 0)
        strBitrate = StringUtils::Format("%i", MathUtils::round_int((double)m_audioInfo.bitrate / 1000.0));
      return strBitrate;
    }
    break;
  case MUSICPLAYER_CHANNELS:
    {
      std::string strChannels = "";
      if (m_audioInfo.channels > 0)
      {
        strChannels = StringUtils::Format("%i", m_audioInfo.channels);
      }
      return strChannels;
    }
    break;
  case MUSICPLAYER_BITSPERSAMPLE:
    {
      std::string strBitsPerSample = "";
      if (m_audioInfo.bitspersample > 0)
        strBitsPerSample = StringUtils::Format("%i", m_audioInfo.bitspersample);
      return strBitsPerSample;
    }
    break;
  case MUSICPLAYER_SAMPLERATE:
    {
      std::string strSampleRate = "";
      if (m_audioInfo.samplerate > 0)
        strSampleRate = StringUtils::Format("%.5g", ((double)m_audioInfo.samplerate / 1000.0));
      return strSampleRate;
    }
    break;
  case MUSICPLAYER_CODEC:
    {
      return StringUtils::Format("%s", m_audioInfo.audioCodecName.c_str());
    }
    break;
  }
  return GetMusicTagLabel(item, m_currentFile);
}

std::string CGUIInfoManager::GetMusicTagLabel(int info, const CFileItem *item)
{
  if (!item->HasMusicInfoTag()) return "";
  const CMusicInfoTag &tag = *item->GetMusicInfoTag();

  switch (info)
  {
  case MUSICPLAYER_TITLE:
    if (tag.GetTitle().size()) { return tag.GetTitle(); }
    break;
  case MUSICPLAYER_ALBUM:
    if (tag.GetAlbum().size()) { return tag.GetAlbum(); }
    break;
  case MUSICPLAYER_ARTIST:
    if (tag.GetArtistString().size()) { return tag.GetArtistString(); }
    break;
  case MUSICPLAYER_ALBUM_ARTIST:
    if (tag.GetAlbumArtistString().size()) { return tag.GetAlbumArtistString(); }
    break;
  case MUSICPLAYER_YEAR:
    if (tag.GetYear()) { return tag.GetYearString(); }
    break;
  case MUSICPLAYER_GENRE:
    if (tag.GetGenre().size()) { return StringUtils::Join(tag.GetGenre(), g_advancedSettings.m_musicItemSeparator); }
    break;
  case MUSICPLAYER_LYRICS:
    if (tag.GetLyrics().size()) { return tag.GetLyrics(); }
  break;
  case MUSICPLAYER_TRACK_NUMBER:
    {
      std::string strTrack;
      if (tag.Loaded() && tag.GetTrackNumber() > 0)
      {
        return StringUtils::Format("%02i", tag.GetTrackNumber());
      }
    }
    break;
  case MUSICPLAYER_DISC_NUMBER:
    return GetItemLabel(item, LISTITEM_DISC_NUMBER);
  case MUSICPLAYER_RATING:
    return GetItemLabel(item, LISTITEM_RATING);
  case MUSICPLAYER_RATING_AND_VOTES:
  {
    std::string strRatingAndVotes;
    if (m_currentFile->GetMusicInfoTag()->GetRating() > 0.f)
    {
      if (m_currentFile->GetMusicInfoTag()->GetRating() > 0)
        strRatingAndVotes = StringUtils::FormatNumber(m_currentFile->GetMusicInfoTag()->GetRating());
      else
        strRatingAndVotes = FormatRatingAndVotes(m_currentFile->GetMusicInfoTag()->GetRating(),
                                                 m_currentFile->GetMusicInfoTag()->GetVotes());
    }
    return strRatingAndVotes;
  }
  break;
  case MUSICPLAYER_USER_RATING:
    return GetItemLabel(item, LISTITEM_USER_RATING);
  case MUSICPLAYER_COMMENT:
    return GetItemLabel(item, LISTITEM_COMMENT);
  case MUSICPLAYER_MOOD:
    return GetItemLabel(item, LISTITEM_MOOD);
  case MUSICPLAYER_CONTRIBUTORS:
    return GetItemLabel(item, LISTITEM_CONTRIBUTORS);
  case MUSICPLAYER_CONTRIBUTOR_AND_ROLE:
    return GetItemLabel(item, LISTITEM_CONTRIBUTOR_AND_ROLE);
  case MUSICPLAYER_DURATION:
    return GetItemLabel(item, LISTITEM_DURATION);
  case MUSICPLAYER_CHANNEL_NAME:
    {
      if (m_currentFile->HasPVRChannelInfoTag())
      {
        if (m_currentFile->HasPVRRadioRDSInfoTag())
        {
          const CPVRRadioRDSInfoTagPtr rdstag(m_currentFile->GetPVRRadioRDSInfoTag());
          if (rdstag && !rdstag->GetProgStation().empty())
            return rdstag->GetProgStation();
        }
        return m_currentFile->GetPVRChannelInfoTag()->ChannelName();
      }
    }
    break;
  case MUSICPLAYER_CHANNEL_NUMBER:
    {
      if (m_currentFile->HasPVRChannelInfoTag())
        return StringUtils::Format("%i", m_currentFile->GetPVRChannelInfoTag()->ChannelNumber());
    }
    break;
  case MUSICPLAYER_SUB_CHANNEL_NUMBER:
    {
      if (m_currentFile->HasPVRChannelInfoTag())
        return StringUtils::Format("%i", m_currentFile->GetPVRChannelInfoTag()->SubChannelNumber());
    }
    break;
  case MUSICPLAYER_CHANNEL_NUMBER_LBL:
    {
      if (m_currentFile->HasPVRChannelInfoTag())
        return m_currentFile->GetPVRChannelInfoTag()->FormattedChannelNumber();
    }
    break;
  case MUSICPLAYER_CHANNEL_GROUP:
    {
      if (m_currentFile->HasPVRChannelInfoTag() && m_currentFile->GetPVRChannelInfoTag()->IsRadio())
        return g_PVRManager.GetPlayingGroup(true)->GroupName();
    }
    break;
  case MUSICPLAYER_PLAYCOUNT:
    return GetItemLabel(item, LISTITEM_PLAYCOUNT);
  case MUSICPLAYER_LASTPLAYED:
    return GetItemLabel(item, LISTITEM_LASTPLAYED);
  }
  return "";
}

std::string CGUIInfoManager::GetVideoLabel(int item)
{
  if (!g_application.m_pPlayer->IsPlaying())
    return "";

  if (m_currentFile->HasPVRChannelInfoTag())
  {
    CPVRChannelPtr tag(m_currentFile->GetPVRChannelInfoTag());
    CEpgInfoTagPtr epgTag;

    switch (item)
    {
    /* Now playing infos */
    case VIDEOPLAYER_TITLE:
      epgTag = tag->GetEPGNow();
      return epgTag ?
          epgTag->Title() :
          CSettings::GetInstance().GetBool(CSettings::SETTING_EPG_HIDENOINFOAVAILABLE) ?
                            "" : g_localizeStrings.Get(19055); // no information available
    case VIDEOPLAYER_GENRE:
      epgTag = tag->GetEPGNow();
      return epgTag ? StringUtils::Join(epgTag->Genre(), g_advancedSettings.m_videoItemSeparator) : "";
    case VIDEOPLAYER_PLOT:
      epgTag = tag->GetEPGNow();
      return epgTag ? epgTag->Plot() : "";
    case VIDEOPLAYER_PLOT_OUTLINE:
      epgTag = tag->GetEPGNow();
      return epgTag ? epgTag->PlotOutline() : "";
    case VIDEOPLAYER_STARTTIME:
      epgTag = tag->GetEPGNow();
      return epgTag ? epgTag->StartAsLocalTime().GetAsLocalizedTime("", false) : CDateTime::GetCurrentDateTime().GetAsLocalizedTime("", false);
    case VIDEOPLAYER_ENDTIME:
      epgTag = tag->GetEPGNow();
      return epgTag ? epgTag->EndAsLocalTime().GetAsLocalizedTime("", false) : CDateTime::GetCurrentDateTime().GetAsLocalizedTime("", false);
    case VIDEOPLAYER_IMDBNUMBER:
      epgTag = tag->GetEPGNow();
      return epgTag ? epgTag->IMDBNumber() : "";
    case VIDEOPLAYER_ORIGINALTITLE:
      epgTag = tag->GetEPGNow();
      return epgTag ? epgTag->OriginalTitle() : "";
    case VIDEOPLAYER_YEAR:
      epgTag = tag->GetEPGNow();
      if (epgTag && epgTag->Year() > 0)
        return StringUtils::Format("%i", epgTag->Year());
      break;
    case VIDEOPLAYER_EPISODE:
      epgTag = tag->GetEPGNow();
      if (epgTag && epgTag->EpisodeNumber() > 0)
      {
        if (epgTag->SeriesNumber() == 0) // prefix episode with 'S'
          return StringUtils::Format("S%i", epgTag->EpisodeNumber());
        else
          return StringUtils::Format("%i", epgTag->EpisodeNumber());
      }
      break;
    case VIDEOPLAYER_SEASON:
      epgTag = tag->GetEPGNow();
      if (epgTag && epgTag->SeriesNumber() > 0)
        return StringUtils::Format("%i", epgTag->SeriesNumber());
      break;
    case VIDEOPLAYER_EPISODENAME:
      epgTag = tag->GetEPGNow();
      return epgTag ? epgTag->EpisodeName() : "";
    case VIDEOPLAYER_CAST:
      epgTag = tag->GetEPGNow();
      return epgTag ? epgTag->Cast() : "";
    case VIDEOPLAYER_DIRECTOR:
      epgTag = tag->GetEPGNow();
      return epgTag ? epgTag->Director() : "";
    case VIDEOPLAYER_WRITER:
      epgTag = tag->GetEPGNow();
      return epgTag ? epgTag->Writer() : "";

    /* Next playing infos */
    case VIDEOPLAYER_NEXT_TITLE:
      epgTag = tag->GetEPGNext();
      return epgTag ?
          epgTag->Title() :
          CSettings::GetInstance().GetBool(CSettings::SETTING_EPG_HIDENOINFOAVAILABLE) ?
                            "" : g_localizeStrings.Get(19055); // no information available
    case VIDEOPLAYER_NEXT_GENRE:
      epgTag = tag->GetEPGNext();
      return epgTag ? StringUtils::Join(epgTag->Genre(), g_advancedSettings.m_videoItemSeparator) : "";
    case VIDEOPLAYER_NEXT_PLOT:
      epgTag = tag->GetEPGNext();
      return epgTag ? epgTag->Plot() : "";
    case VIDEOPLAYER_NEXT_PLOT_OUTLINE:
      epgTag = tag->GetEPGNext();
      return epgTag ? epgTag->PlotOutline() : "";
    case VIDEOPLAYER_NEXT_STARTTIME:
      epgTag = tag->GetEPGNext();
      return epgTag ? epgTag->StartAsLocalTime().GetAsLocalizedTime("", false) : CDateTime::GetCurrentDateTime().GetAsLocalizedTime("", false);
    case VIDEOPLAYER_NEXT_ENDTIME:
      epgTag = tag->GetEPGNext();
      return epgTag ? epgTag->EndAsLocalTime().GetAsLocalizedTime("", false) : CDateTime::GetCurrentDateTime().GetAsLocalizedTime("", false);
    case VIDEOPLAYER_NEXT_DURATION:
      {
        std::string duration;
        epgTag = tag->GetEPGNext();
        if (epgTag && epgTag->GetDuration() > 0)
          duration = StringUtils::SecondsToTimeString(epgTag->GetDuration());
        return duration;
      }

    case VIDEOPLAYER_PARENTAL_RATING:
      {
        std::string rating;
        epgTag = tag->GetEPGNow();
        if (epgTag && epgTag->ParentalRating() > 0)
          rating = StringUtils::Format("%i", epgTag->ParentalRating());
        return rating;
      }
      break;

    /* General channel infos */
    case VIDEOPLAYER_CHANNEL_NAME:
      return tag->ChannelName();

    case VIDEOPLAYER_CHANNEL_NUMBER:
      return StringUtils::Format("%i", tag->ChannelNumber());

    case VIDEOPLAYER_SUB_CHANNEL_NUMBER:
      return StringUtils::Format("%i", tag->SubChannelNumber());

    case VIDEOPLAYER_CHANNEL_NUMBER_LBL:
      return tag->FormattedChannelNumber();

    case VIDEOPLAYER_CHANNEL_GROUP:
      {
        if (tag && !tag->IsRadio())
          return g_PVRManager.GetPlayingTVGroupName();
      }
    }
  }
  else if (m_currentFile->HasPVRRecordingInfoTag())
  {
    const CPVRRecordingPtr tag(m_currentFile->GetPVRRecordingInfoTag());

    switch (item)
    {
      case VIDEOPLAYER_TITLE:
        return tag->m_strTitle;

      case VIDEOPLAYER_GENRE:
        return StringUtils::Join(tag->m_genre, g_advancedSettings.m_videoItemSeparator);

      case VIDEOPLAYER_PLOT:
        return tag->m_strPlot;

      case VIDEOPLAYER_PLOT_OUTLINE:
        return tag->m_strPlotOutline;

      case VIDEOPLAYER_STARTTIME:
        return tag->RecordingTimeAsLocalTime().GetAsLocalizedTime("", false);

      case VIDEOPLAYER_ENDTIME:
        return (tag->RecordingTimeAsLocalTime() + tag->m_duration).GetAsLocalizedTime("", false);

      case VIDEOPLAYER_YEAR:
        if (tag->HasYear())
          return StringUtils::Format("%i", tag->GetYear());
        break;

      case VIDEOPLAYER_EPISODE:
        if (tag->m_iEpisode > 0)
        {
          if (tag->m_iEpisode == 0) // prefix episode with 'S'
            return StringUtils::Format("S%i", tag->m_iEpisode);
          else
            return StringUtils::Format("%i", tag->m_iEpisode);
        }
        break;

      case VIDEOPLAYER_SEASON:
        if (tag->m_iSeason > 0)
          return StringUtils::Format("%i", tag->m_iSeason);
        break;

      case VIDEOPLAYER_EPISODENAME:
        return tag->EpisodeName();

      case VIDEOPLAYER_CHANNEL_NAME:
        return tag->m_strChannelName;

      case VIDEOPLAYER_CHANNEL_NUMBER:
      {
        const CPVRChannelPtr channel(tag->Channel());
        if (channel)
          return StringUtils::Format("%i", channel->ChannelNumber());
        break;
      }

      case VIDEOPLAYER_SUB_CHANNEL_NUMBER:
      {
        const CPVRChannelPtr channel(tag->Channel());
        if (channel)
          return StringUtils::Format("%i", channel->SubChannelNumber());
        break;
      }

      case VIDEOPLAYER_CHANNEL_NUMBER_LBL:
      {
        const CPVRChannelPtr channel(tag->Channel());
        if (channel)
          return channel->FormattedChannelNumber();
        break;
      }

      case VIDEOPLAYER_CHANNEL_GROUP:
      {
        if (!tag->IsRadio())
          return g_PVRManager.GetPlayingTVGroupName();
        break;
      }

      case VIDEOPLAYER_PLAYCOUNT:
      {
        if (tag->m_playCount > 0)
          return StringUtils::Format("%i", tag->m_playCount);
        break;
      }
    }
  }
  else if (m_currentFile->HasVideoInfoTag())
  {
    switch (item)
    {
    case VIDEOPLAYER_ORIGINALTITLE:
      return m_currentFile->GetVideoInfoTag()->m_strOriginalTitle;
      break;
    case VIDEOPLAYER_GENRE:
      return StringUtils::Join(m_currentFile->GetVideoInfoTag()->m_genre, g_advancedSettings.m_videoItemSeparator);
      break;
    case VIDEOPLAYER_DIRECTOR:
      return StringUtils::Join(m_currentFile->GetVideoInfoTag()->m_director, g_advancedSettings.m_videoItemSeparator);
      break;
    case VIDEOPLAYER_IMDBNUMBER:
      return m_currentFile->GetVideoInfoTag()->GetUniqueID();
    case VIDEOPLAYER_RATING:
      {
        std::string strRating;
        float rating = m_currentFile->GetVideoInfoTag()->GetRating().rating;
        if (rating > 0.f)
          strRating = StringUtils::FormatNumber(rating);
        return strRating;
      }
      break;
    case VIDEOPLAYER_RATING_AND_VOTES:
      {
        std::string strRatingAndVotes;
        CRating rating = m_currentFile->GetVideoInfoTag()->GetRating();
        if (rating.rating > 0.f)
        {
          if (rating.votes == 0)
            strRatingAndVotes = StringUtils::FormatNumber(rating.rating);
          else
            strRatingAndVotes = FormatRatingAndVotes(rating.rating, rating.votes);
        }
        return strRatingAndVotes;
      }
      break;
    case VIDEOPLAYER_USER_RATING:
    {
      std::string strUserRating;
      if (m_currentFile->GetVideoInfoTag()->m_iUserRating > 0)
        strUserRating = StringUtils::Format("%i", m_currentFile->GetVideoInfoTag()->m_iUserRating);
      return strUserRating;
    }
    case VIDEOPLAYER_VOTES:
      return StringUtils::FormatNumber(m_currentFile->GetVideoInfoTag()->GetRating().votes);
    case VIDEOPLAYER_YEAR:
      {
        std::string strYear;
        if (m_currentFile->GetVideoInfoTag()->HasYear())
          strYear = StringUtils::Format("%i", m_currentFile->GetVideoInfoTag()->GetYear());
        return strYear;
      }
      break;
    case VIDEOPLAYER_PREMIERED:
      {
        CDateTime dateTime;
        if (m_currentFile->GetVideoInfoTag()->m_firstAired.IsValid())
          dateTime = m_currentFile->GetVideoInfoTag()->m_firstAired;
        else if (m_currentFile->GetVideoInfoTag()->HasPremiered())
          dateTime = m_currentFile->GetVideoInfoTag()->GetPremiered();

        if (dateTime.IsValid())
          return dateTime.GetAsLocalizedDate();
        break;
      }
      break;
    case VIDEOPLAYER_PLOT:
      return m_currentFile->GetVideoInfoTag()->m_strPlot;
    case VIDEOPLAYER_TRAILER:
      return m_currentFile->GetVideoInfoTag()->m_strTrailer;
    case VIDEOPLAYER_PLOT_OUTLINE:
      return m_currentFile->GetVideoInfoTag()->m_strPlotOutline;
    case VIDEOPLAYER_EPISODE:
      if (m_currentFile->GetVideoInfoTag()->m_iEpisode > 0)
      {
        std::string strEpisode;
        if (m_currentFile->GetVideoInfoTag()->m_iSeason == 0) // prefix episode with 'S'
          strEpisode = StringUtils::Format("S%i", m_currentFile->GetVideoInfoTag()->m_iEpisode);
        else 
          strEpisode = StringUtils::Format("%i", m_currentFile->GetVideoInfoTag()->m_iEpisode);
        return strEpisode;
      }
      break;
    case VIDEOPLAYER_SEASON:
      if (m_currentFile->GetVideoInfoTag()->m_iSeason > 0)
      {
        return StringUtils::Format("%i", m_currentFile->GetVideoInfoTag()->m_iSeason);
      }
      break;
    case VIDEOPLAYER_TVSHOW:
      return m_currentFile->GetVideoInfoTag()->m_strShowTitle;

    case VIDEOPLAYER_STUDIO:
      return StringUtils::Join(m_currentFile->GetVideoInfoTag()->m_studio, g_advancedSettings.m_videoItemSeparator);
    case VIDEOPLAYER_COUNTRY:
      return StringUtils::Join(m_currentFile->GetVideoInfoTag()->m_country, g_advancedSettings.m_videoItemSeparator);
    case VIDEOPLAYER_MPAA:
      return m_currentFile->GetVideoInfoTag()->m_strMPAARating;
    case VIDEOPLAYER_TOP250:
      {
        std::string strTop250;
        if (m_currentFile->GetVideoInfoTag()->m_iTop250 > 0)
          strTop250 = StringUtils::Format("%i", m_currentFile->GetVideoInfoTag()->m_iTop250);
        return strTop250;
      }
      break;
    case VIDEOPLAYER_CAST:
      return m_currentFile->GetVideoInfoTag()->GetCast();
    case VIDEOPLAYER_CAST_AND_ROLE:
      return m_currentFile->GetVideoInfoTag()->GetCast(true);
    case VIDEOPLAYER_ARTIST:
      return StringUtils::Join(m_currentFile->GetVideoInfoTag()->m_artist, g_advancedSettings.m_videoItemSeparator);
    case VIDEOPLAYER_ALBUM:
      return m_currentFile->GetVideoInfoTag()->m_strAlbum;
    case VIDEOPLAYER_WRITER:
      return StringUtils::Join(m_currentFile->GetVideoInfoTag()->m_writingCredits, g_advancedSettings.m_videoItemSeparator);
    case VIDEOPLAYER_TAGLINE:
      return m_currentFile->GetVideoInfoTag()->m_strTagLine;
    case VIDEOPLAYER_LASTPLAYED:
      {
        if (m_currentFile->GetVideoInfoTag()->m_lastPlayed.IsValid())
          return m_currentFile->GetVideoInfoTag()->m_lastPlayed.GetAsLocalizedDateTime();
        break;
      }
    case VIDEOPLAYER_PLAYCOUNT:
      {
        std::string strPlayCount;
        if (m_currentFile->GetVideoInfoTag()->m_playCount > 0)
          strPlayCount = StringUtils::Format("%i", m_currentFile->GetVideoInfoTag()->m_playCount);
        return strPlayCount;
      }
    }
  }
  else if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO)
  {
    switch (item)
    {
    case VIDEOPLAYER_PLAYLISTLEN:
      return GetPlaylistLabel(PLAYLIST_LENGTH);
    case VIDEOPLAYER_PLAYLISTPOS:
      return GetPlaylistLabel(PLAYLIST_POSITION);
    default:
      break;
    }
  }
  
  if (item == VIDEOPLAYER_TITLE)
    return GetLabel(PLAYER_TITLE);

  return "";
}

int64_t CGUIInfoManager::GetPlayTime() const
{
  if (g_application.m_pPlayer->IsPlaying())
  {
    int64_t lPTS = (int64_t)(g_application.GetTime() * 1000);
    if (lPTS < 0) lPTS = 0;
    return lPTS;
  }
  return 0;
}

std::string CGUIInfoManager::GetCurrentPlayTime(TIME_FORMAT format) const
{
  if (format == TIME_FORMAT_GUESS && GetTotalPlayTime() >= 3600)
    format = TIME_FORMAT_HH_MM_SS;
  if (g_application.m_pPlayer->IsPlaying())
    return StringUtils::SecondsToTimeString(MathUtils::round_int(GetPlayTime()/1000.0), format);
  return "";
}

std::string CGUIInfoManager::GetCurrentSeekTime(TIME_FORMAT format) const
{
  if (format == TIME_FORMAT_GUESS && GetTotalPlayTime() >= 3600)
    format = TIME_FORMAT_HH_MM_SS;
  return StringUtils::SecondsToTimeString(g_application.GetTime() + CSeekHandler::GetInstance().GetSeekSize(), format);
}

int CGUIInfoManager::GetTotalPlayTime() const
{
  int iTotalTime = MathUtils::round_int(g_application.GetTotalTime());
  return iTotalTime > 0 ? iTotalTime : 0;
}

int CGUIInfoManager::GetPlayTimeRemaining() const
{
  int iReverse = GetTotalPlayTime() - MathUtils::round_int(g_application.GetTime());
  return iReverse > 0 ? iReverse : 0;
}

float CGUIInfoManager::GetSeekPercent() const
{
  if (GetTotalPlayTime() == 0)
    return 0.0f;

  float percentPlayTime = static_cast<float>(GetPlayTime()) / GetTotalPlayTime() * 0.1f;
  float percentPerSecond = 100.0f / static_cast<float>(GetTotalPlayTime());
  float percent = percentPlayTime + percentPerSecond * CSeekHandler::GetInstance().GetSeekSize();

  if (percent > 100.0f)
    percent = 100.0f;
  if (percent < 0.0f)
    percent = 0.0f;

  return percent;
}

std::string CGUIInfoManager::GetCurrentPlayTimeRemaining(TIME_FORMAT format) const
{
  if (format == TIME_FORMAT_GUESS && GetTotalPlayTime() >= 3600)
    format = TIME_FORMAT_HH_MM_SS;
  int timeRemaining = GetPlayTimeRemaining();
  if (timeRemaining && g_application.m_pPlayer->IsPlaying())
    return StringUtils::SecondsToTimeString(timeRemaining, format);
  return "";
}

void CGUIInfoManager::ResetCurrentItem()
{
  m_currentFile->Reset();
  m_currentMovieThumb = "";
  m_currentMovieDuration = "";
}

void CGUIInfoManager::SetCurrentItem(const CFileItemPtr item)
{
  CSetCurrentItemJob *job = new CSetCurrentItemJob(item);
  CJobManager::GetInstance().AddJob(job, NULL);
}

void CGUIInfoManager::SetCurrentItemJob(const CFileItemPtr item)
{
  ResetCurrentItem();

  if (item->IsAudio())
    SetCurrentSong(*item);
  else
    SetCurrentMovie(*item);

  if (item->HasPVRRadioRDSInfoTag())
    m_currentFile->SetPVRRadioRDSInfoTag(item->GetPVRRadioRDSInfoTag());
  if (item->HasEPGInfoTag())
    m_currentFile->SetEPGInfoTag(item->GetEPGInfoTag());
  else if (item->HasPVRChannelInfoTag())
  {
    CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
    if (tag)
      m_currentFile->SetEPGInfoTag(tag);
  }

  SetChanged();
  NotifyObservers(ObservableMessageCurrentItem);
}

void CGUIInfoManager::SetCurrentAlbumThumb(const std::string &thumbFileName)
{
  if (CFile::Exists(thumbFileName))
    m_currentFile->SetArt("thumb", thumbFileName);
  else
  {
    m_currentFile->SetArt("thumb", "");
    m_currentFile->FillInDefaultIcon();
  }
}

void CGUIInfoManager::SetCurrentSong(CFileItem &item)
{
  CLog::Log(LOGDEBUG,"CGUIInfoManager::SetCurrentSong(%s)",item.GetPath().c_str());
  *m_currentFile = item;

  m_currentFile->LoadMusicTag();
  if (m_currentFile->GetMusicInfoTag()->GetTitle().empty())
  {
    // No title in tag, show filename only
    m_currentFile->GetMusicInfoTag()->SetTitle(CUtil::GetTitleFromPath(m_currentFile->GetPath()));
  }
  m_currentFile->GetMusicInfoTag()->SetLoaded(true);

  // find a thumb for this file.
  if (m_currentFile->IsInternetStream())
  {
    if (!g_application.m_strPlayListFile.empty())
    {
      CLog::Log(LOGDEBUG,"Streaming media detected... using %s to find a thumb", g_application.m_strPlayListFile.c_str());
      CFileItem streamingItem(g_application.m_strPlayListFile,false);

      CMusicThumbLoader loader;
      loader.FillThumb(streamingItem);
      if (streamingItem.HasArt("thumb"))
        m_currentFile->SetArt("thumb", streamingItem.GetArt("thumb"));
    }
  }
  else
  {
    CMusicThumbLoader loader;
    loader.LoadItem(m_currentFile);
  }
  m_currentFile->FillInDefaultIcon();

  CMusicInfoLoader::LoadAdditionalTagInfo(m_currentFile);
}

void CGUIInfoManager::SetCurrentMovie(CFileItem &item)
{
  CLog::Log(LOGDEBUG,"CGUIInfoManager::SetCurrentMovie(%s)", CURL::GetRedacted(item.GetPath()).c_str());
  *m_currentFile = item;

  /* also call GetMovieInfo() when a VideoInfoTag is already present or additional info won't be present in the tag */
  if (!m_currentFile->HasPVRChannelInfoTag())
  {
    CVideoDatabase dbs;
    if (dbs.Open())
    {
      std::string path = item.GetPath();
      std::string videoInfoTagPath(item.GetVideoInfoTag()->m_strFileNameAndPath);
      if (videoInfoTagPath.find("removable://") == 0)
        path = videoInfoTagPath;
      dbs.LoadVideoInfo(path, *m_currentFile->GetVideoInfoTag());
      dbs.Close();
    }
  }

  // Find a thumb for this file.
  if (!item.HasArt("thumb"))
  {
    CVideoThumbLoader loader;
    loader.LoadItem(m_currentFile);
  }

  // find a thumb for this stream
  if (item.IsInternetStream())
  {
    // case where .strm is used to start an audio stream
    if (g_application.m_pPlayer->IsPlayingAudio())
    {
      SetCurrentSong(item);
      return;
    }

    // else its a video
    if (!g_application.m_strPlayListFile.empty())
    {
      CLog::Log(LOGDEBUG,"Streaming media detected... using %s to find a thumb", g_application.m_strPlayListFile.c_str());
      CFileItem thumbItem(g_application.m_strPlayListFile,false);

      CVideoThumbLoader loader;
      if (loader.FillThumb(thumbItem))
        item.SetArt("thumb", thumbItem.GetArt("thumb"));
    }
  }

  item.FillInDefaultIcon();
  m_currentMovieThumb = item.GetArt("thumb");
}

std::string CGUIInfoManager::GetSystemHeatInfo(int info)
{
  if (CTimeUtils::GetFrameTime() - m_lastSysHeatInfoTime >= SYSHEATUPDATEINTERVAL)
  { // update our variables
    m_lastSysHeatInfoTime = CTimeUtils::GetFrameTime();
#if defined(TARGET_POSIX)
    g_cpuInfo.getTemperature(m_cpuTemp);
    m_gpuTemp = GetGPUTemperature();
#endif
  }

  std::string text;
  switch(info)
  {
    case SYSTEM_CPU_TEMPERATURE:
      return m_cpuTemp.IsValid() ? g_langInfo.GetTemperatureAsString(m_cpuTemp) : "?";
      break;
    case SYSTEM_GPU_TEMPERATURE:
      return m_gpuTemp.IsValid() ? g_langInfo.GetTemperatureAsString(m_gpuTemp) : "?";
      break;
    case SYSTEM_FAN_SPEED:
      text = StringUtils::Format("%i%%", m_fanSpeed * 2);
      break;
    case SYSTEM_CPU_USAGE:
#if defined(TARGET_DARWIN_OSX)
      text = StringUtils::Format("%4.2f%%", m_resourceCounter.GetCPUUsage());
#elif defined(TARGET_DARWIN) || defined(TARGET_WINDOWS)
      text = StringUtils::Format("%d%%", g_cpuInfo.getUsedPercentage());
#else
      text = StringUtils::Format("%s", g_cpuInfo.GetCoresUsageString().c_str());
#endif
      break;
  }
  return text;
}

CTemperature CGUIInfoManager::GetGPUTemperature()
{
  int  value = 0;
  char scale = 0;

#if defined(TARGET_DARWIN_OSX)
  value = SMCGetTemperature(SMC_KEY_GPU_TEMP);
  return CTemperature::CreateFromCelsius(value);
#else
  std::string  cmd   = g_advancedSettings.m_gpuTempCmd;
  int         ret   = 0;
  FILE        *p    = NULL;

  if (cmd.empty() || !(p = popen(cmd.c_str(), "r")))
    return CTemperature();

  ret = fscanf(p, "%d %c", &value, &scale);
  pclose(p);

  if (ret != 2)
    return CTemperature();
#endif

  if (scale == 'C' || scale == 'c')
    return CTemperature::CreateFromCelsius(value);
  if (scale == 'F' || scale == 'f')
    return CTemperature::CreateFromFahrenheit(value);
  return CTemperature();
}


void CGUIInfoManager::SetDisplayAfterSeek(unsigned int timeOut, int seekOffset)
{
  if (timeOut>0)
  {
    m_AfterSeekTimeout = CTimeUtils::GetFrameTime() +  timeOut;
    if (seekOffset)
      m_seekOffset = seekOffset;
  }
  else
    m_AfterSeekTimeout = 0;
}

bool CGUIInfoManager::GetDisplayAfterSeek()
{
  if (CTimeUtils::GetFrameTime() < m_AfterSeekTimeout)
    return true;
  m_seekOffset = 0;
  return false;
}

void CGUIInfoManager::SetShowInfo(bool showinfo)
{
  m_playerShowInfo = showinfo;

  if (!showinfo)
    m_isPvrChannelPreview = false;
}

bool CGUIInfoManager::ToggleShowInfo()
{
  SetShowInfo(!m_playerShowInfo);
  return m_playerShowInfo;
}

void CGUIInfoManager::Clear()
{
  CSingleLock lock(m_critInfo);
  m_skinVariableStrings.clear();

  /*
    Erase any info bools that are unused. We do this repeatedly as each run
    will remove those bools that are no longer dependencies of other bools
    in the vector.
   */
  std::vector<InfoPtr>::iterator i = std::remove_if(m_bools.begin(), m_bools.end(), std::mem_fun_ref(&InfoPtr::unique));
  while (i != m_bools.end())
  {
    m_bools.erase(i, m_bools.end());
    i = std::remove_if(m_bools.begin(), m_bools.end(), std::mem_fun_ref(&InfoPtr::unique));
  }
  // log which ones are used - they should all be gone by now
  for (std::vector<InfoPtr>::const_iterator i = m_bools.begin(); i != m_bools.end(); ++i)
    CLog::Log(LOGDEBUG, "Infobool '%s' still used by %u instances", (*i)->GetExpression().c_str(), (unsigned int) i->use_count());
}

void CGUIInfoManager::UpdateFPS()
{
  m_frameCounter++;
  unsigned int curTime = CTimeUtils::GetFrameTime();

  float fTimeSpan = (float)(curTime - m_lastFPSTime);
  if (fTimeSpan >= 1000.0f)
  {
    fTimeSpan /= 1000.0f;
    m_fps = m_frameCounter / fTimeSpan;
    m_lastFPSTime = curTime;
    m_frameCounter = 0;
  }
}

void CGUIInfoManager::UpdateAVInfo()
{
  if(g_application.m_pPlayer->IsPlaying())
  {
    if (CServiceBroker::GetDataCacheCore().HasAVInfoChanges())
    {
      SPlayerVideoStreamInfo video;
      SPlayerAudioStreamInfo audio;

      g_application.m_pPlayer->GetVideoStreamInfo(CURRENT_STREAM, video);
      g_application.m_pPlayer->GetAudioStreamInfo(CURRENT_STREAM, audio);

      m_videoInfo = video;
      m_audioInfo = audio;

      m_isPvrChannelPreview = g_PVRManager.IsChannelPreview();
    }
  }
}

int CGUIInfoManager::AddListItemProp(const std::string &str, int offset)
{
  for (int i=0; i < (int)m_listitemProperties.size(); i++)
    if (m_listitemProperties[i] == str)
      return (LISTITEM_PROPERTY_START+offset + i);

  if (m_listitemProperties.size() < LISTITEM_PROPERTY_END - LISTITEM_PROPERTY_START)
  {
    m_listitemProperties.push_back(str);
    return LISTITEM_PROPERTY_START + offset + m_listitemProperties.size() - 1;
  }

  CLog::Log(LOGERROR,"%s - not enough listitem property space!", __FUNCTION__);
  return 0;
}

int CGUIInfoManager::AddMultiInfo(const GUIInfo &info)
{
  // check to see if we have this info already
  for (unsigned int i = 0; i < m_multiInfo.size(); i++)
    if (m_multiInfo[i] == info)
      return (int)i + MULTI_INFO_START;
  // return the new offset
  m_multiInfo.push_back(info);
  int id = (int)m_multiInfo.size() + MULTI_INFO_START - 1;
  if (id > MULTI_INFO_END)
    CLog::Log(LOGERROR, "%s - too many multiinfo bool/labels in this skin", __FUNCTION__);
  return id;
}

int CGUIInfoManager::ConditionalStringParameter(const std::string &parameter, bool caseSensitive /*= false*/)
{
  // check to see if we have this parameter already
  if (caseSensitive)
  {
    std::vector<std::string>::const_iterator i = std::find(m_stringParameters.begin(), m_stringParameters.end(), parameter);
    if (i != m_stringParameters.end())
      return (int)std::distance<std::vector<std::string>::const_iterator>(m_stringParameters.begin(), i);
  }
  else
  {
    for (unsigned int i = 0; i < m_stringParameters.size(); i++)
      if (StringUtils::EqualsNoCase(parameter, m_stringParameters[i]))
        return (int)i;
  }

  // return the new offset
  m_stringParameters.push_back(parameter);
  return (int)m_stringParameters.size() - 1;
}

bool CGUIInfoManager::GetItemInt(int &value, const CGUIListItem *item, int info) const
{
  if (!item)
  {
    value = 0;
    return false;
  }

  if (info >= LISTITEM_PROPERTY_START && info - LISTITEM_PROPERTY_START < (int)m_listitemProperties.size())
  { // grab the property
    std::string property = m_listitemProperties[info - LISTITEM_PROPERTY_START];
    std::string val = item->GetProperty(property).asString();
    value = atoi(val.c_str());
    return true;
  }

  switch (info)
  {
    case LISTITEM_PROGRESS:
    {
      value = 0;
      if (item->IsFileItem())
      {
        const CFileItem *pItem = (const CFileItem *)item;
        if (pItem && pItem->HasPVRChannelInfoTag())
        {
          CEpgInfoTagPtr epgNow(pItem->GetPVRChannelInfoTag()->GetEPGNow());
          if (epgNow)
            value = (int) epgNow->ProgressPercentage();
        }
        else if (pItem && pItem->HasEPGInfoTag())
        {
          value = (int) pItem->GetEPGInfoTag()->ProgressPercentage();
        }
      }

      return true;
    }
    break;
  case LISTITEM_PERCENT_PLAYED:
    if (item->IsFileItem() && ((const CFileItem *)item)->HasVideoInfoTag() && ((const CFileItem *)item)->GetVideoInfoTag()->m_resumePoint.IsPartWay())
      value = (int)(100 * ((const CFileItem *)item)->GetVideoInfoTag()->m_resumePoint.timeInSeconds / ((const CFileItem *)item)->GetVideoInfoTag()->m_resumePoint.totalTimeInSeconds);
    else if (item->IsFileItem() && ((const CFileItem *)item)->HasPVRRecordingInfoTag() && ((const CFileItem *)item)->GetPVRRecordingInfoTag()->m_resumePoint.IsPartWay())
      value = (int)(100 * ((const CFileItem *)item)->GetPVRRecordingInfoTag()->m_resumePoint.timeInSeconds / ((const CFileItem *)item)->GetPVRRecordingInfoTag()->m_resumePoint.totalTimeInSeconds);
    else
      value = 0;
    return true;
  }

  value = 0;
  return false;
}

std::string CGUIInfoManager::GetItemLabel(const CFileItem *item, int info, std::string *fallback)
{
  if (!item) return "";

  if (info >= CONDITIONAL_LABEL_START && info <= CONDITIONAL_LABEL_END)
    return GetSkinVariableString(info, false, item);

  if (info >= LISTITEM_PROPERTY_START + LISTITEM_ART_OFFSET && info - (LISTITEM_PROPERTY_START + LISTITEM_ART_OFFSET) < (int)m_listitemProperties.size())
  { // grab the art
    std::string art = m_listitemProperties[info - (LISTITEM_PROPERTY_START + LISTITEM_ART_OFFSET)];
    return item->GetArt(art);
  }

  if (info >= LISTITEM_PROPERTY_START + LISTITEM_RATING_OFFSET && info - (LISTITEM_PROPERTY_START + LISTITEM_RATING_OFFSET) < (int)m_listitemProperties.size())
  { // grab the rating
    std::string rating = m_listitemProperties[info - (LISTITEM_PROPERTY_START + LISTITEM_RATING_OFFSET)];
    return StringUtils::FormatNumber(item->GetVideoInfoTag()->GetRating(rating).rating);
  }

  if (info >= LISTITEM_PROPERTY_START + LISTITEM_VOTES_OFFSET && info - (LISTITEM_PROPERTY_START + LISTITEM_VOTES_OFFSET) < (int)m_listitemProperties.size())
  { // grab the votes
    std::string votes = m_listitemProperties[info - (LISTITEM_PROPERTY_START + LISTITEM_VOTES_OFFSET)];
    return StringUtils::FormatNumber(item->GetVideoInfoTag()->GetRating(votes).votes);
  }

  if (info >= LISTITEM_PROPERTY_START + LISTITEM_RATING_AND_VOTES_OFFSET && info - (LISTITEM_PROPERTY_START + LISTITEM_RATING_AND_VOTES_OFFSET) < (int)m_listitemProperties.size())
  { // grab the rating and the votes
    std::string ratingName = m_listitemProperties[info - (LISTITEM_PROPERTY_START + LISTITEM_RATING_AND_VOTES_OFFSET)];
    CRating rating = item->GetVideoInfoTag()->GetRating(ratingName);

    if (rating.rating <= 0.f)
      return "";
    
    if (rating.votes == 0)
      return StringUtils::FormatNumber(rating.rating);
    else
      return FormatRatingAndVotes(rating.rating, rating.votes);
  }

  if (info >= LISTITEM_PROPERTY_START && info - LISTITEM_PROPERTY_START < (int)m_listitemProperties.size())
  { 
    std::string property = m_listitemProperties[info - LISTITEM_PROPERTY_START];
    if (StringUtils::StartsWithNoCase(property, "Role.") && item->HasMusicInfoTag())
    { // "Role.xxxx" properties are held in music tag
      property.erase(0, 5); //Remove Role.
      return item->GetMusicInfoTag()->GetArtistStringForRole(property);
    }
    // grab the property
    return item->GetProperty(property).asString();
  }

  if (info >= LISTITEM_PICTURE_START && info <= LISTITEM_PICTURE_END && item->HasPictureInfoTag())
    return item->GetPictureInfoTag()->GetInfo(picture_slide_map[info - LISTITEM_PICTURE_START]);

  switch (info)
  {
  case LISTITEM_LABEL:
    return item->GetLabel();
  case LISTITEM_LABEL2:
    return item->GetLabel2();
  case LISTITEM_TITLE:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr epgTag(item->GetPVRChannelInfoTag()->GetEPGNow());
      return epgTag ?
          epgTag->Title() :
          CSettings::GetInstance().GetBool(CSettings::SETTING_EPG_HIDENOINFOAVAILABLE) ?
                            "" : g_localizeStrings.Get(19055); // no information available
    }
    if (item->HasPVRRecordingInfoTag())
      return item->GetPVRRecordingInfoTag()->m_strTitle;
    if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->Title();
    if (item->HasPVRTimerInfoTag())
      return item->GetPVRTimerInfoTag()->Title();
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strTitle;
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetTitle();
    break;
  case LISTITEM_EPG_EVENT_TITLE:
    if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->Title();
    if (item->HasPVRTimerInfoTag())
    {
      const CEpgInfoTagPtr epgTag(item->GetPVRTimerInfoTag()->GetEpgInfoTag());
      if (epgTag)
        return epgTag->Title();
    }
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr epgTag(item->GetPVRChannelInfoTag()->GetEPGNow());
      if (epgTag)
        return epgTag->Title();
    }
    break;
  case LISTITEM_ORIGINALTITLE:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
      if (tag)
        return tag->OriginalTitle();
    }
    if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->OriginalTitle();
    if (item->HasPVRTimerInfoTag())
    {
      const CEpgInfoTagPtr epgTag(item->GetPVRTimerInfoTag()->GetEpgInfoTag());
      if (epgTag)
        return epgTag->OriginalTitle();
    }
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strOriginalTitle;
    break;
  case LISTITEM_PLAYCOUNT:
    {
      std::string strPlayCount;
      if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_playCount > 0)
        strPlayCount = StringUtils::Format("%i", item->GetVideoInfoTag()->m_playCount);
      if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetPlayCount() > 0)
        strPlayCount = StringUtils::Format("%i", item->GetMusicInfoTag()->GetPlayCount());
      return strPlayCount;
    }
  case LISTITEM_LASTPLAYED:
    {
      CDateTime dateTime;
      if (item->HasVideoInfoTag())
        dateTime = item->GetVideoInfoTag()->m_lastPlayed;
      else if (item->HasMusicInfoTag())
        dateTime = item->GetMusicInfoTag()->GetLastPlayed();

      if (dateTime.IsValid())
        return dateTime.GetAsLocalizedDate();
      break;
    }
  case LISTITEM_TRACKNUMBER:
    {
      std::string track;
      if (item->HasMusicInfoTag())
        track = StringUtils::Format("%i", item->GetMusicInfoTag()->GetTrackNumber());
      if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_iTrack > -1 )
        track = StringUtils::Format("%i", item->GetVideoInfoTag()->m_iTrack);
      return track;
    }
  case LISTITEM_DISC_NUMBER:
    {
      std::string disc;
      if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetDiscNumber() > 0)
        disc = StringUtils::Format("%i", item->GetMusicInfoTag()->GetDiscNumber());
      return disc;
    }
  case LISTITEM_ARTIST:
    if (item->HasVideoInfoTag())
      return StringUtils::Join(item->GetVideoInfoTag()->m_artist, g_advancedSettings.m_videoItemSeparator);
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetArtistString();
    break;
  case LISTITEM_ALBUM_ARTIST:
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetAlbumArtistString();
    break;
  case LISTITEM_CONTRIBUTORS:
    if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->HasContributors())
      return item->GetMusicInfoTag()->GetContributorsText();
    break;
  case LISTITEM_CONTRIBUTOR_AND_ROLE:
    if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->HasContributors())
      return item->GetMusicInfoTag()->GetContributorsAndRolesText();
    break;
  case LISTITEM_DIRECTOR:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
      if (tag)
        return tag->Director();
    }
    if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->Director();
    if (item->HasPVRTimerInfoTag())
    {
      const CEpgInfoTagPtr epgTag(item->GetPVRTimerInfoTag()->GetEpgInfoTag());
      if (epgTag)
        return epgTag->Director();
    }
    if (item->HasVideoInfoTag())
      return StringUtils::Join(item->GetVideoInfoTag()->m_director, g_advancedSettings.m_videoItemSeparator);
    break;
  case LISTITEM_ALBUM:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strAlbum;
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetAlbum();
    break;
  case LISTITEM_YEAR:
    {
      std::string year;
      if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->HasYear())
        year = StringUtils::Format("%i", item->GetVideoInfoTag()->GetYear());
      if (item->HasMusicInfoTag())
        year = item->GetMusicInfoTag()->GetYearString();
      if (item->HasEPGInfoTag() && item->GetEPGInfoTag()->Year() > 0)
        year = StringUtils::Format("%i", item->GetEPGInfoTag()->Year());
      if (item->HasPVRTimerInfoTag())
      {
        const CEpgInfoTagPtr tag(item->GetPVRTimerInfoTag()->GetEpgInfoTag());
        if (tag && tag->Year() > 0)
          year = StringUtils::Format("%i", tag->Year());
      }
      if (item->HasPVRRecordingInfoTag() && item->GetPVRRecordingInfoTag()->HasYear())
          year = StringUtils::Format("%i", item->GetPVRRecordingInfoTag()->GetYear());
      return year;
    }
  case LISTITEM_PREMIERED:
    if (item->HasVideoInfoTag())
    {
      CDateTime dateTime;
      if (item->GetVideoInfoTag()->m_firstAired.IsValid())
        dateTime = item->GetVideoInfoTag()->m_firstAired;
      else if (item->GetVideoInfoTag()->HasPremiered())
        dateTime = item->GetVideoInfoTag()->GetPremiered();

      if (dateTime.IsValid())
        return dateTime.GetAsLocalizedDate();
    }
    else if (item->HasEPGInfoTag())
    {
      if (item->GetEPGInfoTag()->FirstAiredAsLocalTime().IsValid())
        return item->GetEPGInfoTag()->FirstAiredAsLocalTime().GetAsLocalizedDate(true);
    }
    else if (item->HasPVRTimerInfoTag())
    {
      const CEpgInfoTagPtr tag(item->GetPVRTimerInfoTag()->GetEpgInfoTag());
      if (tag && tag->FirstAiredAsLocalTime().IsValid())
        return tag->FirstAiredAsLocalTime().GetAsLocalizedDate(true);
    }
    break;
  case LISTITEM_GENRE:
    if (item->HasPVRRecordingInfoTag())
      return StringUtils::Join(item->GetPVRRecordingInfoTag()->m_genre, g_advancedSettings.m_videoItemSeparator);
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr epgTag(item->GetPVRChannelInfoTag()->GetEPGNow());
      return epgTag ? StringUtils::Join(epgTag->Genre(), g_advancedSettings.m_videoItemSeparator) : "";
    }
    if (item->HasEPGInfoTag())
      return StringUtils::Join(item->GetEPGInfoTag()->Genre(), g_advancedSettings.m_videoItemSeparator);
    if (item->HasPVRTimerInfoTag())
    {
      const CEpgInfoTagPtr epgTag(item->GetPVRTimerInfoTag()->GetEpgInfoTag());
      if (epgTag)
        return StringUtils::Join(epgTag->Genre(), g_advancedSettings.m_videoItemSeparator);
    }
    if (item->HasVideoInfoTag())
      return StringUtils::Join(item->GetVideoInfoTag()->m_genre, g_advancedSettings.m_videoItemSeparator);
    if (item->HasMusicInfoTag())
      return StringUtils::Join(item->GetMusicInfoTag()->GetGenre(), g_advancedSettings.m_musicItemSeparator);
    break;
  case LISTITEM_FILENAME:
  case LISTITEM_FILE_EXTENSION:
    {
      std::string strFile;
      if (item->IsMusicDb() && item->HasMusicInfoTag())
        strFile = URIUtils::GetFileName(item->GetMusicInfoTag()->GetURL());
      else if (item->IsVideoDb() && item->HasVideoInfoTag())
        strFile = URIUtils::GetFileName(item->GetVideoInfoTag()->m_strFileNameAndPath);
      else
        strFile = URIUtils::GetFileName(item->GetPath());

      if (info==LISTITEM_FILE_EXTENSION)
      {
        std::string strExtension = URIUtils::GetExtension(strFile);
        return StringUtils::TrimLeft(strExtension, ".");
      }
      return strFile;
    }
    break;
  case LISTITEM_DATE:
    if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->StartAsLocalTime().GetAsLocalizedDateTime(false, false);
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr epgTag(item->GetPVRChannelInfoTag()->GetEPGNow());
      return epgTag ? epgTag->StartAsLocalTime().GetAsLocalizedDateTime(false, false) : CDateTime::GetCurrentDateTime().GetAsLocalizedDateTime(false, false);
    }
    if (item->HasPVRRecordingInfoTag())
      return item->GetPVRRecordingInfoTag()->RecordingTimeAsLocalTime().GetAsLocalizedDateTime(false, false);
    if (item->HasPVRTimerInfoTag())
      return item->GetPVRTimerInfoTag()->Summary();
    if (item->m_dateTime.IsValid())
      return item->m_dateTime.GetAsLocalizedDate();
    break;
  case LISTITEM_DATETIME:
    if (item->m_dateTime.IsValid())
      return item->m_dateTime.GetAsLocalizedDateTime();
    break;
  case LISTITEM_SIZE:
    if (!item->m_bIsFolder || item->m_dwSize)
      return StringUtils::SizeToString(item->m_dwSize);
    break;
  case LISTITEM_RATING:
    {
      std::string rating;
      float r = 0.f;
      if (item->HasVideoInfoTag()) // movie rating
        r = item->GetVideoInfoTag()->GetRating().rating;
      if (r > 0.f)
        rating = StringUtils::FormatNumber(r);
      else if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetRating() > 0.f) // song rating
        rating = StringUtils::FormatNumber(item->GetMusicInfoTag()->GetRating());
      return rating;
    }
  case LISTITEM_RATING_AND_VOTES:
  {
      CRating r(0.f, 0);
      if (item->HasVideoInfoTag()) // video rating & votes
        r = item->GetVideoInfoTag()->GetRating();
      if (r.rating > 0.f)
      {
        std::string strRatingAndVotes;
        if (r.votes == 0)
          strRatingAndVotes = StringUtils::FormatNumber(r.rating);
        else
          strRatingAndVotes = FormatRatingAndVotes(r.rating, r.votes);
        return strRatingAndVotes;
      }
      else if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetRating() > 0.f) // music rating & votes
      {
        std::string strRatingAndVotes;
        if (item->GetMusicInfoTag()->GetVotes() <= 0)
          strRatingAndVotes = StringUtils::FormatNumber(item->GetMusicInfoTag()->GetRating());
        else
          strRatingAndVotes = FormatRatingAndVotes(item->GetMusicInfoTag()->GetRating(), 
                                                   item->GetMusicInfoTag()->GetVotes());
        return strRatingAndVotes;
      }
    }
    break;
  case LISTITEM_USER_RATING:
    {
      std::string strUserRating;
      if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_iUserRating > 0)
        strUserRating = StringUtils::Format("%i", item->GetVideoInfoTag()->m_iUserRating);
      else if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetUserrating() > 0)
        strUserRating = StringUtils::Format("%i", item->GetMusicInfoTag()->GetUserrating());
      return strUserRating;
    }
    break;
  case LISTITEM_VOTES:
    if (item->HasVideoInfoTag())
      return StringUtils::FormatNumber(item->GetVideoInfoTag()->GetRating().votes);
    else if (item->HasMusicInfoTag())
      return StringUtils::FormatNumber(item->GetMusicInfoTag()->GetVotes());
    break;
  case LISTITEM_PROGRAM_COUNT:
    {
      return StringUtils::Format("%i", item->m_iprogramCount);
    }
  case LISTITEM_DURATION:
    {
      std::string duration;
      if (item->HasPVRChannelInfoTag())
      {
        CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
        return tag ? StringUtils::SecondsToTimeString(tag->GetDuration()) : "";
      }
      else if (item->HasPVRRecordingInfoTag())
      {
        if (item->GetPVRRecordingInfoTag()->GetDuration() > 0)
          duration = StringUtils::SecondsToTimeString(item->GetPVRRecordingInfoTag()->GetDuration());
      }
      else if (item->HasEPGInfoTag())
      {
        if (item->GetEPGInfoTag()->GetDuration() > 0)
          duration = StringUtils::SecondsToTimeString(item->GetEPGInfoTag()->GetDuration());
      }
      else if (item->HasPVRTimerInfoTag())
      {
        const CEpgInfoTagPtr tag(item->GetPVRTimerInfoTag()->GetEpgInfoTag());
        if (tag && tag->GetDuration() > 0)
          duration = StringUtils::SecondsToTimeString(tag->GetDuration());
      }
      else if (item->HasVideoInfoTag())
      {
        if (item->GetVideoInfoTag()->GetDuration() > 0)
          duration = StringUtils::Format("%d", item->GetVideoInfoTag()->GetDuration() / 60);
      }
      else if (item->HasMusicInfoTag())
      {
        if (item->GetMusicInfoTag()->GetDuration() > 0)
          duration = StringUtils::SecondsToTimeString(item->GetMusicInfoTag()->GetDuration());
      }
      return duration;
    }
  case LISTITEM_PLOT:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
      return tag ? tag->Plot() : "";
    }
    if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->Plot();
    if (item->HasPVRRecordingInfoTag())
      return item->GetPVRRecordingInfoTag()->m_strPlot;
    if (item->HasPVRTimerInfoTag())
    {
      const CEpgInfoTagPtr epgTag(item->GetPVRTimerInfoTag()->GetEpgInfoTag());
      if (epgTag)
        return epgTag->Plot();
    }
    if (item->HasVideoInfoTag())
    {
      if (item->GetVideoInfoTag()->m_type != MediaTypeTvShow && item->GetVideoInfoTag()->m_type != MediaTypeVideoCollection)
        if (item->GetVideoInfoTag()->m_playCount == 0 && !CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOLIBRARY_SHOWUNWATCHEDPLOTS))
          return g_localizeStrings.Get(20370);

      return item->GetVideoInfoTag()->m_strPlot;
    }
    break;
  case LISTITEM_PLOT_OUTLINE:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
      return tag ? tag->PlotOutline() : "";
    }
    if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->PlotOutline();
    if (item->HasPVRRecordingInfoTag())
      return item->GetPVRRecordingInfoTag()->m_strPlotOutline;
    if (item->HasPVRTimerInfoTag())
    {
      const CEpgInfoTagPtr epgTag(item->GetPVRTimerInfoTag()->GetEpgInfoTag());
      if (epgTag)
        return epgTag->PlotOutline();
    }
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strPlotOutline;
    break;
  case LISTITEM_EPISODE:
    {
      int iSeason = -1, iEpisode = -1;
      if (item->HasPVRChannelInfoTag())
      {
        CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
        if (tag)
        {
          if (tag->SeriesNumber() > 0)
            iSeason = tag->SeriesNumber();
          if (tag->EpisodeNumber() > 0)
            iEpisode = tag->EpisodeNumber();
        }
      }
      else if (item->HasEPGInfoTag())
      {
        if (item->GetEPGInfoTag()->SeriesNumber() > 0)
          iSeason = item->GetEPGInfoTag()->SeriesNumber();
        if (item->GetEPGInfoTag()->EpisodeNumber() > 0)
          iEpisode = item->GetEPGInfoTag()->EpisodeNumber();
      }
      else if (item->HasPVRTimerInfoTag())
      {
        const CEpgInfoTagPtr tag(item->GetPVRTimerInfoTag()->GetEpgInfoTag());
        if (tag)
        {
          if (tag->SeriesNumber() > 0)
            iSeason = tag->SeriesNumber();
          if (tag->EpisodeNumber() > 0)
            iEpisode = tag->EpisodeNumber();
        }
      }
      else if (item->HasPVRRecordingInfoTag() && item->GetPVRRecordingInfoTag()->m_iEpisode > 0)
      {
        iSeason = item->GetPVRRecordingInfoTag()->m_iSeason;
        iEpisode = item->GetPVRRecordingInfoTag()->m_iEpisode;
      }
      else if (item->HasVideoInfoTag())
      {
        iSeason = item->GetVideoInfoTag()->m_iSeason;
        iEpisode = item->GetVideoInfoTag()->m_iEpisode;
      }

      if (iEpisode >= 0)
      {
        if (iSeason == 0) // prefix episode with 'S'
          return StringUtils::Format("S%d", iEpisode);
        else
          return StringUtils::Format("%d", iEpisode);
      }
    }
    break;
  case LISTITEM_SEASON:
    {
      int iSeason = -1;
      if (item->HasPVRChannelInfoTag())
      {
        CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
        if (tag && tag->SeriesNumber() > 0)
          iSeason = tag->SeriesNumber();
      }
      else if (item->HasEPGInfoTag() &&
               item->GetEPGInfoTag()->SeriesNumber() > 0)
        iSeason = item->GetEPGInfoTag()->SeriesNumber();
      else if (item->HasPVRTimerInfoTag())
      {
        const CEpgInfoTagPtr epgTag(item->GetPVRTimerInfoTag()->GetEpgInfoTag());
        if (epgTag && epgTag->SeriesNumber() > 0)
          iSeason = epgTag->SeriesNumber();
      }
      else if (item->HasPVRRecordingInfoTag() &&
               item->GetPVRRecordingInfoTag()->m_iSeason > 0)
        iSeason = item->GetPVRRecordingInfoTag()->m_iSeason;
      else if (item->HasVideoInfoTag())
        iSeason = item->GetVideoInfoTag()->m_iSeason;

      if (iSeason >= 0)
        return StringUtils::Format("%d", iSeason);
    }
    break;
  case LISTITEM_TVSHOW:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strShowTitle;
    break;
  case LISTITEM_COMMENT:
    if (item->HasPVRTimerInfoTag())
      return item->GetPVRTimerInfoTag()->GetStatus();
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetComment();
    break;
  case LISTITEM_MOOD:
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetMood();
    break;
  case LISTITEM_ACTUAL_ICON:
    return item->GetIconImage();
  case LISTITEM_ICON:
    {
      std::string strThumb = item->GetArt("thumb");
      if (strThumb.empty())
        strThumb = item->GetIconImage();
      if (fallback)
        *fallback = item->GetIconImage();
      return strThumb;
    }
  case LISTITEM_OVERLAY:
    return item->GetOverlayImage();
  case LISTITEM_THUMB:
    return item->GetArt("thumb");
  case LISTITEM_FOLDERPATH:
    return CURL(item->GetPath()).GetWithoutUserDetails();
  case LISTITEM_FOLDERNAME:
  case LISTITEM_PATH:
    {
      std::string path;
      if (item->IsMusicDb() && item->HasMusicInfoTag())
        path = URIUtils::GetDirectory(item->GetMusicInfoTag()->GetURL());
      else if (item->IsVideoDb() && item->HasVideoInfoTag())
      {
        if( item->m_bIsFolder )
          path = item->GetVideoInfoTag()->m_strPath;
        else
          URIUtils::GetParentPath(item->GetVideoInfoTag()->m_strFileNameAndPath, path);
      }
      else
        URIUtils::GetParentPath(item->GetPath(), path);
      path = CURL(path).GetWithoutUserDetails();
      if (info==LISTITEM_FOLDERNAME)
      {
        URIUtils::RemoveSlashAtEnd(path);
        path=URIUtils::GetFileName(path);
      }
      return path;
    }
  case LISTITEM_FILENAME_AND_PATH:
    {
      std::string path;
      if (item->IsMusicDb() && item->HasMusicInfoTag())
        path = item->GetMusicInfoTag()->GetURL();
      else if (item->IsVideoDb() && item->HasVideoInfoTag())
        path = item->GetVideoInfoTag()->m_strFileNameAndPath;
      else
        path = item->GetPath();
      path = CURL(path).GetWithoutUserDetails();
      return path;
    }
  case LISTITEM_PICTURE_PATH:
    if (item->IsPicture() && (!item->IsZIP() || item->IsRAR() || item->IsCBZ() || item->IsCBR()))
      return item->GetPath();
    break;
  case LISTITEM_STUDIO:
    if (item->HasVideoInfoTag())
      return StringUtils::Join(item->GetVideoInfoTag()->m_studio, g_advancedSettings.m_videoItemSeparator);
    break;
  case LISTITEM_COUNTRY:
    if (item->HasVideoInfoTag())
      return StringUtils::Join(item->GetVideoInfoTag()->m_country, g_advancedSettings.m_videoItemSeparator);
    break;
  case LISTITEM_MPAA:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strMPAARating;
    break;
  case LISTITEM_CAST:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->GetCast();
    if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->Cast();
    break;
  case LISTITEM_CAST_AND_ROLE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->GetCast(true);
    break;
  case LISTITEM_WRITER:
    if (item->HasVideoInfoTag())
      return StringUtils::Join(item->GetVideoInfoTag()->m_writingCredits, g_advancedSettings.m_videoItemSeparator);
    if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->Writer();
    break;
  case LISTITEM_TAGLINE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strTagLine;
    break;
  case LISTITEM_STATUS:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strStatus;
    break;
  case LISTITEM_TRAILER:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strTrailer;
    break;
  case LISTITEM_TOP250:
    if (item->HasVideoInfoTag())
    {
      std::string strResult;
      if (item->GetVideoInfoTag()->m_iTop250 > 0)
        strResult = StringUtils::Format("%i",item->GetVideoInfoTag()->m_iTop250);
      return strResult;
    }
    break;
  case LISTITEM_SORT_LETTER:
    {
      std::string letter;
      std::wstring character(1, item->GetSortLabel()[0]);
      StringUtils::ToUpper(character);
      g_charsetConverter.wToUTF8(character, letter);
      return letter;
    }
    break;
  case LISTITEM_TAG:
    if (item->HasVideoInfoTag())
      return StringUtils::Join(item->GetVideoInfoTag()->m_tags, g_advancedSettings.m_videoItemSeparator);
    break;
  case LISTITEM_SET:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strSet;
    break;
  case LISTITEM_SETID:
    if (item->HasVideoInfoTag())
    {
      int iSetId = item->GetVideoInfoTag()->m_iSetId;
      if (iSetId > 0)
        return StringUtils::Format("%d", iSetId);
    }
    break;
  case LISTITEM_VIDEO_CODEC:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_streamDetails.GetVideoCodec();
    break;
  case LISTITEM_VIDEO_RESOLUTION:
    if (item->HasVideoInfoTag())
      return CStreamDetails::VideoDimsToResolutionDescription(item->GetVideoInfoTag()->m_streamDetails.GetVideoWidth(), item->GetVideoInfoTag()->m_streamDetails.GetVideoHeight());
    break;
  case LISTITEM_VIDEO_ASPECT:
    if (item->HasVideoInfoTag())
      return CStreamDetails::VideoAspectToAspectDescription(item->GetVideoInfoTag()->m_streamDetails.GetVideoAspect());
    break;
  case LISTITEM_AUDIO_CODEC:
    if (item->HasVideoInfoTag())
    {
      return item->GetVideoInfoTag()->m_streamDetails.GetAudioCodec();
    }
    break;
  case LISTITEM_AUDIO_CHANNELS:
    if (item->HasVideoInfoTag())
    {
      std::string strResult;
      int iChannels = item->GetVideoInfoTag()->m_streamDetails.GetAudioChannels();
      if (iChannels > 0)
        strResult = StringUtils::Format("%i", iChannels);
      return strResult;
    }
    break;
  case LISTITEM_AUDIO_LANGUAGE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_streamDetails.GetAudioLanguage();
    break;
  case LISTITEM_SUBTITLE_LANGUAGE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_streamDetails.GetSubtitleLanguage();
    break;
  case LISTITEM_STARTTIME:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
      if (tag)
        return tag->StartAsLocalTime().GetAsLocalizedTime("", false);
    }
    if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->StartAsLocalTime().GetAsLocalizedTime("", false);
    if (item->HasPVRTimerInfoTag())
      return item->GetPVRTimerInfoTag()->StartAsLocalTime().GetAsLocalizedTime("", false);
    if (item->HasPVRRecordingInfoTag())
      return item->GetPVRRecordingInfoTag()->RecordingTimeAsLocalTime().GetAsLocalizedTime("", false);
    if (item->m_dateTime.IsValid())
      return item->m_dateTime.GetAsLocalizedTime("", false);
    break;
  case LISTITEM_ENDTIME_RESUME:
    if (item->HasVideoInfoTag())
    {
      auto* tag = item->GetVideoInfoTag();
      CDateTimeSpan duration(0, 0, 0, tag->GetDuration() - tag->m_resumePoint.timeInSeconds);
      return (CDateTime::GetCurrentDateTime() + duration).GetAsLocalizedTime("", false);
    }
    break;
  case LISTITEM_ENDTIME:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
      if (tag)
        return tag->EndAsLocalTime().GetAsLocalizedTime("", false);
    }
    else if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->EndAsLocalTime().GetAsLocalizedTime("", false);
    else if (item->HasPVRTimerInfoTag())
      return item->GetPVRTimerInfoTag()->EndAsLocalTime().GetAsLocalizedTime("", false);
    else if (item->HasPVRRecordingInfoTag())
      return (item->GetPVRRecordingInfoTag()->RecordingTimeAsLocalTime() + CDateTimeSpan(0, 0, 0, item->GetPVRRecordingInfoTag()->GetDuration())).GetAsLocalizedTime("", false);
    else if (item->HasVideoInfoTag())
    {
      CDateTimeSpan duration(0, 0, 0, item->GetVideoInfoTag()->GetDuration());
      return (CDateTime::GetCurrentDateTime() + duration).GetAsLocalizedTime("", false);
    }
    break;
  case LISTITEM_STARTDATE:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
      if (tag)
        return tag->StartAsLocalTime().GetAsLocalizedDate(true);
    }
    if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->StartAsLocalTime().GetAsLocalizedDate(true);
    if (item->HasPVRTimerInfoTag())
      return item->GetPVRTimerInfoTag()->StartAsLocalTime().GetAsLocalizedDate(true);
    if (item->HasPVRRecordingInfoTag())
      return item->GetPVRRecordingInfoTag()->RecordingTimeAsLocalTime().GetAsLocalizedDate(true);
    if (item->m_dateTime.IsValid())
      return item->m_dateTime.GetAsLocalizedDate(true);
    break;
  case LISTITEM_ENDDATE:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
      if (tag)
        return tag->EndAsLocalTime().GetAsLocalizedDate(true);
    }
    if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->EndAsLocalTime().GetAsLocalizedDate(true);
    if (item->HasPVRTimerInfoTag())
      return item->GetPVRTimerInfoTag()->EndAsLocalTime().GetAsLocalizedDate(true);
    break;
  case LISTITEM_CHANNEL_NUMBER:
    {
      std::string number;
      if (item->HasPVRChannelInfoTag())
        number = StringUtils::Format("%i", item->GetPVRChannelInfoTag()->ChannelNumber());
      if (item->HasEPGInfoTag() && item->GetEPGInfoTag()->HasPVRChannel())
        number = StringUtils::Format("%i", item->GetEPGInfoTag()->PVRChannelNumber());
      if (item->HasPVRTimerInfoTag())
        number = StringUtils::Format("%i", item->GetPVRTimerInfoTag()->ChannelNumber());

      return number;
    }
    break;
  case LISTITEM_SUB_CHANNEL_NUMBER:
    {
      std::string number;
      if (item->HasPVRChannelInfoTag())
        number = StringUtils::Format("%i", item->GetPVRChannelInfoTag()->SubChannelNumber());
      if (item->HasEPGInfoTag() && item->GetEPGInfoTag()->HasPVRChannel())
        number = StringUtils::Format("%i", item->GetEPGInfoTag()->ChannelTag()->SubChannelNumber());
      if (item->HasPVRTimerInfoTag())
        number = StringUtils::Format("%i", item->GetPVRTimerInfoTag()->ChannelTag()->SubChannelNumber());

      return number;
    }
    break;
  case LISTITEM_CHANNEL_NUMBER_LBL:
    {
      CPVRChannelPtr channel;
      if (item->HasPVRChannelInfoTag())
        channel = item->GetPVRChannelInfoTag();
      else if (item->HasEPGInfoTag() && item->GetEPGInfoTag()->HasPVRChannel())
        channel = item->GetEPGInfoTag()->ChannelTag();
      else if (item->HasPVRTimerInfoTag())
        channel = item->GetPVRTimerInfoTag()->ChannelTag();

      return channel ?
          channel->FormattedChannelNumber() :
          "";
    }
    break;
  case LISTITEM_CHANNEL_NAME:
    if (item->HasPVRChannelInfoTag())
      return item->GetPVRChannelInfoTag()->ChannelName();
    if (item->HasEPGInfoTag() && item->GetEPGInfoTag()->HasPVRChannel())
      return item->GetEPGInfoTag()->PVRChannelName();
    if (item->HasPVRRecordingInfoTag())
      return item->GetPVRRecordingInfoTag()->m_strChannelName;
    if (item->HasPVRTimerInfoTag())
      return item->GetPVRTimerInfoTag()->ChannelName();
    break;
  case LISTITEM_NEXT_STARTTIME:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNext());
      if (tag)
        return tag->StartAsLocalTime().GetAsLocalizedTime("", false);
    }
    return CDateTime::GetCurrentDateTime().GetAsLocalizedTime("", false);
  case LISTITEM_NEXT_ENDTIME:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNext());
      if (tag)
        return tag->EndAsLocalTime().GetAsLocalizedTime("", false);
    }
    return CDateTime::GetCurrentDateTime().GetAsLocalizedTime("", false);
  case LISTITEM_NEXT_STARTDATE:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNext());
      if (tag)
        return tag->StartAsLocalTime().GetAsLocalizedDate(true);
    }
    return CDateTime::GetCurrentDateTime().GetAsLocalizedDate(true);
  case LISTITEM_NEXT_ENDDATE:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNext());
      if (tag)
        return tag->EndAsLocalTime().GetAsLocalizedDate(true);
    }
    return CDateTime::GetCurrentDateTime().GetAsLocalizedDate(true);
  case LISTITEM_NEXT_PLOT:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNext());
      if (tag)
        return tag->Plot();
    }
    return "";
  case LISTITEM_NEXT_PLOT_OUTLINE:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNext());
      if (tag)
        return tag->PlotOutline();
    }
    return "";
  case LISTITEM_NEXT_DURATION:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNext());
      if (tag)
        return StringUtils::SecondsToTimeString(tag->GetDuration());
    }
    return "";
  case LISTITEM_NEXT_GENRE:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNext());
      if (tag)
        return StringUtils::Join(tag->Genre(), g_advancedSettings.m_videoItemSeparator);
    }
    return "";
  case LISTITEM_NEXT_TITLE:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNext());
      if (tag)
        return tag->Title();
    }
    return "";
  case LISTITEM_PARENTALRATING:
    {
      std::string rating;
      if (item->HasEPGInfoTag() && item->GetEPGInfoTag()->ParentalRating() > 0)
        rating = StringUtils::Format("%i", item->GetEPGInfoTag()->ParentalRating());
      return rating;
    }
    break;
  case LISTITEM_PERCENT_PLAYED:
    {
      int val;
      if (GetItemInt(val, item, info))
      {
        return StringUtils::Format("%d", val);
      }
      break;
    }
  case LISTITEM_DATE_ADDED:
    if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_dateAdded.IsValid())
      return item->GetVideoInfoTag()->m_dateAdded.GetAsLocalizedDate();
    break;
  case LISTITEM_DBTYPE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_type;
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetType();
    break;
  case LISTITEM_DBID:
    if (item->HasVideoInfoTag())
      {
        int dbId = item->GetVideoInfoTag()->m_iDbId;
        if (dbId > -1)
          return StringUtils::Format("%i", dbId);
      }
    if (item->HasMusicInfoTag())
      {
        int dbId = item->GetMusicInfoTag()->GetDatabaseId();
        if (dbId > -1)
          return StringUtils::Format("%i", dbId);
      }
    break;
  case LISTITEM_STEREOSCOPIC_MODE:
    {
      std::string stereoMode = item->GetProperty("stereomode").asString();
      if (stereoMode.empty() && item->HasVideoInfoTag())
        stereoMode = CStereoscopicsManager::GetInstance().NormalizeStereoMode(item->GetVideoInfoTag()->m_streamDetails.GetStereoMode());
      return stereoMode;
    }
  case LISTITEM_IMDBNUMBER:
    {
      if (item->HasPVRChannelInfoTag())
      {
        CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
        if (tag)
          return tag->IMDBNumber();
      }
      if (item->HasEPGInfoTag())
        return item->GetEPGInfoTag()->IMDBNumber();
      if (item->HasVideoInfoTag())
        return item->GetVideoInfoTag()->GetUniqueID();
      break;
    }
  case LISTITEM_EPISODENAME:
    {
      if (item->HasPVRChannelInfoTag())
      {
        CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
        if (tag)
          return tag->EpisodeName();
      }
      if (item->HasEPGInfoTag())
        return item->GetEPGInfoTag()->EpisodeName();
      if (item->HasPVRTimerInfoTag())
      {
        const CEpgInfoTagPtr epgTag(item->GetPVRTimerInfoTag()->GetEpgInfoTag());
        if (epgTag)
          return epgTag->EpisodeName();
      }
      if (item->HasPVRRecordingInfoTag())
        return item->GetPVRRecordingInfoTag()->EpisodeName();
      break;
    }
  case LISTITEM_TIMERTYPE:
    {
      if (item->HasPVRTimerInfoTag())
        return item->GetPVRTimerInfoTag()->GetTypeAsString();
    }
    break;
  case LISTITEM_ADDON_NAME:
    if (item->HasAddonInfo())
      return item->GetAddonInfo()->Name();
    break;
  case LISTITEM_ADDON_VERSION:
    if (item->HasAddonInfo())
      return item->GetAddonInfo()->Version().asString();
    break;
  case LISTITEM_ADDON_CREATOR:
    if (item->HasAddonInfo())
      return item->GetAddonInfo()->Author();
    break;
  case LISTITEM_ADDON_SUMMARY:
    if (item->HasAddonInfo())
      return item->GetAddonInfo()->Summary();
    break;
  case LISTITEM_ADDON_DESCRIPTION:
    if (item->HasAddonInfo())
      return item->GetAddonInfo()->Description();
    break;
  case LISTITEM_ADDON_DISCLAIMER:
    if (item->HasAddonInfo())
      return item->GetAddonInfo()->Disclaimer();
    break;
  case LISTITEM_ADDON_NEWS:
    if (item->HasAddonInfo())
      return item->GetAddonInfo()->ChangeLog();
    break;
  case LISTITEM_ADDON_BROKEN:
    if (item->HasAddonInfo())
    {
      if (item->GetAddonInfo()->Broken() == "DEPSNOTMET")
        return g_localizeStrings.Get(24044);
      return item->GetAddonInfo()->Broken();
    }
    break;
  case LISTITEM_ADDON_TYPE:
    if (item->HasAddonInfo())
      return ADDON::TranslateType(item->GetAddonInfo()->Type(),true);
    break;
  case LISTITEM_ADDON_INSTALL_DATE:
    if (item->HasAddonInfo())
      return item->GetAddonInfo()->InstallDate().GetAsLocalizedDateTime();
    break;
  case LISTITEM_ADDON_LAST_UPDATED:
    if (item->HasAddonInfo() && item->GetAddonInfo()->LastUpdated().IsValid())
      return item->GetAddonInfo()->LastUpdated().GetAsLocalizedDateTime();
    break;
  case LISTITEM_ADDON_LAST_USED:
    if (item->HasAddonInfo() && item->GetAddonInfo()->LastUsed().IsValid())
      return item->GetAddonInfo()->LastUsed().GetAsLocalizedDateTime();
    break;
  case LISTITEM_ADDON_ORIGIN:
    if (item->HasAddonInfo())
    {
      if (item->GetAddonInfo()->Origin() == ORIGIN_SYSTEM)
        return g_localizeStrings.Get(24992);
      AddonPtr origin;
      if (CAddonMgr::GetInstance().GetAddon(item->GetAddonInfo()->Origin(), origin, ADDON_UNKNOWN, false))
        return origin->Name();
      return g_localizeStrings.Get(13205);
    }
    break;
  case LISTITEM_ADDON_SIZE:
    if (item->HasAddonInfo() && item->GetAddonInfo()->PackageSize() > 0)
      return StringUtils::FormatFileSize(item->GetAddonInfo()->PackageSize());
    break;
  }

  return "";
}

std::string CGUIInfoManager::GetItemImage(const CFileItem *item, int info, std::string *fallback)
{
  if (info >= CONDITIONAL_LABEL_START && info <= CONDITIONAL_LABEL_END)
    return GetSkinVariableString(info, true, item);

  return GetItemLabel(item, info, fallback);
}

bool CGUIInfoManager::GetItemBool(const CGUIListItem *item, int condition) const
{
  if (!item) return false;
  if (condition >= LISTITEM_PROPERTY_START && condition - LISTITEM_PROPERTY_START < (int)m_listitemProperties.size())
  { // grab the property
    std::string property = m_listitemProperties[condition - LISTITEM_PROPERTY_START];
    return item->GetProperty(property).asBoolean();
  }
  else if (condition == LISTITEM_ISPLAYING)
  {
    if (item->HasProperty("playlistposition"))
      return (int)item->GetProperty("playlisttype").asInteger() == g_playlistPlayer.GetCurrentPlaylist() && (int)item->GetProperty("playlistposition").asInteger() == g_playlistPlayer.GetCurrentSong();
    else if (item->IsFileItem() && !m_currentFile->GetPath().empty())
    {
      if (!g_application.m_strPlayListFile.empty())
      {
        //playlist file that is currently playing or the playlistitem that is currently playing.
        return ((const CFileItem *)item)->IsPath(g_application.m_strPlayListFile) || m_currentFile->IsSamePath((const CFileItem *)item);
      }
      return m_currentFile->IsSamePath((const CFileItem *)item);
    }
  }
  else if (condition == LISTITEM_ISSELECTED)
    return item->IsSelected();
  else if (condition == LISTITEM_IS_FOLDER)
    return item->m_bIsFolder;
  else if (condition == LISTITEM_IS_PARENTFOLDER)
  {
    if (item->IsFileItem())
    {
      const CFileItem *pItem = (const CFileItem *)item;
      return pItem->IsParentFolder();
    }
  }
  else if (condition == LISTITEM_IS_RESUMABLE)
  {
    if (item->IsFileItem())
    {
      if (((const CFileItem *)item)->HasVideoInfoTag())
        return ((const CFileItem *)item)->GetVideoInfoTag()->m_resumePoint.timeInSeconds > 0;
      else if (((const CFileItem *)item)->HasPVRRecordingInfoTag())
        return ((const CFileItem *)item)->GetPVRRecordingInfoTag()->m_resumePoint.timeInSeconds > 0;
    }
  }
  else if (item->IsFileItem())
  {
    const CFileItem *pItem = (const CFileItem *)item;
    if (condition == LISTITEM_ISRECORDING)
    {
      if (!g_PVRManager.IsStarted())
        return false;

      if (pItem->HasPVRChannelInfoTag())
      {
        return pItem->GetPVRChannelInfoTag()->IsRecording();
      }
      else if (pItem->HasPVRTimerInfoTag())
      {
        const CPVRTimerInfoTagPtr timer = pItem->GetPVRTimerInfoTag();
        if (timer)
          return timer->IsRecording();
      }
      else if (pItem->HasEPGInfoTag())
      {
        const CPVRTimerInfoTagPtr timer = pItem->GetEPGInfoTag()->Timer();
        if (timer)
          return timer->IsRecording();
      }
    }
    else if (condition == LISTITEM_INPROGRESS)
    {
      if (!g_PVRManager.IsStarted())
        return false;

      if (pItem->HasEPGInfoTag())
        return pItem->GetEPGInfoTag()->IsActive();
    }
    else if (condition == LISTITEM_HASTIMER)
    {
      if (pItem->HasEPGInfoTag())
        return pItem->GetEPGInfoTag()->HasTimer();
    }
    else if (condition == LISTITEM_HASTIMERSCHEDULE)
    {
      if (pItem->HasEPGInfoTag())
      {
        CPVRTimerInfoTagPtr timer = pItem->GetEPGInfoTag()->Timer();
        if (timer)
          return timer->GetTimerRuleId() != PVR_TIMER_NO_PARENT;
      }
    }
    else if (condition == LISTITEM_TIMERISACTIVE)
    {
      if (pItem->HasEPGInfoTag())
      {
        CPVRTimerInfoTagPtr timer = pItem->GetEPGInfoTag()->Timer();
        if (timer)
          return timer->IsActive();
      }
    }
    else if (condition == LISTITEM_TIMERHASCONFLICT)
    {
      if (pItem->HasEPGInfoTag())
      {
        CPVRTimerInfoTagPtr timer = pItem->GetEPGInfoTag()->Timer();
        if (timer)
          return timer->HasConflict();
      }
    }
    else if (condition == LISTITEM_TIMERHASERROR)
    {
      if (pItem->HasEPGInfoTag())
      {
        CPVRTimerInfoTagPtr timer = pItem->GetEPGInfoTag()->Timer();
        if (timer)
          return (timer->IsBroken() && !timer->HasConflict());
      }
    }
    else if (condition == LISTITEM_HASRECORDING)
    {
      return pItem->HasEPGInfoTag() && pItem->GetEPGInfoTag()->HasRecording();
    }
    else if (condition == LISTITEM_HAS_EPG)
    {
      if (pItem->HasPVRChannelInfoTag())
      {
        return (pItem->GetPVRChannelInfoTag()->GetEPGNow().get() != NULL);
      }
      if (pItem->HasPVRTimerInfoTag() && pItem->GetPVRTimerInfoTag()->GetEpgInfoTag())
      {
        return true;
      }
      else
      {
        return pItem->HasEPGInfoTag();
      }
    }
    else if (condition == LISTITEM_ISENCRYPTED)
    {
      if (pItem->HasPVRChannelInfoTag())
      {
        return pItem->GetPVRChannelInfoTag()->IsEncrypted();
      }
      else if (pItem->HasEPGInfoTag() && pItem->GetEPGInfoTag()->HasPVRChannel())
      {
        return pItem->GetEPGInfoTag()->ChannelTag()->IsEncrypted();
      }
    }
    else if (condition == LISTITEM_IS_STEREOSCOPIC)
    {
      std::string stereoMode = pItem->GetProperty("stereomode").asString();
      if (stereoMode.empty() && pItem->HasVideoInfoTag())
          stereoMode = CStereoscopicsManager::GetInstance().NormalizeStereoMode(pItem->GetVideoInfoTag()->m_streamDetails.GetStereoMode());
      if (!stereoMode.empty() && stereoMode != "mono")
        return true;
    }
    else if (condition == LISTITEM_IS_COLLECTION)
    {
      if (pItem->HasVideoInfoTag())
        return (pItem->GetVideoInfoTag()->m_type == MediaTypeVideoCollection);
    }
  }

  return false;
}

void CGUIInfoManager::ResetCache()
{
  // reset any animation triggers as well
  m_containerMoves.clear();
  // mark our infobools as dirty
  CSingleLock lock(m_critInfo);
  for (std::vector<InfoPtr>::iterator i = m_bools.begin(); i != m_bools.end(); ++i)
    (*i)->SetDirty();
}

std::string CGUIInfoManager::GetPictureLabel(int info)
{
  if (info == SLIDE_FILE_NAME)
    return GetItemLabel(m_currentSlide, LISTITEM_FILENAME);
  else if (info == SLIDE_FILE_PATH)
  {
    std::string path = URIUtils::GetDirectory(m_currentSlide->GetPath());
    return CURL(path).GetWithoutUserDetails();
  }
  else if (info == SLIDE_FILE_SIZE)
    return GetItemLabel(m_currentSlide, LISTITEM_SIZE);
  else if (info == SLIDE_FILE_DATE)
    return GetItemLabel(m_currentSlide, LISTITEM_DATE);
  else if (info == SLIDE_INDEX)
  {
    CGUIWindowSlideShow *slideshow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    if (slideshow && slideshow->NumSlides())
    {
      return StringUtils::Format("%d/%d", slideshow->CurrentSlide(), slideshow->NumSlides());
    }
  }
  if (m_currentSlide->HasPictureInfoTag())
    return m_currentSlide->GetPictureInfoTag()->GetInfo(info);
  return "";
}

void CGUIInfoManager::SetCurrentSlide(CFileItem &item)
{
  if (m_currentSlide->GetPath() != item.GetPath())
  {
    if (!item.GetPictureInfoTag()->Loaded()) // If picture metadata has not been loaded yet, load it now
      item.GetPictureInfoTag()->Load(item.GetPath());
    *m_currentSlide = item;
  }
}

void CGUIInfoManager::ResetCurrentSlide()
{
  m_currentSlide->Reset();
}

bool CGUIInfoManager::CheckWindowCondition(CGUIWindow *window, int condition) const
{
  // check if it satisfies our condition
  if (!window) return false;
  if ((condition & WINDOW_CONDITION_HAS_LIST_ITEMS) && !window->HasListItems())
    return false;
  if ((condition & WINDOW_CONDITION_IS_MEDIA_WINDOW) && !window->IsMediaWindow())
    return false;
  return true;
}

CGUIWindow *CGUIInfoManager::GetWindowWithCondition(int contextWindow, int condition) const
{
  CGUIWindow *window = g_windowManager.GetWindow(contextWindow);
  if (CheckWindowCondition(window, condition))
    return window;

  // try topmost dialog
  window = g_windowManager.GetWindow(g_windowManager.GetTopMostModalDialogID());
  if (CheckWindowCondition(window, condition))
    return window;

  // try active window
  window = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
  if (CheckWindowCondition(window, condition))
    return window;

  return NULL;
}

void CGUIInfoManager::SetCurrentVideoTag(const CVideoInfoTag &tag)
{
  *m_currentFile->GetVideoInfoTag() = tag;
  m_currentFile->m_lStartOffset = 0;
}

void CGUIInfoManager::SetCurrentSongTag(const MUSIC_INFO::CMusicInfoTag &tag)
{
  //CLog::Log(LOGDEBUG, "Asked to SetCurrentTag");
  *m_currentFile->GetMusicInfoTag() = tag;
  m_currentFile->m_lStartOffset = 0;
}

const CFileItem& CGUIInfoManager::GetCurrentSlide() const
{
  return *m_currentSlide;
}

const MUSIC_INFO::CMusicInfoTag* CGUIInfoManager::GetCurrentSongTag() const
{
  if (m_currentFile->HasMusicInfoTag())
    return m_currentFile->GetMusicInfoTag();

  return NULL;
}

const PVR::CPVRRadioRDSInfoTagPtr CGUIInfoManager::GetCurrentRadioRDSInfoTag() const
{
  if (m_currentFile->HasPVRRadioRDSInfoTag())
    return m_currentFile->GetPVRRadioRDSInfoTag();

  PVR::CPVRRadioRDSInfoTagPtr empty;
  return empty;
}

const CVideoInfoTag* CGUIInfoManager::GetCurrentMovieTag() const
{
  if (m_currentFile->HasVideoInfoTag())
    return m_currentFile->GetVideoInfoTag();

  return NULL;
}

void GUIInfo::SetInfoFlag(uint32_t flag)
{
  assert(flag >= (1 << 24));
  m_data1 |= flag;
}

uint32_t GUIInfo::GetInfoFlag() const
{
  // we strip out the bottom 24 bits, where we keep data
  // and return the flag only
  return m_data1 & 0xff000000;
}

uint32_t GUIInfo::GetData1() const
{
  // we strip out the top 8 bits, where we keep flags
  // and return the unflagged data
  return m_data1 & ((1 << 24) -1);
}

int GUIInfo::GetData2() const
{
  return m_data2;
}

void CGUIInfoManager::SetLibraryBool(int condition, bool value)
{
  switch (condition)
  {
    case LIBRARY_HAS_MUSIC:
      m_libraryHasMusic = value ? 1 : 0;
      break;
    case LIBRARY_HAS_MOVIES:
      m_libraryHasMovies = value ? 1 : 0;
      break;
    case LIBRARY_HAS_MOVIE_SETS:
      m_libraryHasMovieSets = value ? 1 : 0;
      break;
    case LIBRARY_HAS_TVSHOWS:
      m_libraryHasTVShows = value ? 1 : 0;
      break;
    case LIBRARY_HAS_MUSICVIDEOS:
      m_libraryHasMusicVideos = value ? 1 : 0;
      break;
    case LIBRARY_HAS_SINGLES:
      m_libraryHasSingles = value ? 1 : 0;
      break;
    case LIBRARY_HAS_COMPILATIONS:
      m_libraryHasCompilations = value ? 1 : 0;
      break;
    default:
      break;
  }
}

void CGUIInfoManager::ResetLibraryBools()
{
  m_libraryHasMusic = -1;
  m_libraryHasMovies = -1;
  m_libraryHasTVShows = -1;
  m_libraryHasMusicVideos = -1;
  m_libraryHasMovieSets = -1;
  m_libraryHasSingles = -1;
  m_libraryHasCompilations = -1;
  m_libraryRoleCounts.clear();
}

bool CGUIInfoManager::GetLibraryBool(int condition)
{
  if (condition == LIBRARY_HAS_MUSIC)
  {
    if (m_libraryHasMusic < 0)
    { // query
      CMusicDatabase db;
      if (db.Open())
      {
        m_libraryHasMusic = (db.GetSongsCount() > 0) ? 1 : 0;
        db.Close();
      }
    }
    return m_libraryHasMusic > 0;
  }
  else if (condition == LIBRARY_HAS_MOVIES)
  {
    if (m_libraryHasMovies < 0)
    {
      CVideoDatabase db;
      if (db.Open())
      {
        m_libraryHasMovies = db.HasContent(VIDEODB_CONTENT_MOVIES) ? 1 : 0;
        db.Close();
      }
    }
    return m_libraryHasMovies > 0;
  }
  else if (condition == LIBRARY_HAS_MOVIE_SETS)
  {
    if (m_libraryHasMovieSets < 0)
    {
      CVideoDatabase db;
      if (db.Open())
      {
        m_libraryHasMovieSets = db.HasSets() ? 1 : 0;
        db.Close();
      }
    }
    return m_libraryHasMovieSets > 0;
  }
  else if (condition == LIBRARY_HAS_TVSHOWS)
  {
    if (m_libraryHasTVShows < 0)
    {
      CVideoDatabase db;
      if (db.Open())
      {
        m_libraryHasTVShows = db.HasContent(VIDEODB_CONTENT_TVSHOWS) ? 1 : 0;
        db.Close();
      }
    }
    return m_libraryHasTVShows > 0;
  }
  else if (condition == LIBRARY_HAS_MUSICVIDEOS)
  {
    if (m_libraryHasMusicVideos < 0)
    {
      CVideoDatabase db;
      if (db.Open())
      {
        m_libraryHasMusicVideos = db.HasContent(VIDEODB_CONTENT_MUSICVIDEOS) ? 1 : 0;
        db.Close();
      }
    }
    return m_libraryHasMusicVideos > 0;
  }
  else if (condition == LIBRARY_HAS_SINGLES)
  {
    if (m_libraryHasSingles < 0)
    {
      CMusicDatabase db;
      if (db.Open())
      {
        m_libraryHasSingles = (db.GetSinglesCount() > 0) ? 1 : 0;
        db.Close();
      }
    }
    return m_libraryHasSingles > 0;
  }
  else if (condition == LIBRARY_HAS_COMPILATIONS)
  {
    if (m_libraryHasCompilations < 0)
    {
      CMusicDatabase db;
      if (db.Open())
      {
        m_libraryHasCompilations = (db.GetCompilationAlbumsCount() > 0) ? 1 : 0;
        db.Close();
      }
    }
    return m_libraryHasCompilations > 0;
  }
  else if (condition == LIBRARY_HAS_VIDEO)
  {
    return (GetLibraryBool(LIBRARY_HAS_MOVIES) ||
            GetLibraryBool(LIBRARY_HAS_TVSHOWS) ||
            GetLibraryBool(LIBRARY_HAS_MUSICVIDEOS));
  }
  return false;
}

int CGUIInfoManager::RegisterSkinVariableString(const CSkinVariableString* info)
{
  if (!info)
    return 0;

  CSingleLock lock(m_critInfo);
  m_skinVariableStrings.push_back(*info);
  delete info;
  return CONDITIONAL_LABEL_START + m_skinVariableStrings.size() - 1;
}

int CGUIInfoManager::TranslateSkinVariableString(const std::string& name, int context)
{
  for (std::vector<CSkinVariableString>::const_iterator it = m_skinVariableStrings.begin();
       it != m_skinVariableStrings.end(); ++it)
  {
    if (StringUtils::EqualsNoCase(it->GetName(), name) && it->GetContext() == context)
      return it - m_skinVariableStrings.begin() + CONDITIONAL_LABEL_START;
  }
  return 0;
}

std::string CGUIInfoManager::GetSkinVariableString(int info,
                                                  bool preferImage /*= false*/,
                                                  const CGUIListItem *item /*= NULL*/)
{
  info -= CONDITIONAL_LABEL_START;
  if (info >= 0 && info < (int)m_skinVariableStrings.size())
    return m_skinVariableStrings[info].GetValue(preferImage, item);

  return "";
}

bool CGUIInfoManager::ConditionsChangedValues(const std::map<INFO::InfoPtr, bool>& map)
{
  for (std::map<INFO::InfoPtr, bool>::const_iterator it = map.begin() ; it != map.end() ; ++it)
  {
    if (it->first->Get() != it->second)
      return true;
  }
  return false;
}

bool CGUIInfoManager::IsPlayerChannelPreviewActive() const
{
  bool bReturn(false);
  if (m_playerShowInfo)
  {
    if (m_isPvrChannelPreview)
    {
      bReturn = true;
    }
    else
    {
      bReturn = !m_videoInfo.valid;
      if (bReturn && m_currentFile->HasPVRChannelInfoTag() && m_currentFile->GetPVRChannelInfoTag()->IsRadio())
        bReturn = !m_audioInfo.valid;
    }
  }
  return bReturn;
}

CEpgInfoTagPtr CGUIInfoManager::GetEpgInfoTag() const
{
  CEpgInfoTagPtr currentTag;
  if (m_currentFile->HasEPGInfoTag())
  {
    currentTag = m_currentFile->GetEPGInfoTag();
    while (currentTag && !currentTag->IsActive())
      currentTag = currentTag->GetNextEvent();
  }
  return currentTag;
}

int CGUIInfoManager::GetMessageMask()
{
  return TMSG_MASK_GUIINFOMANAGER;
}

void CGUIInfoManager::OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg)
{
  switch (pMsg->dwMessage)
  {
  case TMSG_GUI_INFOLABEL:
  {
    if (pMsg->lpVoid)
    {
      auto infoLabels = static_cast<std::vector<std::string>*>(pMsg->lpVoid);
      for (auto& param : pMsg->params)
        infoLabels->push_back(GetLabel(TranslateString(param)));
    }
  }
  break;

  case TMSG_GUI_INFOBOOL:
  {
    if (pMsg->lpVoid)
    {
      auto infoLabels = static_cast<std::vector<bool>*>(pMsg->lpVoid);
      for (auto& param : pMsg->params)
        infoLabels->push_back(EvaluateBool(param));
    }
  }
  break;

  case TMSG_UPDATE_CURRENT_ITEM:
  {
    auto item = static_cast<CFileItem*>(pMsg->lpVoid);
    if (!item)
      return;

    CFileItemPtr itemptr(item);
    if (pMsg->param1 == 1 && item->HasMusicInfoTag()) // only grab music tag
      SetCurrentSongTag(*item->GetMusicInfoTag());
    else if (pMsg->param1 == 2 && item->HasVideoInfoTag()) // only grab video tag
      SetCurrentVideoTag(*item->GetVideoInfoTag());
    else
      SetCurrentItem(itemptr);
  }
  break;

  default:
    break;
  }
}

std::string CGUIInfoManager::FormatRatingAndVotes(float rating, int votes)
{
  return StringUtils::Format(g_localizeStrings.Get(20350).c_str(),
                             StringUtils::FormatNumber(rating).c_str(),
                             StringUtils::FormatNumber(votes).c_str());
}
