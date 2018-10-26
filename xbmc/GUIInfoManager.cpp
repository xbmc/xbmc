/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIInfoManager.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <memory>

#include "Application.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "cores/DataCacheCore.h"
#include "filesystem/File.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoHelper.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "input/WindowTranslator.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/info/InfoExpression.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/SkinSettings.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace KODI::GUILIB;
using namespace KODI::GUILIB::GUIINFO;
using namespace INFO;

bool InfoBoolComparator(const InfoPtr &right, const InfoPtr &left)
{
  return *right < *left;
}

CGUIInfoManager::CGUIInfoManager(void)
: m_currentFile(new CFileItem),
  m_bools(&InfoBoolComparator)
{
}

CGUIInfoManager::~CGUIInfoManager(void)
{
  delete m_currentFile;
}

void CGUIInfoManager::Initialize()
{
  KODI::MESSAGING::CApplicationMessenger::GetInstance().RegisterReceiver(this);
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
///   \table_row3{   <b>`Player.HasGame`</b>,
///                  \anchor Player_HasGame
///                  _boolean_,
///     Returns true if the player has a game file.
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
///                  _string_,
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
///                  _integer_,
///     Returns how much of the file is cached above current play percentage
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
///                  _string_,
///     Returns the full path of the currently playing song or movie
///   }
///   \table_row3{   <b>`Player.FilenameAndPath`</b>,
///                  \anchor Player_FilenameAndPath
///                  _string_,
///     Returns the full path with filename of the currently playing song or movie
///   }
///   \table_row3{   <b>`Player.Filename`</b>,
///                  \anchor Player_Filename
///                  _string_,
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
///   \table_row3{   <b>`Player.TempoEnabled`</b>,
///                  \anchor Player_TempoEnabled
///                  _boolean_,
///     Returns true if player supports tempo (i.e. speed up/down normal playback speed)
///   }
///   \table_row3{   <b>`Player.IsTempo`</b>,
///                  \anchor Player_IsTempo
///                  _boolean_,
///     Returns true if player has tempo (i.e. is playing with a playback speed higher or
///     lower than normal playback speed)
///   }
///   \table_row3{   <b>`Player.PlaySpeed`</b>,
///                  \anchor Player_PlaySpeed
///                  _string_,
///     Returns the player playback speed with the format %1.2f (1.00 means normal 
///     playback speed). For Tempo\, the default range is 0.80 - 1.50 (it can be changed 
///     in advanced settings). If `Player.PlaySpeed` returns a value different from 1.00
///     and `Player.IsTempo` is false it means the player is in ff/rw mode.
///   }
///   \table_row3{   <b>`Player.HasResolutions`</b>,
///                  \anchor Player_HasResolutions
///                  _boolean_,
///     Returns true if the player is allowed to switch resolution and refresh rate 
///     (i.e. if whitelist modes are configured in Kodi's System/Display settings)
///   }
///   \table_row3{   <b>`Player.HasPrograms`</b>,
///                  \anchor Player_HasPrograms
///                  _boolean_,
///     Returns true if the media file being played has programs\, i.e. groups of streams. 
///     Ex: if a media file has multiple streams (quality\, channels\, etc) a program represents
///     a particular stream combo.
///   }
///   \table_row3{   <b>`Player.FrameAdvance`</b>,
///                  \anchor Player_FrameAdvance
///                  _boolean_,
///     Returns true if player is in frame advance mode. Skins should hide seek bar
///     in this mode)
///   }
///   \table_row3{   <b>`Player.Icon`</b>,
///                  \anchor Player_Icon
///                  _string_,
///     Returns the thumbnail of the currently playing item. If no thumbnail image exists\,
///     the icon will be returned\, if available.
///   }
/// \table_end
/// @}
const infomap player_labels[] =  {{ "hasmedia",         PLAYER_HAS_MEDIA },
                                  { "hasaudio",         PLAYER_HAS_AUDIO },
                                  { "hasvideo",         PLAYER_HAS_VIDEO },
                                  { "hasgame",          PLAYER_HAS_GAME },
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
                                  { "displayafterseek", PLAYER_DISPLAY_AFTER_SEEK },
                                  { "caching",          PLAYER_CACHING },
                                  { "seekbar",          PLAYER_SEEKBAR },
                                  { "seeking",          PLAYER_SEEKING },
                                  { "showtime",         PLAYER_SHOWTIME },
                                  { "showcodec",        PLAYER_SHOWCODEC },
                                  { "showinfo",         PLAYER_SHOWINFO },
                                  { "muted",            PLAYER_MUTED },
                                  { "hasduration",      PLAYER_HASDURATION },
                                  { "passthrough",      PLAYER_PASSTHROUGH },
                                  { "cachelevel",       PLAYER_CACHELEVEL },
                                  { "title",            PLAYER_TITLE },
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
                                  { "channelpreviewactive", PLAYER_IS_CHANNEL_PREVIEW_ACTIVE },
                                  { "tempoenabled",     PLAYER_SUPPORTS_TEMPO },
                                  { "istempo",          PLAYER_IS_TEMPO },
                                  { "playspeed",        PLAYER_PLAYSPEED },
                                  { "hasprograms",      PLAYER_HAS_PROGRAMS },
                                  { "hasresolutions",   PLAYER_HAS_RESOLUTIONS },
                                  { "frameadvance",     PLAYER_FRAMEADVANCE },
                                  { "icon",             PLAYER_ICON }};

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
///     Returns hours (hh)\, minutes (mm) or seconds (ss).
///     Also supported: (hh:mm)\, (mm:ss)\, (hh:mm:ss)\, (h:mm:ss).
///     Added with Leia: (secs)\, (mins)\, (hours) for total time values and (m).
///     Example: 3661 seconds => h=1\, hh=01\, m=1\, mm=01\, ss=01\, hours=1\, mins=61\, secs=3661
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
///     Returns hours (hh)\, minutes (mm) or seconds (ss).
///     Also supported: (hh:mm)\, (mm:ss)\, (hh:mm:ss)\, (h:mm:ss).
///     Added with Leia: (secs)\, (mins)\, (hours) for total time values and (m).
///     Example: 3661 seconds => h=1\, hh=01\, m=1\, mm=01\, ss=01\, hours=1\, mins=61\, secs=3661
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
///     Returns hours (hh)\, minutes (mm) or seconds (ss).
///     Also supported: (hh:mm)\, (mm:ss)\, (hh:mm:ss)\, (h:mm:ss).
///     Added with Leia: (secs)\, (mins)\, (hours) for total time values and (m).
///     Example: 3661 seconds => h=1\, hh=01\, m=1\, mm=01\, ss=01\, hours=1\, mins=61\, secs=3661
///   }
///   \table_row3{   <b>`Player.Duration`</b>,
///                  \anchor Player_Duration
///                  _string_,
///     Total duration of the current playing media
///   }
///   \table_row3{   <b>`Player.Duration(format)`</b>,
///                  \anchor Player_Duration_format
///                  _string_,
///     Returns hours (hh)\, minutes (mm) or seconds (ss).
///     Also supported: (hh:mm)\, (mm:ss)\, (hh:mm:ss)\, (h:mm:ss).
///     Added with Leia: (secs)\, (mins)\, (hours) for total time values and (m).
///     Example: 3661 seconds => h=1\, hh=01\, m=1\, mm=01\, ss=01\, hours=1\, mins=61\, secs=3661
///   }
///   \table_row3{   <b>`Player.FinishTime`</b>,
///                  \anchor Player_FinishTime
///                  _string_,
///     Time playing media will end
///   }
///   \table_row3{   <b>`Player.FinishTime(format)`</b>,
///                  \anchor Player_FinishTime_format
///                  _string_,
///     Returns hours (hh)\, minutes (mm) or seconds (ss). When 12 hour clock is used
///     (xx) will return AM/PM. Also supported: (hh:mm)\, (mm:ss)\, (hh:mm:ss)\, (h:mm:ss).
///     Added with Leia: (secs)\, (mins)\, (hours) for total time values and (m).
///     Example: 3661 seconds => h=1\, hh=01\, m=1\, mm=01\, ss=01\, hours=1\, mins=61\, secs=3661
///   }
///   \table_row3{   <b>`Player.StartTime`</b>,
///                  \anchor Player_StartTime
///                  _string_,
///     Time playing media began
///   }
///   \table_row3{   <b>`Player.StartTime(format)`</b>,
///                  \anchor Player_StartTime_format
///                  _string_,
///     Returns hours (hh)\, minutes (mm) or seconds (ss). When 12 hour clock is used
///     (xx) will return AM/PM. Also supported: (hh:mm)\, (mm:ss)\, (hh:mm:ss)\, (h:mm:ss).
///     Added with Leia: (secs)\, (mins)\, (hours) for total time values and (m).
///     Example: 3661 seconds => h=1\, hh=01\, m=1\, mm=01\, ss=01\, hours=1\, mins=61\, secs=3661
///   }
///   \table_row3{   <b>`Player.SeekNumeric`</b>,
///                  \anchor Player_SeekNumeric
///                  _string_,
///     Time to which the user is seeking via numeric keys.
///   }
///   \table_row3{   <b>`Player.SeekNumeric(format)`</b>,
///                  \anchor Player_SeekNumeric_format
///                  _string_,
///     Returns hours (hh)\, minutes (mm) or seconds (ss). When 12 hour clock is used
///     (xx) will return AM/PM. Also supported: (hh:mm)\, (mm:ss)\, (hh:mm:ss)\, (h:mm:ss).
///     Added with Leia: (secs)\, (mins)\, (hours) for total time values and (m).
///     Example: 3661 seconds => h=1\, hh=01\, m=1\, mm=01\, ss=01\, hours=1\, mins=61\, secs=3661
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
                                  { "starttime",        PLAYER_START_TIME },
                                  { "seeknumeric",      PLAYER_SEEKNUMERIC } };

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
///     Current weather conditions as textual description – this is looked up in a background process.
///   }
///   \table_row3{   <b>`Weather.ConditionsIcon`</b>,
///                  \anchor Weather_ConditionsIcon
///                  _string_,
///     Current weather conditions as icon – this is looked up in a background process.
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
                                  { "conditions",       WEATHER_CONDITIONS_TEXT },         // labels from here
                                  { "temperature",      WEATHER_TEMPERATURE },
                                  { "location",         WEATHER_LOCATION },
                                  { "fanartcode",       WEATHER_FANART_CODE },
                                  { "plugin",           WEATHER_PLUGIN },
                                  { "conditionsicon",   WEATHER_CONDITIONS_ICON }};

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
///     Returns true if system has an active lock mode.
///   }
///   \table_row3{   <b>`System.IsMaster`</b>,
///                  \anchor System_IsMaster
///                  _boolean_,
///     Returns true if system is in master mode.
///   }
///   \table_row3{   <b>`System.ShowExitButton`</b>,
///                  \anchor System_ShowExitButton
///                  _boolean_,
///     Returns true if the exit button should be shown (configurable via advanced settings).
///   }
///   \table_row3{   <b>`System.DPMSActive`</b>,
///                  \anchor System_DPMSActive
///                  _boolean_,
///     Returns true if DPMS (VESA Display Power Management Signaling) mode is active.
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
///   \table_row3{   <b>`System.HasPVRAddon(id)`</b>,
///                  \anchor System_HasPVRAddon
///                  _boolean_,
///     Returns true if at least one pvr client addon is installed and enabled.
///   }
///   \table_row3{   <b>`System.HasCMS`</b>,
///                  \anchor System_HasCMS
///                  _boolean_,
///     Returns true if colour management is supported from Kodi
///     \note currently only supported for OpenGL
///   }
///   \table_row3{   <b>`System.HasActiveModalDialog`</b>,
///                  \anchor System_HasActiveModalDialog
///                  _boolean_,
///     Returns true if a modal dialog is active
///   }
///   \table_row3{   <b>`System.HasVisibleModalDialog`</b>,
///                  \anchor System_HasVisibleModalDialog
///                  _boolean_,
///     Returns true if a modal dialog is visible
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
///   \table_row3{   <b>`System.Platform.UWP`</b>,
///                  \anchor System_PlatformUWP
///                  _boolean_,
///     Returns true if Kodi is running on Universal Windows Platform (UWP).
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
///     Returns hours (hh)\, minutes (mm) or seconds (ss). When 12 hour clock is used
///     (xx) will return AM/PM. Also supported: (hh:mm)\, (mm:ss)\, (hh:mm:ss)\, (h:mm:ss).
///     Added with Leia: (secs)\, (mins)\, (hours) for total time values and (m).
///     Example: 3661 seconds => h=1\, hh=01\, m=1\, mm=01\, ss=01\, hours=1\, mins=61\, secs=3661
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
///     ID of the currently focused control.
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
///     System name + kernel version
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
///     Returns the current language
///   }
///   \table_row3{   <b>`System.ProfileName`</b>,
///                  \anchor System_ProfileName
///                  _string_,
///     Returns the user name of the currently logged in Kodi user
///   }
///   \table_row3{   <b>`System.ProfileThumb`</b>,
///                  \anchor System_ProfileThumb
///                  _string_,
///     Returns the thumbnail image of the currently logged in Kodi user
///   }
///   \table_row3{   <b>`System.ProfileCount`</b>,
///                  \anchor System_ProfileCount
///                  _string_,
///     Returns the number of defined profiles
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
///     Returns Celsius or Fahrenheit symbol
///   }
///   \table_row3{   <b>`System.Progressbar`</b>,
///                  \anchor System_Progressbar
///                  _string_,
///     Returns the percentage of the currently active progress.
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
///   \table_row3{   <b>`System.PrivacyPolicy`</b>,
///                  \anchor System_PrivacyPolicy
///                  _string_,
///     Returns the official Kodi privacy policy
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
                                  { "hasactivemodaldialog",   SYSTEM_HAS_ACTIVE_MODAL_DIALOG },
                                  { "hasvisiblemodaldialog",   SYSTEM_HAS_VISIBLE_MODAL_DIALOG },
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
                                  { "hascms",           SYSTEM_HAS_CMS },
                                  { "privacypolicy",    SYSTEM_PRIVACY_POLICY },
                                  { "haspvraddon",      SYSTEM_HAS_PVR_ADDON }};

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
///     Returns true if the cpu core with the given 'id' exists.
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
///     Returns the moods of the currently playing Album
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Album_Style)`</b>,
///                  \anchor MusicPlayer_Property_Album_Style
///                  _string_,
///     Returns the styles of the currently playing Album
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Album_Theme)`</b>,
///                  \anchor MusicPlayer_Property_Album_Theme
///                  _string_,
///     Returns the themes of the currently playing Album
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Album_Type)`</b>,
///                  \anchor MusicPlayer_Property_Album_Type
///                  _string_,
///     Returns the album type (e.g. compilation\, enhanced\, explicit lyrics) of the
///     currently playing album
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Album_Label)`</b>,
///                  \anchor MusicPlayer_Property_Album_Label
///                  _string_,
///     Returns the record label of the currently playing album
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Album_Description)`</b>,
///                  \anchor MusicPlayer_Property_Album_Description
///                  _string_,
///     Returns a review of the currently playing album
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
///     Album artist of the currently playing song
///   }
///   \table_row3{   <b>`MusicPlayer.Cover`</b>,
///                  \anchor MusicPlayer_Cover
///                  _string_,
///     Album cover of currently playing song
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Sortname)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Sortname
///                  _string_,
///     Sortname of the currently playing Artist
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Type)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Type
///                  _string_,
///     Type of the currently playing Artist - person\, group\, orchestra\, choir etc.
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Gender)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Gender
///                  _string_,
///     Gender of the currently playing Artist - male\, female\, other
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Disambiguation)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Disambiguation
///                  _string_,
///     Brief description of the currently playing Artist that differentiates them
///     from others with the same name
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
///     Returns a biography of the currently playing artist
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Mood)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Mood
///                  _string_,
///     Returns the moods of the currently playing artist
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Style)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Style
///                  _string_,
///     Returns the styles of the currently playing artist
///   }
///   \table_row3{   <b>`MusicPlayer.Property(Artist_Genre)`</b>,
///                  \anchor MusicPlayer_Property_Artist_Genre
///                  _string_,
///     Returns the genre of the currently playing artist
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
///     Returns the scraped rating and votes of currently playing song\, if it's in the database
///   }
///   \table_row3{   <b>`MusicPlayer.UserRating`</b>,
///                  \anchor MusicPlayer_UserRating
///                  _string_,
///     Returns the scraped rating of the currently playing song
///   }
///   \table_row3{   <b>`MusicPlayer.Votes`</b>,
///                  \anchor MusicPlayer_Votes
///                  _string_,
///     Returns the scraped votes of currently playing song\, if it's in the database
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
///     Mood of the currently playing song
///   }
///   \table_row3{   <b>`MusicPlayer.PlaylistPlaying`</b>,
///                  \anchor MusicPlayer_PlaylistPlaying
///                  _boolean_,
///     Returns true if a playlist is currently playing
///   }
///   \table_row3{   <b>`MusicPlayer.Exists(relative\,position)`</b>,
///                  \anchor MusicPlayer_Exists
///                  _boolean_,
///     Returns true if the currently playing playlist has a song queued at the given position.
///     It is possible to define whether the position is relative or not\, default is false.
///   }
///   \table_row3{   <b>`MusicPlayer.HasPrevious`</b>,
///                  \anchor MusicPlayer_HasPrevious
///                  _boolean_,
///     Returns true if the music player has a a Previous Song in the Playlist.
///   }
///   \table_row3{   <b>`MusicPlayer.HasNext`</b>,
///                  \anchor MusicPlayer_HasNext
///                  _boolean_,
///     Returns true if the music player has a next song queued in the Playlist.
///   }
///   \table_row3{   <b>`MusicPlayer.PlayCount`</b>,
///                  \anchor MusicPlayer_PlayCount
///                  _integer_,
///     Returns the play count of currently playing song\, if it's in the database
///   }
///   \table_row3{   <b>`MusicPlayer.LastPlayed`</b>,
///                  \anchor MusicPlayer_LastPlayed
///                  _string_,
///     Returns the last play date of currently playing song\, if it's in the database
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
///   \table_row3{   <b>`MusicPlayer.ChannelName`</b>,
///                  \anchor MusicPlayer_ChannelName
///                  _string_,
///     Channel name of the radio programme that's currently playing (PVR).
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
///     Channel group of the radio programme that's currently playing (PVR).
///   }
///   \table_row3{   <b>`MusicPlayer.Property(propname)`</b>,
///                  \anchor MusicPlayer_Property_Propname
///                  _string_,
///     Get a property of the currently playing item.
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
                                  { "channelnumberlabel", MUSICPLAYER_CHANNEL_NUMBER },
                                  { "channelgroup",     MUSICPLAYER_CHANNEL_GROUP },
                                  { "dbid",             MUSICPLAYER_DBID },
                                  { "property",         MUSICPLAYER_PROPERTY },
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
///     Returns true if epg information is available for the currently playing
///     programme (PVR).
///   }
///   \table_row3{   <b>`VideoPlayer.CanResumeLiveTV`</b>,
///                  \anchor VideoPlayer_CanResumeLiveTV
///                  _boolean_,
///     Returns true if a in-progress PVR recording is playing an the respective live TV channel is available
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
///     Original title of currently playing video. If it's in the database
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
///     Cover of currently playing movie
///   }
///   \table_row3{   <b>`VideoPlayer.Rating`</b>,
///                  \anchor VideoPlayer_Rating
///                  _string_,
///     Returns the scraped rating of current movie\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.UserRating`</b>,
///                  \anchor VideoPlayer_UserRating
///                  _string_,
///     Returns the user rating of the currently playing item
///   }
///   \table_row3{   <b>`VideoPlayer.Votes`</b>,
///                  \anchor VideoPlayer_Votes
///                  _string_,
///     Returns the scraped votes of current movie\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.RatingAndVotes`</b>,
///                  \anchor VideoPlayer_RatingAndVotes
///                  _string_,
///     Returns the scraped rating and votes of current movie\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.mpaa`</b>,
///                  \anchor VideoPlayer_mpaa
///                  _string_,
///     MPAA rating of current movie\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.IMDBNumber`</b>,
///                  \anchor VideoPlayer_IMDBNumber
///                  _string_,
///     The IMDb ID of the current movie\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.Top250`</b>,
///                  \anchor VideoPlayer_Top250
///                  _string_,
///     IMDb Top250 position of the currently playing movie\, if it's in the database
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
///     Release or aired date of the currently playing episode\, show\, movie or EPG item\, if it's in the database
///   }
///   \table_row3{   <b>`VideoPlayer.Trailer`</b>,
///                  \anchor VideoPlayer_Trailer
///                  _string_,
///     The path to the trailer of the currently playing movie\, if it's in the database
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
///     Returns the video codec of the currently playing video (common values: see
///     \ref ListItem_VideoCodec "ListItem.VideoCodec")
///   }
///   \table_row3{   <b>`VideoPlayer.VideoResolution`</b>,
///                  \anchor VideoPlayer_VideoResolution
///                  _string_,
///     Returns the video resolution of the currently playing video (possible
///     values: see \ref ListItem_VideoResolution "ListItem.VideoResolution")
///   }
///   \table_row3{   <b>`VideoPlayer.VideoAspect`</b>,
///                  \anchor VideoPlayer_VideoAspect
///                  _string_,
///     Returns the aspect ratio of the currently playing video (possible values:
///     see \ref ListItem_VideoAspect "ListItem.VideoAspect")
///   }
///   \table_row3{   <b>`VideoPlayer.AudioCodec`</b>,
///                  \anchor VideoPlayer_AudioCodec
///                  _string_,
///     Returns the audio codec of the currently playing video\, optionally 'n'
///     defines the number of the audiostream (common values: see
///     \ref ListItem_AudioCodec "ListItem.AudioCodec")
///   }
///   \table_row3{   <b>`VideoPlayer.AudioChannels`</b>,
///                  \anchor VideoPlayer_AudioChannels
///                  _string_,
///     Returns the number of audio channels of the currently playing video
///     (possible values: see \ref ListItem_AudioChannels "ListItem.AudioChannels")
///   }
///   \table_row3{   <b>`VideoPlayer.AudioLanguage`</b>,
///                  \anchor VideoPlayer_AudioLanguage
///                  _string_,
///     Returns the language of the audio of the currently playing video(possible
///     values: see \ref ListItem_AudioLanguage "ListItem.AudioLanguage")
///   }
///   \table_row3{   <b>`VideoPlayer.SubtitlesLanguage`</b>,
///                  \anchor VideoPlayer_SubtitlesLanguage
///                  _string_,
///     Returns the language of the subtitle of the currently playing video
///     (possible values: see \ref ListItem_SubtitleLanguage "ListItem.SubtitleLanguage")
///   }
///   \table_row3{   <b>`VideoPlayer.StereoscopicMode`</b>,
///                  \anchor VideoPlayer_StereoscopicMode
///                  _string_,
///     Returns the stereoscopic mode of the currently playing video (possible
///     values: see \ref ListItem_StereoscopicMode "ListItem.StereoscopicMode")
///   }
///   \table_row3{   <b>`VideoPlayer.StartTime`</b>,
///                  \anchor VideoPlayer_StartTime
///                  _string_,
///     Start date and time of the currently playing epg event or recording (PVR).
///   }
///   \table_row3{   <b>`VideoPlayer.EndTime`</b>,
///                  \anchor VideoPlayer_EndTime
///                  _string_,
///     End date and time of the currently playing epg event or recording (PVR).
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
///     Name of the currently tuned channel (PVR).
///   }
///   \table_row3{   <b>`VideoPlayer.ChannelNumberLabel`</b>,
///                  \anchor VideoPlayer_ChannelNumberLabel
///                  _string_,
///     Channel and subchannel number of the tv channel that's currently playing (PVR).
///   }
///   \table_row3{   <b>`VideoPlayer.ChannelGroup`</b>,
///                  \anchor VideoPlayer_ChannelGroup
///                  _string_,
///     Group of the currently tuned channel (PVR).
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
                                  { "videobitrate",     VIDEOPLAYER_VIDEO_BITRATE },
                                  { "audiocodec",       VIDEOPLAYER_AUDIO_CODEC },
                                  { "audiochannels",    VIDEOPLAYER_AUDIO_CHANNELS },
                                  { "audiobitrate",     VIDEOPLAYER_AUDIO_BITRATE },
                                  { "audiolanguage",    VIDEOPLAYER_AUDIO_LANG },
                                  { "hasteletext",      VIDEOPLAYER_HASTELETEXT },
                                  { "lastplayed",       VIDEOPLAYER_LASTPLAYED },
                                  { "playcount",        VIDEOPLAYER_PLAYCOUNT },
                                  { "hassubtitles",     VIDEOPLAYER_HASSUBTITLES },
                                  { "subtitlesenabled", VIDEOPLAYER_SUBTITLESENABLED },
                                  { "subtitleslanguage",VIDEOPLAYER_SUBTITLES_LANG },
                                  { "starttime",        VIDEOPLAYER_STARTTIME },
                                  { "endtime",          VIDEOPLAYER_ENDTIME },
                                  { "nexttitle",        VIDEOPLAYER_NEXT_TITLE },
                                  { "nextgenre",        VIDEOPLAYER_NEXT_GENRE },
                                  { "nextplot",         VIDEOPLAYER_NEXT_PLOT },
                                  { "nextplotoutline",  VIDEOPLAYER_NEXT_PLOT_OUTLINE },
                                  { "nextstarttime",    VIDEOPLAYER_NEXT_STARTTIME },
                                  { "nextendtime",      VIDEOPLAYER_NEXT_ENDTIME },
                                  { "nextduration",     VIDEOPLAYER_NEXT_DURATION },
                                  { "channelname",      VIDEOPLAYER_CHANNEL_NAME },
                                  { "channelnumberlabel", VIDEOPLAYER_CHANNEL_NUMBER },
                                  { "channelgroup",     VIDEOPLAYER_CHANNEL_GROUP },
                                  { "hasepg",           VIDEOPLAYER_HAS_EPG },
                                  { "parentalrating",   VIDEOPLAYER_PARENTAL_RATING },
                                  { "isstereoscopic",   VIDEOPLAYER_IS_STEREOSCOPIC },
                                  { "stereoscopicmode", VIDEOPLAYER_STEREOSCOPIC_MODE },
                                  { "canresumelivetv",  VIDEOPLAYER_CAN_RESUME_LIVE_TV },
                                  { "imdbnumber",       VIDEOPLAYER_IMDBNUMBER },
                                  { "episodename",      VIDEOPLAYER_EPISODENAME },
                                  { "dbid", VIDEOPLAYER_DBID }
};

