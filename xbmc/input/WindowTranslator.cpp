/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WindowTranslator.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "guilib/WindowIDs.h"
#include "pvr/PVRManager.h"
#include "pvr/guilib/PVRGUIActionsChannels.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cstring>
#include <stdlib.h>

const CWindowTranslator::WindowMapByName CWindowTranslator::WindowMappingByName = {
    {"home", WINDOW_HOME},
    {"programs", WINDOW_PROGRAMS},
    {"pictures", WINDOW_PICTURES},
    {"filemanager", WINDOW_FILES},
    {"settings", WINDOW_SETTINGS_MENU},
    {"music", WINDOW_MUSIC_NAV},
    {"videos", WINDOW_VIDEO_NAV},
    {"tvchannels", WINDOW_TV_CHANNELS},
    {"tvrecordings", WINDOW_TV_RECORDINGS},
    {"tvguide", WINDOW_TV_GUIDE},
    {"tvtimers", WINDOW_TV_TIMERS},
    {"tvsearch", WINDOW_TV_SEARCH},
    {"radiochannels", WINDOW_RADIO_CHANNELS},
    {"radiorecordings", WINDOW_RADIO_RECORDINGS},
    {"radioguide", WINDOW_RADIO_GUIDE},
    {"radiotimers", WINDOW_RADIO_TIMERS},
    {"radiosearch", WINDOW_RADIO_SEARCH},
    {"gamecontrollers", WINDOW_DIALOG_GAME_CONTROLLERS},
    {"gameports", WINDOW_DIALOG_GAME_PORTS},
    {"games", WINDOW_GAMES},
    {"pvrguidecontrols", WINDOW_DIALOG_PVR_GUIDE_CONTROLS},
    {"pvrguideinfo", WINDOW_DIALOG_PVR_GUIDE_INFO},
    {"pvrrecordinginfo", WINDOW_DIALOG_PVR_RECORDING_INFO},
    {"pvrradiordsinfo", WINDOW_DIALOG_PVR_RADIO_RDS_INFO},
    {"pvrtimersetting", WINDOW_DIALOG_PVR_TIMER_SETTING},
    {"pvrgroupmanager", WINDOW_DIALOG_PVR_GROUP_MANAGER},
    {"pvrchannelmanager", WINDOW_DIALOG_PVR_CHANNEL_MANAGER},
    {"pvrguidesearch", WINDOW_DIALOG_PVR_GUIDE_SEARCH},
    {"pvrchannelscan", WINDOW_DIALOG_PVR_CHANNEL_SCAN},
    {"pvrupdateprogress", WINDOW_DIALOG_PVR_UPDATE_PROGRESS},
    {"pvrosdchannels", WINDOW_DIALOG_PVR_OSD_CHANNELS},
    {"pvrchannelguide", WINDOW_DIALOG_PVR_CHANNEL_GUIDE},
    {"pvrosdguide", WINDOW_DIALOG_PVR_CHANNEL_GUIDE}, // backward compatibility to v17
    {"pvrosdteletext", WINDOW_DIALOG_OSD_TELETEXT},
    {"systeminfo", WINDOW_SYSTEM_INFORMATION},
    {"screencalibration", WINDOW_SCREEN_CALIBRATION},
    {"systemsettings", WINDOW_SETTINGS_SYSTEM},
    {"servicesettings", WINDOW_SETTINGS_SERVICE},
    {"pvrsettings", WINDOW_SETTINGS_MYPVR},
    {"playersettings", WINDOW_SETTINGS_PLAYER},
    {"mediasettings", WINDOW_SETTINGS_MEDIA},
    {"interfacesettings", WINDOW_SETTINGS_INTERFACE},
    {"appearancesettings", WINDOW_SETTINGS_INTERFACE}, // backward compatibility to v16
    {"gamesettings", WINDOW_SETTINGS_MYGAMES},
    {"videoplaylist", WINDOW_VIDEO_PLAYLIST},
    {"loginscreen", WINDOW_LOGIN_SCREEN},
    {"profiles", WINDOW_SETTINGS_PROFILES},
    {"skinsettings", WINDOW_SKIN_SETTINGS},
    {"addonbrowser", WINDOW_ADDON_BROWSER},
    {"yesnodialog", WINDOW_DIALOG_YES_NO},
    {"progressdialog", WINDOW_DIALOG_PROGRESS},
    {"virtualkeyboard", WINDOW_DIALOG_KEYBOARD},
    {"volumebar", WINDOW_DIALOG_VOLUME_BAR},
    {"submenu", WINDOW_DIALOG_SUB_MENU},
    {"contextmenu", WINDOW_DIALOG_CONTEXT_MENU},
    {"notification", WINDOW_DIALOG_KAI_TOAST},
    {"numericinput", WINDOW_DIALOG_NUMERIC},
    {"gamepadinput", WINDOW_DIALOG_GAMEPAD},
    {"shutdownmenu", WINDOW_DIALOG_BUTTON_MENU},
    {"playercontrols", WINDOW_DIALOG_PLAYER_CONTROLS},
    {"playerprocessinfo", WINDOW_DIALOG_PLAYER_PROCESS_INFO},
    {"seekbar", WINDOW_DIALOG_SEEK_BAR},
    {"musicosd", WINDOW_DIALOG_MUSIC_OSD},
    {"addonsettings", WINDOW_DIALOG_ADDON_SETTINGS},
    {"visualisationpresetlist", WINDOW_DIALOG_VIS_PRESET_LIST},
    {"osdcmssettings", WINDOW_DIALOG_CMS_OSD_SETTINGS},
    {"osdvideosettings", WINDOW_DIALOG_VIDEO_OSD_SETTINGS},
    {"osdaudiosettings", WINDOW_DIALOG_AUDIO_OSD_SETTINGS},
    {"osdsubtitlesettings", WINDOW_DIALOG_SUBTITLE_OSD_SETTINGS},
    {"videobookmarks", WINDOW_DIALOG_VIDEO_BOOKMARKS},
    {"filebrowser", WINDOW_DIALOG_FILE_BROWSER},
    {"networksetup", WINDOW_DIALOG_NETWORK_SETUP},
    {"mediasource", WINDOW_DIALOG_MEDIA_SOURCE},
    {"profilesettings", WINDOW_DIALOG_PROFILE_SETTINGS},
    {"locksettings", WINDOW_DIALOG_LOCK_SETTINGS},
    {"contentsettings", WINDOW_DIALOG_CONTENT_SETTINGS},
    {"libexportsettings", WINDOW_DIALOG_LIBEXPORT_SETTINGS},
    {"songinformation", WINDOW_DIALOG_SONG_INFO},
    {"smartplaylisteditor", WINDOW_DIALOG_SMART_PLAYLIST_EDITOR},
    {"smartplaylistrule", WINDOW_DIALOG_SMART_PLAYLIST_RULE},
    {"busydialog", WINDOW_DIALOG_BUSY},
    {"busydialognocancel", WINDOW_DIALOG_BUSY_NOCANCEL},
    {"pictureinfo", WINDOW_DIALOG_PICTURE_INFO},
    {"fullscreeninfo", WINDOW_DIALOG_FULLSCREEN_INFO},
    {"sliderdialog", WINDOW_DIALOG_SLIDER},
    {"addoninformation", WINDOW_DIALOG_ADDON_INFO},
    {"subtitlesearch", WINDOW_DIALOG_SUBTITLES},
    {"musicplaylist", WINDOW_MUSIC_PLAYLIST},
    {"musicplaylisteditor", WINDOW_MUSIC_PLAYLIST_EDITOR},
    {"infoprovidersettings", WINDOW_DIALOG_INFOPROVIDER_SETTINGS},
    {"teletext", WINDOW_DIALOG_OSD_TELETEXT},
    {"selectdialog", WINDOW_DIALOG_SELECT},
    {"musicinformation", WINDOW_DIALOG_MUSIC_INFO},
    {"okdialog", WINDOW_DIALOG_OK},
    {"movieinformation", WINDOW_DIALOG_VIDEO_INFO},
    {"textviewer", WINDOW_DIALOG_TEXT_VIEWER},
    {"fullscreenvideo", WINDOW_FULLSCREEN_VIDEO},
    {"dialogcolorpicker", WINDOW_DIALOG_COLOR_PICKER},

    // Virtual window for fullscreen radio, uses WINDOW_FULLSCREEN_VIDEO as
    // fallback
    {"fullscreenlivetv", WINDOW_FULLSCREEN_LIVETV},

    // Live TV channel preview
    {"fullscreenlivetvpreview", WINDOW_FULLSCREEN_LIVETV_PREVIEW},

    // Live TV direct channel number input
    {"fullscreenlivetvinput", WINDOW_FULLSCREEN_LIVETV_INPUT},

    // Virtual window for fullscreen radio, uses WINDOW_VISUALISATION as fallback
    {"fullscreenradio", WINDOW_FULLSCREEN_RADIO},

    // PVR Radio channel preview
    {"fullscreenradiopreview", WINDOW_FULLSCREEN_RADIO_PREVIEW},

    // PVR radio direct channel number input
    {"fullscreenradioinput", WINDOW_FULLSCREEN_RADIO_INPUT},

    {"fullscreengame", WINDOW_FULLSCREEN_GAME},
    {"visualisation", WINDOW_VISUALISATION},
    {"slideshow", WINDOW_SLIDESHOW},
    {"weather", WINDOW_WEATHER},
    {"screensaver", WINDOW_SCREENSAVER},
    {"videoosd", WINDOW_DIALOG_VIDEO_OSD},
    {"videomenu", WINDOW_VIDEO_MENU},
    {"videotimeseek", WINDOW_VIDEO_TIME_SEEK},
    {"splash", WINDOW_SPLASH},
    {"startwindow", WINDOW_START},
    {"startup", WINDOW_STARTUP_ANIM},
    {"peripheralsettings", WINDOW_DIALOG_PERIPHERAL_SETTINGS},
    {"extendedprogressdialog", WINDOW_DIALOG_EXT_PROGRESS},
    {"mediafilter", WINDOW_DIALOG_MEDIA_FILTER},
    {"addon", WINDOW_ADDON_START},
    {"eventlog", WINDOW_EVENT_LOG},
    {"favouritesbrowser", WINDOW_FAVOURITES},
    {"tvtimerrules", WINDOW_TV_TIMER_RULES},
    {"radiotimerrules", WINDOW_RADIO_TIMER_RULES},
    {"gameosd", WINDOW_DIALOG_GAME_OSD},
    {"gamevideofilter", WINDOW_DIALOG_GAME_VIDEO_FILTER},
    {"gamestretchmode", WINDOW_DIALOG_GAME_STRETCH_MODE},
    {"gamevolume", WINDOW_DIALOG_GAME_VOLUME},
    {"gameadvancedsettings", WINDOW_DIALOG_GAME_ADVANCED_SETTINGS},
    {"gamevideorotation", WINDOW_DIALOG_GAME_VIDEO_ROTATION},
    {"ingamesaves", WINDOW_DIALOG_IN_GAME_SAVES},
    {"gamesaves", WINDOW_DIALOG_GAME_SAVES},
    {"gameagents", WINDOW_DIALOG_GAME_AGENTS},
};

