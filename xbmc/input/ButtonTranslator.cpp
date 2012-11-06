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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include "interfaces/Builtins.h"
#include "ButtonTranslator.h"
#include "utils/URIUtils.h"
#include "settings/Settings.h"
#include "guilib/Key.h"
#include "input/XBMC_keysym.h"
#include "input/XBMC_keytable.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "FileItem.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "utils/XBMCTinyXML.h"
#include "XBIRRemote.h"

#if defined(TARGET_WINDOWS)
#include "input/windows/WINJoystick.h"
#elif defined(HAS_SDL_JOYSTICK) || defined(HAS_EVENT_SERVER)
#include "SDLJoystick.h"
#endif

using namespace std;
using namespace XFILE;

typedef struct
{
  const char* name;
  int action;
} ActionMapping;

static const ActionMapping actions[] =
{
        {"left"              , ACTION_MOVE_LEFT },
        {"right"             , ACTION_MOVE_RIGHT},
        {"up"                , ACTION_MOVE_UP   },
        {"down"              , ACTION_MOVE_DOWN },
        {"pageup"            , ACTION_PAGE_UP   },
        {"pagedown"          , ACTION_PAGE_DOWN},
        {"select"            , ACTION_SELECT_ITEM},
        {"highlight"         , ACTION_HIGHLIGHT_ITEM},
        {"parentdir"         , ACTION_NAV_BACK},       // backward compatibility
        {"parentfolder"      , ACTION_PARENT_DIR},
        {"back"              , ACTION_NAV_BACK},
        {"previousmenu"      , ACTION_PREVIOUS_MENU},
        {"info"              , ACTION_SHOW_INFO},
        {"pause"             , ACTION_PAUSE},
        {"stop"              , ACTION_STOP},
        {"skipnext"          , ACTION_NEXT_ITEM},
        {"skipprevious"      , ACTION_PREV_ITEM},
        {"fullscreen"        , ACTION_SHOW_GUI},
        {"aspectratio"       , ACTION_ASPECT_RATIO},
        {"stepforward"       , ACTION_STEP_FORWARD},
        {"stepback"          , ACTION_STEP_BACK},
        {"bigstepforward"    , ACTION_BIG_STEP_FORWARD},
        {"bigstepback"       , ACTION_BIG_STEP_BACK},
        {"osd"               , ACTION_SHOW_OSD},
        {"showsubtitles"     , ACTION_SHOW_SUBTITLES},
        {"nextsubtitle"      , ACTION_NEXT_SUBTITLE},
        {"codecinfo"         , ACTION_SHOW_CODEC},
        {"nextpicture"       , ACTION_NEXT_PICTURE},
        {"previouspicture"   , ACTION_PREV_PICTURE},
        {"zoomout"           , ACTION_ZOOM_OUT},
        {"zoomin"            , ACTION_ZOOM_IN},
        {"playlist"          , ACTION_SHOW_PLAYLIST},
        {"queue"             , ACTION_QUEUE_ITEM},
        {"zoomnormal"        , ACTION_ZOOM_LEVEL_NORMAL},
        {"zoomlevel1"        , ACTION_ZOOM_LEVEL_1},
        {"zoomlevel2"        , ACTION_ZOOM_LEVEL_2},
        {"zoomlevel3"        , ACTION_ZOOM_LEVEL_3},
        {"zoomlevel4"        , ACTION_ZOOM_LEVEL_4},
        {"zoomlevel5"        , ACTION_ZOOM_LEVEL_5},
        {"zoomlevel6"        , ACTION_ZOOM_LEVEL_6},
        {"zoomlevel7"        , ACTION_ZOOM_LEVEL_7},
        {"zoomlevel8"        , ACTION_ZOOM_LEVEL_8},
        {"zoomlevel9"        , ACTION_ZOOM_LEVEL_9},
        {"nextcalibration"   , ACTION_CALIBRATE_SWAP_ARROWS},
        {"resetcalibration"  , ACTION_CALIBRATE_RESET},
        {"analogmove"        , ACTION_ANALOG_MOVE},
        {"rotate"            , ACTION_ROTATE_PICTURE_CW},
        {"rotateccw"         , ACTION_ROTATE_PICTURE_CCW},
        {"close"             , ACTION_NAV_BACK}, // backwards compatibility
        {"subtitledelayminus", ACTION_SUBTITLE_DELAY_MIN},
        {"subtitledelay"     , ACTION_SUBTITLE_DELAY},
        {"subtitledelayplus" , ACTION_SUBTITLE_DELAY_PLUS},
        {"audiodelayminus"   , ACTION_AUDIO_DELAY_MIN},
        {"audiodelay"        , ACTION_AUDIO_DELAY},
        {"audiodelayplus"    , ACTION_AUDIO_DELAY_PLUS},
        {"subtitleshiftup"   , ACTION_SUBTITLE_VSHIFT_UP},
        {"subtitleshiftdown" , ACTION_SUBTITLE_VSHIFT_DOWN},
        {"subtitlealign"     , ACTION_SUBTITLE_ALIGN},
        {"audionextlanguage" , ACTION_AUDIO_NEXT_LANGUAGE},
        {"verticalshiftup"   , ACTION_VSHIFT_UP},
        {"verticalshiftdown" , ACTION_VSHIFT_DOWN},
        {"nextresolution"    , ACTION_CHANGE_RESOLUTION},
        {"audiotoggledigital", ACTION_TOGGLE_DIGITAL_ANALOG},
        {"number0"           , REMOTE_0},
        {"number1"           , REMOTE_1},
        {"number2"           , REMOTE_2},
        {"number3"           , REMOTE_3},
        {"number4"           , REMOTE_4},
        {"number5"           , REMOTE_5},
        {"number6"           , REMOTE_6},
        {"number7"           , REMOTE_7},
        {"number8"           , REMOTE_8},
        {"number9"           , REMOTE_9},
        {"osdleft"           , ACTION_OSD_SHOW_LEFT},
        {"osdright"          , ACTION_OSD_SHOW_RIGHT},
        {"osdup"             , ACTION_OSD_SHOW_UP},
        {"osddown"           , ACTION_OSD_SHOW_DOWN},
        {"osdselect"         , ACTION_OSD_SHOW_SELECT},
        {"osdvalueplus"      , ACTION_OSD_SHOW_VALUE_PLUS},
        {"osdvalueminus"     , ACTION_OSD_SHOW_VALUE_MIN},
        {"smallstepback"     , ACTION_SMALL_STEP_BACK},
        {"fastforward"       , ACTION_PLAYER_FORWARD},
        {"rewind"            , ACTION_PLAYER_REWIND},
        {"play"              , ACTION_PLAYER_PLAY},
        {"playpause"         , ACTION_PLAYER_PLAYPAUSE},
        {"delete"            , ACTION_DELETE_ITEM},
        {"copy"              , ACTION_COPY_ITEM},
        {"move"              , ACTION_MOVE_ITEM},
        {"mplayerosd"        , ACTION_SHOW_MPLAYER_OSD},
        {"hidesubmenu"       , ACTION_OSD_HIDESUBMENU},
        {"screenshot"        , ACTION_TAKE_SCREENSHOT},
        {"rename"            , ACTION_RENAME_ITEM},
        {"togglewatched"     , ACTION_TOGGLE_WATCHED},
        {"scanitem"          , ACTION_SCAN_ITEM},
        {"reloadkeymaps"     , ACTION_RELOAD_KEYMAPS},
        {"volumeup"          , ACTION_VOLUME_UP},
        {"volumedown"        , ACTION_VOLUME_DOWN},
        {"mute"              , ACTION_MUTE},
        {"backspace"         , ACTION_BACKSPACE},
        {"scrollup"          , ACTION_SCROLL_UP},
        {"scrolldown"        , ACTION_SCROLL_DOWN},
        {"analogfastforward" , ACTION_ANALOG_FORWARD},
        {"analogrewind"      , ACTION_ANALOG_REWIND},
        {"moveitemup"        , ACTION_MOVE_ITEM_UP},
        {"moveitemdown"      , ACTION_MOVE_ITEM_DOWN},
        {"contextmenu"       , ACTION_CONTEXT_MENU},
        {"shift"             , ACTION_SHIFT},
        {"symbols"           , ACTION_SYMBOLS},
        {"cursorleft"        , ACTION_CURSOR_LEFT},
        {"cursorright"       , ACTION_CURSOR_RIGHT},
        {"showtime"          , ACTION_SHOW_OSD_TIME},
        {"analogseekforward" , ACTION_ANALOG_SEEK_FORWARD},
        {"analogseekback"    , ACTION_ANALOG_SEEK_BACK},
        {"showpreset"        , ACTION_VIS_PRESET_SHOW},
        {"presetlist"        , ACTION_VIS_PRESET_LIST},
        {"nextpreset"        , ACTION_VIS_PRESET_NEXT},
        {"previouspreset"    , ACTION_VIS_PRESET_PREV},
        {"lockpreset"        , ACTION_VIS_PRESET_LOCK},
        {"randompreset"      , ACTION_VIS_PRESET_RANDOM},
        {"increasevisrating" , ACTION_VIS_RATE_PRESET_PLUS},
        {"decreasevisrating" , ACTION_VIS_RATE_PRESET_MINUS},
        {"showvideomenu"     , ACTION_SHOW_VIDEOMENU},
        {"enter"             , ACTION_ENTER},
        {"increaserating"    , ACTION_INCREASE_RATING},
        {"decreaserating"    , ACTION_DECREASE_RATING},
        {"togglefullscreen"  , ACTION_TOGGLE_FULLSCREEN},
        {"nextscene"         , ACTION_NEXT_SCENE},
        {"previousscene"     , ACTION_PREV_SCENE},
        {"nextletter"        , ACTION_NEXT_LETTER},
        {"prevletter"        , ACTION_PREV_LETTER},
        {"jumpsms2"          , ACTION_JUMP_SMS2},
        {"jumpsms3"          , ACTION_JUMP_SMS3},
        {"jumpsms4"          , ACTION_JUMP_SMS4},
        {"jumpsms5"          , ACTION_JUMP_SMS5},
        {"jumpsms6"          , ACTION_JUMP_SMS6},
        {"jumpsms7"          , ACTION_JUMP_SMS7},
        {"jumpsms8"          , ACTION_JUMP_SMS8},
        {"jumpsms9"          , ACTION_JUMP_SMS9},
        {"filter"            , ACTION_FILTER},
        {"filterclear"       , ACTION_FILTER_CLEAR},
        {"filtersms2"        , ACTION_FILTER_SMS2},
        {"filtersms3"        , ACTION_FILTER_SMS3},
        {"filtersms4"        , ACTION_FILTER_SMS4},
        {"filtersms5"        , ACTION_FILTER_SMS5},
        {"filtersms6"        , ACTION_FILTER_SMS6},
        {"filtersms7"        , ACTION_FILTER_SMS7},
        {"filtersms8"        , ACTION_FILTER_SMS8},
        {"filtersms9"        , ACTION_FILTER_SMS9},
        {"firstpage"         , ACTION_FIRST_PAGE},
        {"lastpage"          , ACTION_LAST_PAGE},
        {"guiprofile"        , ACTION_GUIPROFILE_BEGIN},
        {"red"               , ACTION_TELETEXT_RED},
        {"green"             , ACTION_TELETEXT_GREEN},
        {"yellow"            , ACTION_TELETEXT_YELLOW},
        {"blue"              , ACTION_TELETEXT_BLUE},
        {"increasepar"       , ACTION_INCREASE_PAR},
        {"decreasepar"       , ACTION_DECREASE_PAR},
        {"volampup"          , ACTION_VOLAMP_UP},
        {"volampdown"        , ACTION_VOLAMP_DOWN},

        // PVR actions
        {"channelup"             , ACTION_CHANNEL_UP},
        {"channeldown"           , ACTION_CHANNEL_DOWN},
        {"previouschannelgroup"  , ACTION_PREVIOUS_CHANNELGROUP},
        {"nextchannelgroup"      , ACTION_NEXT_CHANNELGROUP},

        // Mouse actions
        {"leftclick"         , ACTION_MOUSE_LEFT_CLICK},
        {"rightclick"        , ACTION_MOUSE_RIGHT_CLICK},
        {"middleclick"       , ACTION_MOUSE_MIDDLE_CLICK},
        {"doubleclick"       , ACTION_MOUSE_DOUBLE_CLICK},
        {"wheelup"           , ACTION_MOUSE_WHEEL_UP},
        {"wheeldown"         , ACTION_MOUSE_WHEEL_DOWN},
        {"mousedrag"         , ACTION_MOUSE_DRAG},
        {"mousemove"         , ACTION_MOUSE_MOVE},

        // Do nothing action
        { "noop"             , ACTION_NOOP}
};