/// \page modules__General__List_of_gui_access
/// \section modules__General__List_of_gui_access_RetroPlayer RetroPlayer
/// @{
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`RetroPlayer.VideoFilter`</b>,
///                  \anchor RetroPlayer_VideoFilter
///                  _string_,
///     Returns the video filter of the currently-playing game.\n
///     The following values are possible:
///     - nearest (Nearest neighbor\, i.e. pixelate)
///     - linear (Bilinear filtering\, i.e. smooth blur)
///   }
///   \table_row3{   <b>`RetroPlayer.StretchMode`</b>,
///                  \anchor RetroPlayer_StretchMode
///                  _string_,
///     Returns the stretch mode of the currently-playing game.\n
///     The following values are possible:
///     - normal (Show the game normally)
///     - 4:3 (Stretch to a 4:3 aspect ratio)
///     - fullscreen (Stretch to the full viewing area)
///     - original (Shrink to the original resolution)
///   }
///   \table_row3{   <b>`RetroPlayer.VideoRotation`</b>,
///                  \anchor RetroPlayer_VideoRotation
///                  _integer_,
///     Returns the video rotation of the currently-playing game
///     in degrees counter-clockwise.\n
///     The following values are possible:
///     - 0
///     - 90 (Shown in the GUI as 270 degrees)
///     - 180
///     - 270 (Shown in the GUI as 90 degrees)
///   }
///   \table_row3{   <b>`ListItem.Property(Game.VideoFilter)`</b>,
///                  \anchor ListItem_Property_Game_VideoFilter
///                  _string_,
///     Returns the video filter of the list item representing a
///     gamewindow control.\n
///     See \link RetroPlayer_VideoFilter RetroPlayer.VideoFilter \endlink
///     for the possible values.
///   }
///   \table_row3{   <b>`ListItem.Property(Game.StretchMode)`</b>,
///                  \anchor ListItem_Property_Game_StretchMode
///                  _string_,
///     Returns the stretch mode of the list item representing a
///     gamewindow control.\n
///     See \link RetroPlayer_StretchMode RetroPlayer.StretchMode \endlink
///     for the possible values.
///   }
///   \table_row3{   <b>`ListItem.Property(Game.VideoRotation)`</b>,
///                  \anchor ListItem_Property_Game_VideoRotation
///                  _integer_,
///     Returns the video rotation of the list item representing a
///     gamewindow control.\n
///     See \link RetroPlayer_VideoRotation RetroPlayer.VideoRotation \endlink
///     for the possible values.
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}
const infomap retroplayer[] =
{
  { "videofilter",            RETROPLAYER_VIDEO_FILTER},
  { "stretchmode",            RETROPLAYER_STRETCH_MODE},
  { "videorotation",          RETROPLAYER_VIDEO_ROTATION},
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
///     Returns complete path of currently displayed folder
///   }
///   \table_row3{   <b>`Container.FolderName`</b>,
///                  \anchor Container_FolderName
///                  _string_,
///     Returns top most folder in currently displayed folder
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
///     Returns the number of available skin view modes for the current container listing
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
///     Number of items in the container or grouplist with given id excluding parent folder item. If no id is
///     specified it grabs the current container.
///   }
///   \table_row3{   <b>`Container(id).NumAllItems`</b>,
///                  \anchor Container_NumAllItems
///                  _boolean_,
///     Number of all items in the container or grouplist with given id including parent folder item. If no id is
///     specified it grabs the current container.
///   }
///   \table_row3{   <b>`Container(id).NumNonFolderItems`</b>,
///                  \anchor Container_NumNonFolderItems
///                  _boolean_,
///     Number of items in the container or grouplist with given id excluding all folder items (example: pvr
///     recordings folders\, parent ".." folder). If no id is specified it grabs the current container.
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
                                  { "numnonfolderitems", CONTAINER_NUM_NONFOLDER_ITEMS },
                                  { "numallitems",      CONTAINER_NUM_ALL_ITEMS },
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
///     Sub item in the container or grouplist with given id. If no id is
///     specified it grabs the current container.
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
///   \table_row3{   <b>`Container.Art(type)`</b>,
///                  \anchor Container_Art
///                  _string_,
///     Returns the path to the art image file for the given type of the current container
///     Todo: list of all art types
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
///     Returns the thumbnail (if it exists) of the currently selected item
///     in a list or thumb control.
///     @deprecated but still available\, returns
///     the same as `ListItem.Art(thumb)`.\par
///   }
///   \table_row3{   <b>`ListItem.Icon`</b>,
///                  \anchor ListItem_Icon
///                  _string_,
///     Returns the thumbnail (if it exists) of the currently selected item in a list or thumb control. If no thumbnail image exists\, it will show the icon.
///   }
///   \table_row3{   <b>`ListItem.ActualIcon`</b>,
///                  \anchor ListItem_ActualIcon
///                  _string_,
///     Returns the icon of the currently selected item in a list or thumb control.
///   }
///   \table_row3{   <b>`ListItem.Overlay`</b>,
///                  \anchor ListItem_Overlay
///                  _string_,
///     Returns the overlay icon status of the currently selected item in a list or thumb control.
///       - compressed file -- OverlayRAR.png
///       - watched -- OverlayWatched.png
///       - unwatched -- OverlayUnwatched.png
///       - locked -- OverlayLocked.png
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
///     Returns the left label of the currently selected item in a container
///   }
///   \table_row3{   <b>`ListItem.Label2`</b>,
///                  \anchor ListItem_Label2
///                  _string_,
///     Returns the right label of the currently selected item in a container
///   }
///   \table_row3{   <b>`ListItem.Title`</b>,
///                  \anchor ListItem_Title
///                  _string_,
///     Returns the title of the currently selected song or movie in a container
///   }
///   \table_row3{   <b>`ListItem.OriginalTitle`</b>,
///                  \anchor ListItem_OriginalTitle
///                  _string_,
///     Returns the original title of the currently selected movie in a container
///   }
///   \table_row3{   <b>`ListItem.SortLetter`</b>,
///                  \anchor ListItem_SortLetter
///                  _string_,
///     Returns the first letter of the current file in a container
///   }
///   \table_row3{   <b>`ListItem.TrackNumber`</b>,
///                  \anchor ListItem_TrackNumber
///                  _string_,
///     Returns the track number of the currently selected song in a container
///   }
///   \table_row3{   <b>`ListItem.Artist`</b>,
///                  \anchor ListItem_Artist
///                  _string_,
///     Returns the artist of the currently selected song in a container
///   }
///   \table_row3{   <b>`ListItem.AlbumArtist`</b>,
///                  \anchor ListItem_AlbumArtist
///                  _string_,
///     Returns the artist of the currently selected album in a list
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Sortname)`</b>,
///                  \anchor ListItem_Property_Artist_Sortname
///                  _string_,
///     Sortname of the currently selected Artist
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Type)`</b>,
///                  \anchor ListItem_Property_Artist_Type
///                  _string_,
///     Type of the currently selected Artist - person\, group\, orchestra\, choir etc.
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Gender)`</b>,
///                  \anchor ListItem_Property_Artist_Gender
///                  _string_,
///     Gender of the currently selected Artist - male\, female\, other
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Disambiguation)`</b>,
///                  \anchor ListItem_Property_Artist_Disambiguation
///                  _string_,
///     Brief description of the currently selected Artist that differentiates them
///     from others with the same name
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
///     Returns a biography of the currently selected artist
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Mood)`</b>,
///                  \anchor ListItem_Property_Artist_Mood
///                  _string_,
///     Returns the moods of the currently selected artist
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Style)`</b>,
///                  \anchor ListItem_Property_Artist_Style
///                  _string_,
///     Returns the styles of the currently selected artist
///   }
///   \table_row3{   <b>`ListItem.Property(Artist_Genre)`</b>,
///                  \anchor ListItem_Property_Artist_Genre
///                  _string_,
///     Returns the genre of the currently selected artist
///   }
///   \table_row3{   <b>`ListItem.Album`</b>,
///                  \anchor ListItem_Album
///                  _string_,
///     Returns the album of the currently selected song in a container
///   }
///   \table_row3{   <b>`ListItem.Property(Album_Mood)`</b>,
///                  \anchor ListItem_Property_Album_Mood
///                  _string_,
///     Returns the moods of the currently selected Album
///   }
///   \table_row3{   <b>`ListItem.Property(Album_Style)`</b>,
///                  \anchor ListItem_Property_Album_Style
///                  _string_,
///     Returns the styles of the currently selected Album
///   }
///   \table_row3{   <b>`ListItem.Property(Album_Theme)`</b>,
///                  \anchor ListItem_Property_Album_Theme
///                  _string_,
///     Returns the themes of the currently selected Album
///   }
///   \table_row3{   <b>`ListItem.Property(Album_Type)`</b>,
///                  \anchor ListItem_Property_Album_Type
///                  _string_,
///     Returns the Album Type (e.g. compilation\, enhanced\, explicit lyrics) of
///     the currently selected Album
///   }
///   \table_row3{   <b>`ListItem.Property(Album_Label)`</b>,
///                  \anchor ListItem_Property_Album_Label
///                  _string_,
///     Returns the record label of the currently selected Album
///   }
///   \table_row3{   <b>`ListItem.Property(Album_Description)`</b>,
///                  \anchor ListItem_Property_Album_Description
///                  _string_,
///     Returns a review of the currently selected Album
///   }
///   \table_row3{   <b>`ListItem.DiscNumber`</b>,
///                  \anchor ListItem_DiscNumber
///                  _string_,
///     Returns the disc number of the currently selected song in a container
///   }
///   \table_row3{   <b>`ListItem.Year`</b>,
///                  \anchor ListItem_Year
///                  _string_,
///     Returns the year of the currently selected song\, album or movie in a
///     container
///   }
///   \table_row3{   <b>`ListItem.Premiered`</b>,
///                  \anchor ListItem_Premiered
///                  _string_,
///     Returns the release/aired date of the currently selected episode\, show\,
///     movie or EPG item in a container
///   }
///   \table_row3{   <b>`ListItem.Genre`</b>,
///                  \anchor ListItem_Genre
///                  _string_,
///     Returns the genre of the currently selected song\, album or movie in a
///     container
///   }
///   \table_row3{   <b>`ListItem.Contributors`</b>,
///                  \anchor ListItem_Contributors
///                  _string_,
///     List of all people who've contributed to the selected song
///   }
///   \table_row3{   <b>`ListItem.ContributorAndRole`</b>,
///                  \anchor ListItem_ContributorAndRole
///                  _string_,
///     List of all people and their role who've contributed to the selected song
///   }
///   \table_row3{   <b>`ListItem.Director`</b>,
///                  \anchor ListItem_Director
///                  _string_,
///     Returns the director of the currently selected movie in a container
///   }
///   \table_row3{   <b>`ListItem.Country`</b>,
///                  \anchor ListItem_Country
///                  _string_,
///     Returns the production country of the currently selected movie in a
///     container
///   }
///   \table_row3{   <b>`ListItem.Episode`</b>,
///                  \anchor ListItem_Episode
///                  _string_,
///     Returns the episode number value for the currently selected episode. It
///     also returns the number of total\, watched or unwatched episodes for the
///     currently selected tvshow or season\, based on the the current watched
///     filter.
///   }
///   \table_row3{   <b>`ListItem.Season`</b>,
///                  \anchor ListItem_Season
///                  _string_,
///     Returns the season value for the currently selected tvshow
///   }
///   \table_row3{   <b>`ListItem.TVShowTitle`</b>,
///                  \anchor ListItem_TVShowTitle
///                  _string_,
///     Returns the name value for the currently selected tvshow in the season and
///     episode depth of the video library
///   }
///   \table_row3{   <b>`ListItem.Property(TotalSeasons)`</b>,
///                  \anchor ListItem_Property_TotalSeasons
///                  _string_,
///     Returns the total number of seasons for the currently selected tvshow
///   }
///   \table_row3{   <b>`ListItem.Property(TotalEpisodes)`</b>,
///                  \anchor ListItem_Property_TotalEpisodes
///                  _string_,
///     Returns the total number of episodes for the currently selected tvshow or
///     season
///   }
///   \table_row3{   <b>`ListItem.Property(WatchedEpisodes)`</b>,
///                  \anchor ListItem_Property_WatchedEpisodes
///                  _string_,
///     Returns the number of watched episodes for the currently selected tvshow
///     or season
///   }
///   \table_row3{   <b>`ListItem.Property(UnWatchedEpisodes)`</b>,
///                  \anchor ListItem_Property_UnWatchedEpisodes
///                  _string_,
///     Returns the number of unwatched episodes for the currently selected tvshow
///     or season
///   }
///   \table_row3{   <b>`ListItem.Property(NumEpisodes)`</b>,
///                  \anchor ListItem_Property_NumEpisodes
///                  _string_,
///     Returns the number of total\, watched or unwatched episodes for the
///     currently selected tvshow or season\, based on the the current watched filter.
///   }
///   \table_row3{   <b>`ListItem.PictureAperture`</b>,
///                  \anchor ListItem_PictureAperture
///                  _string_,
///     Returns the F-stop used to take the selected picture. This is the value of the
///     EXIF FNumber tag (hex code 0x829D).
///   }
///   \table_row3{   <b>`ListItem.PictureAuthor`</b>,
///                  \anchor ListItem_PictureAuthor
///                  _string_,
///     Returns the name of the person involved in writing about the selected picture.
///     This is the value of the IPTC Writer tag (hex code 0x7A).
///   }
///   \table_row3{   <b>`ListItem.PictureByline`</b>,
///                  \anchor ListItem_PictureByline
///                  _string_,
///     Returns the name of the person who created the selected picture. This is
///     the value of the IPTC Byline tag (hex code 0x50).
///   }
///   \table_row3{   <b>`ListItem.PictureBylineTitle`</b>,
///                  \anchor ListItem_PictureBylineTitle
///                  _string_,
///     Returns the title of the person who created the selected picture. This is
///     the value of the IPTC BylineTitle tag (hex code 0x55).
///   }
///   \table_row3{   <b>`ListItem.PictureCamMake`</b>,
///                  \anchor ListItem_PictureCamMake
///                  _string_,
///     Returns the manufacturer of the camera used to take the selected picture.
///     This is the value of the EXIF Make tag (hex code 0x010F).
///   }
///   \table_row3{   <b>`ListItem.PictureCamModel`</b>,
///                  \anchor ListItem_PictureCamModel
///                  _string_,
///     Returns the manufacturer's model name or number of the camera used to take
///     the selected picture. This is the value of the EXIF Model tag (hex code
///     0x0110).
///   }
///   \table_row3{   <b>`ListItem.PictureCaption`</b>,
///                  \anchor ListItem_PictureCaption
///                  _string_,
///     Returns a description of the selected picture. This is the value of the IPTC
///     Caption tag (hex code 0x78).
///   }
///   \table_row3{   <b>`ListItem.PictureCategory`</b>,
///                  \anchor ListItem_PictureCategory
///                  _string_,
///     Returns the subject of the selected picture as a category code. This is the
///     value of the IPTC Category tag (hex code 0x0F).
///   }
///   \table_row3{   <b>`ListItem.PictureCCDWidth`</b>,
///                  \anchor ListItem_PictureCCDWidth
///                  _string_,
///     Returns the width of the CCD in the camera used to take the selected
///     picture. This is calculated from three EXIF tags (0xA002 * 0xA210
///     / 0xA20e).
///   }
///   \table_row3{   <b>`ListItem.PictureCity`</b>,
///                  \anchor ListItem_PictureCity
///                  _string_,
///     Returns the city where the selected picture was taken. This is the value of
///     the IPTC City tag (hex code 0x5A).
///   }
///   \table_row3{   <b>`ListItem.PictureColour`</b>,
///                  \anchor ListItem_PictureColour
///                  _string_,
///     Returns whether the selected picture is "Colour" or "Black and White".
///   }
///   \table_row3{   <b>`ListItem.PictureComment`</b>,
///                  \anchor ListItem_PictureComment
///                  _string_,
///     Returns a description of the selected picture. This is the value of the
///     EXIF User Comment tag (hex code 0x9286). This is the same value as
///     \ref Slideshow_SlideComment "Slideshow.SlideComment".
///   }
///   \table_row3{   <b>`ListItem.PictureCopyrightNotice`</b>,
///                  \anchor ListItem_PictureCopyrightNotice
///                  _string_,
///     Returns the copyright notice of the selected picture. This is the value of
///     the IPTC Copyright tag (hex code 0x74).
///   }
///   \table_row3{   <b>`ListItem.PictureCountry`</b>,
///                  \anchor ListItem_PictureCountry
///                  _string_,
///     Returns the full name of the country where the selected picture was taken.
///     This is the value of the IPTC CountryName tag (hex code 0x65).
///   }
///   \table_row3{   <b>`ListItem.PictureCountryCode`</b>,
///                  \anchor ListItem_PictureCountryCode
///                  _string_,
///     Returns the country code of the country where the selected picture was
///     taken. This is the value of the IPTC CountryCode tag (hex code 0x64).
///   }
///   \table_row3{   <b>`ListItem.PictureCredit`</b>,
///                  \anchor ListItem_PictureCredit
///                  _string_,
///     Returns who provided the selected picture. This is the value of the IPTC
///     Credit tag (hex code 0x6E).
///   }
///   \table_row3{   <b>`ListItem.PictureDate`</b>,
///                  \anchor ListItem_PictureDate
///                  _string_,
///     Returns the localized date of the selected picture. The short form of the
///     date is used. The value of the EXIF DateTimeOriginal tag (hex code 0x9003)
///     is preferred. If the DateTimeOriginal tag is not found\, the value of
///     DateTimeDigitized (hex code 0x9004) or of DateTime (hex code 0x0132) might
///     be used.
///   }
///   \table_row3{   <b>`ListItem.PictureDatetime`</b>,
///                  \anchor ListItem_PictureDatetime
///                  _string_,
///     Returns the date/timestamp of the selected picture. The localized short form
///     of the date and time is used. The value of the EXIF DateTimeOriginal tag
///     (hex code 0x9003) is preferred. If the DateTimeOriginal tag is not found\,
///     the value of DateTimeDigitized (hex code 0x9004) or of DateTime (hex code
///     0x0132) might be used.
///   }
///   \table_row3{   <b>`ListItem.PictureDesc`</b>,
///                  \anchor ListItem_PictureDesc
///                  _string_,
///     Returns a short description of the selected picture. The SlideComment\,
///     EXIFComment\, or Caption values might contain a longer description. This
///     is the value of the EXIF ImageDescription tag (hex code 0x010E).
///   }
///   \table_row3{   <b>`ListItem.PictureDigitalZoom`</b>,
///                  \anchor ListItem_PictureDigitalZoom
///                  _string_,
///     Returns the digital zoom ratio when the selected picture was taken. This
///     is the value of the EXIF DigitalZoomRatio tag (hex code 0xA404).
///   }
///   \table_row3{   <b>`ListItem.PictureExpMode`</b>,
///                  \anchor ListItem_PictureExpMode
///                  _string_,
///     Returns the exposure mode of the selected picture. The possible values are
///     "Automatic"\, "Manual"\, and "Auto bracketing". This is the value of the
///     EXIF ExposureMode tag (hex code 0xA402).
///   }
///   \table_row3{   <b>`ListItem.PictureExposure`</b>,
///                  \anchor ListItem_PictureExposure
///                  _string_,
///     Returns the class of the program used by the camera to set exposure when
///     the selected picture was taken. Values include "Manual"\, "Program
///     (Auto)"\, "Aperture priority (Semi-Auto)"\, "Shutter priority (semi-auto)"\,
///     etc. This is the value of the EXIF ExposureProgram tag (hex code 0x8822).
///   }
///   \table_row3{   <b>`ListItem.PictureExposureBias`</b>,
///                  \anchor ListItem_PictureExposureBias
///                  _string_,
///     Returns the exposure bias of the selected picture. Typically this is a
///     number between -99.99 and 99.99. This is the value of the EXIF
///     ExposureBiasValue tag (hex code 0x9204).
///   }
///   \table_row3{   <b>`ListItem.PictureExpTime`</b>,
///                  \anchor ListItem_PictureExpTime
///                  _string_,
///     Returns the exposure time of the selected picture\, in seconds. This is the
///     value of the EXIF ExposureTime tag (hex code 0x829A). If the ExposureTime
///     tag is not found\, the ShutterSpeedValue tag (hex code 0x9201) might be
///     used.
///   }
///   \table_row3{   <b>`ListItem.PictureFlashUsed`</b>,
///                  \anchor ListItem_PictureFlashUsed
///                  _string_,
///     Returns the status of flash when the selected picture was taken. The value
///     will be either "Yes" or "No"\, and might include additional information.
///     This is the value of the EXIF Flash tag (hex code 0x9209).
///   }
///   \table_row3{   <b>`ListItem.PictureFocalLen`</b>,
///                  \anchor ListItem_PictureFocalLen
///                  _string_,
///     Returns the lens focal length of the selected picture
///   }
///   \table_row3{   <b>`ListItem.PictureFocusDist`</b>,
///                  \anchor ListItem_PictureFocusDist
///                  _string_,
///     Returns the focal length of the lens\, in mm. This is the value of the EXIF
///     FocalLength tag (hex code 0x920A).
///   }
///   \table_row3{   <b>`ListItem.PictureGPSLat`</b>,
///                  \anchor ListItem_PictureGPSLat
///                  _string_,
///     Returns the latitude where the selected picture was taken (degrees\,
///     minutes\, seconds North or South). This is the value of the EXIF
///     GPSInfo.GPSLatitude and GPSInfo.GPSLatitudeRef tags.
///   }
///   \table_row3{   <b>`ListItem.PictureGPSLon`</b>,
///                  \anchor ListItem_PictureGPSLon
///                  _string_,
///     Returns the longitude where the selected picture was taken (degrees\,
///     minutes\, seconds East or West). This is the value of the EXIF
///     GPSInfo.GPSLongitude and GPSInfo.GPSLongitudeRef tags.
///   }
///   \table_row3{   <b>`ListItem.PictureGPSAlt`</b>,
///                  \anchor ListItem_PictureGPSAlt
///                  _string_,
///     Returns the altitude in meters where the selected picture was taken. This
///     is the value of the EXIF GPSInfo.GPSAltitude tag.
///   }
///   \table_row3{   <b>`ListItem.PictureHeadline`</b>,
///                  \anchor ListItem_PictureHeadline
///                  _string_,
///     Returns a synopsis of the contents of the selected picture. This is the
///     value of the IPTC Headline tag (hex code 0x69).
///   }
///   \table_row3{   <b>`ListItem.PictureImageType`</b>,
///                  \anchor ListItem_PictureImageType
///                  _string_,
///     Returns the color components of the selected picture. This is the value of
///     the IPTC ImageType tag (hex code 0x82).
///   }
///   \table_row3{   <b>`ListItem.PictureIPTCDate`</b>,
///                  \anchor ListItem_PictureIPTCDate
///                  _string_,
///     Returns the date when the intellectual content of the selected picture was
///     created\, rather than when the picture was created. This is the value of
///     the IPTC DateCreated tag (hex code 0x37).
///   }
///   \table_row3{   <b>`ListItem.PictureIPTCTime`</b>,
///                  \anchor ListItem_PictureIPTCTime
///                  _string_,
///     Returns the time when the intellectual content of the selected picture was
///     created\, rather than when the picture was created. This is the value of
///     the IPTC TimeCreated tag (hex code 0x3C).
///   }
///   \table_row3{   <b>`ListItem.PictureISO`</b>,
///                  \anchor ListItem_PictureISO
///                  _string_,
///     Returns the ISO speed of the camera when the selected picture was taken.
///     This is the value of the EXIF ISOSpeedRatings tag (hex code 0x8827).
///   }
///   \table_row3{   <b>`ListItem.PictureKeywords`</b>,
///                  \anchor ListItem_PictureKeywords
///                  _string_,
///     Returns keywords assigned to the selected picture. This is the value of
///     the IPTC Keywords tag (hex code 0x19).
///   }
///   \table_row3{   <b>`ListItem.PictureLightSource`</b>,
///                  \anchor ListItem_PictureLightSource
///                  _string_,
///     Returns the kind of light source when the picture was taken. Possible
///     values include "Daylight"\, "Fluorescent"\, "Incandescent"\, etc. This is
///     the value of the EXIF LightSource tag (hex code 0x9208).
///   }
///   \table_row3{   <b>`ListItem.PictureLongDate`</b>,
///                  \anchor ListItem_PictureLongDate
///                  _string_,
///     Returns only the localized date of the selected picture. The long form of
///     the date is used. The value of the EXIF DateTimeOriginal tag (hex code
///     0x9003) is preferred. If the DateTimeOriginal tag is not found\, the
///     value of DateTimeDigitized (hex code 0x9004) or of DateTime (hex code
///     0x0132) might be used.
///   }
///   \table_row3{   <b>`ListItem.PictureLongDatetime`</b>,
///                  \anchor ListItem_PictureLongDatetime
///                  _string_,
///     Returns the date/timestamp of the selected picture. The localized long
///     form of the date and time is used. The value of the EXIF DateTimeOriginal
///     tag (hex code 0x9003) is preferred. if the DateTimeOriginal tag is not
///     found\, the value of DateTimeDigitized (hex code 0x9004) or of DateTime
///     (hex code 0x0132) might be used.
///   }
///   \table_row3{   <b>`ListItem.PictureMeteringMode`</b>,
///                  \anchor ListItem_PictureMeteringMode
///                  _string_,
///     Returns the metering mode used when the selected picture was taken. The
///     possible values are "Center weight"\, "Spot"\, or "Matrix". This is the
///     value of the EXIF MeteringMode tag (hex code 0x9207).
///   }
///   \table_row3{   <b>`ListItem.PictureObjectName`</b>,
///                  \anchor ListItem_PictureObjectName
///                  _string_,
///     Returns a shorthand reference for the selected picture. This is the value
///     of the IPTC ObjectName tag (hex code 0x05).
///   }
///   \table_row3{   <b>`ListItem.PictureOrientation`</b>,
///                  \anchor ListItem_PictureOrientation
///                  _string_,
///     Returns the orientation of the selected picture. Possible values are "Top
///     Left"\, "Top Right"\, "Left Top"\, "Right Bottom"\, etc. This is the value
///     of the EXIF Orientation tag (hex code 0x0112).
///   }
///   \table_row3{   <b>`ListItem.PicturePath`</b>,
///                  \anchor ListItem_PicturePath
///                  _string_,
///     Returns the filename and path of the selected picture
///   }
///   \table_row3{   <b>`ListItem.PictureProcess`</b>,
///                  \anchor ListItem_PictureProcess
///                  _string_,
///     Returns the process used to compress the selected picture
///   }
///   \table_row3{   <b>`ListItem.PictureReferenceService`</b>,
///                  \anchor ListItem_PictureReferenceService
///                  _string_,
///     Returns the Service Identifier of a prior envelope to which the selected
///     picture refers. This is the value of the IPTC ReferenceService tag
///     (hex code 0x2D).
///   }
///   \table_row3{   <b>`ListItem.PictureResolution`</b>,
///                  \anchor ListItem_PictureResolution
///                  _string_,
///     Returns the dimensions of the selected picture
///   }
///   \table_row3{   <b>`ListItem.PictureSource`</b>,
///                  \anchor ListItem_PictureSource
///                  _string_,
///     Returns the original owner of the selected picture. This is the value of
///     the IPTC Source tag (hex code 0x73).
///   }
///   \table_row3{   <b>`ListItem.PictureSpecialInstructions`</b>,
///                  \anchor ListItem_PictureSpecialInstructions
///                  _string_,
///     Returns other editorial instructions concerning the use of the selected
///     picture. This is the value of the IPTC SpecialInstructions tag (hex
///     code 0x28).
///   }
///   \table_row3{   <b>`ListItem.PictureState`</b>,
///                  \anchor ListItem_PictureState
///                  _string_,
///     Returns the State/Province where the selected picture was taken. This is
///     the value of the IPTC ProvinceState tag (hex code 0x5F).
///   }
///   \table_row3{   <b>`ListItem.PictureSublocation`</b>,
///                  \anchor ListItem_PictureSublocation
///                  _string_,
///     Returns the location within a city where the selected picture was taken -
///     might indicate the nearest landmark. This is the value of the IPTC
///     SubLocation tag (hex code 0x5C).
///   }
///   \table_row3{   <b>`ListItem.PictureSupplementalCategories`</b>,
///                  \anchor ListItem_PictureSupplementalCategories
///                  _string_,
///     Returns supplemental category codes to further refine the subject of the
///     selected picture. This is the value of the IPTC SuppCategory tag (hex
///     code 0x14).
///   }
///   \table_row3{   <b>`ListItem.PictureTransmissionReference`</b>,
///                  \anchor ListItem_PictureTransmissionReference
///                  _string_,
///     Returns a code representing the location of original transmission of the
///     selected picture. This is the value of the IPTC TransmissionReference
///     tag (hex code 0x67).
///   }
///   \table_row3{   <b>`ListItem.PictureUrgency`</b>,
///                  \anchor ListItem_PictureUrgency
///                  _string_,
///     Returns the urgency of the selected picture. Values are 1-9. The "1" is
///     most urgent. Some image management programs use urgency to indicate
///     picture rating\, where urgency "1" is 5 stars and urgency "5" is 1 star.
///     Urgencies 6-9 are not used for rating. This is the value of the IPTC
///     Urgency tag (hex code 0x0A).
///   }
///   \table_row3{   <b>`ListItem.PictureWhiteBalance`</b>,
///                  \anchor ListItem_PictureWhiteBalance
///                  _string_,
///     Returns the white balance mode set when the selected picture was taken.
///     The possible values are "Manual" and "Auto". This is the value of the
///     EXIF WhiteBalance tag (hex code 0xA403).
///   }
///   \table_row3{   <b>`ListItem.FileName`</b>,
///                  \anchor ListItem_FileName
///                  _string_,
///     Returns the filename of the currently selected song or movie in a container
///   }
///   \table_row3{   <b>`ListItem.Path`</b>,
///                  \anchor ListItem_Path
///                  _string_,
///     Returns the complete path of the currently selected song or movie in a
///     container
///   }
///   \table_row3{   <b>`ListItem.FolderName`</b>,
///                  \anchor ListItem_FolderName
///                  _string_,
///     Returns top most folder of the path of the currently selected song or
///     movie in a container
///   }
///   \table_row3{   <b>`ListItem.FolderPath`</b>,
///                  \anchor ListItem_FolderPath
///                  _string_,
///     Returns the complete path of the currently selected song or movie in a
///     container (without user details).
///   }
///   \table_row3{   <b>`ListItem.FileNameAndPath`</b>,
///                  \anchor ListItem_FileNameAndPath
///                  _string_,
///     Returns the full path with filename of the currently selected song or
///     movie in a container
///   }
///   \table_row3{   <b>`ListItem.FileExtension`</b>,
///                  \anchor ListItem_FileExtension
///                  _string_,
///     Returns the file extension (without leading dot) of the currently selected
///     item in a container
///   }
///   \table_row3{   <b>`ListItem.Date`</b>,
///                  \anchor ListItem_Date
///                  _string_,
///     Returns the file date of the currently selected song or movie in a
///     container / Aired date of an episode / Day\, start time and end time of
///     current selected TV programme (PVR)
///   }
///   \table_row3{   <b>`ListItem.DateAdded`</b>,
///                  \anchor ListItem_DateAdded
///                  _string_,
///     Returns the date the currently selected item was added to the
///     library / Date and time of an event in the EventLog window.
///   }
///   \table_row3{   <b>`ListItem.Size`</b>,
///                  \anchor ListItem_Size
///                  _string_,
///     Returns the file size of the currently selected song or movie in a
///     container
///   }
///   \table_row3{   <b>`ListItem.Rating`</b>,
///                  \anchor ListItem_Rating
///                  _string_,
///     Returns the scraped rating of the currently selected movie in a container
///   }
///   \table_row3{   <b>`ListItem.Set`</b>,
///                  \anchor ListItem_Set
///                  _string_,
///     Returns the name of the set the movie is part of
///   }
///   \table_row3{   <b>`ListItem.SetId`</b>,
///                  \anchor ListItem_SetId
///                  _string_,
///     Returns the id of the set the movie is part of
///   }
///   \table_row3{   <b>`ListItem.UserRating`</b>,
///                  \anchor ListItem_UserRating
///                  _string_,
///     Returns the user rating of the currently selected item in a container
///   }
///   \table_row3{   <b>`ListItem.Votes`</b>,
///                  \anchor ListItem_Votes
///                  _string_,
///     Returns the scraped votes of the currently selected movie in a container
///   }
///   \table_row3{   <b>`ListItem.RatingAndVotes`</b>,
///                  \anchor ListItem_RatingAndVotes
///                  _string_,
///     Returns the scraped rating and votes of the currently selected movie in a
///     container
///   }
///   \table_row3{   <b>`ListItem.Mood`</b>,
///                  \anchor ListItem_Mood
///                  _string_,
///     Mood of the selected song
///   }
///   \table_row3{   <b>`ListItem.Mpaa`</b>,
///                  \anchor ListItem_Mpaa
///                  _string_,
///     Show the MPAA rating of the currently selected movie in a container
///   }
///   \table_row3{   <b>`ListItem.ProgramCount`</b>,
///                  \anchor ListItem_ProgramCount
///                  _string_,
///     Returns the number of times an xbe has been run from "my programs"
///   }
///   \table_row3{   <b>`ListItem.Duration`</b>,
///                  \anchor ListItem_Duration
///                  _string_,
///     Returns the duration of the currently selected item in a container
///     in the format hh:mm:ss. hh: will be omitted if hours value is zero.
///   }
///   \table_row3{   <b>`ListItem.Duration(format)`</b>,
///                  \anchor ListItem.Duration_format
///                  _string_,
///     Returns the duration of the currently selected item in a container in
///     different formats: hours (hh)\, minutes (mm) or seconds (ss).
///     Also supported: (hh:mm)\, (mm:ss)\, (hh:mm:ss)\, (h:mm:ss).
///     Added with Leia: (secs)\, (mins)\, (hours) for total time values and (m).
///     Example: 3661 seconds => h=1\, hh=01\, m=1\, mm=01\, ss=01\, hours=1\, mins=61\, secs=3661
///   }
///   \table_row3{   <b>`ListItem.DBTYPE`</b>,
///                  \anchor ListItem_DBTYPE
///                  _string_,
///     Returns the database type of the ListItem.DBID for videos (movie\, set\,
///     genre\, actor\, tvshow\, season\, episode). It does not return any value
///     for the music library. Beware with season\, the "*all seasons" entry does
///     give a DBTYPE "season" and a DBID\, but you can't get the details of that
///     entry since it's a virtual entry in the Video Library.
///   }
///   \table_row3{   <b>`ListItem.DBID`</b>,
///                  \anchor ListItem_DBID
///                  _string_,
///     Returns the database id of the currently selected listitem in a container
///   }
///   \table_row3{   <b>`ListItem.Cast`</b>,
///                  \anchor ListItem_Cast
///                  _string_,
///     Returns a concatenated string of cast members of the currently selected
///     movie\, for use in dialogvideoinfo.xml
///   }
///   \table_row3{   <b>`ListItem.CastAndRole`</b>,
///                  \anchor ListItem_CastAndRole
///                  _string_,
///     Returns a concatenated string of cast members and roles of the currently
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
///     Returns the IMDb top250 position of the currently selected listitem in a
///     container.
///   }
///   \table_row3{   <b>`ListItem.Trailer`</b>,
///                  \anchor ListItem_Trailer
///                  _string_,
///     Returns the full trailer path with filename of the currently selected
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
///     The IMDb ID of the selected Video in a container
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
///   \table_row3{   <b>`ListItem.ChannelName`</b>,
///                  \anchor ListItem_ChannelName
///                  _string_,
///     Name of current selected TV channel in a container
///   }
///   \table_row3{   <b>`ListItem.VideoCodec`</b>,
///                  \anchor ListItem_VideoCodec
///                  _string_,
///     Returns the video codec of the currently selected video (common values:
///     3iv2\, avc1\, div2\, div3\, divx\, divx 4\, dx50\, flv\, h264\, microsoft\, mp42\,
///     mp43\, mp4v\, mpeg1video\, mpeg2video\, mpg4\, rv40\, svq1\, svq3\,
///     theora\, vp6f\, wmv2\, wmv3\, wvc1\, xvid)
///   }
///   \table_row3{   <b>`ListItem.VideoResolution`</b>,
///                  \anchor ListItem_VideoResolution
///                  _string_,
///     Returns the resolution of the currently selected video (possible values:
///     480\, 576\, 540\, 720\, 1080\, 4K). Note that 540 usually means a widescreen
///     format (around 960x540) while 576 means PAL resolutions (normally
///     720x576)\, therefore 540 is actually better resolution than 576.
///   }
///   \table_row3{   <b>`ListItem.VideoAspect`</b>,
///                  \anchor ListItem_VideoAspect
///                  _string_,
///     Returns the aspect ratio of the currently selected video (possible values:
///     1.33\, 1.37\, 1.66\, 1.78\, 1.85\, 2.20\, 2.35\, 2.40\, 2.55\, 2.76)
///   }
///   \table_row3{   <b>`ListItem.AudioCodec`</b>,
///                  \anchor ListItem_AudioCodec
///                  _string_,
///     Returns the audio codec of the currently selected video (common values:
///     aac\, ac3\, cook\, dca\, dtshd_hra\, dtshd_ma\, eac3\, mp1\, mp2\, mp3\, pcm_s16be\, pcm_s16le\, pcm_u8\, truehd\, vorbis\, wmapro\, wmav2)
///   }
///   \table_row3{   <b>`ListItem.AudioChannels`</b>,
///                  \anchor ListItem_AudioChannels
///                  _string_,
///     Returns the number of audio channels of the currently selected video
///     (possible values: 1\, 2\, 4\, 5\, 6\, 8\, 10)
///   }
///   \table_row3{   <b>`ListItem.AudioLanguage`</b>,
///                  \anchor ListItem_AudioLanguage
///                  _string_,
///     Returns the audio language of the currently selected video (returns an
///     ISO 639-2 three character code\, e.g. eng\, epo\, deu)
///   }
///   \table_row3{   <b>`ListItem.SubtitleLanguage`</b>,
///                  \anchor ListItem_SubtitleLanguage
///                  _string_,
///     Returns the subtitle language of the currently selected video (returns an
///     ISO 639-2 three character code\, e.g. eng\, epo\, deu)
///   }
///   \table_row3{   <b>`ListItem.Property(AudioCodec.[n])`</b>,
///                  \anchor ListItem_Property_AudioCodec
///                  _string_,
///     Returns the audio codec of the currently selected video\, 'n' defines the
///     number of the audiostream (values: see \ref ListItem_AudioCodec "ListItem.AudioCodec")
///   }
///   \table_row3{   <b>`ListItem.Property(AudioChannels.[n])`</b>,
///                  \anchor ListItem_Property_AudioChannels
///                  _string_,
///     Returns the number of audio channels of the currently selected video\, 'n'
///     defines the number of the audiostream (values: see
///     \ref ListItem_AudioChannels "ListItem.AudioChannels")
///   }
///   \table_row3{   <b>`ListItem.Property(AudioLanguage.[n])`</b>,
///                  \anchor ListItem_Property_AudioLanguage
///                  _string_,
///     Returns the audio language of the currently selected video\, 'n' defines
///     the number of the audiostream (values: see \ref ListItem_AudioLanguage "ListItem.AudioLanguage")
///   }
///   \table_row3{   <b>`ListItem.Property(SubtitleLanguage.[n])`</b>,
///                  \anchor ListItem_Property_SubtitleLanguage
///                  _string_,
///     Returns the subtitle language of the currently selected video\, 'n' defines
///     the number of the subtitle (values: see \ref ListItem_SubtitleLanguage "ListItem.SubtitleLanguage")
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Name)`</b>,
///                  \anchor ListItem_Property_AddonName
///                  _string_,
///     Returns the name of the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Version)`</b>,
///                  \anchor ListItem_Property_AddonVersion
///                  _string_,
///     Returns the version of the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Summary)`</b>,
///                  \anchor ListItem_Property_AddonSummary
///                  _string_,
///     Returns a short description of the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Description)`</b>,
///                  \anchor ListItem_Property_AddonDescription
///                  _string_,
///     Returns the full description of the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Type)`</b>,
///                  \anchor ListItem_Property_AddonType
///                  _string_,
///     Returns the type (screensaver\, script\, skin\, etc...) of the currently
///     selected addon
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Creator)`</b>,
///                  \anchor ListItem_Property_AddonCreator
///                  _string_,
///     Returns the name of the author the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Disclaimer)`</b>,
///                  \anchor ListItem_Property_AddonDisclaimer
///                  _string_,
///     Returns the disclaimer of the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Changelog)`</b>,
///                  \anchor ListItem_Property_AddonChangelog
///                  _string_,
///     Returns the changelog of the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.ID)`</b>,
///                  \anchor ListItem_Property_AddonID
///                  _string_,
///     Returns the identifier of the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Status)`</b>,
///                  \anchor ListItem_Property_AddonStatus
///                  _string_,
///     Returns the status of the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Broken)`</b>,
///                  \anchor ListItem_Property_AddonBroken
///                  _string_,
///     Returns a message when the addon is marked as broken in the repo
///   }
///   \table_row3{   <b>`ListItem.Property(Addon.Path)`</b>,
///                  \anchor ListItem_Property_AddonPath
///                  _string_,
///     Returns the path of the currently selected addon
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
///     Returns true if a given epg tag item currently gets recorded or has been recorded
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
///     Returns the type of the PVR timer / timer rule item as a human readable string
///   }
///   \table_row3{   <b>`ListItem.EpgEventTitle`</b>,
///                  \anchor ListItem_EpgEventTitle
///                  _string_,
///     Returns the title of the epg event associated with the item\, if any
///   }
///   \table_row3{   <b>`ListItem.EpgEventIcon`</b>,
///                  \anchor ListItem_EpgEventIcon
///                  _string_,
///     Returns the thumbnail for the epg event associated with the item (if it exists)
///   }
///   \table_row3{   <b>`ListItem.InProgress`</b>,
///                  \anchor ListItem_InProgress
///                  _boolean_,
///     Returns true if the epg event item is currently active (time-wise)
///   }
///   \table_row3{   <b>`ListItem.IsParentFolder`</b>,
///                  \anchor ListItem_IsParentFolder
///                  _boolean_,
///     Returns true if the current list item is the goto parent folder '..'
///   }
///   \table_row3{   <b>`ListItem.AddonName`</b>,
///                  \anchor ListItem_AddonName
///                  _string_,
///     Returns the name of the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.AddonVersion`</b>,
///                  \anchor ListItem_AddonVersion
///                  _string_,
///     Returns the version of the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.AddonCreator`</b>,
///                  \anchor ListItem_AddonCreator
///                  _string_,
///     Returns the name of the author the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.AddonSummary`</b>,
///                  \anchor ListItem_AddonSummary
///                  _string_,
///     Returns a short description of the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.AddonDescription`</b>,
///                  \anchor ListItem_AddonDescription
///                  _string_,
///     Returns the full description of the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.AddonDisclaimer`</b>,
///                  \anchor ListItem_AddonDisclaimer
///                  _string_,
///     Returns the disclaimer of the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.AddonBroken`</b>,
///                  \anchor ListItem_AddonBroken
///                  _string_,
///     Returns a message when the addon is marked as broken in the repo
///   }
///   \table_row3{   <b>`ListItem.AddonType`</b>,
///                  \anchor ListItem_AddonType
///                  _string_,
///     Returns the type (screensaver\, script\, skin\, etc...) of the currently selected addon
///   }
///   \table_row3{   <b>`ListItem.AddonInstallDate`</b>,
///                  \anchor ListItem_AddonInstallDate
///                  _string_,
///     Returns the date the addon was installed
///   }
///   \table_row3{   <b>`ListItem.AddonLastUpdated`</b>,
///                  \anchor ListItem_AddonLastUpdated
///                  _string_,
///     Returns the date the addon was last updated
///   }
///   \table_row3{   <b>`ListItem.AddonLastUsed`</b>,
///                  \anchor ListItem_AddonLastUsed
///                  _string_,
///     Returns the date the addon was used last
///   }
//    \table_row3{   <b>`ListItem.AddonOrigin`</b>,
///                  \anchor ListItem_AddonOrigin
///                  _string_,
///     Name of the repository the add-on originates from.
///   }
///   \table_row3{   <b>`ListItem.ExpirationDate`</b>,
///                  \anchor ListItem_ExpirationDate
///                  _string_,
///     Expiration date of the selected item in a container\, empty string if not supported
///   }
///   \table_row3{   <b>`ListItem.ExpirationTime`</b>,
///                  \anchor ListItem_ExpirationTime
///                  _string_,
///     Expiration time of the selected item in a container\, empty string if not supported
///   }
///   \table_row3{   <b>`ListItem.Art(type)`</b>,
///                  \anchor ListItem_Art_Type
///                  _string_,
///     Get a particular art type for an item.
///   }
///   \table_row3{   <b>`ListItem.Property(propname)`</b>,
///                  \anchor ListItem_Property_Propname
///                  _string_,
///     Get a property of an item.
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
                                  { "channelnumberlabel", LISTITEM_CHANNEL_NUMBER },
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
                                  { "appearances",      LISTITEM_APPEARANCES },
                                  { "stereoscopicmode", LISTITEM_STEREOSCOPIC_MODE },
                                  { "isstereoscopic",   LISTITEM_IS_STEREOSCOPIC },
                                  { "imdbnumber",       LISTITEM_IMDBNUMBER },
                                  { "episodename",      LISTITEM_EPISODENAME },
                                  { "timertype",        LISTITEM_TIMERTYPE },
                                  { "epgeventtitle",    LISTITEM_EPG_EVENT_TITLE },
                                  { "epgeventicon",     LISTITEM_EPG_EVENT_ICON },
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
                                  { "expirationdate",   LISTITEM_EXPIRATION_DATE },
                                  { "expirationtime",   LISTITEM_EXPIRATION_TIME },
                                  { "art",              LISTITEM_ART },
                                  { "property",         LISTITEM_PROPERTY },
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
///     Returns the current preset of the visualisation.
///   }
///   \table_row3{   <b>`Visualisation.Name`</b>,
///                  \anchor Visualisation_Name
///                  _string_,
///     Returns the name of the visualisation.
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
///     Returns the fanart image\, if any
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
///                  _string_,
///     Returns the current selected skin theme.
///   }
///   \table_row3{   <b>`Skin.CurrentColourTheme`</b>,
///                  \anchor Skin_CurrentColourTheme
///                  _string_,
///     Returns the current selected colour theme of the skin.
///   }
///   \table_row3{   <b>`Skin.AspectRatio`</b>,
///                  \anchor Skin_AspectRatio
///                  _string_,
///     Returns the closest aspect ratio match using the resolution info from the skin's addon.xml file.
///   }
///   \table_row3{   <b>`Skin.Font`</b>,
///                  \anchor Skin_Font
///                  _string_,
///     Returns the current fontset from Font.xml.
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}
const infomap skin_labels[] =    {{ "currenttheme",      SKIN_THEME },
                                  { "currentcolourtheme",SKIN_COLOUR_THEME },
                                  { "aspectratio",       SKIN_ASPECT_RATIO},
                                  { "font",              SKIN_FONT}};

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
///     Returns true if the window with the given name is the window which is currently rendered.
///     Useful in xml files that are shared between multiple windows or dialogs.
///   }
///   \table_row3{   <b>`Window.IsActive(window)`</b>,
///                  \anchor Window_IsActive
///                  _boolean_,
///     Returns true if the window with id or title _window_ is active
///     (excludes fade out time on dialogs)
///   }
///   \table_row3{   <b>`Window.IsVisible(window)`</b>,
///                  \anchor Window_IsVisible
///                  _boolean_,
///     Returns true if the window is visible (includes fade out time on dialogs)
///   }
///   \table_row3{   <b>`Window.IsTopmost(window)`</b>,
///                  \anchor Window_IsTopmost
///                  _boolean_,
///     Returns true if the window with id or title _window_ is on top of the
///     window stack (excludes fade out time on dialogs)
///     @deprecated use `Window.IsDialogTopmost(dialog)` instead \par
///   }
///   \table_row3{   <b>`Window.IsDialogTopmost(dialog)`</b>,
///                  \anchor Window_IsDialogTopmost
///                  _boolean_,
///     Returns true if the dialog with id or title _dialog_ is on top of the
///     dialog stack (excludes fade out time on dialogs)
///   }
///   \table_row3{   <b>`Window.IsModalDialogTopmost(dialog)`</b>,
///                  \anchor Window_IsModalDialogTopmost
///                  _boolean_,
///     Returns true if the dialog with id or title _dialog_ is on top of the
///     modal dialog stack (excludes fade out time on dialogs)
///   }
///   \table_row3{   <b>`Window.Previous(window)`</b>,
///                  \anchor Window_Previous
///                  _boolean_,
///     Returns true if the window with id or title _window_ is being moved from.
///     Only valid while windows are changing.
///   }
///   \table_row3{   <b>`Window.Next(window)`</b>,
///                  \anchor Window_Next
///                  _boolean_,
///     Returns true if the window with id or title _window_ is being moved to.
///     Only valid while windows are changing.
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}
const infomap window_bools[] =   {{ "ismedia",          WINDOW_IS_MEDIA },
                                  { "is",               WINDOW_IS },
                                  { "isactive",         WINDOW_IS_ACTIVE },
                                  { "isvisible",        WINDOW_IS_VISIBLE },
                                  { "istopmost",        WINDOW_IS_DIALOG_TOPMOST }, // deprecated, remove in v19
                                  { "isdialogtopmost",  WINDOW_IS_DIALOG_TOPMOST },
                                  { "ismodaldialogtopmost", WINDOW_IS_MODAL_DIALOG_TOPMOST },
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
///   \table_row3{   <b>`PVR.IsRecording`</b>,
///                  \anchor PVR_IsRecording
///                  _boolean_,
///     Returns true when the system is recording a tv or radio programme.
///   }
///   \table_row3{   <b>`PVR.HasTimer`</b>,
///                  \anchor PVR_HasTimer
///                  _boolean_,
///     Returns true when a recording timer is active.
///   }
///   \table_row3{   <b>`PVR.HasTVChannels`</b>,
///                  \anchor PVR_HasTVChannels
///                  _boolean_,
///     Returns true if there are TV channels available
///   }
///   \table_row3{   <b>`PVR.HasRadioChannels`</b>,
///                  \anchor PVR_HasRadioChannels
///                  _boolean_,
///     Returns true if there are radio channels available
///   }
///   \table_row3{   <b>`PVR.HasNonRecordingTimer`</b>,
///                  \anchor PVR_HasNonRecordingTimer
///                  _boolean_,
///     Returns true if there are timers present who currently not do recording
///   }
///   \table_row3{   <b>`PVR.BackendName`</b>,
///                  \anchor PVR_BackendName
///                  _string_,
///     Name of the backend being used
///   }
///   \table_row3{   <b>`PVR.BackendVersion`</b>,
///                  \anchor PVR_BackendVersion
///                  _string_,
///     Version of the backend that's being used
///   }
///   \table_row3{   <b>`PVR.BackendHost`</b>,
///                  \anchor PVR_BackendHost
///                  _string_,
///     Backend hostname
///   }
///   \table_row3{   <b>`PVR.BackendDiskSpace`</b>,
///                  \anchor PVR_BackendDiskSpace
///                  _string_,
///     Available diskspace on the backend as string with size
///   }
///   \table_row3{   <b>`PVR.BackendDiskSpaceProgr`</b>,
///                  \anchor PVR_BackendDiskSpaceProgr
///                  _integer_,
///     Available diskspace on the backend as percent value
///   }
///   \table_row3{   <b>`PVR.BackendChannels`</b>,
///                  \anchor PVR_BackendChannels
///                  _string (integer)_,
///     Number of available channels the backend provides
///   }
///   \table_row3{   <b>`PVR.BackendTimers`</b>,
///                  \anchor PVR_BackendTimers
///                  _string (integer)_,
///     Number of timers set for the backend
///   }
///   \table_row3{   <b>`PVR.BackendRecordings`</b>,
///                  \anchor PVR_BackendRecordings
///                  _string (integer)_,
///     Number of recording available on the backend
///   }
///   \table_row3{   <b>`PVR.BackendDeletedRecordings`</b>,
///                  \anchor PVR_BackendDeletedRecordings
///                  _string (integer)_,
///     Number of deleted recording present on the backend
///   }
///   \table_row3{   <b>`PVR.BackendNumber`</b>,
///                  \anchor PVR_BackendNumber
///                  _string_,
///     Backend number
///   }
///   \table_row3{   <b>`PVR.TotalDiscSpace`</b>,
///                  \anchor PVR_TotalDiscSpace
///                  _string_,
///     Total diskspace available for recordings
///   }
///   \table_row3{   <b>`PVR.NextTimer`</b>,
///                  \anchor PVR_NextTimer
///                  _boolean_,
///     Next timer date
///   }
///   \table_row3{   <b>`PVR.IsPlayingTV`</b>,
///                  \anchor PVR_IsPlayingTV
///                  _boolean_,
///     Returns true when live tv is being watched.
///   }
///   \table_row3{   <b>`PVR.IsPlayingRadio`</b>,
///                  \anchor PVR_IsPlayingRadio
///                  _boolean_,
///     Returns true when live radio is being listened to.
///   }
///   \table_row3{   <b>`PVR.IsPlayingRecording`</b>,
///                  \anchor PVR_IsPlayingRecording
///                  _boolean_,
///     Returns true when a recording is being watched.
///   }
///   \table_row3{   <b>`PVR.IsPlayingEpgTag`</b>,
///                  \anchor PVR_IsPlayingEpgTag
///                  _boolean_,
///     Returns true when an epg tag is being watched.
///   }
///   \table_row3{   <b>`PVR.EpgEventProgress`</b>,
///                  \anchor PVR_EpgEventProgress
///                  _integer_,
///     Returns the percentage complete of the currently playing epg event
///   }
///   \table_row3{   <b>`PVR.ActStreamClient`</b>,
///                  \anchor PVR_ActStreamClient
///                  _string_,
///     Stream client name
///   }
///   \table_row3{   <b>`PVR.ActStreamDevice`</b>,
///                  \anchor PVR_ActStreamDevice
///                  _string_,
///     Stream device name
///   }
///   \table_row3{   <b>`PVR.ActStreamStatus`</b>,
///                  \anchor PVR_ActStreamStatus
///                  _string_,
///     Status of the stream
///   }
///   \table_row3{   <b>`PVR.ActStreamSignal`</b>,
///                  \anchor PVR_ActStreamSignal
///                  _string_,
///     Signal quality of the stream
///   }
///   \table_row3{   <b>`PVR.ActStreamSnr`</b>,
///                  \anchor PVR_ActStreamSnr
///                  _string_,
///     Signal to noise ratio of the stream
///   }
///   \table_row3{   <b>`PVR.ActStreamBer`</b>,
///                  \anchor PVR_ActStreamBer
///                  _string_,
///     Bit error rate of the stream
///   }
///   \table_row3{   <b>`PVR.ActStreamUnc`</b>,
///                  \anchor PVR_ActStreamUnc
///                  _string_,
///     UNC value of the stream
///   }
///   \table_row3{   <b>`PVR.ActStreamProgrSignal`</b>,
///                  \anchor PVR_ActStreamProgrSignal
///                  _integer_,
///     Signal quality of the programme
///   }
///   \table_row3{   <b>`PVR.ActStreamProgrSnr`</b>,
///                  \anchor PVR_ActStreamProgrSnr
///                  _integer_,
///     Signal to noise ratio of the programme
///   }
///   \table_row3{   <b>`PVR.ActStreamIsEncrypted`</b>,
///                  \anchor PVR_ActStreamIsEncrypted
///                  _boolean_,
///     Returns true when channel is encrypted on source
///   }
///   \table_row3{   <b>`PVR.ActStreamEncryptionName`</b>,
///                  \anchor PVR_ActStreamEncryptionName
///                  _string_,
///     Encryption used on the stream
///   }
///   \table_row3{   <b>`PVR.ActStreamServiceName`</b>,
///                  \anchor PVR_ActStreamServiceName
///                  _string_,
///     Returns the service name of played channel if available
///   }
///   \table_row3{   <b>`PVR.ActStreamMux`</b>,
///                  \anchor PVR_ActStreamMux
///                  _string_,
///     Returns the multiplex type of played channel if available
///   }
///   \table_row3{   <b>`PVR.ActStreamProviderName`</b>,
///                  \anchor PVR_ActStreamProviderName
///                  _string_,
///     Returns the provider name of the played channel if available
///   }
///   \table_row3{   <b>`PVR.IsTimeShift`</b>,
///                  \anchor PVR_IsTimeShift
///                  _boolean_,
///     Returns true when for channel is timeshift available
///   }
///   \table_row3{   <b>`PVR.TimeShiftProgress`</b>,
///                  \anchor PVR_TimeShiftProgress
///                  _integer_,
///     Returns the position of currently timeshifted title on TV as integer
///   }
///   \table_row3{   <b>`PVR.NowRecordingTitle`</b>,
///                  \anchor PVR_NowRecordingTitle
///                  _string_,
///     Title of the programme being recorded
///   }
///   \table_row3{   <b>`PVR.NowRecordingDateTime`</b>,
///                  \anchor PVR_NowRecordingDateTime
///                  _Date/Time string_,
///     Start date and time of the current recording
///   }
///   \table_row3{   <b>`PVR.NowRecordingChannel`</b>,
///                  \anchor PVR_NowRecordingChannel
///                  _string_,
///     Channel name of the current recording
///   }
///   \table_row3{   <b>`PVR.NowRecordingChannelIcon`</b>,
///                  \anchor PVR_NowRecordingChannelIcon
///                  _string_,
///     Icon of the current recording channel
///   }
///   \table_row3{   <b>`PVR.NextRecordingTitle`</b>,
///                  \anchor PVR_NextRecordingTitle
///                  _string_,
///     Title of the next programme that will be recorded
///   }
///   \table_row3{   <b>`PVR.NextRecordingDateTime`</b>,
///                  \anchor PVR_NextRecordingDateTime
///                  _Date/Time string_,
///     Start date and time of the next recording
///   }
///   \table_row3{   <b>`PVR.NextRecordingChannel`</b>,
///                  \anchor PVR_NextRecordingChannel
///                  _string_,
///     Channel name of the next recording
///   }
///   \table_row3{   <b>`PVR.NextRecordingChannelIcon`</b>,
///                  \anchor PVR_NextRecordingChannelIcon
///                  _string_,
///     Icon of the next recording channel
///   }
///   \table_row3{   <b>`PVR.TVNowRecordingTitle`</b>,
///                  \anchor PVR_TVNowRecordingTitle
///                  _string_,
///     Title of the tv programme being recorded
///   }
///   \table_row3{   <b>`PVR.TVNowRecordingDateTime`</b>,
///                  \anchor PVR_TVNowRecordingDateTime
///                  _Date/Time string_,
///     Start date and time of the current tv recording
///   }
///   \table_row3{   <b>`PVR.TVNowRecordingChannel`</b>,
///                  \anchor PVR_TVNowRecordingChannel
///                  _string_,
///     Channel name of the current tv recording
///   }
///   \table_row3{   <b>`PVR.TVNowRecordingChannelIcon`</b>,
///                  \anchor PVR_TVNowRecordingChannelIcon
///                  _string_,
///     Icon of the current recording TV channel
///   }
///   \table_row3{   <b>`PVR.TVNextRecordingTitle`</b>,
///                  \anchor PVR_TVNextRecordingTitle
///                  _string_,
///     Title of the next tv programme that will be recorded
///   }
///   \table_row3{   <b>`PVR.TVNextRecordingDateTime`</b>,
///                  \anchor PVR_TVNextRecordingDateTime
///                  _Date/Time string_,
///     Start date and time of the next tv recording
///   }
///   \table_row3{   <b>`PVR.TVNextRecordingChannel`</b>,
///                  \anchor PVR_TVNextRecordingChannel
///                  _string_,
///     Channel name of the next tv recording
///   }
///   \table_row3{   <b>`PVR.TVNextRecordingChannelIcon`</b>,
///                  \anchor PVR_TVNextRecordingChannelIcon
///                     ,
///     Icon of the next recording tv channel
///   }
///   \table_row3{   <b>`PVR.RadioNowRecordingTitle`</b>,
///                  \anchor PVR_RadioNowRecordingTitle
///                  _string_,
///     Title of the radio programme being recorded
///   }
///   \table_row3{   <b>`PVR.RadioNowRecordingDateTime`</b>,
///                  \anchor PVR_RadioNowRecordingDateTime
///                  _Date/Time string_,
///     Start date and time of the current radio recording
///   }
///   \table_row3{   <b>`PVR.RadioNowRecordingChannel`</b>,
///                  \anchor PVR_RadioNowRecordingChannel
///                  _string_,
///     Channel name of the current radio recording
///   }
///   \table_row3{   <b>`PVR.RadioNowRecordingChannelIcon`</b>,
///                  \anchor PVR_RadioNowRecordingChannelIcon
///                  _string_,
///     Icon of the current recording radio channel
///   }
///   \table_row3{   <b>`PVR.RadioNextRecordingTitle`</b>,
///                  \anchor PVR_RadioNextRecordingTitle
///                  _string_,
///     Title of the next radio programme that will be recorded
///   }
///   \table_row3{   <b>`PVR.RadioNextRecordingDateTime`</b>,
///                  \anchor PVR_RadioNextRecordingDateTime
///                  _Date/Time string_,
///     Start date and time of the next radio recording
///   }
///   \table_row3{   <b>`PVR.RadioNextRecordingChannel`</b>,
///                  \anchor PVR_RadioNextRecordingChannel
///                  _string_,
///     Channel name of the next radio recording
///   }
///   \table_row3{   <b>`PVR.RadioNextRecordingChannelIcon`</b>,
///                  \anchor PVR_RadioNextRecordingChannelIcon
///                  _string_,
///     Icon of the next recording radio channel
///   }
///   \table_row3{   <b>`PVR.IsRecordingTV`</b>,
///                  \anchor PVR_IsRecordingTV
///                  _boolean_,
///     Returns true when the system is recording a tv programme.
///   }
///   \table_row3{   <b>`PVR.HasTVTimer`</b>,
///                  \anchor PVR_HasTVTimer
///                  _boolean_,
///     Returns true if at least one tv timer is active.
///   }
///   \table_row3{   <b>`PVR.HasNonRecordingTVTimer`</b>,
///                  \anchor PVR_HasNonRecordingTVTimer
///                  _boolean_,
///     Returns true if there are tv timers present who currently not do recording
///   }
///   \table_row3{   <b>`PVR.IsRecordingRadio`</b>,
///                  \anchor PVR_IsRecordingRadio
///                  _boolean_,
///     Returns true when the system is recording a radio programme.
///   }
///   \table_row3{   <b>`PVR.HasRadioTimer`</b>,
///                  \anchor PVR_HasRadioTimer
///                  _boolean_,
///     Returns true if at least one radio timer is active.
///   }
///   \table_row3{   <b>`PVR.HasNonRecordingRadioTimer`</b>,
///                  \anchor PVR_HasNonRecordingRadioTimer
///                  _boolean_,
///     Returns true if there are radio timers present who currently not do recording
///   }
///   \table_row3{   <b>`PVR.ChannelNumberInput`</b>,
///                  \anchor PVR_ChannelNumberInput
///                  _string_,
///     Returns the currently entered channel number while in numeric channel input mode\, an empty string otherwise
///   }
///   \table_row3{   <b>`PVR.CanRecordPlayingChannel`</b>,
///                  \anchor PVR_CanRecordPlayingChannel
///                  _boolean_,
///     Returns true if PVR is currently playing a channel and if this channel can be recorded.
///   }
///   \table_row3{   <b>`PVR.IsRecordingPlayingChannel`</b>,
///                  \anchor PVR_IsRecordingPlayingChannel
///                  _boolean_,
///     Returns true if PVR is currently playing a channel and if this channel is currently recorded.
///   }
///   \table_row3{   <b>`PVR.TimeshiftProgressPlayPos`</b>,
///                  \anchor PVR_TimeshiftProgressPlayPos
///                  _integer_,
///     Returns the percentage of the current play position within the PVR timeshift progress.
///   }
///   \table_row3{   <b>`PVR.TimeshiftProgressEpgStart`</b>,
///                  \anchor PVR_TimeshiftProgressEpgStart
///                  _integer_,
///     Returns the percentage of the start of the currently playing epg event within the PVR timeshift progress.
///   }
///   \table_row3{   <b>`PVR.TimeshiftProgressEpgEnd`</b>,
///                  \anchor PVR_TimeshiftProgressEpgEnd
///                  _integer_,
///     Returns the percentage of the end of the currently playing epg event within the PVR timeshift progress.
///   }
///   \table_row3{   <b>`PVR.TimeshiftProgressBufferStart`</b>,
///                  \anchor PVR_TimeshiftProgressBufferStart
///                  _integer_,
///     Returns the percentage of the start of the timeshift buffer within the PVR timeshift progress.
///   }
///   \table_row3{   <b>`PVR.TimeshiftProgressBufferEnd`</b>,
///                  \anchor PVR_TimeshiftProgressBufferEnd
///                  _integer_,
///     Returns the percentage of the end of the timeshift buffer within the PVR timeshift progress.
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
                                  { "totaldiscspace",           PVR_TOTAL_DISKSPACE },
                                  { "nexttimer",                PVR_NEXT_TIMER },
                                  { "isplayingtv",              PVR_IS_PLAYING_TV },
                                  { "isplayingradio",           PVR_IS_PLAYING_RADIO },
                                  { "isplayingrecording",       PVR_IS_PLAYING_RECORDING },
                                  { "isplayingepgtag",          PVR_IS_PLAYING_EPGTAG },
                                  { "epgeventprogress",         PVR_EPG_EVENT_PROGRESS },
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
                                  { "timeshiftprogress",        PVR_TIMESHIFT_PROGRESS },
                                  { "nowrecordingtitle",        PVR_NOW_RECORDING_TITLE },
                                  { "nowrecordingdatetime",     PVR_NOW_RECORDING_DATETIME },
                                  { "nowrecordingchannel",      PVR_NOW_RECORDING_CHANNEL },
                                  { "nowrecordingchannelicon",  PVR_NOW_RECORDING_CHAN_ICO },
                                  { "nextrecordingtitle",       PVR_NEXT_RECORDING_TITLE },
                                  { "nextrecordingdatetime",    PVR_NEXT_RECORDING_DATETIME },
                                  { "nextrecordingchannel",     PVR_NEXT_RECORDING_CHANNEL },
                                  { "nextrecordingchannelicon", PVR_NEXT_RECORDING_CHAN_ICO },
                                  { "tvnowrecordingtitle",            PVR_TV_NOW_RECORDING_TITLE },
                                  { "tvnowrecordingdatetime",         PVR_TV_NOW_RECORDING_DATETIME },
                                  { "tvnowrecordingchannel",          PVR_TV_NOW_RECORDING_CHANNEL },
                                  { "tvnowrecordingchannelicon",      PVR_TV_NOW_RECORDING_CHAN_ICO },
                                  { "tvnextrecordingtitle",           PVR_TV_NEXT_RECORDING_TITLE },
                                  { "tvnextrecordingdatetime",        PVR_TV_NEXT_RECORDING_DATETIME },
                                  { "tvnextrecordingchannel",         PVR_TV_NEXT_RECORDING_CHANNEL },
                                  { "tvnextrecordingchannelicon",     PVR_TV_NEXT_RECORDING_CHAN_ICO },
                                  { "radionowrecordingtitle",         PVR_RADIO_NOW_RECORDING_TITLE },
                                  { "radionowrecordingdatetime",      PVR_RADIO_NOW_RECORDING_DATETIME },
                                  { "radionowrecordingchannel",       PVR_RADIO_NOW_RECORDING_CHANNEL },
                                  { "radionowrecordingchannelicon",   PVR_RADIO_NOW_RECORDING_CHAN_ICO },
                                  { "radionextrecordingtitle",        PVR_RADIO_NEXT_RECORDING_TITLE },
                                  { "radionextrecordingdatetime",     PVR_RADIO_NEXT_RECORDING_DATETIME },
                                  { "radionextrecordingchannel",      PVR_RADIO_NEXT_RECORDING_CHANNEL },
                                  { "radionextrecordingchannelicon",  PVR_RADIO_NEXT_RECORDING_CHAN_ICO },
                                  { "isrecordingtv",              PVR_IS_RECORDING_TV },
                                  { "hastvtimer",                 PVR_HAS_TV_TIMER },
                                  { "hasnonrecordingtvtimer",     PVR_HAS_NONRECORDING_TV_TIMER },
                                  { "isrecordingradio",           PVR_IS_RECORDING_RADIO },
                                  { "hasradiotimer",              PVR_HAS_RADIO_TIMER },
                                  { "hasnonrecordingradiotimer",  PVR_HAS_NONRECORDING_RADIO_TIMER },
                                  { "channelnumberinput",         PVR_CHANNEL_NUMBER_INPUT },
                                  { "canrecordplayingchannel",    PVR_CAN_RECORD_PLAYING_CHANNEL },
                                  { "isrecordingplayingchannel",  PVR_IS_RECORDING_PLAYING_CHANNEL },
                                  { "timeshiftprogressplaypos",   PVR_TIMESHIFT_PROGRESS_PLAY_POS },
                                  { "timeshiftprogressepgstart",  PVR_TIMESHIFT_PROGRESS_EPG_START },
                                  { "timeshiftprogressepgend",    PVR_TIMESHIFT_PROGRESS_EPG_END },
                                  { "timeshiftprogressbufferstart", PVR_TIMESHIFT_PROGRESS_BUFFER_START },
                                  { "timeshiftprogressbufferend", PVR_TIMESHIFT_PROGRESS_BUFFER_END }};