namespace
{
struct FallbackWindowMapping
{
  int origin;
  int target;
};

static const std::vector<FallbackWindowMapping> FallbackWindows = {
    {WINDOW_FULLSCREEN_LIVETV, WINDOW_FULLSCREEN_VIDEO},
    {WINDOW_FULLSCREEN_LIVETV_INPUT, WINDOW_FULLSCREEN_LIVETV},
    {WINDOW_FULLSCREEN_LIVETV_PREVIEW, WINDOW_FULLSCREEN_LIVETV},
    {WINDOW_FULLSCREEN_RADIO, WINDOW_VISUALISATION},
    {WINDOW_FULLSCREEN_RADIO_INPUT, WINDOW_FULLSCREEN_RADIO},
    {WINDOW_FULLSCREEN_RADIO_PREVIEW, WINDOW_FULLSCREEN_RADIO},
};
} // anonymous namespace

bool CWindowTranslator::WindowNameCompare::operator()(const WindowMapItem& lhs,
                                                      const WindowMapItem& rhs) const
{
  return std::strcmp(lhs.windowName, rhs.windowName) < 0;
}

bool CWindowTranslator::WindowIDCompare::operator()(const WindowMapItem& lhs,
                                                    const WindowMapItem& rhs) const
{
  return lhs.windowId < rhs.windowId;
}

