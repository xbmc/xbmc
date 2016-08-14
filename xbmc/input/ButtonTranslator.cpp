/*
 *      Copyright (C) 2005-2015 Team XBMC
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ButtonTranslator.h"

#include <algorithm>
#include <utility>

#include "FileItem.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "guilib/WindowIDs.h"
#include "input/Key.h"
#include "input/MouseStat.h"
#include "input/XBMC_keytable.h"
#include "interfaces/builtins/Builtins.h"
#include "profiles/ProfilesManager.h"
#include "system.h"
#include "Util.h"
#include "utils/log.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "XBIRRemote.h"

using namespace XFILE;

typedef struct
{
  const char* name;
  int action;
} ActionMapping;

typedef struct
{
  int origin;
  int target;
} WindowMapping;

static const ActionMapping actions[] =
{
    { "left"                     , ACTION_MOVE_LEFT },
    { "right"                    , ACTION_MOVE_RIGHT },
    { "up"                       , ACTION_MOVE_UP },
    { "down"                     , ACTION_MOVE_DOWN },
    { "pageup"                   , ACTION_PAGE_UP },
    { "pagedown"                 , ACTION_PAGE_DOWN },
    { "select"                   , ACTION_SELECT_ITEM },
    { "highlight"                , ACTION_HIGHLIGHT_ITEM },
    { "parentdir"                , ACTION_NAV_BACK },                   // backward compatibility
    { "parentfolder"             , ACTION_PARENT_DIR },
    { "back"                     , ACTION_NAV_BACK },
    { "menu"                     , ACTION_MENU},
    { "previousmenu"             , ACTION_PREVIOUS_MENU },
    { "info"                     , ACTION_SHOW_INFO },
    { "pause"                    , ACTION_PAUSE },
    { "stop"                     , ACTION_STOP },
    { "skipnext"                 , ACTION_NEXT_ITEM },
    { "skipprevious"             , ACTION_PREV_ITEM },
    { "fullscreen"               , ACTION_SHOW_GUI },
    { "aspectratio"              , ACTION_ASPECT_RATIO },
    { "stepforward"              , ACTION_STEP_FORWARD },
    { "stepback"                 , ACTION_STEP_BACK },
    { "bigstepforward"           , ACTION_BIG_STEP_FORWARD },
    { "bigstepback"              , ACTION_BIG_STEP_BACK },
    { "chapterorbigstepforward"  , ACTION_CHAPTER_OR_BIG_STEP_FORWARD },
    { "chapterorbigstepback"     , ACTION_CHAPTER_OR_BIG_STEP_BACK },
    { "osd"                      , ACTION_SHOW_OSD },
    { "showsubtitles"            , ACTION_SHOW_SUBTITLES },
    { "nextsubtitle"             , ACTION_NEXT_SUBTITLE },
    { "cyclesubtitle"            , ACTION_CYCLE_SUBTITLE },
    { "playerdebug"              , ACTION_PLAYER_DEBUG },
    { "playerprocessinfo"        , ACTION_PLAYER_PROCESS_INFO },
    { "nextpicture"              , ACTION_NEXT_PICTURE },
    { "previouspicture"          , ACTION_PREV_PICTURE },
    { "zoomout"                  , ACTION_ZOOM_OUT },
    { "zoomin"                   , ACTION_ZOOM_IN },
    { "playlist"                 , ACTION_SHOW_PLAYLIST },
    { "queue"                    , ACTION_QUEUE_ITEM },
    { "zoomnormal"               , ACTION_ZOOM_LEVEL_NORMAL },
    { "zoomlevel1"               , ACTION_ZOOM_LEVEL_1 },
    { "zoomlevel2"               , ACTION_ZOOM_LEVEL_2 },
    { "zoomlevel3"               , ACTION_ZOOM_LEVEL_3 },
    { "zoomlevel4"               , ACTION_ZOOM_LEVEL_4 },
    { "zoomlevel5"               , ACTION_ZOOM_LEVEL_5 },
    { "zoomlevel6"               , ACTION_ZOOM_LEVEL_6 },
    { "zoomlevel7"               , ACTION_ZOOM_LEVEL_7 },
    { "zoomlevel8"               , ACTION_ZOOM_LEVEL_8 },
    { "zoomlevel9"               , ACTION_ZOOM_LEVEL_9 },
    { "nextcalibration"          , ACTION_CALIBRATE_SWAP_ARROWS },
    { "resetcalibration"         , ACTION_CALIBRATE_RESET },
    { "analogmove"               , ACTION_ANALOG_MOVE },
    { "analogmovex"              , ACTION_ANALOG_MOVE_X },
    { "analogmovey"              , ACTION_ANALOG_MOVE_Y },
    { "rotate"                   , ACTION_ROTATE_PICTURE_CW },
    { "rotateccw"                , ACTION_ROTATE_PICTURE_CCW },
    { "close"                    , ACTION_NAV_BACK },                    // backwards compatibility
    { "subtitledelayminus"       , ACTION_SUBTITLE_DELAY_MIN },
    { "subtitledelay"            , ACTION_SUBTITLE_DELAY },
    { "subtitledelayplus"        , ACTION_SUBTITLE_DELAY_PLUS },
    { "audiodelayminus"          , ACTION_AUDIO_DELAY_MIN },
    { "audiodelay"               , ACTION_AUDIO_DELAY },
    { "audiodelayplus"           , ACTION_AUDIO_DELAY_PLUS },
    { "subtitleshiftup"          , ACTION_SUBTITLE_VSHIFT_UP },
    { "subtitleshiftdown"        , ACTION_SUBTITLE_VSHIFT_DOWN },
    { "subtitlealign"            , ACTION_SUBTITLE_ALIGN },
    { "audionextlanguage"        , ACTION_AUDIO_NEXT_LANGUAGE },
    { "verticalshiftup"          , ACTION_VSHIFT_UP },
    { "verticalshiftdown"        , ACTION_VSHIFT_DOWN },
    { "nextresolution"           , ACTION_CHANGE_RESOLUTION },
    { "audiotoggledigital"       , ACTION_TOGGLE_DIGITAL_ANALOG },
    { "number0"                  , REMOTE_0 },
    { "number1"                  , REMOTE_1 },
    { "number2"                  , REMOTE_2 },
    { "number3"                  , REMOTE_3 },
    { "number4"                  , REMOTE_4 },
    { "number5"                  , REMOTE_5 },
    { "number6"                  , REMOTE_6 },
    { "number7"                  , REMOTE_7 },
    { "number8"                  , REMOTE_8 },
    { "number9"                  , REMOTE_9 },
    { "smallstepback"            , ACTION_SMALL_STEP_BACK },
    { "fastforward"              , ACTION_PLAYER_FORWARD },
    { "rewind"                   , ACTION_PLAYER_REWIND },
    { "play"                     , ACTION_PLAYER_PLAY },
    { "playpause"                , ACTION_PLAYER_PLAYPAUSE },
    { "switchplayer"             , ACTION_SWITCH_PLAYER },
    { "delete"                   , ACTION_DELETE_ITEM },
    { "copy"                     , ACTION_COPY_ITEM },
    { "move"                     , ACTION_MOVE_ITEM },
    { "screenshot"               , ACTION_TAKE_SCREENSHOT },
    { "rename"                   , ACTION_RENAME_ITEM },
    { "togglewatched"            , ACTION_TOGGLE_WATCHED },
    { "scanitem"                 , ACTION_SCAN_ITEM },
    { "reloadkeymaps"            , ACTION_RELOAD_KEYMAPS },
    { "volumeup"                 , ACTION_VOLUME_UP },
    { "volumedown"               , ACTION_VOLUME_DOWN },
    { "mute"                     , ACTION_MUTE },
    { "backspace"                , ACTION_BACKSPACE },
    { "scrollup"                 , ACTION_SCROLL_UP },
    { "scrolldown"               , ACTION_SCROLL_DOWN },
    { "analogfastforward"        , ACTION_ANALOG_FORWARD },
    { "analogrewind"             , ACTION_ANALOG_REWIND },
    { "moveitemup"               , ACTION_MOVE_ITEM_UP },
    { "moveitemdown"             , ACTION_MOVE_ITEM_DOWN },
    { "contextmenu"              , ACTION_CONTEXT_MENU },
    { "shift"                    , ACTION_SHIFT },
    { "symbols"                  , ACTION_SYMBOLS },
    { "cursorleft"               , ACTION_CURSOR_LEFT },
    { "cursorright"              , ACTION_CURSOR_RIGHT },
    { "showtime"                 , ACTION_SHOW_OSD_TIME },
    { "analogseekforward"        , ACTION_ANALOG_SEEK_FORWARD },
    { "analogseekback"           , ACTION_ANALOG_SEEK_BACK },
    { "showpreset"               , ACTION_VIS_PRESET_SHOW },
    { "nextpreset"               , ACTION_VIS_PRESET_NEXT },
    { "previouspreset"           , ACTION_VIS_PRESET_PREV },
    { "lockpreset"               , ACTION_VIS_PRESET_LOCK },
    { "randompreset"             , ACTION_VIS_PRESET_RANDOM },
    { "increasevisrating"        , ACTION_VIS_RATE_PRESET_PLUS },
    { "decreasevisrating"        , ACTION_VIS_RATE_PRESET_MINUS },
    { "showvideomenu"            , ACTION_SHOW_VIDEOMENU },
    { "enter"                    , ACTION_ENTER },
    { "increaserating"           , ACTION_INCREASE_RATING },
    { "decreaserating"           , ACTION_DECREASE_RATING },
    { "setrating"                , ACTION_SET_RATING },
    { "togglefullscreen"         , ACTION_TOGGLE_FULLSCREEN },
    { "nextscene"                , ACTION_NEXT_SCENE },
    { "previousscene"            , ACTION_PREV_SCENE },
    { "nextletter"               , ACTION_NEXT_LETTER },
    { "prevletter"               , ACTION_PREV_LETTER },
    { "jumpsms2"                 , ACTION_JUMP_SMS2 },
    { "jumpsms3"                 , ACTION_JUMP_SMS3 },
    { "jumpsms4"                 , ACTION_JUMP_SMS4 },
    { "jumpsms5"                 , ACTION_JUMP_SMS5 },
    { "jumpsms6"                 , ACTION_JUMP_SMS6 },
    { "jumpsms7"                 , ACTION_JUMP_SMS7 },
    { "jumpsms8"                 , ACTION_JUMP_SMS8 },
    { "jumpsms9"                 , ACTION_JUMP_SMS9 },
    { "filter"                   , ACTION_FILTER },
    { "filterclear"              , ACTION_FILTER_CLEAR },
    { "filtersms2"               , ACTION_FILTER_SMS2 },
    { "filtersms3"               , ACTION_FILTER_SMS3 },
    { "filtersms4"               , ACTION_FILTER_SMS4 },
    { "filtersms5"               , ACTION_FILTER_SMS5 },
    { "filtersms6"               , ACTION_FILTER_SMS6 },
    { "filtersms7"               , ACTION_FILTER_SMS7 },
    { "filtersms8"               , ACTION_FILTER_SMS8 },
    { "filtersms9"               , ACTION_FILTER_SMS9 },
    { "firstpage"                , ACTION_FIRST_PAGE },
    { "lastpage"                 , ACTION_LAST_PAGE },
    { "guiprofile"               , ACTION_GUIPROFILE_BEGIN },
    { "red"                      , ACTION_TELETEXT_RED },
    { "green"                    , ACTION_TELETEXT_GREEN },
    { "yellow"                   , ACTION_TELETEXT_YELLOW },
    { "blue"                     , ACTION_TELETEXT_BLUE },
    { "increasepar"              , ACTION_INCREASE_PAR },
    { "decreasepar"              , ACTION_DECREASE_PAR },
    { "volampup"                 , ACTION_VOLAMP_UP },
    { "volampdown"               , ACTION_VOLAMP_DOWN },
    { "volumeamplification"      , ACTION_VOLAMP },
    { "createbookmark"           , ACTION_CREATE_BOOKMARK },
    { "createepisodebookmark"    , ACTION_CREATE_EPISODE_BOOKMARK },
    { "settingsreset"            , ACTION_SETTINGS_RESET },
    { "settingslevelchange"      , ACTION_SETTINGS_LEVEL_CHANGE },

    // 3D movie playback/GUI
    { "stereomode"               , ACTION_STEREOMODE_SELECT },          // cycle 3D modes, for now an alias for next
    { "nextstereomode"           , ACTION_STEREOMODE_NEXT },
    { "previousstereomode"       , ACTION_STEREOMODE_PREVIOUS },
    { "togglestereomode"         , ACTION_STEREOMODE_TOGGLE },
    { "stereomodetomono"         , ACTION_STEREOMODE_TOMONO },

    // PVR actions
    { "channelup"                , ACTION_CHANNEL_UP },
    { "channeldown"              , ACTION_CHANNEL_DOWN },
    { "previouschannelgroup"     , ACTION_PREVIOUS_CHANNELGROUP },
    { "nextchannelgroup"         , ACTION_NEXT_CHANNELGROUP },
    { "playpvr"                  , ACTION_PVR_PLAY },
    { "playpvrtv"                , ACTION_PVR_PLAY_TV },
    { "playpvrradio"             , ACTION_PVR_PLAY_RADIO },
    { "record"                   , ACTION_RECORD },
    { "togglecommskip"           , ACTION_TOGGLE_COMMSKIP },
    { "showtimerrule"            , ACTION_PVR_SHOW_TIMER_RULE },

    // Mouse actions
    { "leftclick"                , ACTION_MOUSE_LEFT_CLICK },
    { "rightclick"               , ACTION_MOUSE_RIGHT_CLICK },
    { "middleclick"              , ACTION_MOUSE_MIDDLE_CLICK },
    { "doubleclick"              , ACTION_MOUSE_DOUBLE_CLICK },
    { "longclick"                , ACTION_MOUSE_LONG_CLICK },
    { "wheelup"                  , ACTION_MOUSE_WHEEL_UP },
    { "wheeldown"                , ACTION_MOUSE_WHEEL_DOWN },
    { "mousedrag"                , ACTION_MOUSE_DRAG },
    { "mousemove"                , ACTION_MOUSE_MOVE },

    // Touch
    { "tap"                      , ACTION_TOUCH_TAP },
    { "longpress"                , ACTION_TOUCH_LONGPRESS },
    { "pangesture"               , ACTION_GESTURE_PAN },
    { "zoomgesture"              , ACTION_GESTURE_ZOOM },
    { "rotategesture"            , ACTION_GESTURE_ROTATE },
    { "swipeleft"                , ACTION_GESTURE_SWIPE_LEFT },
    { "swiperight"               , ACTION_GESTURE_SWIPE_RIGHT },
    { "swipeup"                  , ACTION_GESTURE_SWIPE_UP },
    { "swipedown"                , ACTION_GESTURE_SWIPE_DOWN },

    // Do nothing / error action
    { "error"                    , ACTION_ERROR },
    { "noop"                     , ACTION_NOOP }
};

bool CButtonTranslator::IsAnalog(int actionID)
{
  switch (actionID)
  {
  case ACTION_ANALOG_SEEK_FORWARD:
  case ACTION_ANALOG_SEEK_BACK:
  case ACTION_SCROLL_UP:
  case ACTION_SCROLL_DOWN:
  case ACTION_ANALOG_FORWARD:
  case ACTION_ANALOG_REWIND:
  case ACTION_ANALOG_MOVE:
  case ACTION_ANALOG_MOVE_X:
  case ACTION_ANALOG_MOVE_Y:
  case ACTION_CURSOR_LEFT:
  case ACTION_CURSOR_RIGHT:
  case ACTION_VOLUME_UP:
  case ACTION_VOLUME_DOWN:
  case ACTION_ZOOM_IN:
  case ACTION_ZOOM_OUT:
    return true;
  default:
    return false;
  }
}

static const ActionMapping windows[] =
{
    { "home"                     , WINDOW_HOME },
    { "programs"                 , WINDOW_PROGRAMS },
    { "pictures"                 , WINDOW_PICTURES },
    { "filemanager"              , WINDOW_FILES },
    { "settings"                 , WINDOW_SETTINGS_MENU },
    { "music"                    , WINDOW_MUSIC_NAV },
    { "videos"                   , WINDOW_VIDEO_NAV },
    { "tvchannels"               , WINDOW_TV_CHANNELS },
    { "tvrecordings"             , WINDOW_TV_RECORDINGS },
    { "tvguide"                  , WINDOW_TV_GUIDE },
    { "tvtimers"                 , WINDOW_TV_TIMERS },
    { "tvsearch"                 , WINDOW_TV_SEARCH },
    { "radiochannels"            , WINDOW_RADIO_CHANNELS },
    { "radiorecordings"          , WINDOW_RADIO_RECORDINGS },
    { "radioguide"               , WINDOW_RADIO_GUIDE },
    { "radiotimers"              , WINDOW_RADIO_TIMERS },
    { "radiosearch"              , WINDOW_RADIO_SEARCH },
    { "gamecontrollers"          , WINDOW_DIALOG_GAME_CONTROLLERS },
    { "pvrguideinfo"             , WINDOW_DIALOG_PVR_GUIDE_INFO },
    { "pvrrecordinginfo"         , WINDOW_DIALOG_PVR_RECORDING_INFO },
    { "pvrradiordsinfo"          , WINDOW_DIALOG_PVR_RADIO_RDS_INFO },
    { "pvrtimersetting"          , WINDOW_DIALOG_PVR_TIMER_SETTING },
    { "pvrgroupmanager"          , WINDOW_DIALOG_PVR_GROUP_MANAGER },
    { "pvrchannelmanager"        , WINDOW_DIALOG_PVR_CHANNEL_MANAGER },
    { "pvrguidesearch"           , WINDOW_DIALOG_PVR_GUIDE_SEARCH },
    { "pvrchannelscan"           , WINDOW_DIALOG_PVR_CHANNEL_SCAN },
    { "pvrupdateprogress"        , WINDOW_DIALOG_PVR_UPDATE_PROGRESS },
    { "pvrosdchannels"           , WINDOW_DIALOG_PVR_OSD_CHANNELS },
    { "pvrosdguide"              , WINDOW_DIALOG_PVR_OSD_GUIDE },
    { "pvrosdteletext"           , WINDOW_DIALOG_OSD_TELETEXT },
    { "systeminfo"               , WINDOW_SYSTEM_INFORMATION },
    { "testpattern"              , WINDOW_TEST_PATTERN },
    { "screencalibration"        , WINDOW_SCREEN_CALIBRATION },
    { "systemsettings"           , WINDOW_SETTINGS_SYSTEM },
    { "servicesettings"          , WINDOW_SETTINGS_SERVICE },
    { "pvrsettings"              , WINDOW_SETTINGS_MYPVR },
    { "playersettings"           , WINDOW_SETTINGS_PLAYER },
    { "mediasettings"            , WINDOW_SETTINGS_MEDIA },
    { "interfacesettings"        , WINDOW_SETTINGS_INTERFACE },
    { "appearancesettings"       , WINDOW_SETTINGS_INTERFACE },	// backward compatibility to v16
    { "videoplaylist"            , WINDOW_VIDEO_PLAYLIST },
    { "loginscreen"              , WINDOW_LOGIN_SCREEN },
    { "profiles"                 , WINDOW_SETTINGS_PROFILES },
    { "skinsettings"             , WINDOW_SKIN_SETTINGS },
    { "addonbrowser"             , WINDOW_ADDON_BROWSER },
    { "yesnodialog"              , WINDOW_DIALOG_YES_NO },
    { "progressdialog"           , WINDOW_DIALOG_PROGRESS },
    { "virtualkeyboard"          , WINDOW_DIALOG_KEYBOARD },
    { "volumebar"                , WINDOW_DIALOG_VOLUME_BAR },
    { "submenu"                  , WINDOW_DIALOG_SUB_MENU },
    { "favourites"               , WINDOW_DIALOG_FAVOURITES },
    { "contextmenu"              , WINDOW_DIALOG_CONTEXT_MENU },
    { "notification"             , WINDOW_DIALOG_KAI_TOAST },
    { "numericinput"             , WINDOW_DIALOG_NUMERIC },
    { "gamepadinput"             , WINDOW_DIALOG_GAMEPAD },
    { "shutdownmenu"             , WINDOW_DIALOG_BUTTON_MENU },
    { "playercontrols"           , WINDOW_DIALOG_PLAYER_CONTROLS },
    { "playerprocessinfo"        , WINDOW_DIALOG_PLAYER_PROCESS_INFO },
    { "seekbar"                  , WINDOW_DIALOG_SEEK_BAR },
    { "musicosd"                 , WINDOW_DIALOG_MUSIC_OSD },
    { "addonsettings"            , WINDOW_DIALOG_ADDON_SETTINGS },
    { "visualisationpresetlist"  , WINDOW_DIALOG_VIS_PRESET_LIST },
    { "osdcmssettings"           , WINDOW_DIALOG_CMS_OSD_SETTINGS },
    { "osdvideosettings"         , WINDOW_DIALOG_VIDEO_OSD_SETTINGS },
    { "osdaudiosettings"         , WINDOW_DIALOG_AUDIO_OSD_SETTINGS },
    { "audiodspmanager"          , WINDOW_DIALOG_AUDIO_DSP_MANAGER },
    { "osdaudiodspsettings"      , WINDOW_DIALOG_AUDIO_DSP_OSD_SETTINGS },
    { "videobookmarks"           , WINDOW_DIALOG_VIDEO_BOOKMARKS },
    { "filebrowser"              , WINDOW_DIALOG_FILE_BROWSER },
    { "networksetup"             , WINDOW_DIALOG_NETWORK_SETUP },
    { "mediasource"              , WINDOW_DIALOG_MEDIA_SOURCE },
    { "profilesettings"          , WINDOW_DIALOG_PROFILE_SETTINGS },
    { "locksettings"             , WINDOW_DIALOG_LOCK_SETTINGS },
    { "contentsettings"          , WINDOW_DIALOG_CONTENT_SETTINGS },
    { "songinformation"          , WINDOW_DIALOG_SONG_INFO },
    { "smartplaylisteditor"      , WINDOW_DIALOG_SMART_PLAYLIST_EDITOR },
    { "smartplaylistrule"        , WINDOW_DIALOG_SMART_PLAYLIST_RULE },
    { "busydialog"               , WINDOW_DIALOG_BUSY },
    { "pictureinfo"              , WINDOW_DIALOG_PICTURE_INFO },
    { "accesspoints"             , WINDOW_DIALOG_ACCESS_POINTS },
    { "fullscreeninfo"           , WINDOW_DIALOG_FULLSCREEN_INFO },
    { "sliderdialog"             , WINDOW_DIALOG_SLIDER },
    { "addoninformation"         , WINDOW_DIALOG_ADDON_INFO },
    { "subtitlesearch"           , WINDOW_DIALOG_SUBTITLES },
    { "musicplaylist"            , WINDOW_MUSIC_PLAYLIST },
    { "musicplaylisteditor"      , WINDOW_MUSIC_PLAYLIST_EDITOR },
    { "teletext"                 , WINDOW_DIALOG_OSD_TELETEXT },
    { "selectdialog"             , WINDOW_DIALOG_SELECT },
    { "musicinformation"         , WINDOW_DIALOG_MUSIC_INFO },
    { "okdialog"                 , WINDOW_DIALOG_OK },
    { "movieinformation"         , WINDOW_DIALOG_VIDEO_INFO },
    { "textviewer"               , WINDOW_DIALOG_TEXT_VIEWER },
    { "fullscreenvideo"          , WINDOW_FULLSCREEN_VIDEO },
    { "fullscreenlivetv"         , WINDOW_FULLSCREEN_LIVETV },         // virtual window/keymap section for PVR specific bindings in fullscreen playback (which internally uses WINDOW_FULLSCREEN_VIDEO)
    { "fullscreenradio"          , WINDOW_FULLSCREEN_RADIO },          // virtual window for fullscreen radio, uses WINDOW_VISUALISATION as fallback
    { "visualisation"            , WINDOW_VISUALISATION },
    { "slideshow"                , WINDOW_SLIDESHOW },
    { "weather"                  , WINDOW_WEATHER },
    { "screensaver"              , WINDOW_SCREENSAVER },
    { "videoosd"                 , WINDOW_DIALOG_VIDEO_OSD },
    { "videomenu"                , WINDOW_VIDEO_MENU },
    { "videotimeseek"            , WINDOW_VIDEO_TIME_SEEK },
    { "startwindow"              , WINDOW_START },
    { "startup"                  , WINDOW_STARTUP_ANIM },
    { "peripheralsettings"       , WINDOW_DIALOG_PERIPHERAL_SETTINGS },
    { "extendedprogressdialog"   , WINDOW_DIALOG_EXT_PROGRESS },
    { "mediafilter"              , WINDOW_DIALOG_MEDIA_FILTER },
    { "addon"                    , WINDOW_ADDON_START },
    { "eventlog"                 , WINDOW_EVENT_LOG},
    { "tvtimerrules"             , WINDOW_TV_TIMER_RULES},
    { "radiotimerrules"          , WINDOW_RADIO_TIMER_RULES}
};

static const ActionMapping mousekeys[] =
{
    { "click"                    , KEY_MOUSE_CLICK },
    { "leftclick"                , KEY_MOUSE_CLICK },
    { "rightclick"               , KEY_MOUSE_RIGHTCLICK },
    { "middleclick"              , KEY_MOUSE_MIDDLECLICK },
    { "doubleclick"              , KEY_MOUSE_DOUBLE_CLICK },
    { "longclick"                , KEY_MOUSE_LONG_CLICK },
    { "wheelup"                  , KEY_MOUSE_WHEEL_UP },
    { "wheeldown"                , KEY_MOUSE_WHEEL_DOWN },
    { "mousemove"                , KEY_MOUSE_MOVE },
    { "mousedrag"                , KEY_MOUSE_DRAG },
    { "mousedragstart"           , KEY_MOUSE_DRAG_START },
    { "mousedragend"             , KEY_MOUSE_DRAG_END },
    { "mouserdrag"               , KEY_MOUSE_RDRAG },
    { "mouserdragstart"          , KEY_MOUSE_RDRAG_START },
    { "mouserdragend"            , KEY_MOUSE_RDRAG_END }
};

static const ActionMapping touchcommands[] =
{
    { "tap"                      , ACTION_TOUCH_TAP },
    { "longpress"                , ACTION_TOUCH_LONGPRESS },
    { "pan"                      , ACTION_GESTURE_PAN },
    { "zoom"                     , ACTION_GESTURE_ZOOM },
    { "rotate"                   , ACTION_GESTURE_ROTATE },
    { "swipeleft"                , ACTION_GESTURE_SWIPE_LEFT },
    { "swiperight"               , ACTION_GESTURE_SWIPE_RIGHT },
    { "swipeup"                  , ACTION_GESTURE_SWIPE_UP },
    { "swipedown"                , ACTION_GESTURE_SWIPE_DOWN }
};

static const WindowMapping fallbackWindows[] =
{
    { WINDOW_FULLSCREEN_LIVETV   , WINDOW_FULLSCREEN_VIDEO },
    { WINDOW_FULLSCREEN_RADIO    , WINDOW_VISUALISATION }
};

#ifdef TARGET_WINDOWS
static const ActionMapping appcommands[] =
{
    { "browser_back"             , APPCOMMAND_BROWSER_BACKWARD },
    { "browser_forward"          , APPCOMMAND_BROWSER_FORWARD },
    { "browser_refresh"          , APPCOMMAND_BROWSER_REFRESH },
    { "browser_stop"             , APPCOMMAND_BROWSER_STOP },
    { "browser_search"           , APPCOMMAND_BROWSER_SEARCH },
    { "browser_favorites"        , APPCOMMAND_BROWSER_FAVORITES },
    { "browser_home"             , APPCOMMAND_BROWSER_HOME },
    { "volume_mute"              , APPCOMMAND_VOLUME_MUTE },
    { "volume_down"              , APPCOMMAND_VOLUME_DOWN },
    { "volume_up"                , APPCOMMAND_VOLUME_UP },
    { "next_track"               , APPCOMMAND_MEDIA_NEXTTRACK },
    { "prev_track"               , APPCOMMAND_MEDIA_PREVIOUSTRACK },
    { "stop"                     , APPCOMMAND_MEDIA_STOP },
    { "play_pause"               , APPCOMMAND_MEDIA_PLAY_PAUSE },
    { "launch_mail"              , APPCOMMAND_LAUNCH_MAIL },
    { "launch_media_select"      , APPCOMMAND_LAUNCH_MEDIA_SELECT },
    { "launch_app1"              , APPCOMMAND_LAUNCH_APP1 },
    { "launch_app2"              , APPCOMMAND_LAUNCH_APP2 },
    { "play"                     , APPCOMMAND_MEDIA_PLAY },
    { "pause"                    , APPCOMMAND_MEDIA_PAUSE },
    { "fastforward"              , APPCOMMAND_MEDIA_FAST_FORWARD },
    { "rewind"                   , APPCOMMAND_MEDIA_REWIND },
    { "channelup"                , APPCOMMAND_MEDIA_CHANNEL_UP },
    { "channeldown"              , APPCOMMAND_MEDIA_CHANNEL_DOWN }
};
#endif

CButtonTranslator& CButtonTranslator::GetInstance()
{
  static CButtonTranslator sl_instance;
  return sl_instance;
}

CButtonTranslator::CButtonTranslator()
{
  m_deviceList.clear();
  m_Loaded = false;
}

#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
void CButtonTranslator::ClearLircButtonMapEntries()
{
  std::vector<lircButtonMap*> maps;
  for (std::map<std::string,lircButtonMap*>::iterator it  = lircRemotesMap.begin();
                                                 it != lircRemotesMap.end();++it)
    maps.push_back(it->second);
  sort(maps.begin(),maps.end());
  std::vector<lircButtonMap*>::iterator itend = unique(maps.begin(),maps.end());
  for (std::vector<lircButtonMap*>::iterator it = maps.begin(); it != itend;++it)
    delete *it;
}
#endif

CButtonTranslator::~CButtonTranslator()
{
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  ClearLircButtonMapEntries();
#endif
}

// Add the supplied device name to the list of connected devices
void CButtonTranslator::AddDevice(std::string& strDevice)
{
  // Only add the device if it isn't already in the list
  std::list<std::string>::iterator it;
  for (it = m_deviceList.begin(); it != m_deviceList.end(); ++it)
    if (*it == strDevice)
      return;

  // Add the device
  m_deviceList.push_back(strDevice);
  m_deviceList.sort();

  // New device added so reload the key mappings
  Load();
}

void CButtonTranslator::RemoveDevice(std::string& strDevice)
{
  // Find the device
  std::list<std::string>::iterator it;
  for (it = m_deviceList.begin(); it != m_deviceList.end(); ++it)
    if (*it == strDevice)
      break;
  if (it == m_deviceList.end())
    return;

  // Remove the device
  m_deviceList.remove(strDevice);

  // Device removed so reload the key mappings
  Load();
}

bool CButtonTranslator::Load(bool AlwaysLoad)
{
  m_translatorMap.clear();

  // Directories to search for keymaps. They're applied in this order,
  // so keymaps in profile/keymaps/ override e.g. system/keymaps
  static const char* DIRS_TO_CHECK[] = {
    "special://xbmc/system/keymaps/",
    "special://masterprofile/keymaps/",
    "special://profile/keymaps/"
  };
  bool success = false;

  for (unsigned int dirIndex = 0; dirIndex < ARRAY_SIZE(DIRS_TO_CHECK); ++dirIndex)
  {
    if (XFILE::CDirectory::Exists(DIRS_TO_CHECK[dirIndex]))
    {
      CFileItemList files;
      XFILE::CDirectory::GetDirectory(DIRS_TO_CHECK[dirIndex], files, ".xml");
      // Sort the list for filesystem based priorities, e.g. 01-keymap.xml, 02-keymap-overrides.xml
      files.Sort(SortByFile, SortOrderAscending);
      for(int fileIndex = 0; fileIndex<files.Size(); ++fileIndex)
      {
        if (!files[fileIndex]->m_bIsFolder)
          success |= LoadKeymap(files[fileIndex]->GetPath());
      }

      // Load mappings for any HID devices we have connected
      std::list<std::string>::iterator it;
      for (it = m_deviceList.begin(); it != m_deviceList.end(); ++it)
      {
        std::string devicedir = DIRS_TO_CHECK[dirIndex];
        devicedir.append(*it);
        devicedir.append("/");
        if( XFILE::CDirectory::Exists(devicedir) )
        {
          CFileItemList files;
          XFILE::CDirectory::GetDirectory(devicedir, files, ".xml");
          // Sort the list for filesystem based priorities, e.g. 01-keymap.xml, 02-keymap-overrides.xml
          files.Sort(SortByFile, SortOrderAscending);
          for(int fileIndex = 0; fileIndex<files.Size(); ++fileIndex)
          {
            if (!files[fileIndex]->m_bIsFolder)
              success |= LoadKeymap(files[fileIndex]->GetPath());
          }
        }
      }
    }
  }

  if (!success)
  {
    CLog::Log(LOGERROR, "Error loading keymaps from: %s or %s or %s", DIRS_TO_CHECK[0], DIRS_TO_CHECK[1], DIRS_TO_CHECK[2]);
    return false;
  }

#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
#ifdef TARGET_POSIX
#define REMOTEMAP "Lircmap.xml"
#else
#define REMOTEMAP "IRSSmap.xml"
#endif
  std::string lircmapPath = URIUtils::AddFileToFolder("special://xbmc/system/", REMOTEMAP);
  lircRemotesMap.clear();
  if(CFile::Exists(lircmapPath))
    success |= LoadLircMap(lircmapPath);
  else
    CLog::Log(LOGDEBUG, "CButtonTranslator::Load - no system %s found, skipping", REMOTEMAP);

  lircmapPath = CProfilesManager::GetInstance().GetUserDataItem(REMOTEMAP);
  if(CFile::Exists(lircmapPath))
    success |= LoadLircMap(lircmapPath);
  else
    CLog::Log(LOGDEBUG, "CButtonTranslator::Load - no userdata %s found, skipping", REMOTEMAP);

  if (!success)
    CLog::Log(LOGERROR, "CButtonTranslator::Load - unable to load remote map %s", REMOTEMAP);
  // don't return false - it is to only indicate a fatal error (which this is not)
#endif

  // Done!
  m_Loaded = true;
  return true;
}

bool CButtonTranslator::LoadKeymap(const std::string &keymapPath)
{
  CXBMCTinyXML xmlDoc;

  CLog::Log(LOGINFO, "Loading %s", keymapPath.c_str());
  if (!xmlDoc.LoadFile(keymapPath))
  {
    CLog::Log(LOGERROR, "Error loading keymap: %s, Line %d\n%s", keymapPath.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }
  TiXmlElement* pRoot = xmlDoc.RootElement();
  if (!pRoot)
  {
    CLog::Log(LOGERROR, "Error getting keymap root: %s", keymapPath.c_str());
    return false;
  }
  std::string strValue = pRoot->Value();
  if ( strValue != "keymap")
  {
    CLog::Log(LOGERROR, "%s Doesn't contain <keymap>", keymapPath.c_str());
    return false;
  }
  // run through our window groups
  TiXmlNode* pWindow = pRoot->FirstChild();
  while (pWindow)
  {
    if (pWindow->Type() == TiXmlNode::TINYXML_ELEMENT)
    {
      int windowID = WINDOW_INVALID;
      const char *szWindow = pWindow->Value();
      if (szWindow)
      {
        if (strcmpi(szWindow, "global") == 0)
          windowID = -1;
        else
          windowID = TranslateWindow(szWindow);
      }
      MapWindowActions(pWindow, windowID);
    }
    pWindow = pWindow->NextSibling();
  }

  return true;
}

bool CButtonTranslator::LoadLircMap(const std::string &lircmapPath)
{
#ifdef TARGET_POSIX
#define REMOTEMAPTAG "lircmap"
#else
#define REMOTEMAPTAG "irssmap"
#endif
  // load our xml file, and fill up our mapping tables
  CXBMCTinyXML xmlDoc;

  // Load the config file
  CLog::Log(LOGINFO, "Loading %s", lircmapPath.c_str());
  if (!xmlDoc.LoadFile(lircmapPath))
  {
    CLog::Log(LOGERROR, "%s, Line %d\n%s", lircmapPath.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false; // This is so people who don't have the file won't fail, just warn
  }

  TiXmlElement* pRoot = xmlDoc.RootElement();
  std::string strValue = pRoot->Value();
  if (strValue != REMOTEMAPTAG)
  {
    CLog::Log(LOGERROR, "%sl Doesn't contain <%s>", lircmapPath.c_str(), REMOTEMAPTAG);
    return false;
  }

  // run through our window groups
  TiXmlNode* pRemote = pRoot->FirstChild();
  while (pRemote)
  {
    if (pRemote->Type() == TiXmlNode::TINYXML_ELEMENT)
    {
      const char *szRemote = pRemote->Value();
      if (szRemote)
      {
        TiXmlAttribute* pAttr = pRemote->ToElement()->FirstAttribute();
        if (pAttr)
          MapRemote(pRemote, pAttr->Value());
      }
    }
    pRemote = pRemote->NextSibling();
  }

  return true;
}

void CButtonTranslator::MapRemote(TiXmlNode *pRemote, const char* szDevice)
{
  CLog::Log(LOGINFO, "* Adding remote mapping for device '%s'", szDevice);
  std::vector<std::string> RemoteNames;
  std::map<std::string, lircButtonMap*>::iterator it = lircRemotesMap.find(szDevice);
  if (it == lircRemotesMap.end())
    lircRemotesMap[szDevice] = new lircButtonMap;
  lircButtonMap& buttons = *lircRemotesMap[szDevice];

  TiXmlElement *pButton = pRemote->FirstChildElement();
  while (pButton)
  {
    if (!pButton->NoChildren())
    {
      if (pButton->ValueStr() == "altname")
        RemoteNames.push_back(pButton->FirstChild()->ValueStr());
      else
        buttons[pButton->FirstChild()->ValueStr()] = pButton->ValueStr();
    }
    pButton = pButton->NextSiblingElement();
  }
  for (std::vector<std::string>::iterator it  = RemoteNames.begin();
                                it != RemoteNames.end();++it)
  {
    CLog::Log(LOGINFO, "* Linking remote mapping for '%s' to '%s'", szDevice, it->c_str());
    lircRemotesMap[*it] = &buttons;
  }
}

int CButtonTranslator::TranslateLircRemoteString(const char* szDevice, const char *szButton)
{
  // Find the device
  std::map<std::string, lircButtonMap*>::iterator it = lircRemotesMap.find(szDevice);
  if (it == lircRemotesMap.end())
    return 0;

  // Find the button
  lircButtonMap::iterator it2 = (*it).second->find(szButton);
  if (it2 == (*it).second->end())
    return 0;

  // Convert the button to code
  if (strnicmp((*it2).second.c_str(), "obc", 3) == 0)
    return TranslateUniversalRemoteString((*it2).second.c_str());

  return TranslateRemoteString((*it2).second.c_str());
}

int CButtonTranslator::GetCustomControllerActionCode(int windowId, int buttonId, const CustomControllerWindowMap *windowMap, std::string& strAction) const
{
  int action = 0;
  
  auto it = windowMap->find(windowId);
  if (it != windowMap->end())
  {
    const CustomControllerButtonMap &buttonMap = it->second;
    auto it2 = buttonMap.find(buttonId);
    if (it2 != buttonMap.end())
    {
      strAction = it2->second;
      TranslateActionString(strAction.c_str(), action);
    }
  }
  
  return action;
}

bool CButtonTranslator::TranslateCustomControllerString(int windowId, const std::string& controllerName, int buttonId, int& action, std::string& strAction)
{
  // resolve the correct custom controller
  auto it = m_customControllersMap.find(controllerName);
  if (it == m_customControllersMap.end())
  {
    return false;
  }
  
  const CustomControllerWindowMap *wmap = &it->second;
  
  // try to get the action from the current window
  action = GetCustomControllerActionCode(windowId, buttonId, wmap, strAction);
  
  // if it's invalid, try to get it from a fallback window or the global map
  if (action == 0)
  {
    int fallbackWindow = GetFallbackWindow(windowId);
    if (fallbackWindow > -1)
      action = GetCustomControllerActionCode(fallbackWindow, buttonId, wmap, strAction);
    // still no valid action? use global map
    if (action == 0)
      action = GetCustomControllerActionCode(-1, buttonId, wmap, strAction);
  }
  
  return (action > 0);
}


bool CButtonTranslator::TranslateTouchAction(int window, int touchAction, int touchPointers, int &action, std::string &actionString)
{
  action = 0;
  if (touchPointers <= 0)
    touchPointers = 1;

  touchAction += touchPointers - 1;
  touchAction |= KEY_TOUCH;

  action = GetTouchActionCode(window, touchAction, actionString);
  if (action <= 0)
  {
    int fallbackWindow = GetFallbackWindow(window);
    if (fallbackWindow > -1)
      action = GetTouchActionCode(fallbackWindow, touchAction, actionString);
    if (action <= 0)
      action = GetTouchActionCode(-1, touchAction, actionString);
  }

  return action > 0;
}

int CButtonTranslator::GetActionCode(int window, int action)
{
  std::map<int, buttonMap>::const_iterator it = m_translatorMap.find(window);
  if (it == m_translatorMap.end())
    return 0;

  buttonMap::const_iterator it2 = it->second.find(action);
  if (it2 == it->second.end())
    return 0;

  return it2->second.id;
}

void CButtonTranslator::GetActions(std::vector<std::string> &actionList)
{
  unsigned int size = sizeof(actions) / sizeof(ActionMapping);
  actionList.clear();
  actionList.reserve(size);
  for (unsigned int index = 0; index < size; index++)
    actionList.push_back(actions[index].name);
}

void CButtonTranslator::GetWindows(std::vector<std::string> &windowList)
{
  unsigned int size = sizeof(windows) / sizeof(ActionMapping);
  windowList.clear();
  windowList.reserve(size);
  for (unsigned int index = 0; index < size; index++)
    windowList.push_back(windows[index].name);
}

int CButtonTranslator::GetFallbackWindow(int windowID)
{
  for (unsigned int index = 0; index < ARRAY_SIZE(fallbackWindows); ++index)
  {
    if (fallbackWindows[index].origin == windowID)
      return fallbackWindows[index].target;
  }
  // for addon windows use WINDOW_ADDON_START because id is dynamic
  if (windowID > WINDOW_ADDON_START && windowID <= WINDOW_ADDON_END)
    return WINDOW_ADDON_START;

  return -1;
}

CAction CButtonTranslator::GetAction(int window, const CKey &key, bool fallback)
{
  std::string strAction;
  // try to get the action from the current window
  int actionID = GetActionCode(window, key, strAction);
  // if it's invalid, try to get it from the global map
  if (actionID == 0 && fallback)
  {
    int fallbackWindow = GetFallbackWindow(window);
    if (fallbackWindow > -1)
      actionID = GetActionCode(fallbackWindow, key, strAction);
    // still no valid action? use global map
    if (actionID == 0)
      actionID = GetActionCode( -1, key, strAction);
  }
  // Now fill our action structure
  CAction action(actionID, strAction, key);
  return action;
}

CAction CButtonTranslator::GetGlobalAction(const CKey &key)
{
  return GetAction(-1, key, true);
}

bool CButtonTranslator::HasLonpressMapping(int window, const CKey &key)
{
  std::map<int, buttonMap>::const_iterator it = m_translatorMap.find(window);
  if (it != m_translatorMap.end())
  {
    uint32_t code = key.GetButtonCode();
    code |= CKey::MODIFIER_LONG;
    buttonMap::const_iterator it2 = (*it).second.find(code);

    if (it2 != (*it).second.end())
      return true;

#ifdef TARGET_POSIX
    // Some buttoncodes changed in Hardy
    if ((code & KEY_VKEY) == KEY_VKEY && (code & 0x0F00))
    {
      code &= ~0x0F00;
      it2 = (*it).second.find(code);
      if (it2 != (*it).second.end())
        return true;
    }
#endif
  }

  // no key mapping found for the current window do the fallback handling
  if (window > -1)
  {
    // first check if we have a fallback for the window
    int fallbackWindow = GetFallbackWindow(window);
    if (fallbackWindow > -1 && HasLonpressMapping(fallbackWindow, key))
      return true;

    // fallback to default section
    return HasLonpressMapping(-1, key);
  }

  return false;
}

int CButtonTranslator::GetActionCode(int window, const CKey &key, std::string &strAction) const
{
  uint32_t code = key.GetButtonCode();

  std::map<int, buttonMap>::const_iterator it = m_translatorMap.find(window);
  if (it == m_translatorMap.end())
    return 0;
  buttonMap::const_iterator it2 = (*it).second.find(code);
  int action = 0;
  if (it2 == (*it).second.end() && code & CKey::MODIFIER_LONG) // If long action not found, try short one
  {
    code &= ~CKey::MODIFIER_LONG;
    it2 = (*it).second.find(code);
  }
  if (it2 != (*it).second.end())
  {
    action = (*it2).second.id;
    strAction = (*it2).second.strID;
  }
#ifdef TARGET_POSIX
  // Some buttoncodes changed in Hardy
  if (action == 0 && (code & KEY_VKEY) == KEY_VKEY && (code & 0x0F00))
  {
    CLog::Log(LOGDEBUG, "%s: Trying Hardy keycode for %#04x", __FUNCTION__, code);
    code &= ~0x0F00;
    it2 = (*it).second.find(code);
    if (it2 != (*it).second.end())
    {
      action = (*it2).second.id;
      strAction = (*it2).second.strID;
    }
  }
#endif
  return action;
}

void CButtonTranslator::MapAction(uint32_t buttonCode, const char *szAction, buttonMap &map)
{
  int action = ACTION_NONE;
  if (!TranslateActionString(szAction, action) || !buttonCode)
    return;   // no valid action, or an invalid buttoncode

  // have a valid action, and a valid button - map it.
  // check to see if we've already got this (button,action) pair defined
  buttonMap::iterator it = map.find(buttonCode);
  if (it == map.end() || (*it).second.id != action || (*it).second.strID != szAction)
  {
    // NOTE: This multimap is only being used as a normal map at this point (no support
    //       for multiple actions per key)
    if (it != map.end())
      map.erase(it);
    CButtonAction button;
    button.id = action;
    button.strID = szAction;
    map.insert(std::pair<uint32_t, CButtonAction>(buttonCode, button));
  }
}

void CButtonTranslator::MapCustomControllerActions(int windowID, TiXmlNode *pCustomController)
{
  CustomControllerButtonMap buttonMap;
  std::string controllerName;
  
  TiXmlElement *pController = pCustomController->ToElement();
  if (pController)
  {
    // transform loose name to new family, including altnames
    if(pController->Attribute("name"))
    {
      controllerName = pController->Attribute("name");
    }
    else
    {
      CLog::Log(LOGERROR, "Missing attribute \"name\" for tag \"customcontroller\"");
      return;
    }
  }
  
  // parse map
  TiXmlElement *pButton = pCustomController->FirstChildElement();
  int id = 0;
  while (pButton)
  {
    std::string action;
    if (!pButton->NoChildren())
      action = pButton->FirstChild()->ValueStr();
    
    if ((pButton->QueryIntAttribute("id", &id) == TIXML_SUCCESS) && id >= 0)
    {
      buttonMap[id] = action;
    }
    else
      CLog::Log(LOGERROR, "Error reading customController map element, Invalid id: %d", id);
    
    pButton = pButton->NextSiblingElement();
  }
  
  // add/overwrite button with mapped actions
  for (auto button : buttonMap)
    m_customControllersMap[controllerName][windowID][button.first] = button.second;
}

bool CButtonTranslator::HasDeviceType(TiXmlNode *pWindow, std::string type)
{
  return pWindow->FirstChild(type) != NULL;
}

void CButtonTranslator::MapWindowActions(TiXmlNode *pWindow, int windowID)
{
  if (!pWindow || windowID == WINDOW_INVALID) 
    return;

  TiXmlNode* pDevice;

  const char* types[] = {"gamepad", "remote", "universalremote", "keyboard", "mouse", "appcommand", "joystick", NULL};
  for (int i = 0; types[i]; ++i)
  {
    std::string type(types[i]);
    if (HasDeviceType(pWindow, type))
    {
      buttonMap map;
      std::map<int, buttonMap>::iterator it = m_translatorMap.find(windowID);
      if (it != m_translatorMap.end())
      {
        map = it->second;
        m_translatorMap.erase(it);
      }

      pDevice = pWindow->FirstChild(type);

      TiXmlElement *pButton = pDevice->FirstChildElement();

      while (pButton)
      {
        uint32_t buttonCode=0;
        if (type == "gamepad")
            buttonCode = TranslateGamepadString(pButton->Value());
        else if (type == "remote")
            buttonCode = TranslateRemoteString(pButton->Value());
        else if (type == "universalremote")
            buttonCode = TranslateUniversalRemoteString(pButton->Value());
        else if (type == "keyboard")
            buttonCode = TranslateKeyboardButton(pButton);
        else if (type == "mouse")
            buttonCode = TranslateMouseCommand(pButton);
        else if (type == "appcommand")
            buttonCode = TranslateAppCommand(pButton->Value());
        else if (type == "joystick")
            buttonCode = TranslateJoystickString(pButton->Value());

        if (buttonCode)
        {
          if (pButton->FirstChild() && pButton->FirstChild()->Value()[0])
            MapAction(buttonCode, pButton->FirstChild()->Value(), map);
          else
          {
            buttonMap::iterator it = map.find(buttonCode);
            while (it != map.end())
            {
              map.erase(it);
              it = map.find(buttonCode);
            }
          }
        }
        pButton = pButton->NextSiblingElement();
      }

      // add our map to our table
      if (!map.empty())
        m_translatorMap.insert(std::pair<int, buttonMap>( windowID, map));
    }
  }

  if ((pDevice = pWindow->FirstChild("touch")) != NULL)
  {
    // map touch actions
    while (pDevice)
    {
      MapTouchActions(windowID, pDevice);
      pDevice = pDevice->NextSibling("touch");
    }
  }
  
  if ((pDevice = pWindow->FirstChild("customcontroller")) != NULL)
  {
    // map custom controller actions
    while (pDevice)
    {
      MapCustomControllerActions(windowID, pDevice);
      pDevice = pDevice->NextSibling("customcontroller");
    }
  }

}

bool CButtonTranslator::TranslateActionString(const char *szAction, int &action)
{
  action = ACTION_NONE;
  std::string strAction = szAction;
  StringUtils::ToLower(strAction);
  if (CBuiltins::GetInstance().HasCommand(strAction))
    action = ACTION_BUILT_IN_FUNCTION;

  for (unsigned int index=0;index < ARRAY_SIZE(actions);++index)
  {
    if (strAction == actions[index].name)
    {
      action = actions[index].action;
      break;
    }
  }

  if (action == ACTION_NONE)
  {
    CLog::Log(LOGERROR, "Keymapping error: no such action '%s' defined", strAction.c_str());
    return false;
  }

  return true;
}

std::string CButtonTranslator::TranslateWindow(int windowID)
{
  for (unsigned int index = 0; index < ARRAY_SIZE(windows); ++index)
  {
    if (windows[index].action == windowID)
      return windows[index].name;
  }
  return "";
}

int CButtonTranslator::TranslateWindow(const std::string &window)
{
  std::string strWindow(window);
  if (strWindow.empty()) 
    return WINDOW_INVALID;
  StringUtils::ToLower(strWindow);
  // eliminate .xml
  if (StringUtils::EndsWith(strWindow, ".xml"))
    strWindow = strWindow.substr(0, strWindow.size() - 4);

  // window12345, for custom window to be keymapped
  if (strWindow.length() > 6 && StringUtils::StartsWithNoCase(strWindow, "window"))
    strWindow = strWindow.substr(6);
  if (StringUtils::StartsWithNoCase(strWindow, "my"))  // drop "my" prefix
    strWindow = strWindow.substr(2);
  if (StringUtils::IsNaturalNumber(strWindow))
  {
    // allow a full window id or a delta id
    int iWindow = atoi(strWindow.c_str());
    if (iWindow > WINDOW_INVALID)
      return iWindow;
    return WINDOW_HOME + iWindow;
  }

  // run through the window structure
  for (unsigned int index = 0; index < ARRAY_SIZE(windows); ++index)
  {
    if (strWindow == windows[index].name)
      return windows[index].action;
  }

  CLog::Log(LOGERROR, "Window Translator: Can't find window %s", strWindow.c_str());
  return WINDOW_INVALID;
}

uint32_t CButtonTranslator::TranslateGamepadString(const char *szButton)
{
  if (!szButton) 
    return 0;
  uint32_t buttonCode = 0;
  std::string strButton = szButton;
  StringUtils::ToLower(strButton);
  if (strButton == "a") buttonCode = KEY_BUTTON_A;
  else if (strButton == "b") buttonCode = KEY_BUTTON_B;
  else if (strButton == "x") buttonCode = KEY_BUTTON_X;
  else if (strButton == "y") buttonCode = KEY_BUTTON_Y;
  else if (strButton == "white") buttonCode = KEY_BUTTON_WHITE;
  else if (strButton == "black") buttonCode = KEY_BUTTON_BLACK;
  else if (strButton == "start") buttonCode = KEY_BUTTON_START;
  else if (strButton == "back") buttonCode = KEY_BUTTON_BACK;
  else if (strButton == "leftthumbbutton") buttonCode = KEY_BUTTON_LEFT_THUMB_BUTTON;
  else if (strButton == "rightthumbbutton") buttonCode = KEY_BUTTON_RIGHT_THUMB_BUTTON;
  else if (strButton == "leftthumbstick") buttonCode = KEY_BUTTON_LEFT_THUMB_STICK;
  else if (strButton == "leftthumbstickup") buttonCode = KEY_BUTTON_LEFT_THUMB_STICK_UP;
  else if (strButton == "leftthumbstickdown") buttonCode = KEY_BUTTON_LEFT_THUMB_STICK_DOWN;
  else if (strButton == "leftthumbstickleft") buttonCode = KEY_BUTTON_LEFT_THUMB_STICK_LEFT;
  else if (strButton == "leftthumbstickright") buttonCode = KEY_BUTTON_LEFT_THUMB_STICK_RIGHT;
  else if (strButton == "rightthumbstick") buttonCode = KEY_BUTTON_RIGHT_THUMB_STICK;
  else if (strButton == "rightthumbstickup") buttonCode = KEY_BUTTON_RIGHT_THUMB_STICK_UP;
  else if (strButton == "rightthumbstickdown") buttonCode = KEY_BUTTON_RIGHT_THUMB_STICK_DOWN;
  else if (strButton == "rightthumbstickleft") buttonCode = KEY_BUTTON_RIGHT_THUMB_STICK_LEFT;
  else if (strButton == "rightthumbstickright") buttonCode = KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT;
  else if (strButton == "lefttrigger") buttonCode = KEY_BUTTON_LEFT_TRIGGER;
  else if (strButton == "righttrigger") buttonCode = KEY_BUTTON_RIGHT_TRIGGER;
  else if (strButton == "leftanalogtrigger") buttonCode = KEY_BUTTON_LEFT_ANALOG_TRIGGER;
  else if (strButton == "rightanalogtrigger") buttonCode = KEY_BUTTON_RIGHT_ANALOG_TRIGGER;
  else if (strButton == "dpadleft") buttonCode = KEY_BUTTON_DPAD_LEFT;
  else if (strButton == "dpadright") buttonCode = KEY_BUTTON_DPAD_RIGHT;
  else if (strButton == "dpadup") buttonCode = KEY_BUTTON_DPAD_UP;
  else if (strButton == "dpaddown") buttonCode = KEY_BUTTON_DPAD_DOWN;
  else CLog::Log(LOGERROR, "Gamepad Translator: Can't find button %s", strButton.c_str());
  return buttonCode;
}

uint32_t CButtonTranslator::TranslateRemoteString(const char *szButton)
{
  if (!szButton) 
    return 0;
  uint32_t buttonCode = 0;
  std::string strButton = szButton;
  StringUtils::ToLower(strButton);
  if (strButton == "left") buttonCode = XINPUT_IR_REMOTE_LEFT;
  else if (strButton == "right") buttonCode = XINPUT_IR_REMOTE_RIGHT;
  else if (strButton == "up") buttonCode = XINPUT_IR_REMOTE_UP;
  else if (strButton == "down") buttonCode = XINPUT_IR_REMOTE_DOWN;
  else if (strButton == "select") buttonCode = XINPUT_IR_REMOTE_SELECT;
  else if (strButton == "back") buttonCode = XINPUT_IR_REMOTE_BACK;
  else if (strButton == "menu") buttonCode = XINPUT_IR_REMOTE_MENU;
  else if (strButton == "info") buttonCode = XINPUT_IR_REMOTE_INFO;
  else if (strButton == "display") buttonCode = XINPUT_IR_REMOTE_DISPLAY;
  else if (strButton == "title") buttonCode = XINPUT_IR_REMOTE_TITLE;
  else if (strButton == "play") buttonCode = XINPUT_IR_REMOTE_PLAY;
  else if (strButton == "pause") buttonCode = XINPUT_IR_REMOTE_PAUSE;
  else if (strButton == "reverse") buttonCode = XINPUT_IR_REMOTE_REVERSE;
  else if (strButton == "forward") buttonCode = XINPUT_IR_REMOTE_FORWARD;
  else if (strButton == "skipplus") buttonCode = XINPUT_IR_REMOTE_SKIP_PLUS;
  else if (strButton == "skipminus") buttonCode = XINPUT_IR_REMOTE_SKIP_MINUS;
  else if (strButton == "stop") buttonCode = XINPUT_IR_REMOTE_STOP;
  else if (strButton == "zero") buttonCode = XINPUT_IR_REMOTE_0;
  else if (strButton == "one") buttonCode = XINPUT_IR_REMOTE_1;
  else if (strButton == "two") buttonCode = XINPUT_IR_REMOTE_2;
  else if (strButton == "three") buttonCode = XINPUT_IR_REMOTE_3;
  else if (strButton == "four") buttonCode = XINPUT_IR_REMOTE_4;
  else if (strButton == "five") buttonCode = XINPUT_IR_REMOTE_5;
  else if (strButton == "six") buttonCode = XINPUT_IR_REMOTE_6;
  else if (strButton == "seven") buttonCode = XINPUT_IR_REMOTE_7;
  else if (strButton == "eight") buttonCode = XINPUT_IR_REMOTE_8;
  else if (strButton == "nine") buttonCode = XINPUT_IR_REMOTE_9;
  // additional keys from the media center extender for xbox remote
  else if (strButton == "power") buttonCode = XINPUT_IR_REMOTE_POWER;
  else if (strButton == "mytv") buttonCode = XINPUT_IR_REMOTE_MY_TV;
  else if (strButton == "mymusic") buttonCode = XINPUT_IR_REMOTE_MY_MUSIC;
  else if (strButton == "mypictures") buttonCode = XINPUT_IR_REMOTE_MY_PICTURES;
  else if (strButton == "myvideo") buttonCode = XINPUT_IR_REMOTE_MY_VIDEOS;
  else if (strButton == "record") buttonCode = XINPUT_IR_REMOTE_RECORD;
  else if (strButton == "start") buttonCode = XINPUT_IR_REMOTE_START;
  else if (strButton == "volumeplus") buttonCode = XINPUT_IR_REMOTE_VOLUME_PLUS;
  else if (strButton == "volumeminus") buttonCode = XINPUT_IR_REMOTE_VOLUME_MINUS;
  else if (strButton == "channelplus") buttonCode = XINPUT_IR_REMOTE_CHANNEL_PLUS;
  else if (strButton == "channelminus") buttonCode = XINPUT_IR_REMOTE_CHANNEL_MINUS;
  else if (strButton == "pageplus") buttonCode = XINPUT_IR_REMOTE_CHANNEL_PLUS;
  else if (strButton == "pageminus") buttonCode = XINPUT_IR_REMOTE_CHANNEL_MINUS;
  else if (strButton == "mute") buttonCode = XINPUT_IR_REMOTE_MUTE;
  else if (strButton == "recordedtv") buttonCode = XINPUT_IR_REMOTE_RECORDED_TV;
  else if (strButton == "guide") buttonCode = XINPUT_IR_REMOTE_GUIDE;
  else if (strButton == "livetv") buttonCode = XINPUT_IR_REMOTE_LIVE_TV;
  else if (strButton == "liveradio") buttonCode = XINPUT_IR_REMOTE_LIVE_RADIO;
  else if (strButton == "epgsearch") buttonCode = XINPUT_IR_REMOTE_EPG_SEARCH;
  else if (strButton == "star") buttonCode = XINPUT_IR_REMOTE_STAR;
  else if (strButton == "hash") buttonCode = XINPUT_IR_REMOTE_HASH;
  else if (strButton == "clear") buttonCode = XINPUT_IR_REMOTE_CLEAR;
  else if (strButton == "enter") buttonCode = XINPUT_IR_REMOTE_ENTER;
  else if (strButton == "xbox") buttonCode = XINPUT_IR_REMOTE_DISPLAY; // same as display
  else if (strButton == "playlist") buttonCode = XINPUT_IR_REMOTE_PLAYLIST;
  else if (strButton == "teletext") buttonCode = XINPUT_IR_REMOTE_TELETEXT;
  else if (strButton == "red") buttonCode = XINPUT_IR_REMOTE_RED;
  else if (strButton == "green") buttonCode = XINPUT_IR_REMOTE_GREEN;
  else if (strButton == "yellow") buttonCode = XINPUT_IR_REMOTE_YELLOW;
  else if (strButton == "blue") buttonCode = XINPUT_IR_REMOTE_BLUE;
  else if (strButton == "subtitle") buttonCode = XINPUT_IR_REMOTE_SUBTITLE;
  else if (strButton == "language") buttonCode = XINPUT_IR_REMOTE_LANGUAGE;
  else if (strButton == "eject") buttonCode = XINPUT_IR_REMOTE_EJECT;
  else if (strButton == "contentsmenu") buttonCode = XINPUT_IR_REMOTE_CONTENTS_MENU;
  else if (strButton == "rootmenu") buttonCode = XINPUT_IR_REMOTE_ROOT_MENU;
  else if (strButton == "topmenu") buttonCode = XINPUT_IR_REMOTE_TOP_MENU;
  else if (strButton == "dvdmenu") buttonCode = XINPUT_IR_REMOTE_DVD_MENU;
  else if (strButton == "print") buttonCode = XINPUT_IR_REMOTE_PRINT;
  else CLog::Log(LOGERROR, "Remote Translator: Can't find button %s", strButton.c_str());
  return buttonCode;
}

uint32_t CButtonTranslator::TranslateUniversalRemoteString(const char *szButton)
{
  if (!szButton || strlen(szButton) < 4 || strnicmp(szButton, "obc", 3)) 
    return 0;
  const char *szCode = szButton + 3;
  // Button Code is 255 - OBC (Original Button Code) of the button
  uint32_t buttonCode = 255 - atol(szCode);
  if (buttonCode > 255) 
    buttonCode = 0;
  return buttonCode;
}

uint32_t CButtonTranslator::TranslateKeyboardString(const char *szButton)
{
  uint32_t buttonCode = 0;
  XBMCKEYTABLE keytable;

  // Look up the key name
  if (KeyTableLookupName(szButton, &keytable))
  {
    buttonCode = keytable.vkey;
  }

  // The lookup failed i.e. the key name wasn't found
  else
  {
    CLog::Log(LOGERROR, "Keyboard Translator: Can't find button %s", szButton);
  }

  buttonCode |= KEY_VKEY;

  return buttonCode;
}

uint32_t CButtonTranslator::TranslateKeyboardButton(TiXmlElement *pButton)
{
  uint32_t button_id = 0;
  const char *szButton = pButton->Value();

  if (!szButton) 
    return 0;
  const std::string strKey = szButton;
  if (strKey == "key")
  {
    std::string strID;
    if (pButton->QueryValueAttribute("id", &strID) == TIXML_SUCCESS)
    {
      const char *str = strID.c_str();
      char *endptr;
      long int id = strtol(str, &endptr, 0);
      if (endptr - str != (int)strlen(str) || id <= 0 || id > 0x00FFFFFF)
        CLog::Log(LOGDEBUG, "%s - invalid key id %s", __FUNCTION__, strID.c_str());
      else
        button_id = (uint32_t) id;
    }
    else
      CLog::Log(LOGERROR, "Keyboard Translator: `key' button has no id");
  }
  else
    button_id = TranslateKeyboardString(szButton);

  // Process the ctrl/shift/alt modifiers
  std::string strMod;
  if (pButton->QueryValueAttribute("mod", &strMod) == TIXML_SUCCESS)
  {
    StringUtils::ToLower(strMod);

    std::vector<std::string> modArray = StringUtils::Split(strMod, ",");
    for (std::vector<std::string>::const_iterator i = modArray.begin(); i != modArray.end(); ++i)
    {
      std::string substr = *i;
      StringUtils::Trim(substr);

      if (substr == "ctrl" || substr == "control")
        button_id |= CKey::MODIFIER_CTRL;
      else if (substr == "shift")
        button_id |= CKey::MODIFIER_SHIFT;
      else if (substr == "alt")
        button_id |= CKey::MODIFIER_ALT;
      else if (substr == "super" || substr == "win")
        button_id |= CKey::MODIFIER_SUPER;
      else if (substr == "meta" || substr == "cmd")
        button_id |= CKey::MODIFIER_META;
      else if (substr == "longpress")
        button_id |= CKey::MODIFIER_LONG;
      else
        CLog::Log(LOGERROR, "Keyboard Translator: Unknown key modifier %s in %s", substr.c_str(), strMod.c_str());
     }
  }

  return button_id;
}

uint32_t CButtonTranslator::TranslateAppCommand(const char *szButton)
{
#ifdef TARGET_WINDOWS
  std::string strAppCommand = szButton;
  StringUtils::ToLower(strAppCommand);

  for (int i = 0; i < ARRAY_SIZE(appcommands); i++)
    if (strAppCommand == appcommands[i].name)
      return appcommands[i].action | KEY_APPCOMMAND;

  CLog::Log(LOGERROR, "%s: Can't find appcommand %s", __FUNCTION__, szButton);
#endif

  return 0;
}

uint32_t CButtonTranslator::TranslateMouseCommand(TiXmlElement *pButton)
{
  uint32_t buttonId = 0;

  if (pButton)
  {
    std::string szKey = pButton->ValueStr();
    if (!szKey.empty())
    {
      StringUtils::ToLower(szKey);
      for (unsigned int i = 0; i < ARRAY_SIZE(mousekeys); i++)
      {
        if (szKey == mousekeys[i].name)
        {
          buttonId = mousekeys[i].action;
          break;
        }
      }
      if (!buttonId)
      {
        CLog::Log(LOGERROR, "Unknown mouse action (%s), skipping", pButton->Value());
      }
      else
      {
        int id = 0;
        if ((pButton->QueryIntAttribute("id", &id) == TIXML_SUCCESS) && id>=0 && id<MOUSE_MAX_BUTTON)
        {
          buttonId += id;
        }
      }
    }
  }

  return buttonId;
}

void CButtonTranslator::Clear()
{
  m_translatorMap.clear();
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  ClearLircButtonMapEntries();
  lircRemotesMap.clear();
#endif
  
  m_customControllersMap.clear();

  m_Loaded = false;
}

uint32_t CButtonTranslator::TranslateTouchCommand(TiXmlElement *pButton, CButtonAction &action)
{
  const char *szButton = pButton->Value();
  if (szButton == NULL || pButton->FirstChild() == NULL)
    return ACTION_NONE;

  const char *szAction = pButton->FirstChild()->Value();
  if (szAction == NULL)
    return ACTION_NONE;

  std::string strTouchCommand = szButton;
  StringUtils::ToLower(strTouchCommand);

  const char *attrVal = pButton->Attribute("direction");
  if (attrVal != NULL)
    strTouchCommand += attrVal;

  uint32_t actionId = ACTION_NONE;
  for (unsigned int i = 0; i < ARRAY_SIZE(touchcommands); i++)
  {
    if (strTouchCommand == touchcommands[i].name)
    {
      actionId = touchcommands[i].action;
      break;
    }
  }

  if (actionId <= ACTION_NONE)
  {
    CLog::Log(LOGERROR, "%s: Can't find touch command %s", __FUNCTION__, szButton);
    return ACTION_NONE;
  }

  attrVal = pButton->Attribute("pointers");
  if (attrVal != NULL)
  {
    int pointers = (int)strtol(attrVal, NULL, 0);
    if (pointers >= 1)
      actionId += pointers - 1;
  }

  action.strID = szAction;
  if (!TranslateActionString(szAction, action.id) || action.id <= ACTION_NONE)
    return ACTION_NONE;

  return actionId | KEY_TOUCH;
}

void CButtonTranslator::MapTouchActions(int windowID, TiXmlNode *pTouch)
{
  if (pTouch == NULL)
    return;

  buttonMap map;
  // check if there already is a touch map for the window ID
  std::map<int, buttonMap>::iterator it = m_touchMap.find(windowID);
  if (it != m_touchMap.end())
  {
    // get the existing touch map and remove it from the window mapping
    // as it will be inserted later on
    map = it->second;
    m_touchMap.erase(it);
  }

  uint32_t actionId = 0;
  TiXmlElement *pTouchElem = pTouch->ToElement();
  if (pTouchElem == NULL)
    return;

  TiXmlElement *pButton = pTouchElem->FirstChildElement();
  while (pButton != NULL)
  {
    CButtonAction action;
    actionId = TranslateTouchCommand(pButton, action);
    if (actionId > 0)
    {
      // check if there already is a mapping for the parsed action
      // and remove it if necessary
      buttonMap::iterator actionIt = map.find(actionId);
      if (actionIt != map.end())
        map.erase(actionIt);

      map.insert(std::make_pair(actionId, action));
    }

    pButton = pButton->NextSiblingElement();
  }

  // add the modified touch map with the window ID
  if (!map.empty())
    m_touchMap.insert(std::pair<int, buttonMap>(windowID, map));
}

int CButtonTranslator::GetTouchActionCode(int window, int action, std::string &actionString)
{
  std::map<int, buttonMap>::const_iterator windowIt = m_touchMap.find(window);
  if (windowIt == m_touchMap.end())
    return ACTION_NONE;

  buttonMap::const_iterator touchIt = windowIt->second.find(action);
  if (touchIt == windowIt->second.end())
    return ACTION_NONE;

  actionString = touchIt->second.strID;
  return touchIt->second.id;
}

uint32_t CButtonTranslator::TranslateJoystickString(const char *szButton)
{
  if (!szButton)
    return 0;
  uint32_t buttonCode = 0;
  std::string strButton = szButton;
  StringUtils::ToLower(strButton);

  if (strButton == "a") buttonCode = KEY_JOYSTICK_BUTTON_A;
  else if (strButton == "b") buttonCode = KEY_JOYSTICK_BUTTON_B;
  else if (strButton == "x") buttonCode = KEY_JOYSTICK_BUTTON_X;
  else if (strButton == "y") buttonCode = KEY_JOYSTICK_BUTTON_Y;
  else if (strButton == "start") buttonCode = KEY_JOYSTICK_BUTTON_START;
  else if (strButton == "back") buttonCode = KEY_JOYSTICK_BUTTON_BACK;
  else if (strButton == "left") buttonCode = KEY_JOYSTICK_BUTTON_DPAD_LEFT;
  else if (strButton == "right") buttonCode = KEY_JOYSTICK_BUTTON_DPAD_RIGHT;
  else if (strButton == "up") buttonCode = KEY_JOYSTICK_BUTTON_DPAD_UP;
  else if (strButton == "down") buttonCode = KEY_JOYSTICK_BUTTON_DPAD_DOWN;
  else if (strButton == "leftthumb") buttonCode = KEY_JOYSTICK_BUTTON_LEFT_STICK_BUTTON;
  else if (strButton == "rightthumb") buttonCode = KEY_JOYSTICK_BUTTON_RIGHT_STICK_BUTTON;
  else if (strButton == "leftstickup") buttonCode = KEY_JOYSTICK_BUTTON_LEFT_THUMB_STICK_UP;
  else if (strButton == "leftstickdown") buttonCode = KEY_JOYSTICK_BUTTON_LEFT_THUMB_STICK_DOWN;
  else if (strButton == "leftstickleft") buttonCode = KEY_JOYSTICK_BUTTON_LEFT_THUMB_STICK_LEFT;
  else if (strButton == "leftstickright") buttonCode = KEY_JOYSTICK_BUTTON_LEFT_THUMB_STICK_RIGHT;
  else if (strButton == "rightstickup") buttonCode = KEY_JOYSTICK_BUTTON_RIGHT_THUMB_STICK_UP;
  else if (strButton == "rightstickdown") buttonCode = KEY_JOYSTICK_BUTTON_RIGHT_THUMB_STICK_DOWN;
  else if (strButton == "rightstickleft") buttonCode = KEY_JOYSTICK_BUTTON_RIGHT_THUMB_STICK_LEFT;
  else if (strButton == "rightstickright") buttonCode = KEY_JOYSTICK_BUTTON_RIGHT_THUMB_STICK_RIGHT;
  else if (strButton == "lefttrigger") buttonCode = KEY_JOYSTICK_BUTTON_LEFT_TRIGGER;
  else if (strButton == "righttrigger") buttonCode = KEY_JOYSTICK_BUTTON_RIGHT_TRIGGER;
  else if (strButton == "leftbumper") buttonCode = KEY_JOYSTICK_BUTTON_LEFT_SHOULDER;
  else if (strButton == "rightbumper") buttonCode = KEY_JOYSTICK_BUTTON_RIGHT_SHOULDER;
  else if (strButton == "guide") buttonCode = KEY_JOYSTICK_BUTTON_GUIDE;
  else CLog::Log(LOGERROR, "Joystick Translator: Can't find button %s", strButton.c_str());

  return buttonCode;
}