/// \page modules__General__List_of_gui_access
/// \section modules__General__List_of_gui_access_PvrTimes PvrTimes
/// @{
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`PVR.EpgEventIcon`</b>,
///                  \anchor PVR_EpgEventIcon
///                  _string_,
///     Returns the icon of the currently playing epg event, if any.
///   }
///   \table_row3{   <b>`PVR.EpgEventDuration`</b>,
///                  \anchor PVR_EpgEventDuration
///                  _string_,
///     Returns the duration of the currently playing epg event in the
///     format hh:mm:ss. hh: will be omitted if hours value is zero.
///   }
///   \table_row3{   <b>`PVR.EpgEventDuration(format)`</b>,
///                  \anchor PVR_EpgEventDuration_format
///                  _string_,
///     Returns the duration of the currently playing epg event in different formats:
///     Hours (hh)\, minutes (mm) or seconds (ss).
///     Also supported: (hh:mm)\, (mm:ss)\, (hh:mm:ss)\, (h:mm:ss).
///     Added with Leia: (secs)\, (mins)\, (hours) for total time values and (m).
///     Example: 3661 seconds => h=1\, hh=01\, m=1\, mm=01\, ss=01\, hours=1\, mins=61\, secs=3661
///   }
///   \table_row3{   <b>`PVR.EpgEventElapsedTime`</b>,
///                  \anchor PVR_EpgEventElapsedTime
///                  _string_,
///     Returns the time of the current position of the currently playing epg event in the
///     format hh:mm:ss. hh: will be omitted if hours value is zero.
///   }
///   \table_row3{   <b>`PVR.EpgEventElapsedTime(format)`</b>,
///                  \anchor PVR_EpgEventElapsedTime_format
///                  _string_,
///     Returns the time of the current position of the currently playing epg event in different formats:
///     Hours (hh)\, minutes (mm) or seconds (ss).
///     Also supported: (hh:mm)\, (mm:ss)\, (hh:mm:ss)\, (h:mm:ss).
///     Added with Leia: (secs)\, (mins)\, (hours) for total time values and (m).
///     Example: 3661 seconds => h=1\, hh=01\, m=1\, mm=01\, ss=01\, hours=1\, mins=61\, secs=3661
///   }
///   \table_row3{   <b>`PVR.EpgEventRemainingTime`</b>,
///                  \anchor PVR_EpgEventRemainingTime
///                  _string_,
///     Returns the remaining time for currently playing epg event in the
///     format hh:mm:ss. hh: will be omitted if hours value is zero.
///   }
///   \table_row3{   <b>`PVR.EpgEventRemainingTime(format)`</b>,
///                  \anchor PVR_EpgEventRemainingTime_format
///                  _string_,
///     Returns the remaining time for currently playing epg event in different formats:
///     Hours (hh)\, minutes (mm) or seconds (ss).
///     Also supported: (hh:mm)\, (mm:ss)\, (hh:mm:ss)\, (h:mm:ss).
///     Added with Leia: (secs)\, (mins)\, (hours) for total time values and (m).
///     Example: 3661 seconds => h=1\, hh=01\, m=1\, mm=01\, ss=01\, hours=1\, mins=61\, secs=3661
///   }
///   \table_row3{   <b>`PVR.EpgEventSeekTime`</b>,
///                  \anchor PVR_EpgEventSeekTime
///                  _string_,
///     Returns the time the user is seeking within the currently playing epg event in the
///     format hh:mm:ss. hh: will be omitted if hours value is zero.
///   }
///   \table_row3{   <b>`PVR.EpgEventSeekTime(format)`</b>,
///                  \anchor PVR_EpgEventSeekTime_format
///                  _string_,
///     Returns the time the user is seeking within the currently playing epg event in different formats:
///     Hours (hh)\, minutes (mm) or seconds (ss). When 12 hour clock is used
///     (xx) will return AM/PM. Also supported: (hh:mm)\, (mm:ss)\, (hh:mm:ss)\, (h:mm:ss).
///     Added with Leia: (secs)\, (mins)\, (hours) for total time values and (m).
///     Example: 3661 seconds => h=1\, hh=01\, m=1\, mm=01\, ss=01\, hours=1\, mins=61\, secs=3661
///   }
///   \table_row3{   <b>`PVR.EpgEventFinishTime`</b>,
///                  \anchor PVR_EpgEventFinishTime
///                  _string_,
///     Returns the time the currently playing epg event will end in the
///     format hh:mm:ss. hh: will be omitted if hours value is zero.
///   }
///   \table_row3{   <b>`PVR.EpgEventFinishTime(format)`</b>,
///                  \anchor PVR_EpgEventFinishTime_format
///                  _string_,
///     Returns the time the currently playing epg event will end in different formats:
///     Hours (hh)\, minutes (mm) or seconds (ss). When 12 hour clock is used
///     (xx) will return AM/PM. Also supported: (hh:mm)\, (mm:ss)\, (hh:mm:ss)\, (h:mm:ss).
///     Added with Leia: (secs)\, (mins)\, (hours) for total time values and (m).
///     Example: 3661 seconds => h=1\, hh=01\, m=1\, mm=01\, ss=01\, hours=1\, mins=61\, secs=3661
///   }
///   \table_row3{   <b>`PVR.TimeShiftStart`</b>,
///                  \anchor PVR_TimeShiftStart
///                  _string_,
///     Returns the start time of the timeshift buffer in the
///     format hh:mm:ss. hh: will be omitted if hours value is zero.
///   }
///   \table_row3{   <b>`PVR.TimeShiftStart(format)`</b>,
///                  \anchor PVR_TimeShiftStart_format
///                  _string_,
///     Returns the start time of the timeshift buffer in different formats:
///     Hours (hh)\, minutes (mm) or seconds (ss). When 12 hour clock is used
///     (xx) will return AM/PM. Also supported: (hh:mm)\, (mm:ss)\, (hh:mm:ss)\, (h:mm:ss).
///     Added with Leia: (secs)\, (mins)\, (hours) for total time values and (m).
///     Example: 3661 seconds => h=1\, hh=01\, m=1\, mm=01\, ss=01\, hours=1\, mins=61\, secs=3661
///   }
///   \table_row3{   <b>`PVR.TimeShiftEnd`</b>,
///                  \anchor PVR_TimeShiftEnd
///                  _string_,
///     Returns the end time of the timeshift buffer in the
///     format hh:mm:ss. hh: will be omitted if hours value is zero.
///   }
///   \table_row3{   <b>`PVR.TimeShiftEnd(format)`</b>,
///                  \anchor PVR_TimeShiftEnd_format
///                  _string_,
///     Returns the end time of the timeshift buffer in different formats:
///     Hours (hh)\, minutes (mm) or seconds (ss). When 12 hour clock is used
///     (xx) will return AM/PM. Also supported: (hh:mm)\, (mm:ss)\, (hh:mm:ss)\, (h:mm:ss).
///     Added with Leia: (secs)\, (mins)\, (hours) for total time values and (m).
///     Example: 3661 seconds => h=1\, hh=01\, m=1\, mm=01\, ss=01\, hours=1\, mins=61\, secs=3661
///   }
///   \table_row3{   <b>`PVR.TimeShiftCur`</b>,
///                  \anchor PVR_TimeShiftCur
///                  _string_,
///     Returns the current playback time within the timeshift buffer in the
///     format hh:mm:ss. hh: will be omitted if hours value is zero.
///   }
///   \table_row3{   <b>`PVR.TimeShiftCur(format)`</b>,
///                  \anchor PVR_TimeShiftCur_format
///                  _string_,
///     Returns the current playback time within the timeshift buffer in different formats:
///     Hours (hh)\, minutes (mm) or seconds (ss). When 12 hour clock is used
///     (xx) will return AM/PM. Also supported: (hh:mm)\, (mm:ss)\, (hh:mm:ss)\, (h:mm:ss).
///     Added with Leia: (secs)\, (mins)\, (hours) for total time values and (m).
///     Example: 3661 seconds => h=1\, hh=01\, m=1\, mm=01\, ss=01\, hours=1\, mins=61\, secs=3661
///   }
///   \table_row3{   <b>`PVR.TimeShiftOffset`</b>,
///                  \anchor PVR_TimeShiftOffset
///                  _string_,
///     Returns the delta of timeshifted time to actual time in the
///     format hh:mm:ss. hh: will be omitted if hours value is zero.
///   }
///   \table_row3{   <b>`PVR.TimeShiftOffset(format)`</b>,
///                  \anchor PVR_TimeShiftOffset_format
///                  _string_,
///     Returns the delta of timeshifted time to actual time in different formats:
///     Hours (hh)\, minutes (mm) or seconds (ss).
///     Also supported: (hh:mm)\, (mm:ss)\, (hh:mm:ss)\, (h:mm:ss).
///     Added with Leia: (secs)\, (mins)\, (hours) for total time values and (m).
///     Example: 3661 seconds => h=1\, hh=01\, m=1\, mm=01\, ss=01\, hours=1\, mins=61\, secs=3661
///   }
///   \table_row3{   <b>`PVR.TimeshiftProgressDuration`</b>,
///                  \anchor PVR_TimeshiftProgressDuration
///                  _string_,
///     Returns the duration of the PVR timeshift progress in the
///     format hh:mm:ss. hh: will be omitted if hours value is zero.
///   }
///   \table_row3{   <b>`PVR.TimeshiftProgressDuration(format)`</b>,
///                  \anchor PVR_TimeshiftProgressDuration_format
///                  _string_,
///     Returns the duration of the PVR timeshift progress in different formats:
///     Hours (hh)\, minutes (mm) or seconds (ss).
///     Also supported: (hh:mm)\, (mm:ss)\, (hh:mm:ss)\, (h:mm:ss).
///     Added with Leia: (secs)\, (mins)\, (hours) for total time values and (m).
///     Example: 3661 seconds => h=1\, hh=01\, m=1\, mm=01\, ss=01\, hours=1\, mins=61\, secs=3661
///   }
///   \table_row3{   <b>`PVR.TimeshiftProgressStartTime`</b>,
///                  \anchor PVR_TimeshiftProgressStartTime
///                  _string_,
///     Returns the start time of the PVR timeshift progress in the
///     format hh:mm:ss. hh: will be omitted if hours value is zero.
///   }
///   \table_row3{   <b>`PVR.TimeshiftProgressStartTime(format)`</b>,
///                  \anchor PVR_TimeshiftProgressStartTime_format
///                  _string_,
///     Returns the start time of the PVR timeshift progress in different formats:
///     Hours (hh)\, minutes (mm) or seconds (ss).
///     Also supported: (hh:mm)\, (mm:ss)\, (hh:mm:ss)\, (h:mm:ss).
///     Added with Leia: (secs)\, (mins)\, (hours) for total time values and (m).
///     Example: 3661 seconds => h=1\, hh=01\, m=1\, mm=01\, ss=01\, hours=1\, mins=61\, secs=3661
///   }
///   \table_row3{   <b>`PVR.TimeshiftProgressEndTime`</b>,
///                  \anchor PVR_TimeshiftProgressEndTime
///                  _string_,
///     Returns the end time of the PVR timeshift progress in the
///     format hh:mm:ss. hh: will be omitted if hours value is zero.
///   }
///   \table_row3{   <b>`PVR.TimeshiftProgressEndTime(format)`</b>,
///                  \anchor PVR_TimeshiftProgressEndTime_format
///                  _string_,
///     Returns the end time of the PVR timeshift progress in different formats:
///     Hours (hh)\, minutes (mm) or seconds (ss).
///     Also supported: (hh:mm)\, (mm:ss)\, (hh:mm:ss)\, (h:mm:ss).
///     Added with Leia: (secs)\, (mins)\, (hours) for total time values and (m).
///     Example: 3661 seconds => h=1\, hh=01\, m=1\, mm=01\, ss=01\, hours=1\, mins=61\, secs=3661
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}
const infomap pvr_times[] =      {{ "epgeventicon",           PVR_EPG_EVENT_ICON },
                                  { "epgeventduration",       PVR_EPG_EVENT_DURATION },
                                  { "epgeventelapsedtime",    PVR_EPG_EVENT_ELAPSED_TIME },
                                  { "epgeventremainingtime",  PVR_EPG_EVENT_REMAINING_TIME },
                                  { "epgeventfinishtime",     PVR_EPG_EVENT_FINISH_TIME },
                                  { "epgeventseektime",       PVR_EPG_EVENT_SEEK_TIME },
                                  { "timeshiftstart",         PVR_TIMESHIFT_START_TIME },
                                  { "timeshiftend",           PVR_TIMESHIFT_END_TIME },
                                  { "timeshiftcur",           PVR_TIMESHIFT_PLAY_TIME },
                                  { "timeshiftoffset",        PVR_TIMESHIFT_OFFSET },
                                  { "timeshiftprogressduration",  PVR_TIMESHIFT_PROGRESS_DURATION },
                                  { "timeshiftprogressstarttime", PVR_TIMESHIFT_PROGRESS_START_TIME },
                                  { "timeshiftprogressendtime",   PVR_TIMESHIFT_PROGRESS_END_TIME }};

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
///     Returns the last sended RDS text messages on given number\, 0 is the
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
///     The email address of the radio stations hotline (if available)\n
///     (Only be available on RadiotextPlus)
///   }
///   \table_row3{   <b>`RDS.EmailStudio`</b>,
///                  \anchor RDS_EmailStudio
///                  _string_,
///     The email address of the radio stations studio (if available)\n
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
///   \table_row3{   <b>`Slideshow.Altitude`</b>,
///                  \anchor Slideshow_Altitude
///                  _string_,
///     Shows the altitude in meters where the current picture was taken. This
///     is the value of the EXIF GPSInfo.GPSAltitude tag.
///   }
///   \table_row3{   <b>`Slideshow.Aperture`</b>,
///                  \anchor Slideshow_Aperture
///                  _string_,
///     Shows the F-stop used to take the current picture. This is the value of
///     the EXIF FNumber tag (hex code 0x829D).
///   }
///   \table_row3{   <b>`Slideshow.Author`</b>,
///                  \anchor Slideshow_Author
///                  _string_,
///     Shows the name of the person involved in writing about the current
///     picture. This is the value of the IPTC Writer tag (hex code 0x7A).
///   }
///   \table_row3{   <b>`Slideshow.Byline`</b>,
///                  \anchor Slideshow_Byline
///                  _string_,
///     Shows the name of the person who created the current picture. This is
///     the value of the IPTC Byline tag (hex code 0x50).
///   }
///   \table_row3{   <b>`Slideshow.BylineTitle`</b>,
///                  \anchor Slideshow_BylineTitle
///                  _string_,
///     Shows the title of the person who created the current picture. This is
///     the value of the IPTC BylineTitle tag (hex code 0x55).
///   }
///   \table_row3{   <b>`Slideshow.CameraMake`</b>,
///                  \anchor Slideshow_CameraMake
///                  _string_,
///     Shows the manufacturer of the camera used to take the current picture.
///     This is the value of the EXIF Make tag (hex code 0x010F).
///   }
///   \table_row3{   <b>`Slideshow.CameraModel`</b>,
///                  \anchor Slideshow_CameraModel
///                  _string_,
///     Shows the manufacturer's model name or number of the camera used to take
///     the current picture. This is the value of the EXIF Model tag (hex code
///     0x0110).
///   }
///   \table_row3{   <b>`Slideshow.Caption`</b>,
///                  \anchor Slideshow_Caption
///                  _string_,
///     Shows a description of the current picture. This is the value of the
///     IPTC Caption tag (hex code 0x78).
///   }
///   \table_row3{   <b>`Slideshow.Category`</b>,
///                  \anchor Slideshow_Category
///                  _string_,
///     Shows the subject of the current picture as a category code. This is the
///     value of the IPTC Category tag (hex code 0x0F).
///   }
///   \table_row3{   <b>`Slideshow.CCDWidth`</b>,
///                  \anchor Slideshow_CCDWidth
///                  _string_,
///     Shows the width of the CCD in the camera used to take the current
///     picture. This is calculated from three EXIF tags (0xA002 * 0xA210 / 0xA20e).
///   }
///   \table_row3{   <b>`Slideshow.City`</b>,
///                  \anchor Slideshow_City
///                  _string_,
///     Shows the city where the current picture was taken. This is the value of
///     the IPTC City tag (hex code 0x5A).
///   }
///   \table_row3{   <b>`Slideshow.Colour`</b>,
///                  \anchor Slideshow_Colour
///                  _string_,
///     Shows whether the current picture is "Colour" or "Black and White".
///   }
///   \table_row3{   <b>`Slideshow.CopyrightNotice`</b>,
///                  \anchor Slideshow_CopyrightNotice
///                  _string_,
///     Shows the copyright notice of the current picture. This is the value of
///     the IPTC Copyright tag (hex code 0x74).
///   }
///   \table_row3{   <b>`Slideshow.Country`</b>,
///                  \anchor Slideshow_Country
///                  _string_,
///     Shows the full name of the country where the current picture was taken.
///     This is the value of the IPTC CountryName tag (hex code 0x65).
///   }
///   \table_row3{   <b>`Slideshow.CountryCode`</b>,
///                  \anchor Slideshow_CountryCode
///                  _string_,
///     Shows the country code of the country where the current picture was
///     taken. This is the value of the IPTC CountryCode tag (hex code 0x64).
///   }
///   \table_row3{   <b>`Slideshow.Credit`</b>,
///                  \anchor Slideshow_Credit
///                  _string_,
///     Shows who provided the current picture. This is the value of the IPTC
///     Credit tag (hex code 0x6E).
///   }
///   \table_row3{   <b>`Slideshow.DigitalZoom`</b>,
///                  \anchor Slideshow_DigitalZoom
///                  _string_,
///     Shows the digital zoom ratio when the current picture was taken. This is
///     the value of the EXIF .DigitalZoomRatio tag (hex code 0xA404).
///   }
///   \table_row3{   <b>`Slideshow.EXIFComment`</b>,
///                  \anchor Slideshow_EXIFComment
///                  _string_,
///     Shows a description of the current picture. This is the value of the
///     EXIF User Comment tag (hex code 0x9286). This is the same value as
///     Slideshow.SlideComment.
///   }
///   \table_row3{   <b>`Slideshow.EXIFDate`</b>,
///                  \anchor Slideshow_EXIFDate
///                  _string_,
///     Shows the localized date of the current picture. The short form of the
///     date is used. The value of the EXIF DateTimeOriginal tag (hex code
///     0x9003) is preferred. If the DateTimeOriginal tag is not found\, the
///     value of DateTimeDigitized (hex code 0x9004) or of DateTime (hex code
///     0x0132) might be used.
///   }
///   \table_row3{   <b>`Slideshow.EXIFDescription`</b>,
///                  \anchor Slideshow_EXIFDescription
///                  _string_,
///     Shows a short description of the current picture. The SlideComment\,
///     EXIFComment or Caption values might contain a longer description. This
///     is the value of the EXIF ImageDescription tag (hex code 0x010E).
///   }
///   \table_row3{   <b>`Slideshow.EXIFSoftware`</b>,
///                  \anchor Slideshow_EXIFSoftware
///                  _string_,
///     Shows the name and version of the firmware used by the camera that took
///     the current picture. This is the value of the EXIF Software tag (hex
///     code 0x0131).
///   }
///   \table_row3{   <b>`Slideshow.EXIFTime`</b>,
///                  \anchor Slideshow_EXIFTime
///                  _string_,
///     Shows the date/timestamp of the current picture. The localized short
///     form of the date and time is used. The value of the EXIF
///     DateTimeOriginal tag (hex code 0x9003) is preferred. If the
///     DateTimeOriginal tag is not found\, the value of DateTimeDigitized (hex
///     code 0x9004) or of DateTime (hex code 0x0132) might be used.
///   }
///   \table_row3{   <b>`Slideshow.Exposure`</b>,
///                  \anchor Slideshow_Exposure
///                  _string_,
///     Shows the class of the program used by the camera to set exposure when
///     the current picture was taken. Values include "Manual"\,
///     "Program (Auto)"\, "Aperture priority (Semi-Auto)"\, "Shutter priority
///     (semi-auto)"\, etc. This is the value of the EXIF ExposureProgram tag
///     (hex code 0x8822).
///   }
///   \table_row3{   <b>`Slideshow.ExposureBias`</b>,
///                  \anchor Slideshow_ExposureBias
///                  _string_,
///     Shows the exposure bias of the current picture. Typically this is a
///     number between -99.99 and 99.99. This is the value of the EXIF
///     ExposureBiasValue tag (hex code 0x9204).
///   }
///   \table_row3{   <b>`Slideshow.ExposureMode`</b>,
///                  \anchor Slideshow_ExposureMode
///                  _string_,
///     Shows the exposure mode of the current picture. The possible values are
///     "Automatic"\, "Manual"\, and "Auto bracketing". This is the value of the
///     EXIF ExposureMode tag (hex code 0xA402).
///   }
///   \table_row3{   <b>`Slideshow.ExposureTime`</b>,
///                  \anchor Slideshow_ExposureTime
///                  _string_,
///     Shows the exposure time of the current picture\, in seconds. This is the
///     value of the EXIF ExposureTime tag (hex code 0x829A). If the
///     ExposureTime tag is not found\, the ShutterSpeedValue tag (hex code
///     0x9201) might be used.
///   }
///   \table_row3{   <b>`Slideshow.Filedate`</b>,
///                  \anchor Slideshow_Filedate
///                  _string_,
///     Shows the file date of the current picture
///   }
///   \table_row3{   <b>`Slideshow.Filename`</b>,
///                  \anchor Slideshow_Filename
///                  _string_,
///     Shows the file name of the current picture
///   }
///   \table_row3{   <b>`Slideshow.Filesize`</b>,
///                  \anchor Slideshow_Filesize
///                  _string_,
///     Shows the file size of the current picture
///   }
///   \table_row3{   <b>`Slideshow.FlashUsed`</b>,
///                  \anchor Slideshow_FlashUsed
///                  _string_,
///     Shows the status of flash when the current picture was taken. The value
///     will be either "Yes" or "No"\, and might include additional information.
///     This is the value of the EXIF Flash tag (hex code 0x9209).
///   }
///   \table_row3{   <b>`Slideshow.FocalLength`</b>,
///                  \anchor Slideshow_FocalLength
///                  _string_,
///     Shows the focal length of the lens\, in mm. This is the value of the EXIF
///     FocalLength tag (hex code 0x920A).
///   }
///   \table_row3{   <b>`Slideshow.FocusDistance`</b>,
///                  \anchor Slideshow_FocusDistance
///                  _string_,
///     Shows the distance to the subject\, in meters. This is the value of the
///     EXIF SubjectDistance tag (hex code 0x9206).
///   }
///   \table_row3{   <b>`Slideshow.Headline`</b>,
///                  \anchor Slideshow_Headline
///                  _string_,
///     Shows a synopsis of the contents of the current picture. This is the
///     value of the IPTC Headline tag (hex code 0x69).
///   }
///   \table_row3{   <b>`Slideshow.ImageType`</b>,
///                  \anchor Slideshow_ImageType
///                  _string_,
///     Shows the color components of the current picture. This is the value of
///     the IPTC ImageType tag (hex code 0x82).
///   }
///   \table_row3{   <b>`Slideshow.IPTCDate`</b>,
///                  \anchor Slideshow_IPTCDate
///                  _string_,
///     Shows the date when the intellectual content of the current picture was
///     created\, rather than when the picture was created. This is the value of
///     the IPTC DateCreated tag (hex code 0x37).
///   }
///   \table_row3{   <b>`Slideshow.ISOEquivalence`</b>,
///                  \anchor Slideshow_ISOEquivalence
///                  _string_,
///     Shows the ISO speed of the camera when the current picture was taken.
///     This is the value of the EXIF ISOSpeedRatings tag (hex code 0x8827).
///   }
///   \table_row3{   <b>`Slideshow.Keywords`</b>,
///                  \anchor Slideshow_Keywords
///                  _string_,
///     Shows keywords assigned to the current picture. This is the value of the
///     IPTC Keywords tag (hex code 0x19).
///   }
///   \table_row3{   <b>`Slideshow.Latitude`</b>,
///                  \anchor Slideshow_Latitude
///                  _string_,
///     Shows the latitude where the current picture was taken (degrees\,
///     minutes\, seconds North or South). This is the value of the EXIF
///     GPSInfo.GPSLatitude and GPSInfo.GPSLatitudeRef tags.
///   }
///   \table_row3{   <b>`Slideshow.LightSource`</b>,
///                  \anchor Slideshow_LightSource
///                  _string_,
///     Shows the kind of light source when the picture was taken. Possible
///     values include "Daylight"\, "Fluorescent"\, "Incandescent"\, etc. This is
///     the value of the EXIF LightSource tag (hex code 0x9208).
///   }
///   \table_row3{   <b>`Slideshow.LongEXIFDate`</b>,
///                  \anchor Slideshow_LongEXIFDate
///                  _string_,
///     Shows only the localized date of the current picture. The long form of
///     the date is used. The value of the EXIF DateTimeOriginal tag (hex code
///     0x9003) is preferred. If the DateTimeOriginal tag is not found\, the
///     value of DateTimeDigitized (hex code 0x9004) or of DateTime (hex code
///     0x0132) might be used.
///   }
///   \table_row3{   <b>`Slideshow.LongEXIFTime`</b>,
///                  \anchor Slideshow_LongEXIFTime
///                  _string_,
///     Shows the date/timestamp of the current picture. The localized long form
///     of the date and time is used. The value of the EXIF DateTimeOriginal tag
///     (hex code 0x9003) is preferred. if the DateTimeOriginal tag is not found\,
///     the value of DateTimeDigitized (hex code 0x9004) or of DateTime (hex
///     code 0x0132) might be used.
///   }
///   \table_row3{   <b>`Slideshow.Longitude`</b>,
///                  \anchor Slideshow_Longitude
///                  _string_,
///     Shows the longitude where the current picture was taken (degrees\,
///     minutes\, seconds East or West). This is the value of the EXIF
///     GPSInfo.GPSLongitude and GPSInfo.GPSLongitudeRef tags.
///   }
///   \table_row3{   <b>`Slideshow.MeteringMode`</b>,
///                  \anchor Slideshow_MeteringMode
///                  _string_,
///     Shows the metering mode used when the current picture was taken. The
///     possible values are "Center weight"\, "Spot"\, or "Matrix". This is the
///     value of the EXIF MeteringMode tag (hex code 0x9207).
///   }
///   \table_row3{   <b>`Slideshow.ObjectName`</b>,
///                  \anchor Slideshow_ObjectName
///                  _string_,
///     Shows a shorthand reference for the current picture. This is the value
///     of the IPTC ObjectName tag (hex code 0x05).
///   }
///   \table_row3{   <b>`Slideshow.Orientation`</b>,
///                  \anchor Slideshow_Orientation
///                  _string_,
///     Shows the orientation of the current picture. Possible values are "Top
///     Left"\, "Top Right"\, "Left Top"\, "Right Bottom"\, etc. This is the value
///     of the EXIF Orientation tag (hex code 0x0112).
///   }
///   \table_row3{   <b>`Slideshow.Path`</b>,
///                  \anchor Slideshow_Path
///                  _string_,
///     Shows the file path of the current picture
///   }
///   \table_row3{   <b>`Slideshow.Process`</b>,
///                  \anchor Slideshow_Process
///                  _string_,
///     Shows the process used to compress the current picture
///   }
///   \table_row3{   <b>`Slideshow.ReferenceService`</b>,
///                  \anchor Slideshow_ReferenceService
///                  _string_,
///     Shows the Service Identifier of a prior envelope to which the current
///     picture refers. This is the value of the IPTC ReferenceService tag (hex
///     code 0x2D).
///   }
///   \table_row3{   <b>`Slideshow.Resolution`</b>,
///                  \anchor Slideshow_Resolution
///                  _string_,
///     Shows the dimensions of the current picture (Width x Height)
///   }
///   \table_row3{   <b>`Slideshow.SlideComment`</b>,
///                  \anchor Slideshow_SlideComment
///                  _string_,
///     Shows a description of the current picture. This is the value of the
///     EXIF User Comment tag (hex code 0x9286). This is the same value as
///     Slideshow.EXIFComment.
///   }
///   \table_row3{   <b>`Slideshow.SlideIndex`</b>,
///                  \anchor Slideshow_SlideIndex
///                  _string_,
///     Shows the slide index of the current picture
///   }
///   \table_row3{   <b>`Slideshow.Source`</b>,
///                  \anchor Slideshow_Source
///                  _string_,
///     Shows the original owner of the current picture. This is the value of
///     the IPTC Source tag (hex code 0x73).
///   }
///   \table_row3{   <b>`Slideshow.SpecialInstructions`</b>,
///                  \anchor Slideshow_SpecialInstructions
///                  _string_,
///     Shows other editorial instructions concerning the use of the current
///     picture. This is the value of the IPTC SpecialInstructions tag (hex
///     code 0x28).
///   }
///   \table_row3{   <b>`Slideshow.State`</b>,
///                  \anchor Slideshow_State
///                  _string_,
///     Shows the State/Province where the current picture was taken. This is
///     the value of the IPTC ProvinceState tag (hex code 0x5F).
///   }
///   \table_row3{   <b>`Slideshow.Sublocation`</b>,
///                  \anchor Slideshow_Sublocation
///                  _string_,
///     Shows the location within a city where the current picture was taken -
///     might indicate the nearest landmark. This is the value of the IPTC
///     SubLocation tag (hex code 0x5C).
///   }
///   \table_row3{   <b>`Slideshow.SupplementalCategories`</b>,
///                  \anchor Slideshow_SupplementalCategories
///                  _string_,
///     Shows supplemental category codes to further refine the subject of the
///     current picture. This is the value of the IPTC SuppCategory tag (hex
///     code 0x14).
///   }
///   \table_row3{   <b>`Slideshow.TimeCreated`</b>,
///                  \anchor Slideshow_TimeCreated
///                  _string_,
///     Shows the time when the intellectual content of the current picture was
///     created\, rather than when the picture was created. This is the value of
///     the IPTC TimeCreated tag (hex code 0x3C).
///   }
///   \table_row3{   <b>`Slideshow.TransmissionReference`</b>,
///                  \anchor Slideshow_TransmissionReference
///                  _string_,
///     Shows a code representing the location of original transmission of the
///     current picture. This is the value of the IPTC TransmissionReference tag
///     (hex code 0x67).
///   }
///   \table_row3{   <b>`Slideshow.Urgency`</b>,
///                  \anchor Slideshow_Urgency
///                  _string_,
///     Shows the urgency of the current picture. Values are 1-9. The 1 is most
///     urgent. Some image management programs use urgency to indicate picture
///     rating\, where urgency 1 is 5 stars and urgency 5 is 1 star. Urgencies
///     6-9 are not used for rating. This is the value of the IPTC Urgency tag
///     (hex code 0x0A).
///   }
///   \table_row3{   <b>`Slideshow.WhiteBalance`</b>,
///                  \anchor Slideshow_WhiteBalance
///                  _string_,
///     Shows the white balance mode set when the current picture was taken.
///     The possible values are "Manual" and "Auto". This is the value of the
///     EXIF WhiteBalance tag (hex code 0xA403).
///   }
/// \table_end
///
/// -----------------------------------------------------------------------------
/// @}
const infomap slideshow[] =      {{ "ispaused",               SLIDESHOW_ISPAUSED },
                                  { "isactive",               SLIDESHOW_ISACTIVE },
                                  { "isvideo",                SLIDESHOW_ISVIDEO },
                                  { "israndom",               SLIDESHOW_ISRANDOM },
                                  { "filename",               SLIDESHOW_FILE_NAME },
                                  { "path",                   SLIDESHOW_FILE_PATH },
                                  { "filesize",               SLIDESHOW_FILE_SIZE },
                                  { "filedate",               SLIDESHOW_FILE_DATE },
                                  { "slideindex",             SLIDESHOW_INDEX },
                                  { "resolution",             SLIDESHOW_RESOLUTION },
                                  { "slidecomment",           SLIDESHOW_COMMENT },
                                  { "colour",                 SLIDESHOW_COLOUR },
                                  { "process",                SLIDESHOW_PROCESS },
                                  { "exiftime",               SLIDESHOW_EXIF_DATE_TIME },
                                  { "exifdate",               SLIDESHOW_EXIF_DATE },
                                  { "longexiftime",           SLIDESHOW_EXIF_LONG_DATE_TIME },
                                  { "longexifdate",           SLIDESHOW_EXIF_LONG_DATE },
                                  { "exifdescription",        SLIDESHOW_EXIF_DESCRIPTION },
                                  { "cameramake",             SLIDESHOW_EXIF_CAMERA_MAKE },
                                  { "cameramodel",            SLIDESHOW_EXIF_CAMERA_MODEL },
                                  { "exifcomment",            SLIDESHOW_EXIF_COMMENT },
                                  { "exifsoftware",           SLIDESHOW_EXIF_SOFTWARE },
                                  { "aperture",               SLIDESHOW_EXIF_APERTURE },
                                  { "focallength",            SLIDESHOW_EXIF_FOCAL_LENGTH },
                                  { "focusdistance",          SLIDESHOW_EXIF_FOCUS_DIST },
                                  { "exposure",               SLIDESHOW_EXIF_EXPOSURE },
                                  { "exposuretime",           SLIDESHOW_EXIF_EXPOSURE_TIME },
                                  { "exposurebias",           SLIDESHOW_EXIF_EXPOSURE_BIAS },
                                  { "exposuremode",           SLIDESHOW_EXIF_EXPOSURE_MODE },
                                  { "flashused",              SLIDESHOW_EXIF_FLASH_USED },
                                  { "whitebalance",           SLIDESHOW_EXIF_WHITE_BALANCE },
                                  { "lightsource",            SLIDESHOW_EXIF_LIGHT_SOURCE },
                                  { "meteringmode",           SLIDESHOW_EXIF_METERING_MODE },
                                  { "isoequivalence",         SLIDESHOW_EXIF_ISO_EQUIV },
                                  { "digitalzoom",            SLIDESHOW_EXIF_DIGITAL_ZOOM },
                                  { "ccdwidth",               SLIDESHOW_EXIF_CCD_WIDTH },
                                  { "orientation",            SLIDESHOW_EXIF_ORIENTATION },
                                  { "supplementalcategories", SLIDESHOW_IPTC_SUP_CATEGORIES },
                                  { "keywords",               SLIDESHOW_IPTC_KEYWORDS },
                                  { "caption",                SLIDESHOW_IPTC_CAPTION },
                                  { "author",                 SLIDESHOW_IPTC_AUTHOR },
                                  { "headline",               SLIDESHOW_IPTC_HEADLINE },
                                  { "specialinstructions",    SLIDESHOW_IPTC_SPEC_INSTR },
                                  { "category",               SLIDESHOW_IPTC_CATEGORY },
                                  { "byline",                 SLIDESHOW_IPTC_BYLINE },
                                  { "bylinetitle",            SLIDESHOW_IPTC_BYLINE_TITLE },
                                  { "credit",                 SLIDESHOW_IPTC_CREDIT },
                                  { "source",                 SLIDESHOW_IPTC_SOURCE },
                                  { "copyrightnotice",        SLIDESHOW_IPTC_COPYRIGHT_NOTICE },
                                  { "objectname",             SLIDESHOW_IPTC_OBJECT_NAME },
                                  { "city",                   SLIDESHOW_IPTC_CITY },
                                  { "state",                  SLIDESHOW_IPTC_STATE },
                                  { "country",                SLIDESHOW_IPTC_COUNTRY },
                                  { "transmissionreference",  SLIDESHOW_IPTC_TX_REFERENCE },
                                  { "iptcdate",               SLIDESHOW_IPTC_DATE },
                                  { "urgency",                SLIDESHOW_IPTC_URGENCY },
                                  { "countrycode",            SLIDESHOW_IPTC_COUNTRY_CODE },
                                  { "referenceservice",       SLIDESHOW_IPTC_REF_SERVICE },
                                  { "latitude",               SLIDESHOW_EXIF_GPS_LATITUDE },
                                  { "longitude",              SLIDESHOW_EXIF_GPS_LONGITUDE },
                                  { "altitude",               SLIDESHOW_EXIF_GPS_ALTITUDE },
                                  { "timecreated",            SLIDESHOW_IPTC_TIMECREATED },
                                  { "sublocation",            SLIDESHOW_IPTC_SUBLOCATION },
                                  { "imagetype",              SLIDESHOW_IPTC_IMAGETYPE },
};