void CWindowTranslator::GetWindows(std::vector<std::string>& windowList)
{
  windowList.clear();
  windowList.reserve(WindowMappingByName.size());
  for (auto itMapping : WindowMappingByName)
    windowList.emplace_back(itMapping.windowName);
}

int CWindowTranslator::TranslateWindow(const std::string& window)
{
  std::string strWindow(window);
  if (strWindow.empty())
    return WINDOW_INVALID;

  StringUtils::ToLower(strWindow);

  // Eliminate .xml
  if (StringUtils::EndsWith(strWindow, ".xml"))
    strWindow.resize(strWindow.size() - 4);

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
  auto it = WindowMappingByName.find({strWindow.c_str(), {}});
  if (it != WindowMappingByName.end())
    return it->windowId;

  CLog::Log(LOGERROR, "Window Translator: Can't find window {}", window);

  return WINDOW_INVALID;
}

std::string CWindowTranslator::TranslateWindow(int windowId)
{
  static auto reverseWindowMapping = CreateWindowMappingByID();

  windowId = GetVirtualWindow(windowId);

  auto it = reverseWindowMapping.find(WindowMapItem{"", windowId});
  if (it != reverseWindowMapping.end())
    return it->windowName;

  return "";
}

int CWindowTranslator::GetFallbackWindow(int windowId)
{
  auto it = std::find_if(
      FallbackWindows.begin(), FallbackWindows.end(),
      [windowId](const FallbackWindowMapping& mapping) { return mapping.origin == windowId; });

  if (it != FallbackWindows.end())
    return it->target;

  // For add-on windows use WINDOW_ADDON_START because ID is dynamic
  if (WINDOW_ADDON_START < windowId && windowId <= WINDOW_ADDON_END)
    return WINDOW_ADDON_START;

  return -1;
}