static const ActionMapping windows[] =
       {{"home"                     , WINDOW_HOME},
        {"programs"                 , WINDOW_PROGRAMS},
        {"pictures"                 , WINDOW_PICTURES},
        {"filemanager"              , WINDOW_FILES},
        {"files"                    , WINDOW_FILES}, // backward compat
        {"settings"                 , WINDOW_SETTINGS_MENU},
        {"music"                    , WINDOW_MUSIC},
        {"video"                    , WINDOW_VIDEOS},
        {"videos"                   , WINDOW_VIDEO_NAV},
        {"tv"                       , WINDOW_PVR}, // backward compat
        {"pvr"                      , WINDOW_PVR},
        {"pvrguideinfo"             , WINDOW_DIALOG_PVR_GUIDE_INFO},
        {"pvrrecordinginfo"         , WINDOW_DIALOG_PVR_RECORDING_INFO},
        {"pvrtimersetting"          , WINDOW_DIALOG_PVR_TIMER_SETTING},
        {"pvrgroupmanager"          , WINDOW_DIALOG_PVR_GROUP_MANAGER},
        {"pvrchannelmanager"        , WINDOW_DIALOG_PVR_CHANNEL_MANAGER},
        {"pvrguidesearch"           , WINDOW_DIALOG_PVR_GUIDE_SEARCH},
        {"pvrchannelscan"           , WINDOW_DIALOG_PVR_CHANNEL_SCAN},
        {"pvrupdateprogress"        , WINDOW_DIALOG_PVR_UPDATE_PROGRESS},
        {"pvrosdchannels"           , WINDOW_DIALOG_PVR_OSD_CHANNELS},
        {"pvrosdguide"              , WINDOW_DIALOG_PVR_OSD_GUIDE},
        {"pvrosddirector"           , WINDOW_DIALOG_PVR_OSD_DIRECTOR},
        {"pvrosdcutter"             , WINDOW_DIALOG_PVR_OSD_CUTTER},
        {"pvrosdteletext"           , WINDOW_DIALOG_OSD_TELETEXT},
        {"systeminfo"               , WINDOW_SYSTEM_INFORMATION},
        {"testpattern"              , WINDOW_TEST_PATTERN},
        {"screencalibration"        , WINDOW_SCREEN_CALIBRATION},
        {"guicalibration"           , WINDOW_SCREEN_CALIBRATION}, // backward compat
        {"picturessettings"         , WINDOW_SETTINGS_MYPICTURES},
        {"programssettings"         , WINDOW_SETTINGS_MYPROGRAMS},
        {"weathersettings"          , WINDOW_SETTINGS_MYWEATHER},
        {"musicsettings"            , WINDOW_SETTINGS_MYMUSIC},
        {"systemsettings"           , WINDOW_SETTINGS_SYSTEM},
        {"videossettings"           , WINDOW_SETTINGS_MYVIDEOS},
        {"networksettings"          , WINDOW_SETTINGS_SERVICE}, // backward compat
        {"servicesettings"          , WINDOW_SETTINGS_SERVICE},
        {"appearancesettings"       , WINDOW_SETTINGS_APPEARANCE},
        {"pvrsettings"              , WINDOW_SETTINGS_MYPVR},
        {"tvsettings"               , WINDOW_SETTINGS_MYPVR},  // backward compat
        {"scripts"                  , WINDOW_PROGRAMS}, // backward compat
        {"videofiles"               , WINDOW_VIDEO_FILES},
        {"videolibrary"             , WINDOW_VIDEO_NAV},
        {"videoplaylist"            , WINDOW_VIDEO_PLAYLIST},
        {"loginscreen"              , WINDOW_LOGIN_SCREEN},
        {"profiles"                 , WINDOW_SETTINGS_PROFILES},
        {"skinsettings"             , WINDOW_SKIN_SETTINGS},
        {"addonbrowser"             , WINDOW_ADDON_BROWSER},
        {"yesnodialog"              , WINDOW_DIALOG_YES_NO},
        {"progressdialog"           , WINDOW_DIALOG_PROGRESS},
        {"virtualkeyboard"          , WINDOW_DIALOG_KEYBOARD},
        {"volumebar"                , WINDOW_DIALOG_VOLUME_BAR},
        {"submenu"                  , WINDOW_DIALOG_SUB_MENU},
        {"favourites"               , WINDOW_DIALOG_FAVOURITES},
        {"contextmenu"              , WINDOW_DIALOG_CONTEXT_MENU},
        {"infodialog"               , WINDOW_DIALOG_KAI_TOAST},
        {"numericinput"             , WINDOW_DIALOG_NUMERIC},
        {"gamepadinput"             , WINDOW_DIALOG_GAMEPAD},
        {"shutdownmenu"             , WINDOW_DIALOG_BUTTON_MENU},
        {"mutebug"                  , WINDOW_DIALOG_MUTE_BUG},
        {"playercontrols"           , WINDOW_DIALOG_PLAYER_CONTROLS},
        {"seekbar"                  , WINDOW_DIALOG_SEEK_BAR},
        {"musicosd"                 , WINDOW_DIALOG_MUSIC_OSD},
        {"addonsettings"            , WINDOW_DIALOG_ADDON_SETTINGS},
        {"visualisationsettings"    , WINDOW_DIALOG_ADDON_SETTINGS}, // backward compat
        {"visualisationpresetlist"  , WINDOW_DIALOG_VIS_PRESET_LIST},
        {"osdvideosettings"         , WINDOW_DIALOG_VIDEO_OSD_SETTINGS},
        {"osdaudiosettings"         , WINDOW_DIALOG_AUDIO_OSD_SETTINGS},
        {"videobookmarks"           , WINDOW_DIALOG_VIDEO_BOOKMARKS},
        {"filebrowser"              , WINDOW_DIALOG_FILE_BROWSER},
        {"networksetup"             , WINDOW_DIALOG_NETWORK_SETUP},
        {"mediasource"              , WINDOW_DIALOG_MEDIA_SOURCE},
        {"profilesettings"          , WINDOW_DIALOG_PROFILE_SETTINGS},
        {"locksettings"             , WINDOW_DIALOG_LOCK_SETTINGS},
        {"contentsettings"          , WINDOW_DIALOG_CONTENT_SETTINGS},
        {"songinformation"          , WINDOW_DIALOG_SONG_INFO},
        {"smartplaylisteditor"      , WINDOW_DIALOG_SMART_PLAYLIST_EDITOR},
        {"smartplaylistrule"        , WINDOW_DIALOG_SMART_PLAYLIST_RULE},
        {"busydialog"               , WINDOW_DIALOG_BUSY},
        {"pictureinfo"              , WINDOW_DIALOG_PICTURE_INFO},
        {"accesspoints"             , WINDOW_DIALOG_ACCESS_POINTS},
        {"fullscreeninfo"           , WINDOW_DIALOG_FULLSCREEN_INFO},
        {"karaokeselector"          , WINDOW_DIALOG_KARAOKE_SONGSELECT},
        {"karaokelargeselector"     , WINDOW_DIALOG_KARAOKE_SELECTOR},
        {"sliderdialog"             , WINDOW_DIALOG_SLIDER},
        {"addoninformation"         , WINDOW_DIALOG_ADDON_INFO},
        {"musicplaylist"            , WINDOW_MUSIC_PLAYLIST},
        {"musicfiles"               , WINDOW_MUSIC_FILES},
        {"musiclibrary"             , WINDOW_MUSIC_NAV},
        {"musicplaylisteditor"      , WINDOW_MUSIC_PLAYLIST_EDITOR},
        {"teletext"                 , WINDOW_DIALOG_OSD_TELETEXT},
        {"selectdialog"             , WINDOW_DIALOG_SELECT},
        {"musicinformation"         , WINDOW_DIALOG_MUSIC_INFO},
        {"okdialog"                 , WINDOW_DIALOG_OK},
        {"movieinformation"         , WINDOW_DIALOG_VIDEO_INFO},
        {"textviewer"               , WINDOW_DIALOG_TEXT_VIEWER},
        {"fullscreenvideo"          , WINDOW_FULLSCREEN_VIDEO},
        {"fullscreenlivetv"         , WINDOW_FULLSCREEN_LIVETV}, // virtual window/keymap section for PVR specific bindings in fullscreen playback (which internally uses WINDOW_FULLSCREEN_VIDEO)
        {"visualisation"            , WINDOW_VISUALISATION},
        {"slideshow"                , WINDOW_SLIDESHOW},
        {"filestackingdialog"       , WINDOW_DIALOG_FILESTACKING},
        {"karaoke"                  , WINDOW_KARAOKELYRICS},
        {"weather"                  , WINDOW_WEATHER},
        {"screensaver"              , WINDOW_SCREENSAVER},
        {"videoosd"                 , WINDOW_DIALOG_VIDEO_OSD},
        {"videomenu"                , WINDOW_VIDEO_MENU},
        {"videotimeseek"            , WINDOW_VIDEO_TIME_SEEK},
        {"musicoverlay"             , WINDOW_DIALOG_MUSIC_OVERLAY},
        {"videooverlay"             , WINDOW_DIALOG_VIDEO_OVERLAY},
        {"startwindow"              , WINDOW_START},
        {"startup"                  , WINDOW_STARTUP_ANIM},
        {"peripherals"              , WINDOW_DIALOG_PERIPHERAL_MANAGER},
        {"peripheralsettings"       , WINDOW_DIALOG_PERIPHERAL_SETTINGS},
        {"extendedprogressdialog"   , WINDOW_DIALOG_EXT_PROGRESS},
        {"mediafilter"              , WINDOW_DIALOG_MEDIA_FILTER}};