// Crazy part, to use tableofcontents must it be on end
/// \page modules__General__List_of_gui_access
/// \tableofcontents

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
        info.emplace_back(Property(property, param));
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
    info.emplace_back(Property(property, param));
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
  }
  else if (info.size() == 2)
  {
    const Property &prop = info[1];
    if (cat.name == "string")
    {
      if (prop.name == "isempty")
      {
        return AddMultiInfo(CGUIInfo(STRING_IS_EMPTY, TranslateSingleString(prop.param(), listItemDependent)));
      }
      else if (prop.num_params() == 2)
      {
        for (const infomap& string_bool : string_bools)
        {
          if (prop.name == string_bool.str)
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
                return AddMultiInfo(CGUIInfo(string_bool.val, data1, -data2));
            }
            return AddMultiInfo(CGUIInfo(string_bool.val, data1, label));
          }
        }
      }
    }
    if (cat.name == "integer")
    {
      for (const infomap& integer_bool : integer_bools)
      {
        if (prop.name == integer_bool.str)
        {
          int data1 = TranslateSingleString(prop.param(0), listItemDependent);
          int data2 = atoi(prop.param(1).c_str());
          return AddMultiInfo(CGUIInfo(integer_bool.val, data1, data2));
        }
      }
    }
    else if (cat.name == "player")
    {
      for (const infomap& player_label : player_labels)
      {
        if (prop.name == player_label.str)
          return player_label.val;
      }
      for (const infomap& player_time : player_times)
      {
        if (prop.name == player_time.str)
          return AddMultiInfo(CGUIInfo(player_time.val, TranslateTimeFormat(prop.param())));
      }
      if (prop.name == "process" && prop.num_params())
      {
        for (const infomap& player_proces : player_process)
        {
          if (StringUtils::EqualsNoCase(prop.param(), player_proces.str))
            return player_proces.val;
        }
      }
      if (prop.num_params() == 1)
      {
        for (const infomap& i : player_param)
        {
          if (prop.name == i.str)
            return AddMultiInfo(CGUIInfo(i.val, prop.param()));
        }
      }
    }
    else if (cat.name == "weather")
    {
      for (const infomap& i : weather)
      {
        if (prop.name == i.str)
          return i.val;
      }
    }
    else if (cat.name == "network")
    {
      for (const infomap& network_label : network_labels)
      {
        if (prop.name == network_label.str)
          return network_label.val;
      }
    }
    else if (cat.name == "musicpartymode")
    {
      for (const infomap& i : musicpartymode)
      {
        if (prop.name == i.str)
          return i.val;
      }
    }
    else if (cat.name == "system")
    {
      for (const infomap& system_label : system_labels)
      {
        if (prop.name == system_label.str)
          return system_label.val;
      }
      if (prop.num_params() == 1)
      {
        const std::string &param = prop.param();
        if (prop.name == "getbool")
        {
          std::string paramCopy = param;
          StringUtils::ToLower(paramCopy);
          return AddMultiInfo(CGUIInfo(SYSTEM_GET_BOOL, paramCopy));
        }
        for (const infomap& i : system_param)
        {
          if (prop.name == i.str)
            return AddMultiInfo(CGUIInfo(i.val, param));
        }
        if (prop.name == "memory")
        {
          if (param == "free")
            return SYSTEM_FREE_MEMORY;
          else if (param == "free.percent")
            return SYSTEM_FREE_MEMORY_PERCENT;
          else if (param == "used")
            return SYSTEM_USED_MEMORY;
          else if (param == "used.percent")
            return SYSTEM_USED_MEMORY_PERCENT;
          else if (param == "total")
            return SYSTEM_TOTAL_MEMORY;
        }
        else if (prop.name == "addontitle")
        {
          // Example: System.AddonTitle(Skin.String(HomeVideosButton1)) => skin string HomeVideosButton1 holds an addon identifier string
          int infoLabel = TranslateSingleString(param, listItemDependent);
          if (infoLabel > 0)
            return AddMultiInfo(CGUIInfo(SYSTEM_ADDON_TITLE, infoLabel, 0));
          std::string label = CGUIInfoLabel::GetLabel(param);
          StringUtils::ToLower(label);
          return AddMultiInfo(CGUIInfo(SYSTEM_ADDON_TITLE, label, 1));
        }
        else if (prop.name == "addonicon")
        {
          int infoLabel = TranslateSingleString(param, listItemDependent);
          if (infoLabel > 0)
            return AddMultiInfo(CGUIInfo(SYSTEM_ADDON_ICON, infoLabel, 0));
          std::string label = CGUIInfoLabel::GetLabel(param);
          StringUtils::ToLower(label);
          return AddMultiInfo(CGUIInfo(SYSTEM_ADDON_ICON, label, 1));
        }
        else if (prop.name == "addonversion")
        {
          int infoLabel = TranslateSingleString(param, listItemDependent);
          if (infoLabel > 0)
            return AddMultiInfo(CGUIInfo(SYSTEM_ADDON_VERSION, infoLabel, 0));
          std::string label = CGUIInfoLabel::GetLabel(param);
          StringUtils::ToLower(label);
          return AddMultiInfo(CGUIInfo(SYSTEM_ADDON_VERSION, label, 1));
        }
        else if (prop.name == "idletime")
          return AddMultiInfo(CGUIInfo(SYSTEM_IDLE_TIME, atoi(param.c_str())));
      }
      if (prop.name == "alarmlessorequal" && prop.num_params() == 2)
        return AddMultiInfo(CGUIInfo(SYSTEM_ALARM_LESS_OR_EQUAL, prop.param(0), atoi(prop.param(1).c_str())));
      else if (prop.name == "date")
      {
        if (prop.num_params() == 2)
          return AddMultiInfo(CGUIInfo(SYSTEM_DATE, StringUtils::DateStringToYYYYMMDD(prop.param(0)) % 10000, StringUtils::DateStringToYYYYMMDD(prop.param(1)) % 10000));
        else if (prop.num_params() == 1)
        {
          int dateformat = StringUtils::DateStringToYYYYMMDD(prop.param(0));
          if (dateformat <= 0) // not concrete date
            return AddMultiInfo(CGUIInfo(SYSTEM_DATE, prop.param(0), -1));
          else
            return AddMultiInfo(CGUIInfo(SYSTEM_DATE, dateformat % 10000));
        }
        return SYSTEM_DATE;
      }
      else if (prop.name == "time")
      {
        if (prop.num_params() == 0)
          return AddMultiInfo(CGUIInfo(SYSTEM_TIME, TIME_FORMAT_GUESS));
        if (prop.num_params() == 1)
        {
          TIME_FORMAT timeFormat = TranslateTimeFormat(prop.param(0));
          if (timeFormat == TIME_FORMAT_GUESS)
            return AddMultiInfo(CGUIInfo(SYSTEM_TIME, StringUtils::TimeStringToSeconds(prop.param(0))));
          return AddMultiInfo(CGUIInfo(SYSTEM_TIME, timeFormat));
        }
        else
          return AddMultiInfo(CGUIInfo(SYSTEM_TIME, StringUtils::TimeStringToSeconds(prop.param(0)), StringUtils::TimeStringToSeconds(prop.param(1))));
      }
    }
    else if (cat.name == "library")
    {
      if (prop.name == "isscanning")
        return LIBRARY_IS_SCANNING;
      else if (prop.name == "isscanningvideo")
        return LIBRARY_IS_SCANNING_VIDEO; //! @todo change to IsScanning(Video)
      else if (prop.name == "isscanningmusic")
        return LIBRARY_IS_SCANNING_MUSIC;
      else if (prop.name == "hascontent" && prop.num_params())
      {
        std::string cat = prop.param(0);
        StringUtils::ToLower(cat);
        if (cat == "music")
          return LIBRARY_HAS_MUSIC;
        else if (cat == "video")
          return LIBRARY_HAS_VIDEO;
        else if (cat == "movies")
          return LIBRARY_HAS_MOVIES;
        else if (cat == "tvshows")
          return LIBRARY_HAS_TVSHOWS;
        else if (cat == "musicvideos")
          return LIBRARY_HAS_MUSICVIDEOS;
        else if (cat == "moviesets")
          return LIBRARY_HAS_MOVIE_SETS;
        else if (cat == "singles")
          return LIBRARY_HAS_SINGLES;
        else if (cat == "compilations")
          return LIBRARY_HAS_COMPILATIONS;
        else if (cat == "role" && prop.num_params() > 1)
          return AddMultiInfo(CGUIInfo(LIBRARY_HAS_ROLE, prop.param(1), 0));
      }
    }
    else if (cat.name == "musicplayer")
    {
      for (const infomap& player_time : player_times) //! @todo remove these, they're repeats
      {
        if (prop.name == player_time.str)
          return AddMultiInfo(CGUIInfo(player_time.val, TranslateTimeFormat(prop.param())));
      }
      if (prop.name == "content" && prop.num_params())
        return AddMultiInfo(CGUIInfo(MUSICPLAYER_CONTENT, prop.param(), 0));
      else if (prop.name == "property")
      {
        if (StringUtils::EqualsNoCase(prop.param(), "fanart_image"))
          return AddMultiInfo(CGUIInfo(PLAYER_ITEM_ART, "fanart"));

        return AddMultiInfo(CGUIInfo(MUSICPLAYER_PROPERTY, prop.param()));
      }
      return TranslateMusicPlayerString(prop.name);
    }
    else if (cat.name == "videoplayer")
    {
      if (prop.name != "starttime") // player.starttime is semantically different from videoplayer.starttime which has its own implementation!
      {
        for (const infomap& player_time : player_times) //! @todo remove these, they're repeats
        {
          if (prop.name == player_time.str)
            return AddMultiInfo(CGUIInfo(player_time.val, TranslateTimeFormat(prop.param())));
        }
      }
      if (prop.name == "content" && prop.num_params())
      {
        return AddMultiInfo(CGUIInfo(VIDEOPLAYER_CONTENT, prop.param(), 0));
      }
      for (const infomap& i : videoplayer)
      {
        if (prop.name == i.str)
          return i.val;
      }
    }
    else if (cat.name == "retroplayer")
    {
      for (const infomap& i : retroplayer)
      {
        if (prop.name == i.str)
          return i.val;
      }
    }
    else if (cat.name == "slideshow")
    {
      for (const infomap& i : slideshow)
      {
        if (prop.name == i.str)
          return i.val;
      }
    }
    else if (cat.name == "container")
    {
      for (const infomap& i : mediacontainer) // these ones don't have or need an id
      {
        if (prop.name == i.str)
          return i.val;
      }
      int id = atoi(cat.param().c_str());
      for (const infomap& container_bool : container_bools) // these ones can have an id (but don't need to?)
      {
        if (prop.name == container_bool.str)
          return id ? AddMultiInfo(CGUIInfo(container_bool.val, id)) : container_bool.val;
      }
      for (const infomap& container_int : container_ints) // these ones can have an int param on the property
      {
        if (prop.name == container_int.str)
          return AddMultiInfo(CGUIInfo(container_int.val, id, atoi(prop.param().c_str())));
      }
      for (const infomap& i : container_str) // these ones have a string param on the property
      {
        if (prop.name == i.str)
          return AddMultiInfo(CGUIInfo(i.val, id, prop.param()));
      }
      if (prop.name == "sortdirection")
      {
        SortOrder order = SortOrderNone;
        if (StringUtils::EqualsNoCase(prop.param(), "ascending"))
          order = SortOrderAscending;
        else if (StringUtils::EqualsNoCase(prop.param(), "descending"))
          order = SortOrderDescending;
        return AddMultiInfo(CGUIInfo(CONTAINER_SORT_DIRECTION, order));
      }
    }
    else if (cat.name == "listitem" ||
             cat.name == "listitemposition" ||
             cat.name == "listitemnowrap" ||
             cat.name == "listitemabsolute")
    {
      int ret = TranslateListItem(cat, prop, 0, false);
      if (ret)
        listItemDependent = true;
      return ret;
    }
    else if (cat.name == "visualisation")
    {
      for (const infomap& i : visualisation)
      {
        if (prop.name == i.str)
          return i.val;
      }
    }
    else if (cat.name == "fanart")
    {
      for (const infomap& fanart_label : fanart_labels)
      {
        if (prop.name == fanart_label.str)
          return fanart_label.val;
      }
    }
    else if (cat.name == "skin")
    {
      for (const infomap& skin_label : skin_labels)
      {
        if (prop.name == skin_label.str)
          return skin_label.val;
      }
      if (prop.num_params())
      {
        if (prop.name == "string")
        {
          if (prop.num_params() == 2)
            return AddMultiInfo(CGUIInfo(SKIN_STRING_IS_EQUAL, CSkinSettings::GetInstance().TranslateString(prop.param(0)), prop.param(1)));
          else
            return AddMultiInfo(CGUIInfo(SKIN_STRING, CSkinSettings::GetInstance().TranslateString(prop.param(0))));
        }
        if (prop.name == "hassetting")
          return AddMultiInfo(CGUIInfo(SKIN_BOOL, CSkinSettings::GetInstance().TranslateBool(prop.param(0))));
        else if (prop.name == "hastheme")
          return AddMultiInfo(CGUIInfo(SKIN_HAS_THEME, prop.param(0)));
      }
    }
    else if (cat.name == "window")
    {
      if (prop.name == "property" && prop.num_params() == 1)
      { //! @todo this doesn't support foo.xml
        int winID = cat.param().empty() ? 0 : CWindowTranslator::TranslateWindow(cat.param());
        if (winID != WINDOW_INVALID)
          return AddMultiInfo(CGUIInfo(WINDOW_PROPERTY, winID, prop.param()));
      }
      for (const infomap& window_bool : window_bools)
      {
        if (prop.name == window_bool.str)
        { //! @todo The parameter for these should really be on the first not the second property
          if (prop.param().find("xml") != std::string::npos)
            return AddMultiInfo(CGUIInfo(window_bool.val, 0, prop.param()));
          int winID = prop.param().empty() ? WINDOW_INVALID : CWindowTranslator::TranslateWindow(prop.param());
          return AddMultiInfo(CGUIInfo(window_bool.val, winID, 0));
        }
      }
    }
    else if (cat.name == "control")
    {
      for (const infomap& control_label : control_labels)
      {
        if (prop.name == control_label.str)
        { //! @todo The parameter for these should really be on the first not the second property
          int controlID = atoi(prop.param().c_str());
          if (controlID)
            return AddMultiInfo(CGUIInfo(control_label.val, controlID, 0));
          return 0;
        }
      }
    }
    else if (cat.name == "controlgroup" && prop.name == "hasfocus")
    {
      int groupID = atoi(cat.param().c_str());
      if (groupID)
        return AddMultiInfo(CGUIInfo(CONTROL_GROUP_HAS_FOCUS, groupID, atoi(prop.param(0).c_str())));
    }
    else if (cat.name == "playlist")
    {
      int ret = -1;
      for (const infomap& i : playlist)
      {
        if (prop.name == i.str)
        {
          ret = i.val;
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
            return AddMultiInfo(CGUIInfo(ret, playlistid, 1));
        }
      }
    }
    else if (cat.name == "pvr")
    {
      for (const infomap& i : pvr)
      {
        if (prop.name == i.str)
          return i.val;
      }
      for (const infomap& pvr_time : pvr_times)
      {
        if (prop.name == pvr_time.str)
          return AddMultiInfo(CGUIInfo(pvr_time.val, TranslateTimeFormat(prop.param())));
      }
    }
    else if (cat.name == "rds")
    {
      if (prop.name == "getline")
        return AddMultiInfo(CGUIInfo(RDS_GET_RADIOTEXT_LINE, atoi(prop.param(0).c_str())));

      for (const infomap& rd : rds)
      {
        if (prop.name == rd.str)
          return rd.val;
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
          if (device == "raspberrypi")
            return SYSTEM_PLATFORM_LINUX_RASPBERRY_PI;
        }
        else return SYSTEM_PLATFORM_LINUX;
      }
      else if (platform == "windows")
        return SYSTEM_PLATFORM_WINDOWS;
      else if (platform == "uwp")
        return SYSTEM_PLATFORM_UWP;
      else if (platform == "darwin")
        return SYSTEM_PLATFORM_DARWIN;
      else if (platform == "osx")
        return SYSTEM_PLATFORM_DARWIN_OSX;
      else if (platform == "ios")
        return SYSTEM_PLATFORM_DARWIN_IOS;
      else if (platform == "android")
        return SYSTEM_PLATFORM_ANDROID;
    }
    if (info[0].name == "musicplayer")
    { //! @todo these two don't allow duration(foo) and also don't allow more than this number of levels...
      if (info[1].name == "position")
      {
        int position = atoi(info[1].param().c_str());
        int value = TranslateMusicPlayerString(info[2].name); // musicplayer.position(foo).bar
        return AddMultiInfo(CGUIInfo(value, 0, position));
      }
      else if (info[1].name == "offset")
      {
        int position = atoi(info[1].param().c_str());
        int value = TranslateMusicPlayerString(info[2].name); // musicplayer.offset(foo).bar
        return AddMultiInfo(CGUIInfo(value, 1, position));
      }
    }
    else if (info[0].name == "container")
    {
      if (info[1].name == "listitem" ||
          info[1].name == "listitemposition" ||
          info[1].name == "listitemabsolute" ||
          info[1].name == "listitemnowrap")
      {
        int id = atoi(info[0].param().c_str());
        int ret = TranslateListItem(info[1], info[2], id, true);
        if (ret)
          listItemDependent = true;
        return ret;
      }
    }
    else if (info[0].name == "control")
    {
      const Property &prop = info[1];
      for (const infomap& control_label : control_labels)
      {
        if (prop.name == control_label.str)
        { //! @todo The parameter for these should really be on the first not the second property
          int controlID = atoi(prop.param().c_str());
          if (controlID)
            return AddMultiInfo(CGUIInfo(control_label.val, controlID, atoi(info[2].param(0).c_str())));
          return 0;
        }
      }
    }
  }

  return 0;
}