CWindowTranslator::WindowMapByID CWindowTranslator::CreateWindowMappingByID()
{
  WindowMapByID reverseWindowMapping;

  reverseWindowMapping.insert(WindowMappingByName.begin(), WindowMappingByName.end());

  return reverseWindowMapping;
}

int CWindowTranslator::GetVirtualWindow(int windowId)
{
  if (windowId == WINDOW_FULLSCREEN_VIDEO)
  {
    if (g_application.CurrentFileItem().HasPVRChannelInfoTag())
    {
      // special casing for Live TV
      if (CServiceBroker::GetPVRManager()
              .Get<PVR::GUI::Channels>()
              .GetChannelNumberInputHandler()
              .HasChannelNumber())
        return WINDOW_FULLSCREEN_LIVETV_INPUT;
      else if (CServiceBroker::GetPVRManager()
                   .Get<PVR::GUI::Channels>()
                   .GetChannelNavigator()
                   .IsPreview())
        return WINDOW_FULLSCREEN_LIVETV_PREVIEW;
      else
        return WINDOW_FULLSCREEN_LIVETV;
    }
    else
    {
      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();

      // check if we're in a DVD menu
      if (appPlayer->IsInMenu())
        return WINDOW_VIDEO_MENU;
      // special casing for numeric seek
      else if (appPlayer->GetSeekHandler().HasTimeCode())
        return WINDOW_VIDEO_TIME_SEEK;
    }
  }
  else if (windowId == WINDOW_VISUALISATION)
  {
    if (g_application.CurrentFileItem().HasPVRChannelInfoTag())
    {
      // special casing for PVR radio
      if (CServiceBroker::GetPVRManager()
              .Get<PVR::GUI::Channels>()
              .GetChannelNumberInputHandler()
              .HasChannelNumber())
        return WINDOW_FULLSCREEN_RADIO_INPUT;
      else if (CServiceBroker::GetPVRManager()
                   .Get<PVR::GUI::Channels>()
                   .GetChannelNavigator()
                   .IsPreview())
        return WINDOW_FULLSCREEN_RADIO_PREVIEW;
      else
        return WINDOW_FULLSCREEN_RADIO;
    }
    else
    {
      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();

      // special casing for numeric seek
      if (appPlayer->GetSeekHandler().HasTimeCode())
        return WINDOW_VIDEO_TIME_SEEK;
    }
  }

  return windowId;
}
