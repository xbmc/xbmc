/*
 *      Copyright (C) 2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "WindowTranslator.h"
#include "guilib/WindowIDs.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <map>
#include <stdlib.h>

namespace
{
using WindowName = std::string;
using WindowID = int;

static const std::map<WindowName, WindowID> WindowMapping =
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
    { "games"                    , WINDOW_GAMES },
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
    { "pvrchannelguide"          , WINDOW_DIALOG_PVR_CHANNEL_GUIDE },
    { "pvrosdguide"              , WINDOW_DIALOG_PVR_CHANNEL_GUIDE }, // backward compatibility to v17
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
    { "gamesettings"             , WINDOW_SETTINGS_MYGAMES },
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
    { "fullscreengame"           , WINDOW_FULLSCREEN_GAME },           // virtual window for fullscreen games, uses WINDOW_FULLSCREEN_VIDEO as fallback
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

struct FallbackWindowMapping
{
  int origin;
  int target;
};

static const std::vector<FallbackWindowMapping> FallbackWindows =
{
    { WINDOW_FULLSCREEN_LIVETV   , WINDOW_FULLSCREEN_VIDEO },
    { WINDOW_FULLSCREEN_RADIO    , WINDOW_VISUALISATION },
    { WINDOW_FULLSCREEN_GAME     , WINDOW_FULLSCREEN_VIDEO }
};
} // anonymous namespace

void CWindowTranslator::GetWindows(std::vector<std::string> &windowList)
{
  windowList.clear();
  windowList.reserve(WindowMapping.size());
  for (auto itMapping : WindowMapping)
    windowList.push_back(itMapping.first);
}

int CWindowTranslator::TranslateWindow(const std::string &window)
{
  std::string strWindow(window);
  if (strWindow.empty())
    return WINDOW_INVALID;

  StringUtils::ToLower(strWindow);

  // Eliminate .xml
  if (StringUtils::EndsWith(strWindow, ".xml"))
    strWindow = strWindow.substr(0, strWindow.size() - 4);

  // window12345, for custom window to be keymapped
  if (strWindow.length() > 6 && StringUtils::StartsWith(strWindow, "window"))
    strWindow = strWindow.substr(6);

  // Drop "my" prefix
  if (StringUtils::StartsWith(strWindow, "my"))
    strWindow = strWindow.substr(2);

  if (StringUtils::IsNaturalNumber(strWindow))
  {
    // Allow a full window ID or a delta ID
    int iWindow = atoi(strWindow.c_str());
    if (iWindow > WINDOW_INVALID)
      return iWindow;

    return WINDOW_HOME + iWindow;
  }

  // Run through the window structure
  auto it = WindowMapping.find(strWindow);
  if (it != WindowMapping.end())
    return it->second;

  CLog::Log(LOGERROR, "Window Translator: Can't find window %s", window.c_str());

  return WINDOW_INVALID;
}

std::string CWindowTranslator::TranslateWindow(int windowId)
{
  static auto reverseWindowMapping = CreateReverseWindowMapping();

  auto it = reverseWindowMapping.find(windowId);
  if (it != reverseWindowMapping.end())
    return it->second;

  return "";
}

int CWindowTranslator::GetFallbackWindow(int windowId)
{
  auto it = std::find_if(FallbackWindows.begin(), FallbackWindows.end(),
    [windowId](const FallbackWindowMapping& mapping)
    {
      return mapping.origin == windowId;
    });

  if (it != FallbackWindows.end())
    return it->target;

  // For add-on windows use WINDOW_ADDON_START because ID is dynamic
  if (WINDOW_ADDON_START < windowId && windowId <= WINDOW_ADDON_END)
    return WINDOW_ADDON_START;

  return -1;
}

std::map<int, const char*> CWindowTranslator::CreateReverseWindowMapping()
{
  std::map<WindowID, const char*> reverseWindowMapping;

  for (auto itMapping : WindowMapping)
    reverseWindowMapping.insert(std::make_pair(itMapping.second, itMapping.first.c_str()));

  return reverseWindowMapping;
}