int CGUIInfoManager::TranslateListItem(const Property& cat, const Property& prop, int id, bool container)
{
  int ret = 0;
  std::string data3;
  int data4 = 0;
  if (prop.num_params() == 1)
  {
    // special case: map 'property(fanart_image)' to 'art(fanart)'
    if (prop.name == "property" && StringUtils::EqualsNoCase(prop.param(), "fanart_image"))
    {
      ret = LISTITEM_ART;
      data3 = "fanart";
    }
    else if (prop.name == "property" ||
             prop.name == "art" ||
             prop.name == "rating" ||
             prop.name == "votes" ||
             prop.name == "ratingandvotes")
    {
      data3 = prop.param();
    }
    else if (prop.name == "duration")
    {
      data4 = TranslateTimeFormat(prop.param());
    }
  }

  if (ret == 0)
  {
    for (const infomap& listitem_label : listitem_labels) // these ones don't have or need an id
    {
      if (prop.name == listitem_label.str)
      {
        ret = listitem_label.val;
        break;
      }
    }
  }

  if (ret)
  {
    int offset = std::atoi(cat.param().c_str());

    int flags = 0;
    if (cat.name == "listitem")
      flags = INFOFLAG_LISTITEM_WRAP;
    else if (cat.name == "listitemposition")
      flags = INFOFLAG_LISTITEM_POSITION;
    else if (cat.name == "listitemabsolute")
      flags = INFOFLAG_LISTITEM_ABSOLUTE;
    else if (cat.name == "listitemnowrap")
      flags = INFOFLAG_LISTITEM_NOWRAP;

    if (container)
      flags |= INFOFLAG_LISTITEM_CONTAINER;

    return AddMultiInfo(CGUIInfo(ret, id, offset, flags, data3, data4));
  }

  return 0;
}

