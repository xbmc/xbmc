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

#include "ActionTranslator.h"
#include "CustomControllerTranslator.h"
#include "IRTranslator.h"
#include "TouchTranslator.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "guilib/WindowIDs.h"
#include "input/joysticks/JoystickIDs.h"
#include "input/Key.h"
#include "input/MouseStat.h"
#include "input/XBMC_keytable.h"
#include "Util.h"
#include "utils/log.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

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

static const WindowMapping fallbackWindows[] =
{
    { WINDOW_FULLSCREEN_LIVETV   , WINDOW_FULLSCREEN_VIDEO },
    { WINDOW_FULLSCREEN_RADIO    , WINDOW_VISUALISATION },
    { WINDOW_FULLSCREEN_GAME     , WINDOW_FULLSCREEN_VIDEO }
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

CButtonTranslator::CButtonTranslator() :
  m_customControllerTranslator(new CCustomControllerTranslator),
  m_irTranslator(new CIRTranslator),
  m_touchTranslator(new CTouchTranslator)
{
  m_deviceList.clear();
  m_Loaded = false;
}

CButtonTranslator::~CButtonTranslator()
{
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
  m_customControllerTranslator->Clear();

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

  m_irTranslator->Load();

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

int CButtonTranslator::TranslateLircRemoteString(const char* szDevice, const char *szButton)
{
  return m_irTranslator->TranslateIRRemoteString(szDevice, szButton);
}

bool CButtonTranslator::TranslateCustomControllerString(int windowId, const std::string& controllerName, int buttonId, int& action, std::string& strAction)
{
  unsigned int actionId = ACTION_NONE;

  // Try to get the action from the current window
  if (!m_customControllerTranslator->TranslateCustomControllerString(windowId, controllerName, buttonId, actionId, strAction))
  {
    // If it's invalid, try to get it from a fallback window or the global map
    int fallbackWindow = GetFallbackWindow(windowId);
    if (fallbackWindow > -1)
      m_customControllerTranslator->TranslateCustomControllerString(fallbackWindow, controllerName, buttonId, actionId, strAction);

    // Still no valid action? Use global map
    if (action == ACTION_NONE)
      m_customControllerTranslator->TranslateCustomControllerString(-1, controllerName, buttonId, actionId, strAction);
  }

  if (actionId != ACTION_NONE)
  {
    action = actionId;
    return true;
  }

  return false;
}

bool CButtonTranslator::TranslateTouchAction(int window, int touchAction, int touchPointers, int &action, std::string &actionString)
{
  if (touchAction < 0)
    return false;

  unsigned int actionId = ACTION_NONE;

  if (!m_touchTranslator->TranslateTouchAction(window, touchAction, touchPointers, actionId, actionString))
  {
    int fallbackWindow = GetFallbackWindow(window);
    if (fallbackWindow > -1)
      m_touchTranslator->TranslateTouchAction(fallbackWindow, touchAction, touchPointers, actionId, actionString);

    if (actionId == ACTION_NONE)
      m_touchTranslator->TranslateTouchAction(-1, touchAction, touchPointers, actionId, actionString);
  }

  action = actionId;
  return actionId != ACTION_NONE;
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
    //! @todo Refactor fallback logic
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

bool CButtonTranslator::HasLongpressMapping(int window, const CKey &key)
{
  std::map<int, buttonMap>::const_iterator it = m_translatorMap.find(window);
  if (it != m_translatorMap.end())
  {
    uint32_t code = key.GetButtonCode();
    code |= CKey::MODIFIER_LONG;
    buttonMap::const_iterator it2 = (*it).second.find(code);

    if (it2 != (*it).second.end())
      return it2->second.id != ACTION_NOOP;

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
    if (fallbackWindow > -1 && HasLongpressMapping(fallbackWindow, key))
      return true;

    // fallback to default section
    return HasLongpressMapping(-1, key);
  }

  return false;
}

unsigned int CButtonTranslator::GetHoldTimeMs(int window, const CKey &key, bool fallback /* = true */)
{
  unsigned int holdtimeMs = 0;

  std::map<int, buttonMap>::const_iterator it = m_translatorMap.find(window);
  if (it != m_translatorMap.end())
  {
    uint32_t code = key.GetButtonCode();

    buttonMap::const_iterator it2 = (*it).second.find(code);

    if (it2 != (*it).second.end())
    {
      holdtimeMs = (*it2).second.holdtimeMs;
    }
    else if (fallback)
    {
      //! @todo Refactor fallback logic
      int fallbackWindow = GetFallbackWindow(window);
      if (fallbackWindow > -1)
        holdtimeMs = GetHoldTimeMs(fallbackWindow, key, false);
      else
      {
        // still no valid action? use global map
        holdtimeMs = GetHoldTimeMs(-1, key, false);
      }
    }
  }
  else if (fallback)
  {
    //! @todo Refactor fallback logic
    int fallbackWindow = GetFallbackWindow(window);
    if (fallbackWindow > -1)
      holdtimeMs = GetHoldTimeMs(fallbackWindow, key, false);
    else
    {
      // still no valid action? use global map
      holdtimeMs = GetHoldTimeMs(-1, key, false);
    }
  }

  return holdtimeMs;
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

void CButtonTranslator::MapAction(uint32_t buttonCode, const char *szAction, unsigned int holdtimeMs, buttonMap &map)
{
  unsigned int action = ACTION_NONE;
  if (!CActionTranslator::TranslateActionString(szAction, action) || buttonCode == 0)
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
    button.holdtimeMs = holdtimeMs;
    map.insert(std::pair<uint32_t, CButtonAction>(buttonCode, button));
  }
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

    for (pDevice = pWindow->FirstChild(type);
         pDevice != nullptr;
         pDevice = pDevice->NextSiblingElement(type))
    {
      buttonMap map;
      std::map<int, buttonMap>::iterator it = m_translatorMap.find(windowID);
      if (it != m_translatorMap.end())
      {
        map = it->second;
        m_translatorMap.erase(it);
      }

      TiXmlElement *pButton = pDevice->FirstChildElement();

      while (pButton)
      {
        uint32_t buttonCode=0;
        unsigned int holdtimeMs = 0;

        if (type == "gamepad")
            buttonCode = TranslateGamepadString(pButton->Value());
        else if (type == "remote")
            buttonCode = CIRTranslator::TranslateRemoteString(pButton->Value());
        else if (type == "universalremote")
            buttonCode = CIRTranslator::TranslateUniversalRemoteString(pButton->Value());
        else if (type == "keyboard")
            buttonCode = TranslateKeyboardButton(pButton);
        else if (type == "mouse")
            buttonCode = TranslateMouseCommand(pButton);
        else if (type == "appcommand")
            buttonCode = TranslateAppCommand(pButton->Value());
        else if (type == "joystick")
        {
          std::string controllerId = DEFAULT_CONTROLLER_ID;

          TiXmlElement* deviceElem = pDevice->ToElement();
          if (deviceElem != nullptr)
            deviceElem->QueryValueAttribute("profile", &controllerId);

          buttonCode = TranslateJoystickCommand(pButton, controllerId, holdtimeMs);
        }

        if (buttonCode)
        {
          if (pButton->FirstChild() && pButton->FirstChild()->Value()[0])
            MapAction(buttonCode, pButton->FirstChild()->Value(), holdtimeMs, map);
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
      m_touchTranslator->MapTouchActions(windowID, pDevice);
      pDevice = pDevice->NextSibling("touch");
    }
  }

  if ((pDevice = pWindow->FirstChild("customcontroller")) != NULL)
  {
    // map custom controller actions
    while (pDevice)
    {
      m_customControllerTranslator->MapCustomControllerActions(windowID, pDevice);
      pDevice = pDevice->NextSibling("customcontroller");
    }
  }

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

  m_irTranslator->Clear();
  m_customControllerTranslator->Clear();
  m_touchTranslator->Clear();

  m_Loaded = false;
}

uint32_t CButtonTranslator::TranslateJoystickCommand(const TiXmlElement *pButton, const std::string& controllerId, unsigned int& holdtimeMs)
{
  holdtimeMs = 0;

  const char *szButton = pButton->Value();
  if (!szButton)
    return 0;

  uint32_t buttonCode = 0;
  std::string strButton = szButton;
  StringUtils::ToLower(strButton);

  if (controllerId == DEFAULT_CONTROLLER_ID)
  {
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
  }
  else if (controllerId == DEFAULT_REMOTE_ID)
  {
    if (strButton == "ok") buttonCode = KEY_REMOTE_BUTTON_OK;
    else if (strButton == "back") buttonCode = KEY_REMOTE_BUTTON_BACK;
    else if (strButton == "up") buttonCode = KEY_REMOTE_BUTTON_UP;
    else if (strButton == "down") buttonCode = KEY_REMOTE_BUTTON_DOWN;
    else if (strButton == "left") buttonCode = KEY_REMOTE_BUTTON_LEFT;
    else if (strButton == "right") buttonCode = KEY_REMOTE_BUTTON_RIGHT;
    else if (strButton == "home") buttonCode = KEY_REMOTE_BUTTON_HOME;
  }

  if (buttonCode == 0)
  {
    CLog::Log(LOGERROR, "Joystick Translator: Can't find button %s for controller %s", strButton.c_str(), controllerId.c_str());
  }
  else
  {
    // Process holdtime parameter
    std::string strHoldTime;
    if (pButton->QueryValueAttribute("holdtime", &strHoldTime) == TIXML_SUCCESS)
    {
      std::stringstream ss(strHoldTime);
      ss >> holdtimeMs;
    }
  }

  return buttonCode;
}