static const ActionMapping mousecommands[] =
{
  { "leftclick",   ACTION_MOUSE_LEFT_CLICK },
  { "rightclick",  ACTION_MOUSE_RIGHT_CLICK },
  { "middleclick", ACTION_MOUSE_MIDDLE_CLICK },
  { "doubleclick", ACTION_MOUSE_DOUBLE_CLICK },
  { "wheelup",     ACTION_MOUSE_WHEEL_UP },
  { "wheeldown",   ACTION_MOUSE_WHEEL_DOWN },
  { "mousedrag",   ACTION_MOUSE_DRAG },
  { "mousemove",   ACTION_MOUSE_MOVE }
};

#ifdef WIN32
static const ActionMapping appcommands[] =
{
  { "browser_back",        APPCOMMAND_BROWSER_BACKWARD },
  { "browser_forward",     APPCOMMAND_BROWSER_FORWARD },
  { "browser_refresh",     APPCOMMAND_BROWSER_REFRESH },
  { "browser_stop",        APPCOMMAND_BROWSER_STOP },
  { "browser_search",      APPCOMMAND_BROWSER_SEARCH },
  { "browser_favorites",   APPCOMMAND_BROWSER_FAVORITES },
  { "browser_home",        APPCOMMAND_BROWSER_HOME },
  { "volume_mute",         APPCOMMAND_VOLUME_MUTE },
  { "volume_down",         APPCOMMAND_VOLUME_DOWN },
  { "volume_up",           APPCOMMAND_VOLUME_UP },
  { "next_track",          APPCOMMAND_MEDIA_NEXTTRACK },
  { "prev_track",          APPCOMMAND_MEDIA_PREVIOUSTRACK },
  { "stop",                APPCOMMAND_MEDIA_STOP },
  { "play_pause",          APPCOMMAND_MEDIA_PLAY_PAUSE },
  { "launch_mail",         APPCOMMAND_LAUNCH_MAIL },
  { "launch_media_select", APPCOMMAND_LAUNCH_MEDIA_SELECT },
  { "launch_app1",         APPCOMMAND_LAUNCH_APP1 },
  { "launch_app2",         APPCOMMAND_LAUNCH_APP2 },
  { "play",                APPCOMMAND_MEDIA_PLAY },
  { "pause",               APPCOMMAND_MEDIA_PAUSE },
  { "fastforward",         APPCOMMAND_MEDIA_FAST_FORWARD },
  { "rewind",              APPCOMMAND_MEDIA_REWIND },
  { "channelup",           APPCOMMAND_MEDIA_CHANNEL_UP },
  { "channeldown",         APPCOMMAND_MEDIA_CHANNEL_DOWN }
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

CButtonTranslator::~CButtonTranslator()
{
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  vector<lircButtonMap*> maps;
  for (map<CStdString,lircButtonMap*>::iterator it  = lircRemotesMap.begin();
                                                it != lircRemotesMap.end();++it)
    maps.push_back(it->second);
  sort(maps.begin(),maps.end());
  vector<lircButtonMap*>::iterator itend = unique(maps.begin(),maps.end());
  for (vector<lircButtonMap*>::iterator it = maps.begin(); it != itend;++it)
    delete *it;
#endif
}

// Add the supplied device name to the list of connected devices
void CButtonTranslator::AddDevice(CStdString& strDevice)
{
  // Only add the device if it isn't already in the list
  std::list<CStdString>::iterator it;
  for (it = m_deviceList.begin(); it != m_deviceList.end(); it++)
    if (*it == strDevice)
      return;

  // Add the device
  m_deviceList.push_back(strDevice);
  m_deviceList.sort();

  // New device added so reload the key mappings
  Load();
}

void CButtonTranslator::RemoveDevice(CStdString& strDevice)
{
  // Find the device
  std::list<CStdString>::iterator it;
  for (it = m_deviceList.begin(); it != m_deviceList.end(); it++)
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

  for (unsigned int dirIndex = 0; dirIndex < sizeof(DIRS_TO_CHECK)/sizeof(DIRS_TO_CHECK[0]); ++dirIndex)
  {
    if (XFILE::CDirectory::Exists(DIRS_TO_CHECK[dirIndex]))
    {
      CFileItemList files;
      XFILE::CDirectory::GetDirectory(DIRS_TO_CHECK[dirIndex], files, ".xml");
      // Sort the list for filesystem based priorities, e.g. 01-keymap.xml, 02-keymap-overrides.xml
      files.Sort(SORT_METHOD_FILE, SortOrderAscending);
      for(int fileIndex = 0; fileIndex<files.Size(); ++fileIndex)
      {
        if (!files[fileIndex]->m_bIsFolder)
          success |= LoadKeymap(files[fileIndex]->GetPath());
      }

      // Load mappings for any HID devices we have connected
      std::list<CStdString>::iterator it;
      for (it = m_deviceList.begin(); it != m_deviceList.end(); it++)
      {
        CStdString devicedir = DIRS_TO_CHECK[dirIndex];
        devicedir.append(*it);
        devicedir.append("/");
        if( XFILE::CDirectory::Exists(devicedir) )
        {
          CFileItemList files;
          XFILE::CDirectory::GetDirectory(devicedir, files, ".xml");
          // Sort the list for filesystem based priorities, e.g. 01-keymap.xml, 02-keymap-overrides.xml
          files.Sort(SORT_METHOD_FILE, SortOrderAscending);
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
#ifdef _LINUX
#define REMOTEMAP "Lircmap.xml"
#else
#define REMOTEMAP "IRSSmap.xml"
#endif
  CStdString lircmapPath;
  URIUtils::AddFileToFolder("special://xbmc/system/", REMOTEMAP, lircmapPath);
  lircRemotesMap.clear();
  if(CFile::Exists(lircmapPath))
    success |= LoadLircMap(lircmapPath);
  else
    CLog::Log(LOGDEBUG, "CButtonTranslator::Load - no system %s found, skipping", REMOTEMAP);

  lircmapPath = g_settings.GetUserDataItem(REMOTEMAP);
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

bool CButtonTranslator::LoadKeymap(const CStdString &keymapPath)
{
  CXBMCTinyXML xmlDoc;

  CLog::Log(LOGINFO, "Loading %s", keymapPath.c_str());
  if (!xmlDoc.LoadFile(keymapPath))
  {
    CLog::Log(LOGERROR, "Error loading keymap: %s, Line %d\n%s", keymapPath.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }
  TiXmlElement* pRoot = xmlDoc.RootElement();
  CStdString strValue = pRoot->Value();
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

#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
bool CButtonTranslator::LoadLircMap(const CStdString &lircmapPath)
{
#ifdef _LINUX
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
  CStdString strValue = pRoot->Value();
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
        const char* szDeviceName = pAttr->Value();
        MapRemote(pRemote, szDeviceName);
      }
    }
    pRemote = pRemote->NextSibling();
  }

  return true;
}

void CButtonTranslator::MapRemote(TiXmlNode *pRemote, const char* szDevice)
{
  CLog::Log(LOGINFO, "* Adding remote mapping for device '%s'", szDevice);
  vector<string> RemoteNames;
  map<CStdString, lircButtonMap*>::iterator it = lircRemotesMap.find(szDevice);
  if (it == lircRemotesMap.end())
    lircRemotesMap[szDevice] = new lircButtonMap;
  lircButtonMap& buttons = *lircRemotesMap[szDevice];

  TiXmlElement *pButton = pRemote->FirstChildElement();
  while (pButton)
  {
    if (strcmpi(pButton->Value(), "altname")==0)
      RemoteNames.push_back(string(pButton->GetText()));
    else
    {
      if (pButton->FirstChild() && pButton->FirstChild()->Value())
        buttons[pButton->FirstChild()->Value()] = pButton->Value();
    }

    pButton = pButton->NextSiblingElement();
  }
  for (vector<string>::iterator it  = RemoteNames.begin();
                                it != RemoteNames.end();++it)
  {
    CLog::Log(LOGINFO, "* Linking remote mapping for '%s' to '%s'", szDevice, it->c_str());
    lircRemotesMap[*it] = &buttons;
  }
}

int CButtonTranslator::TranslateLircRemoteString(const char* szDevice, const char *szButton)
{
  // Find the device
  map<CStdString, lircButtonMap*>::iterator it = lircRemotesMap.find(szDevice);
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
#endif

#if defined(HAS_SDL_JOYSTICK) || defined(HAS_EVENT_SERVER)
void CButtonTranslator::MapJoystickActions(int windowID, TiXmlNode *pJoystick)
{
  string joyname = "_xbmc_"; // default global map name
  vector<string> joynames;
  map<int, string> buttonMap;
  map<int, string> axisMap;
  map<int, string> hatMap;

  TiXmlElement *pJoy = pJoystick->ToElement();
  if (pJoy && pJoy->Attribute("name"))
    joyname = pJoy->Attribute("name");
  else
    CLog::Log(LOGNOTICE, "No Joystick name specified, loading default map");

  joynames.push_back(joyname);

  // parse map
  TiXmlElement *pButton = pJoystick->FirstChildElement();
  int id = 0;
  //char* szId;
  const char* szType;
  const char *szAction;
  while (pButton)
  {
    szType = pButton->Value();
    szAction = pButton->GetText();
    if (szAction == NULL)
      szAction = "";
    if (szType)
    {
      if ((pButton->QueryIntAttribute("id", &id) == TIXML_SUCCESS) && id>=0 && id<=256)
      {
        if (strcmpi(szType, "button")==0)
        {
          buttonMap[id] = string(szAction);
        }
        else if (strcmpi(szType, "axis")==0)
        {
          int limit = 0;
          if (pButton->QueryIntAttribute("limit", &limit) == TIXML_SUCCESS)
          {
            if (limit==-1)
              axisMap[-id] = string(szAction);
            else if (limit==1)
              axisMap[id] = string(szAction);
            else if (limit==0)
              axisMap[id|0xFFFF0000] = string(szAction);
            else
            {
              axisMap[id] = string(szAction);
              axisMap[-id] = string(szAction);
              CLog::Log(LOGERROR, "Error in joystick map, invalid limit specified %d for axis %d", limit, id);
            }
          }
          else
          {
            axisMap[id] = string(szAction);
            axisMap[-id] = string(szAction);
          }
        }
        else if (strcmpi(szType, "hat")==0)
        {
          string position;
          if (pButton->QueryValueAttribute("position", &position) == TIXML_SUCCESS)
          {
            uint32_t hatID = id|0xFFF00000;
            if (position.compare("up") == 0)
              hatMap[(JACTIVE_HAT_UP<<16)|hatID] = string(szAction);
            else if (position.compare("down") == 0)
              hatMap[(JACTIVE_HAT_DOWN<<16)|hatID] = string(szAction);
            else if (position.compare("right") == 0)
              hatMap[(JACTIVE_HAT_RIGHT<<16)|hatID] = string(szAction);
            else if (position.compare("left") == 0)
              hatMap[(JACTIVE_HAT_LEFT<<16)|hatID] = string(szAction);
            else
              CLog::Log(LOGERROR, "Error in joystick map, invalid position specified %s for axis %d", position.c_str(), id);
          }
        }
        else
          CLog::Log(LOGERROR, "Error reading joystick map element, unknown button type: %s", szType);
      }
      else if (strcmpi(szType, "altname")==0)
        joynames.push_back(string(szAction));
      else
        CLog::Log(LOGERROR, "Error reading joystick map element, Invalid id: %d", id);
    }
    else
      CLog::Log(LOGERROR, "Error reading joystick map element, skipping");

    pButton = pButton->NextSiblingElement();
  }
  vector<string>::iterator it = joynames.begin();
  while (it!=joynames.end())
  {
    m_joystickButtonMap[*it][windowID] = buttonMap;
    m_joystickAxisMap[*it][windowID] = axisMap;
    m_joystickHatMap[*it][windowID] = hatMap;
//    CLog::Log(LOGDEBUG, "Found Joystick map for window %d using %s", windowID, it->c_str());
    it++;
  }
}

bool CButtonTranslator::TranslateJoystickString(int window, const char* szDevice, int id, short inputType, int& action, CStdString& strAction, bool &fullrange)
{
  bool found = false;

  map<string, JoystickMap>::iterator it;
  map<string, JoystickMap> *jmap;

  fullrange = false;
  if (inputType == JACTIVE_AXIS)
    jmap = &m_joystickAxisMap;
  else if (inputType == JACTIVE_BUTTON)
    jmap = &m_joystickButtonMap;
  else if (inputType == JACTIVE_HAT)
  	jmap = &m_joystickHatMap;
  else
  {
    CLog::Log(LOGERROR, "Error reading joystick input type");
    return false;
  }

  it = jmap->find(szDevice);
  if (it==jmap->end())
    return false;

  JoystickMap wmap = it->second;
  JoystickMap::iterator it2;
  map<int, string> windowbmap;
  map<int, string> globalbmap;
  map<int, string>::iterator it3;

  it2 = wmap.find(window);

  // first try local window map
  if (it2!=wmap.end())
  {
    windowbmap = it2->second;
    it3 = windowbmap.find(id);
    if (it3 != windowbmap.end())
    {
      strAction = (it3->second).c_str();
      found = true;
    }
    it3 = windowbmap.find(abs(id)|0xFFFF0000);
    if (it3 != windowbmap.end())
    {
      strAction = (it3->second).c_str();
      found = true;
      fullrange = true;
    }
    // Hats joystick
    it3 = windowbmap.find(id|0xFFF00000);
    if (it3 != windowbmap.end())
    {
      strAction = (it3->second).c_str();
      found = true;
    }
  }

  // if not found, try global map
  if (!found)
  {
    it2 = wmap.find(-1);
    if (it2 != wmap.end())
    {
      globalbmap = it2->second;
      it3 = globalbmap.find(id);
      if (it3 != globalbmap.end())
      {
        strAction = (it3->second).c_str();
        found = true;
      }
      it3 = globalbmap.find(abs(id)|0xFFFF0000);
      if (it3 != globalbmap.end())
      {
        strAction = (it3->second).c_str();
        found = true;
        fullrange = true;
      }
      it3 = globalbmap.find(id|0xFFF00000);
      if (it3 != globalbmap.end())
      {
        strAction = (it3->second).c_str();
        found = true;
      }
    }
  }

  // translated found action
  if (found)
    return TranslateActionString(strAction.c_str(), action);

  return false;
}
#endif

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

CAction CButtonTranslator::GetAction(int window, const CKey &key, bool fallback)
{
  CStdString strAction;
  // try to get the action from the current window
  int actionID = GetActionCode(window, key, strAction);
  // if it's invalid, try to get it from the global map
  if (actionID == 0 && fallback)
    actionID = GetActionCode( -1, key, strAction);
  // Now fill our action structure
  CAction action(actionID, strAction, key);
  return action;
}

int CButtonTranslator::GetActionCode(int window, const CKey &key, CStdString &strAction) const
{
  uint32_t code = key.GetButtonCode();

  map<int, buttonMap>::const_iterator it = m_translatorMap.find(window);
  if (it == m_translatorMap.end())
    return 0;
  buttonMap::const_iterator it2 = (*it).second.find(code);
  int action = 0;
  while (it2 != (*it).second.end())
  {
    action = (*it2).second.id;
    strAction = (*it2).second.strID;
    it2 = (*it).second.end();
  }
#ifdef _LINUX
  // Some buttoncodes changed in Hardy
  if (action == 0 && (code & KEY_VKEY) == KEY_VKEY && (code & 0x0F00))
  {
    CLog::Log(LOGDEBUG, "%s: Trying Hardy keycode for %#04x", __FUNCTION__, code);
    code &= ~0x0F00;
    buttonMap::const_iterator it2 = (*it).second.find(code);
    while (it2 != (*it).second.end())
    {
      action = (*it2).second.id;
      strAction = (*it2).second.strID;
      it2 = (*it).second.end();
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
    map.insert(pair<uint32_t, CButtonAction>(buttonCode, button));
  }
}

bool CButtonTranslator::HasDeviceType(TiXmlNode *pWindow, CStdString type)
{
  return pWindow->FirstChild(type) != NULL;
}

void CButtonTranslator::MapWindowActions(TiXmlNode *pWindow, int windowID)
{
  if (!pWindow || windowID == WINDOW_INVALID) 
    return;

  TiXmlNode* pDevice;

  const char* types[] = {"gamepad", "remote", "universalremote", "keyboard", "mouse", "appcommand", NULL};
  for (int i = 0; types[i]; ++i)
  {
    CStdString type(types[i]);
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
            buttonCode = TranslateMouseCommand(pButton->Value());
        else if (type == "appcommand")
            buttonCode = TranslateAppCommand(pButton->Value());

        if (buttonCode && pButton->FirstChild())
          MapAction(buttonCode, pButton->FirstChild()->Value(), map);
        pButton = pButton->NextSiblingElement();
      }

      // add our map to our table
      if (map.size() > 0)
        m_translatorMap.insert(pair<int, buttonMap>( windowID, map));
    }
  }

#if defined(HAS_SDL_JOYSTICK) || defined(HAS_EVENT_SERVER)
  if ((pDevice = pWindow->FirstChild("joystick")) != NULL)
  {
    // map joystick actions
    while (pDevice)
    {
      MapJoystickActions(windowID, pDevice);
      pDevice = pDevice->NextSibling("joystick");
    }
  }
#endif
}

bool CButtonTranslator::TranslateActionString(const char *szAction, int &action)
{
  action = ACTION_NONE;
  CStdString strAction = szAction;
  strAction.ToLower();
  if (CBuiltins::HasCommand(strAction)) 
    action = ACTION_BUILT_IN_FUNCTION;

  for (unsigned int index=0;index < sizeof(actions)/sizeof(actions[0]);++index)
  {
    if (strAction.Equals(actions[index].name))
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

CStdString CButtonTranslator::TranslateWindow(int windowID)
{
  for (unsigned int index = 0; index < sizeof(windows) / sizeof(windows[0]); ++index)
  {
    if (windows[index].action == windowID)
      return windows[index].name;
  }
  return "";
}

int CButtonTranslator::TranslateWindow(const CStdString &window)
{
  CStdString strWindow(window);
  if (strWindow.IsEmpty()) 
    return WINDOW_INVALID;
  strWindow.ToLower();
  // eliminate .xml
  if (strWindow.Mid(strWindow.GetLength() - 4) == ".xml" )
    strWindow = strWindow.Mid(0, strWindow.GetLength() - 4);

  // window12345, for custom window to be keymapped
  if (strWindow.length() > 6 && strWindow.Left(6).Equals("window"))
    strWindow = strWindow.Mid(6);
  if (strWindow.Left(2) == "my")  // drop "my" prefix
    strWindow = strWindow.Mid(2);
  if (StringUtils::IsNaturalNumber(strWindow))
  {
    // allow a full window id or a delta id
    int iWindow = atoi(strWindow.c_str());
    if (iWindow > WINDOW_INVALID)
      return iWindow;
    return WINDOW_HOME + iWindow;
  }

  // run through the window structure
  for (unsigned int index = 0; index < sizeof(windows) / sizeof(windows[0]); ++index)
  {
    if (strWindow.Equals(windows[index].name))
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
  CStdString strButton = szButton;
  strButton.ToLower();
  if (strButton.Equals("a")) buttonCode = KEY_BUTTON_A;
  else if (strButton.Equals("b")) buttonCode = KEY_BUTTON_B;
  else if (strButton.Equals("x")) buttonCode = KEY_BUTTON_X;
  else if (strButton.Equals("y")) buttonCode = KEY_BUTTON_Y;
  else if (strButton.Equals("white")) buttonCode = KEY_BUTTON_WHITE;
  else if (strButton.Equals("black")) buttonCode = KEY_BUTTON_BLACK;
  else if (strButton.Equals("start")) buttonCode = KEY_BUTTON_START;
  else if (strButton.Equals("back")) buttonCode = KEY_BUTTON_BACK;
  else if (strButton.Equals("leftthumbbutton")) buttonCode = KEY_BUTTON_LEFT_THUMB_BUTTON;
  else if (strButton.Equals("rightthumbbutton")) buttonCode = KEY_BUTTON_RIGHT_THUMB_BUTTON;
  else if (strButton.Equals("leftthumbstick")) buttonCode = KEY_BUTTON_LEFT_THUMB_STICK;
  else if (strButton.Equals("leftthumbstickup")) buttonCode = KEY_BUTTON_LEFT_THUMB_STICK_UP;
  else if (strButton.Equals("leftthumbstickdown")) buttonCode = KEY_BUTTON_LEFT_THUMB_STICK_DOWN;
  else if (strButton.Equals("leftthumbstickleft")) buttonCode = KEY_BUTTON_LEFT_THUMB_STICK_LEFT;
  else if (strButton.Equals("leftthumbstickright")) buttonCode = KEY_BUTTON_LEFT_THUMB_STICK_RIGHT;
  else if (strButton.Equals("rightthumbstick")) buttonCode = KEY_BUTTON_RIGHT_THUMB_STICK;
  else if (strButton.Equals("rightthumbstickup")) buttonCode = KEY_BUTTON_RIGHT_THUMB_STICK_UP;
  else if (strButton.Equals("rightthumbstickdown")) buttonCode = KEY_BUTTON_RIGHT_THUMB_STICK_DOWN;
  else if (strButton.Equals("rightthumbstickleft")) buttonCode = KEY_BUTTON_RIGHT_THUMB_STICK_LEFT;
  else if (strButton.Equals("rightthumbstickright")) buttonCode = KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT;
  else if (strButton.Equals("lefttrigger")) buttonCode = KEY_BUTTON_LEFT_TRIGGER;
  else if (strButton.Equals("righttrigger")) buttonCode = KEY_BUTTON_RIGHT_TRIGGER;
  else if (strButton.Equals("leftanalogtrigger")) buttonCode = KEY_BUTTON_LEFT_ANALOG_TRIGGER;
  else if (strButton.Equals("rightanalogtrigger")) buttonCode = KEY_BUTTON_RIGHT_ANALOG_TRIGGER;
  else if (strButton.Equals("dpadleft")) buttonCode = KEY_BUTTON_DPAD_LEFT;
  else if (strButton.Equals("dpadright")) buttonCode = KEY_BUTTON_DPAD_RIGHT;
  else if (strButton.Equals("dpadup")) buttonCode = KEY_BUTTON_DPAD_UP;
  else if (strButton.Equals("dpaddown")) buttonCode = KEY_BUTTON_DPAD_DOWN;
  else CLog::Log(LOGERROR, "Gamepad Translator: Can't find button %s", strButton.c_str());
  return buttonCode;
}

uint32_t CButtonTranslator::TranslateRemoteString(const char *szButton)
{
  if (!szButton) 
    return 0;
  uint32_t buttonCode = 0;
  CStdString strButton = szButton;
  strButton.ToLower();
  if (strButton.Equals("left")) buttonCode = XINPUT_IR_REMOTE_LEFT;
  else if (strButton.Equals("right")) buttonCode = XINPUT_IR_REMOTE_RIGHT;
  else if (strButton.Equals("up")) buttonCode = XINPUT_IR_REMOTE_UP;
  else if (strButton.Equals("down")) buttonCode = XINPUT_IR_REMOTE_DOWN;
  else if (strButton.Equals("select")) buttonCode = XINPUT_IR_REMOTE_SELECT;
  else if (strButton.Equals("back")) buttonCode = XINPUT_IR_REMOTE_BACK;
  else if (strButton.Equals("menu")) buttonCode = XINPUT_IR_REMOTE_MENU;
  else if (strButton.Equals("info")) buttonCode = XINPUT_IR_REMOTE_INFO;
  else if (strButton.Equals("display")) buttonCode = XINPUT_IR_REMOTE_DISPLAY;
  else if (strButton.Equals("title")) buttonCode = XINPUT_IR_REMOTE_TITLE;
  else if (strButton.Equals("play")) buttonCode = XINPUT_IR_REMOTE_PLAY;
  else if (strButton.Equals("pause")) buttonCode = XINPUT_IR_REMOTE_PAUSE;
  else if (strButton.Equals("reverse")) buttonCode = XINPUT_IR_REMOTE_REVERSE;
  else if (strButton.Equals("forward")) buttonCode = XINPUT_IR_REMOTE_FORWARD;
  else if (strButton.Equals("skipplus")) buttonCode = XINPUT_IR_REMOTE_SKIP_PLUS;
  else if (strButton.Equals("skipminus")) buttonCode = XINPUT_IR_REMOTE_SKIP_MINUS;
  else if (strButton.Equals("stop")) buttonCode = XINPUT_IR_REMOTE_STOP;
  else if (strButton.Equals("zero")) buttonCode = XINPUT_IR_REMOTE_0;
  else if (strButton.Equals("one")) buttonCode = XINPUT_IR_REMOTE_1;
  else if (strButton.Equals("two")) buttonCode = XINPUT_IR_REMOTE_2;
  else if (strButton.Equals("three")) buttonCode = XINPUT_IR_REMOTE_3;
  else if (strButton.Equals("four")) buttonCode = XINPUT_IR_REMOTE_4;
  else if (strButton.Equals("five")) buttonCode = XINPUT_IR_REMOTE_5;
  else if (strButton.Equals("six")) buttonCode = XINPUT_IR_REMOTE_6;
  else if (strButton.Equals("seven")) buttonCode = XINPUT_IR_REMOTE_7;
  else if (strButton.Equals("eight")) buttonCode = XINPUT_IR_REMOTE_8;
  else if (strButton.Equals("nine")) buttonCode = XINPUT_IR_REMOTE_9;
  // additional keys from the media center extender for xbox remote
  else if (strButton.Equals("power")) buttonCode = XINPUT_IR_REMOTE_POWER;
  else if (strButton.Equals("mytv")) buttonCode = XINPUT_IR_REMOTE_MY_TV;
  else if (strButton.Equals("mymusic")) buttonCode = XINPUT_IR_REMOTE_MY_MUSIC;
  else if (strButton.Equals("mypictures")) buttonCode = XINPUT_IR_REMOTE_MY_PICTURES;
  else if (strButton.Equals("myvideo")) buttonCode = XINPUT_IR_REMOTE_MY_VIDEOS;
  else if (strButton.Equals("record")) buttonCode = XINPUT_IR_REMOTE_RECORD;
  else if (strButton.Equals("start")) buttonCode = XINPUT_IR_REMOTE_START;
  else if (strButton.Equals("volumeplus")) buttonCode = XINPUT_IR_REMOTE_VOLUME_PLUS;
  else if (strButton.Equals("volumeminus")) buttonCode = XINPUT_IR_REMOTE_VOLUME_MINUS;
  else if (strButton.Equals("channelplus")) buttonCode = XINPUT_IR_REMOTE_CHANNEL_PLUS;
  else if (strButton.Equals("channelminus")) buttonCode = XINPUT_IR_REMOTE_CHANNEL_MINUS;
  else if (strButton.Equals("pageplus")) buttonCode = XINPUT_IR_REMOTE_CHANNEL_PLUS;
  else if (strButton.Equals("pageminus")) buttonCode = XINPUT_IR_REMOTE_CHANNEL_MINUS;
  else if (strButton.Equals("mute")) buttonCode = XINPUT_IR_REMOTE_MUTE;
  else if (strButton.Equals("recordedtv")) buttonCode = XINPUT_IR_REMOTE_RECORDED_TV;
  else if (strButton.Equals("guide")) buttonCode = XINPUT_IR_REMOTE_GUIDE;
  else if (strButton.Equals("livetv")) buttonCode = XINPUT_IR_REMOTE_LIVE_TV;
  else if (strButton.Equals("liveradio")) buttonCode = XINPUT_IR_REMOTE_LIVE_RADIO;
  else if (strButton.Equals("epgsearch")) buttonCode = XINPUT_IR_REMOTE_EPG_SEARCH;
  else if (strButton.Equals("star")) buttonCode = XINPUT_IR_REMOTE_STAR;
  else if (strButton.Equals("hash")) buttonCode = XINPUT_IR_REMOTE_HASH;
  else if (strButton.Equals("clear")) buttonCode = XINPUT_IR_REMOTE_CLEAR;
  else if (strButton.Equals("enter")) buttonCode = XINPUT_IR_REMOTE_ENTER;
  else if (strButton.Equals("xbox")) buttonCode = XINPUT_IR_REMOTE_DISPLAY; // same as display
  else if (strButton.Equals("playlist")) buttonCode = XINPUT_IR_REMOTE_PLAYLIST;
  else if (strButton.Equals("guide")) buttonCode = XINPUT_IR_REMOTE_GUIDE;
  else if (strButton.Equals("teletext")) buttonCode = XINPUT_IR_REMOTE_TELETEXT;
  else if (strButton.Equals("red")) buttonCode = XINPUT_IR_REMOTE_RED;
  else if (strButton.Equals("green")) buttonCode = XINPUT_IR_REMOTE_GREEN;
  else if (strButton.Equals("yellow")) buttonCode = XINPUT_IR_REMOTE_YELLOW;
  else if (strButton.Equals("blue")) buttonCode = XINPUT_IR_REMOTE_BLUE;
  else if (strButton.Equals("subtitle")) buttonCode = XINPUT_IR_REMOTE_SUBTITLE;
  else if (strButton.Equals("language")) buttonCode = XINPUT_IR_REMOTE_LANGUAGE;
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
  CStdString strKey = szButton;
  if (strKey.Equals("key"))
  {
    int id = 0;
    if (pButton->QueryIntAttribute("id", &id) == TIXML_SUCCESS)
      button_id = (uint32_t)id;
    else
      CLog::Log(LOGERROR, "Keyboard Translator: `key' button has no id");
  }
  else
    button_id = TranslateKeyboardString(szButton);

  // Process the ctrl/shift/alt modifiers
  CStdString strMod;
  if (pButton->QueryValueAttribute("mod", &strMod) == TIXML_SUCCESS)
  {
    strMod.ToLower();

    CStdStringArray modArray;
    StringUtils::SplitString(strMod, ",", modArray);
    for (unsigned int i = 0; i < modArray.size(); i++)
    {
      CStdString& substr = modArray[i];
      substr.Trim();

      if (substr == "ctrl" || substr == "control")
        button_id |= CKey::MODIFIER_CTRL;
      else if (substr == "shift")
        button_id |= CKey::MODIFIER_SHIFT;
      else if (substr == "alt")
        button_id |= CKey::MODIFIER_ALT;
      else if (substr == "super" || substr == "win")
        button_id |= CKey::MODIFIER_SUPER;
      else
        CLog::Log(LOGERROR, "Keyboard Translator: Unknown key modifier %s in %s", substr.c_str(), strMod.c_str());
     }
  }

  return button_id;
}

uint32_t CButtonTranslator::TranslateAppCommand(const char *szButton)
{
#ifdef WIN32
  CStdString strAppCommand = szButton;
  strAppCommand.ToLower();

  for (int i = 0; i < sizeof(appcommands)/sizeof(appcommands[0]); i++)
    if (strAppCommand.Equals(appcommands[i].name))
      return appcommands[i].action | KEY_APPCOMMAND;

  CLog::Log(LOGERROR, "%s: Can't find appcommand %s", __FUNCTION__, szButton);
#endif

  return 0;
}

uint32_t CButtonTranslator::TranslateMouseCommand(const char *szButton)
{
  CStdString strMouseCommand = szButton;
  strMouseCommand.ToLower();

  for (unsigned int i = 0; i < sizeof(mousecommands)/sizeof(mousecommands[0]); i++)
    if (strMouseCommand.Equals(mousecommands[i].name))
      return mousecommands[i].action | KEY_MOUSE;

  CLog::Log(LOGERROR, "%s: Can't find mouse command %s", __FUNCTION__, szButton);

  return 0;
}

void CButtonTranslator::Clear()
{
  m_translatorMap.clear();
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  lircRemotesMap.clear();
#endif

#if defined(HAS_SDL_JOYSTICK) || defined(HAS_EVENT_SERVER)
  m_joystickButtonMap.clear();
  m_joystickAxisMap.clear();
  m_joystickHatMap.clear();
#endif

  m_Loaded = false;
}