int CGUIInfoManager::TranslateMusicPlayerString(const std::string &info) const
{
  for (const infomap& i : musicplayer)
  {
    if (info == i.str)
      return i.val;
  }
  return 0;
}

TIME_FORMAT CGUIInfoManager::TranslateTimeFormat(const std::string &format)
{
  if (format.empty())
    return TIME_FORMAT_GUESS;
  else if (StringUtils::EqualsNoCase(format, "hh"))
    return TIME_FORMAT_HH;
  else if (StringUtils::EqualsNoCase(format, "mm"))
    return TIME_FORMAT_MM;
  else if (StringUtils::EqualsNoCase(format, "ss"))
    return TIME_FORMAT_SS;
  else if (StringUtils::EqualsNoCase(format, "hh:mm"))
    return TIME_FORMAT_HH_MM;
  else if (StringUtils::EqualsNoCase(format, "mm:ss"))
    return TIME_FORMAT_MM_SS;
  else if (StringUtils::EqualsNoCase(format, "hh:mm:ss"))
    return TIME_FORMAT_HH_MM_SS;
  else if (StringUtils::EqualsNoCase(format, "hh:mm:ss xx"))
    return TIME_FORMAT_HH_MM_SS_XX;
  else if (StringUtils::EqualsNoCase(format, "h"))
    return TIME_FORMAT_H;
  else if (StringUtils::EqualsNoCase(format, "m"))
    return TIME_FORMAT_M;
  else if (StringUtils::EqualsNoCase(format, "h:mm:ss"))
    return TIME_FORMAT_H_MM_SS;
  else if (StringUtils::EqualsNoCase(format, "h:mm:ss xx"))
    return TIME_FORMAT_H_MM_SS_XX;
  else if (StringUtils::EqualsNoCase(format, "xx"))
    return TIME_FORMAT_XX;
  else if (StringUtils::EqualsNoCase(format, "secs"))
    return TIME_FORMAT_SECS;
  else if (StringUtils::EqualsNoCase(format, "mins"))
    return TIME_FORMAT_MINS;
  else if (StringUtils::EqualsNoCase(format, "hours"))
    return TIME_FORMAT_HOURS;
  return TIME_FORMAT_GUESS;
}

std::string CGUIInfoManager::GetLabel(int info, int contextWindow, std::string *fallback) const
{
  if (info >= CONDITIONAL_LABEL_START && info <= CONDITIONAL_LABEL_END)
  {
    return GetSkinVariableString(info, false);
  }
  else if (info >= MULTI_INFO_START && info <= MULTI_INFO_END)
  {
    return GetMultiInfoLabel(m_multiInfo[info - MULTI_INFO_START], contextWindow);
  }
  else if (info >= LISTITEM_START && info <= LISTITEM_END)
  {
    const CGUIListItemPtr item = GUIINFO::GetCurrentListItem(contextWindow);
    if (item && item->IsFileItem())
      return GetItemLabel(static_cast<CFileItem*>(item.get()), contextWindow, info, fallback);
  }

  std::string strLabel;
  m_infoProviders.GetLabel(strLabel, m_currentFile, contextWindow, CGUIInfo(info), fallback);
  return strLabel;
}

bool CGUIInfoManager::GetInt(int &value, int info, int contextWindow, const CGUIListItem *item /* = nullptr */) const
{
  if (info >= MULTI_INFO_START && info <= MULTI_INFO_END)
  {
    return GetMultiInfoInt(value, m_multiInfo[info - MULTI_INFO_START], contextWindow, item);
  }
  else if (info >= LISTITEM_START && info <= LISTITEM_END)
  {
    CGUIListItemPtr itemPtr;
    if (!item)
    {
      itemPtr = GUIINFO::GetCurrentListItem(contextWindow);
      item = itemPtr.get();
    }
    return GetItemInt(value, item, contextWindow, info);
  }

  value = 0;
  return m_infoProviders.GetInt(value, m_currentFile, contextWindow, CGUIInfo(info));
}

INFO::InfoPtr CGUIInfoManager::Register(const std::string &expression, int context)
{
  std::string condition(CGUIInfoLabel::ReplaceLocalize(expression));
  StringUtils::Trim(condition);

  if (condition.empty())
    return INFO::InfoPtr();

  CSingleLock lock(m_critInfo);
  std::pair<INFOBOOLTYPE::iterator, bool> res;

  if (condition.find_first_of("|+[]!") != condition.npos)
    res = m_bools.insert(std::make_shared<InfoExpression>(condition, context, m_refreshCounter));
  else
    res = m_bools.insert(std::make_shared<InfoSingle>(condition, context, m_refreshCounter));

  if (res.second)
    res.first->get()->Initialize();

  return *(res.first);
}

bool CGUIInfoManager::EvaluateBool(const std::string &expression, int contextWindow /* = 0 */, const CGUIListItemPtr &item /* = nullptr */)
{
  INFO::InfoPtr info = Register(expression, contextWindow);
  if (info)
    return info->Get(item.get());
  return false;
}

bool CGUIInfoManager::GetBool(int condition1, int contextWindow, const CGUIListItem *item)
{
  bool bReturn = false;
  int condition = std::abs(condition1);

  if (condition >= LISTITEM_START && condition < LISTITEM_END)
  {
    CGUIListItemPtr itemPtr;
    if (!item)
    {
      itemPtr = GUIINFO::GetCurrentListItem(contextWindow);
      item = itemPtr.get();
    }
    bReturn = GetItemBool(item, contextWindow, condition);
  }
  else if (condition >= MULTI_INFO_START && condition <= MULTI_INFO_END)
  {
    bReturn = GetMultiInfoBool(m_multiInfo[condition - MULTI_INFO_START], contextWindow, item);
  }
  else if (!m_infoProviders.GetBool(bReturn, m_currentFile, contextWindow, CGUIInfo(condition)))
  {
    // default: use integer value different from 0 as true
    int val;
    bReturn = GetInt(val, condition) && val != 0;
  }

  return (condition1 < 0) ? !bReturn : bReturn;
}

bool CGUIInfoManager::GetMultiInfoBool(const CGUIInfo &info, int contextWindow, const CGUIListItem *item)
{
  bool bReturn = false;
  int condition = std::abs(info.m_info);

  if (condition >= LISTITEM_START && condition <= LISTITEM_END)
  {
    CGUIListItemPtr itemPtr;
    if (!item)
    {
      itemPtr = GUIINFO::GetCurrentListItem(contextWindow, info.GetData1(), info.GetData2(), info.GetInfoFlag());
      item = itemPtr.get();
    }
    if (item)
    {
      if (condition == LISTITEM_PROPERTY)
      {
        if (item->HasProperty(info.GetData3()))
          bReturn = item->GetProperty(info.GetData3()).asBoolean();
      }
      else
        bReturn = GetItemBool(item, contextWindow, condition);
    }
    else
    {
      bReturn = false;
    }
  }
  else if (!m_infoProviders.GetBool(bReturn, m_currentFile, contextWindow, info))
  {
    switch (condition)
    {
      case STRING_IS_EMPTY:
        // note: Get*Image() falls back to Get*Label(), so this should cover all of them
        if (item && item->IsFileItem() && IsListItemInfo(info.GetData1()))
          bReturn = GetItemImage(item, contextWindow, info.GetData1()).empty();
        else
          bReturn = GetImage(info.GetData1(), contextWindow).empty();
        break;
      case STRING_IS_EQUAL:
        {
          std::string compare;
          if (info.GetData2() < 0) // info labels are stored with negative numbers
          {
            int info2 = -info.GetData2();
            CGUIListItemPtr item2;

            if (IsListItemInfo(info2))
            {
              int iResolvedInfo2 = ResolveMultiInfo(info2);
              if (iResolvedInfo2 != 0)
              {
                const GUIINFO::CGUIInfo& resolvedInfo2 = m_multiInfo[iResolvedInfo2 - MULTI_INFO_START];
                if (resolvedInfo2.GetInfoFlag() & INFOFLAG_LISTITEM_CONTAINER)
                  item2 = GUIINFO::GetCurrentListItem(contextWindow, resolvedInfo2.GetData1()); // data1 contains the container id
              }
            }

            if (item2 && item2->IsFileItem())
              compare = GetItemImage(item2.get(), contextWindow, info2);
            else if (item && item->IsFileItem())
              compare = GetItemImage(item, contextWindow, info2);
            else
              compare = GetImage(info2, contextWindow);
          }
          else if (!info.GetData3().empty())
          { // conditional string
            compare = info.GetData3();
          }
          if (item && item->IsFileItem() && IsListItemInfo(info.GetData1()))
            bReturn = StringUtils::EqualsNoCase(GetItemImage(item, contextWindow, info.GetData1()), compare);
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
          int integer = 0;
          if (!GetInt(integer, info.GetData1(), contextWindow, item))
          {
            std::string value;
            if (item && item->IsFileItem() && IsListItemInfo(info.GetData1()))
              value = GetItemImage(item, contextWindow, info.GetData1());
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
      case STRING_STARTS_WITH:
      case STRING_ENDS_WITH:
      case STRING_CONTAINS:
        {
          std::string compare = info.GetData3();
          // our compare string is already in lowercase, so lower case our label as well
          // as std::string::Find() is case sensitive
          std::string label;
          if (item && item->IsFileItem() && IsListItemInfo(info.GetData1()))
            label = GetItemImage(item, contextWindow, info.GetData1());
          else
            label = GetImage(info.GetData1(), contextWindow);
          StringUtils::ToLower(label);
          if (condition == STRING_STARTS_WITH)
            bReturn = StringUtils::StartsWith(label, compare);
          else if (condition == STRING_ENDS_WITH)
            bReturn = StringUtils::EndsWith(label, compare);
          else
            bReturn = label.find(compare) != std::string::npos;
        }
        break;
    }
  }
  return (info.m_info < 0) ? !bReturn : bReturn;
}

bool CGUIInfoManager::GetMultiInfoInt(int &value, const CGUIInfo &info, int contextWindow, const CGUIListItem *item) const
{
  if (info.m_info >= LISTITEM_START && info.m_info <= LISTITEM_END)
  {
    CGUIListItemPtr itemPtr;
    if (!item)
    {
      itemPtr = GUIINFO::GetCurrentListItem(contextWindow, info.GetData1(), info.GetData2(), info.GetInfoFlag());
      item = itemPtr.get();
    }
    if (item)
    {
      if (info.m_info == LISTITEM_PROPERTY)
      {
        if (item->HasProperty(info.GetData3()))
        {
          value = item->GetProperty(info.GetData3()).asInteger();
          return true;
        }
        return false;
      }
      else
        return GetItemInt(value, item, contextWindow, info.m_info);
    }
    else
    {
      return false;
    }
  }

  return m_infoProviders.GetInt(value, m_currentFile, contextWindow, info);
}

std::string CGUIInfoManager::GetMultiInfoLabel(const CGUIInfo &constinfo, int contextWindow, std::string *fallback) const
{
  CGUIInfo info(constinfo);

  if (info.m_info >= LISTITEM_START && info.m_info <= LISTITEM_END)
  {
    const CGUIListItemPtr item = GUIINFO::GetCurrentListItem(contextWindow, info.GetData1(), info.GetData2(), info.GetInfoFlag());
    if (item)
    {
      // Image prioritizes images over labels (in the case of music item ratings for instance)
      return GetMultiInfoItemImage(dynamic_cast<CFileItem*>(item.get()), contextWindow, info, fallback);
    }
    else
    {
      return std::string();
    }
  }
  else if (info.m_info == SYSTEM_ADDON_TITLE ||
           info.m_info == SYSTEM_ADDON_ICON ||
           info.m_info == SYSTEM_ADDON_VERSION)
  {
    if (info.GetData2() == 0)
    {
      // resolve the addon id
      const std::string addonId = GetLabel(info.GetData1(), contextWindow);
      info = CGUIInfo(info.m_info, addonId);
    }
  }

  std::string strValue;
  m_infoProviders.GetLabel(strValue, m_currentFile, contextWindow, info, fallback);
  return strValue;
}

/// \brief Obtains the filename of the image to show from whichever subsystem is needed
std::string CGUIInfoManager::GetImage(int info, int contextWindow, std::string *fallback)
{
  if (info >= CONDITIONAL_LABEL_START && info <= CONDITIONAL_LABEL_END)
  {
    return GetSkinVariableString(info, true);
  }
  else if (info >= MULTI_INFO_START && info <= MULTI_INFO_END)
  {
    return GetMultiInfoLabel(m_multiInfo[info - MULTI_INFO_START], contextWindow, fallback);
  }
  else if (info == LISTITEM_THUMB ||
           info == LISTITEM_ICON ||
           info == LISTITEM_ACTUAL_ICON ||
           info == LISTITEM_OVERLAY ||
           info == LISTITEM_ART)
  {
    const CGUIListItemPtr item = GUIINFO::GetCurrentListItem(contextWindow);
    if (item && item->IsFileItem())
      return GetItemImage(item.get(), contextWindow, info, fallback);
  }

  return GetLabel(info, contextWindow, fallback);
}

void CGUIInfoManager::ResetCurrentItem()
{
  m_currentFile->Reset();
  m_infoProviders.InitCurrentItem(nullptr);
}

void CGUIInfoManager::UpdateCurrentItem(const CFileItem &item)
{
  m_currentFile->UpdateInfo(item);
}

void CGUIInfoManager::SetCurrentItem(const CFileItem &item)
{
  *m_currentFile = item;
  m_currentFile->FillInDefaultIcon();

  m_infoProviders.InitCurrentItem(m_currentFile);

  SetChanged();
  NotifyObservers(ObservableMessageCurrentItem);
  // todo this should be handled by one of the observers above and forwarded
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Info, "xbmc", "OnChanged");
}

void CGUIInfoManager::SetCurrentAlbumThumb(const std::string &thumbFileName)
{
  if (XFILE::CFile::Exists(thumbFileName))
    m_currentFile->SetArt("thumb", thumbFileName);
  else
  {
    m_currentFile->SetArt("thumb", "");
    m_currentFile->FillInDefaultIcon();
  }
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
  INFOBOOLTYPE swapList(&InfoBoolComparator);
  do
  {
    swapList.clear();
    for (auto &item : m_bools)
      if (!item.unique())
        swapList.insert(item);
    m_bools.swap(swapList);
  } while (swapList.size() != m_bools.size());

  // log which ones are used - they should all be gone by now
  for (INFOBOOLTYPE::const_iterator i = m_bools.begin(); i != m_bools.end(); ++i)
    CLog::Log(LOGDEBUG, "Infobool '%s' still used by %u instances", (*i)->GetExpression().c_str(), (unsigned int) i->use_count());
}

void CGUIInfoManager::UpdateAVInfo()
{
  if (CServiceBroker::GetDataCacheCore().HasAVInfoChanges())
  {
    VideoStreamInfo video;
    AudioStreamInfo audio;
    SubtitleStreamInfo subtitle;

    g_application.GetAppPlayer().GetVideoStreamInfo(CURRENT_STREAM, video);
    g_application.GetAppPlayer().GetAudioStreamInfo(CURRENT_STREAM, audio);
    g_application.GetAppPlayer().GetSubtitleStreamInfo(CURRENT_STREAM, subtitle);

    m_infoProviders.UpdateAVInfo(audio, video, subtitle);
  }
}

int CGUIInfoManager::AddMultiInfo(const CGUIInfo &info)
{
  // check to see if we have this info already
  for (unsigned int i = 0; i < m_multiInfo.size(); ++i)
    if (m_multiInfo[i] == info)
      return static_cast<int>(i) + MULTI_INFO_START;
  // return the new offset
  m_multiInfo.emplace_back(info);
  int id = static_cast<int>(m_multiInfo.size()) + MULTI_INFO_START - 1;
  if (id > MULTI_INFO_END)
    CLog::Log(LOGERROR, "%s - too many multiinfo bool/labels in this skin", __FUNCTION__);
  return id;
}

int CGUIInfoManager::ResolveMultiInfo(int info) const
{
  int iLastInfo = 0;

  int iResolvedInfo = info;
  while (iResolvedInfo >= MULTI_INFO_START && iResolvedInfo <= MULTI_INFO_END)
  {
    iLastInfo = iResolvedInfo;
    iResolvedInfo = m_multiInfo[iResolvedInfo - MULTI_INFO_START].m_info;
  }

  return iLastInfo;
}

bool CGUIInfoManager::IsListItemInfo(int info) const
{
  int iResolvedInfo = info;
  while (iResolvedInfo >= MULTI_INFO_START && iResolvedInfo <= MULTI_INFO_END)
    iResolvedInfo = m_multiInfo[iResolvedInfo - MULTI_INFO_START].m_info;

  return (iResolvedInfo >= LISTITEM_START && iResolvedInfo <= LISTITEM_END);
}

bool CGUIInfoManager::GetItemInt(int &value, const CGUIListItem *item, int contextWindow, int info) const
{
  value = 0;

  if (!item)
    return false;

  return m_infoProviders.GetInt(value, item, contextWindow, CGUIInfo(info));
}

std::string CGUIInfoManager::GetItemLabel(const CFileItem *item, int contextWindow, int info, std::string *fallback /* = nullptr */) const
{
  return GetMultiInfoItemLabel(item, contextWindow, CGUIInfo(info), fallback);
}

std::string CGUIInfoManager::GetMultiInfoItemLabel(const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback /* = nullptr */) const
{
  if (!item)
    return std::string();

  std::string value;

  if (info.m_info >= CONDITIONAL_LABEL_START && info.m_info <= CONDITIONAL_LABEL_END)
  {
    return GetSkinVariableString(info.m_info, false, item);
  }
  else if (info.m_info >= MULTI_INFO_START && info.m_info <= MULTI_INFO_END)
  {
    return GetMultiInfoItemLabel(item, contextWindow, m_multiInfo[info.m_info - MULTI_INFO_START], fallback);
  }
  else if (!m_infoProviders.GetLabel(value, item, contextWindow, info, fallback))
  {
    switch (info.m_info)
    {
      case LISTITEM_PROPERTY:
        return item->GetProperty(info.GetData3()).asString();
      case LISTITEM_LABEL:
        return item->GetLabel();
      case LISTITEM_LABEL2:
        return item->GetLabel2();
      case LISTITEM_FILENAME:
      case LISTITEM_FILE_EXTENSION:
      {
        std::string strFile = URIUtils::GetFileName(item->GetPath());
        if (info.m_info == LISTITEM_FILE_EXTENSION)
        {
          std::string strExtension = URIUtils::GetExtension(strFile);
          return StringUtils::TrimLeft(strExtension, ".");
        }
        return strFile;
      }
      case LISTITEM_DATE:
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
      case LISTITEM_PROGRAM_COUNT:
        return StringUtils::Format("%i", item->m_iprogramCount);
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
      case LISTITEM_ART:
        return item->GetArt(info.GetData3());
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
        URIUtils::GetParentPath(item->GetPath(), path);
        path = CURL(path).GetWithoutUserDetails();
        if (info.m_info == LISTITEM_FOLDERNAME)
        {
          URIUtils::RemoveSlashAtEnd(path);
          path = URIUtils::GetFileName(path);
        }
        return path;
      }
      case LISTITEM_FILENAME_AND_PATH:
      {
        std::string path = item->GetPath();
        path = CURL(path).GetWithoutUserDetails();
        return path;
      }
      case LISTITEM_SORT_LETTER:
      {
        std::string letter;
        std::wstring character(1, item->GetSortLabel()[0]);
        StringUtils::ToUpper(character);
        g_charsetConverter.wToUTF8(character, letter);
        return letter;
      }
      case LISTITEM_STARTTIME:
      {
        if (item->m_dateTime.IsValid())
          return item->m_dateTime.GetAsLocalizedTime("", false);
        break;
      }
      case LISTITEM_STARTDATE:
      {
        if (item->m_dateTime.IsValid())
          return item->m_dateTime.GetAsLocalizedDate(true);
        break;
      }
    }
  }

  return value;
}

std::string CGUIInfoManager::GetItemImage(const CGUIListItem *item, int contextWindow, int info, std::string *fallback /*= nullptr*/) const
{
  if (!item || !item->IsFileItem())
    return std::string();

  return GetMultiInfoItemImage(static_cast<const CFileItem*>(item), contextWindow, CGUIInfo(info), fallback);
}

std::string CGUIInfoManager::GetMultiInfoItemImage(const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback /*= nullptr*/) const
{
  if (info.m_info >= CONDITIONAL_LABEL_START && info.m_info <= CONDITIONAL_LABEL_END)
  {
    return GetSkinVariableString(info.m_info, true, item);
  }
  else if (info.m_info >= MULTI_INFO_START && info.m_info <= MULTI_INFO_END)
  {
    return GetMultiInfoItemImage(item, contextWindow, m_multiInfo[info.m_info - MULTI_INFO_START], fallback);
  }

  return GetMultiInfoItemLabel(item, contextWindow, info, fallback);
}

bool CGUIInfoManager::GetItemBool(const CGUIListItem *item, int contextWindow, int condition) const
{
  if (!item)
    return false;

  bool value = false;
  if (!m_infoProviders.GetBool(value, item, contextWindow, CGUIInfo(condition)))
  {
    switch (condition)
    {
      case LISTITEM_ISSELECTED:
        return item->IsSelected();
      case LISTITEM_IS_FOLDER:
        return item->m_bIsFolder;
      case LISTITEM_IS_PARENTFOLDER:
      {
        if (item->IsFileItem())
        {
          const CFileItem *pItem = static_cast<const CFileItem *>(item);
          return pItem->IsParentFolder();
        }
        break;
      }
    }
  }

  return value;
}

void CGUIInfoManager::ResetCache()
{
  // mark our infobools as dirty
  CSingleLock lock(m_critInfo);
  ++m_refreshCounter;
}

void CGUIInfoManager::SetCurrentVideoTag(const CVideoInfoTag &tag)
{
  m_currentFile->SetFromVideoInfoTag(tag);
  m_currentFile->m_lStartOffset = 0;
}

void CGUIInfoManager::SetCurrentSongTag(const MUSIC_INFO::CMusicInfoTag &tag)
{
  m_currentFile->SetFromMusicInfoTag(tag);
  m_currentFile->m_lStartOffset = 0;
}

const MUSIC_INFO::CMusicInfoTag* CGUIInfoManager::GetCurrentSongTag() const
{
  if (m_currentFile->HasMusicInfoTag())
    return m_currentFile->GetMusicInfoTag();

  return nullptr;
}

const CVideoInfoTag* CGUIInfoManager::GetCurrentMovieTag() const
{
  if (m_currentFile->HasVideoInfoTag())
    return m_currentFile->GetVideoInfoTag();

  return nullptr;
}

int CGUIInfoManager::RegisterSkinVariableString(const CSkinVariableString* info)
{
  if (!info)
    return 0;

  CSingleLock lock(m_critInfo);
  m_skinVariableStrings.emplace_back(*info);
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
                                                   const CGUIListItem *item /*= nullptr*/) const
{
  info -= CONDITIONAL_LABEL_START;
  if (info >= 0 && info < static_cast<int>(m_skinVariableStrings.size()))
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
        infoLabels->emplace_back(GetLabel(TranslateString(param)));
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
    CFileItem* item = static_cast<CFileItem*>(pMsg->lpVoid);
    if (!item)
      return;

    if (pMsg->param1 == 1 && item->HasMusicInfoTag()) // only grab music tag
      SetCurrentSongTag(*item->GetMusicInfoTag());
    else if (pMsg->param1 == 2 && item->HasVideoInfoTag()) // only grab video tag
      SetCurrentVideoTag(*item->GetVideoInfoTag());
    else
      SetCurrentItem(*item);

    delete item;
  }
  break;

  default:
    break;
  }
}

void CGUIInfoManager::RegisterInfoProvider(IGUIInfoProvider *provider)
{
  if (!CServiceBroker::GetWinSystem())
    return;

  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  m_infoProviders.RegisterProvider(provider, false);
}

void CGUIInfoManager::UnregisterInfoProvider(IGUIInfoProvider *provider)
{
  if (!CServiceBroker::GetWinSystem())
    return;

  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  m_infoProviders.UnregisterProvider(provider);
}
