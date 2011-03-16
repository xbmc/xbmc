/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "system.h"
#include "interfaces/Builtins.h"
#include "ButtonTranslator.h"
#include "utils/URIUtils.h"
#include "settings/Settings.h"
#include "guilib/Key.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "FileItem.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "tinyXML/tinyxml.h"

using namespace std;
using namespace XFILE;

typedef struct
{
  const char* name;
  int action;
} ActionMapping;

static const ActionMapping actions[] =
       {{"left"              , ACTION_MOVE_LEFT },
        {"right"             , ACTION_MOVE_RIGHT},
        {"up"                , ACTION_MOVE_UP   },
        {"down"              , ACTION_MOVE_DOWN },
        {"pageup"            , ACTION_PAGE_UP   },
        {"pagedown"          , ACTION_PAGE_DOWN},
        {"select"            , ACTION_SELECT_ITEM},
        {"highlight"         , ACTION_HIGHLIGHT_ITEM},
        {"parentdir"         , ACTION_PARENT_DIR},
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
        {"rotate"            , ACTION_ROTATE_PICTURE},
        {"close"             , ACTION_CLOSE_DIALOG},
        {"subtitledelayminus", ACTION_SUBTITLE_DELAY_MIN},
        {"subtitledelay"     , ACTION_SUBTITLE_DELAY},
        {"subtitledelayplus" , ACTION_SUBTITLE_DELAY_PLUS},
        {"audiodelayminus"   , ACTION_AUDIO_DELAY_MIN},
        {"audiodelay"        , ACTION_AUDIO_DELAY},
        {"audiodelayplus"    , ACTION_AUDIO_DELAY_PLUS},
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
        {"decreasepar"       , ACTION_DECREASE_PAR}};

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
        {"networksettings"          , WINDOW_SETTINGS_NETWORK},
        {"appearancesettings"       , WINDOW_SETTINGS_APPEARANCE},
        {"scripts"                  , WINDOW_PROGRAMS}, // backward compat
        {"videofiles"               , WINDOW_VIDEO_FILES},
        {"videolibrary"             , WINDOW_VIDEO_NAV},
        {"videoplaylist"            , WINDOW_VIDEO_PLAYLIST},
        {"loginscreen"              , WINDOW_LOGIN_SCREEN},
        {"profiles"                 , WINDOW_SETTINGS_PROFILES},
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
        {"musicscan"                , WINDOW_DIALOG_MUSIC_SCAN},
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
        {"videoscan"                , WINDOW_DIALOG_VIDEO_SCAN},
        {"favourites"               , WINDOW_DIALOG_FAVOURITES},
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
        {"startup"                  , WINDOW_STARTUP_ANIM}};


CButtonTranslator& CButtonTranslator::GetInstance()
{
  static CButtonTranslator sl_instance;
  return sl_instance;
}

CButtonTranslator::CButtonTranslator()
{}

CButtonTranslator::~CButtonTranslator()
{}

bool CButtonTranslator::Load()
{
  translatorMap.clear();

  //directories to search for keymaps
  //they're applied in this order,
  //so keymaps in profile/keymaps/
  //override e.g. system/keymaps
  static const char* DIRS_TO_CHECK[] = {
    "special://xbmc/system/keymaps/",
    "special://masterprofile/keymaps/",
    "special://profile/keymaps/"
  };
  bool success = false;

  for(unsigned int dirIndex = 0; dirIndex < sizeof(DIRS_TO_CHECK)/sizeof(DIRS_TO_CHECK[0]); ++dirIndex) {
    if( XFILE::CDirectory::Exists(DIRS_TO_CHECK[dirIndex]) )
    {
      CFileItemList files;
      XFILE::CDirectory::GetDirectory(DIRS_TO_CHECK[dirIndex], files, "*.xml");
      //sort the list for filesystem based prioties, e.g. 01-keymap.xml, 02-keymap-overrides.xml
      files.Sort(SORT_METHOD_FILE, SORT_ORDER_ASC);
      for(int fileIndex = 0; fileIndex<files.Size(); ++fileIndex)
        success |= LoadKeymap(files[fileIndex]->m_strPath);
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
  return true;
}

bool CButtonTranslator::LoadKeymap(const CStdString &keymapPath)
{
  TiXmlDocument xmlDoc;

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
    if (pWindow->Type() == TiXmlNode::ELEMENT)
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
  TiXmlDocument xmlDoc;

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
    if (pRemote->Type() == TiXmlNode::ELEMENT)
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
  lircButtonMap buttons;
  vector<string> RemoteNames;
  map<CStdString, lircButtonMap>::iterator it = lircRemotesMap.find(szDevice);
  if (it != lircRemotesMap.end())
  {
    buttons = it->second;
    lircRemotesMap.erase(it);
  }

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

  lircRemotesMap[szDevice] = buttons;
  vector<string>::iterator itr = RemoteNames.begin();
  while (itr!=RemoteNames.end())
  {
    it = lircRemotesMap.find(itr->c_str());
    if (it != lircRemotesMap.end())
    {
      buttons = it->second;
      lircRemotesMap.erase(it);
    }
    CLog::Log(LOGINFO, "* Linking remote mapping for '%s' to '%s'", szDevice, itr->c_str());
    lircRemotesMap[itr->c_str()] = buttons;
    itr++;
  }
  RemoteNames.clear();
}

int CButtonTranslator::TranslateLircRemoteString(const char* szDevice, const char *szButton)
{
  // Find the device
  map<CStdString, lircButtonMap>::iterator it = lircRemotesMap.find(szDevice);
  if (it == lircRemotesMap.end())
    return 0;

  // Find the button
  lircButtonMap::iterator it2 = (*it).second.find(szButton);
  if (it2 == (*it).second.end())
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

int CButtonTranslator::GetActionCode(int window, const CKey &key, CStdString &strAction)
{
  uint32_t code = key.GetButtonCode();
  map<int, buttonMap>::iterator it = translatorMap.find(window);
  if (it == translatorMap.end())
    return 0;
  buttonMap::iterator it2 = (*it).second.find(code);
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
    buttonMap::iterator it2 = (*it).second.find(code);
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
  buttonMap map;
  std::map<int, buttonMap>::iterator it = translatorMap.find(windowID);
  if (it != translatorMap.end())
  {
    map = it->second;
    translatorMap.erase(it);
  }
  TiXmlNode* pDevice;

  const char* types[] = {"gamepad", "remote", "keyboard", "universalremote", NULL};
  for (int i = 0; types[i]; ++i)
  {
    CStdString type(types[i]);
    if (HasDeviceType(pWindow, type))
    {
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

        if (buttonCode && pButton->FirstChild())
          MapAction(buttonCode, pButton->FirstChild()->Value(), map);
        pButton = pButton->NextSiblingElement();
      }
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
  // add our map to our table
  if (map.size() > 0)
    translatorMap.insert(pair<int, buttonMap>( windowID, map));
}

bool CButtonTranslator::TranslateActionString(const char *szAction, int &action)
{
  action = ACTION_NONE;
  CStdString strAction = szAction;
  strAction.ToLower();
  if (CBuiltins::HasCommand(strAction)) 
    action = ACTION_BUILT_IN_FUNCTION;

  if (strAction.Equals("noop"))
    return true;

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
  else if (strButton.Equals("guide")) buttonCode = XINPUT_IR_REMOTE_TITLE;   // same as title
  else if (strButton.Equals("livetv")) buttonCode = XINPUT_IR_REMOTE_LIVE_TV;
  else if (strButton.Equals("star")) buttonCode = XINPUT_IR_REMOTE_STAR;
  else if (strButton.Equals("hash")) buttonCode = XINPUT_IR_REMOTE_HASH;
  else if (strButton.Equals("clear")) buttonCode = XINPUT_IR_REMOTE_CLEAR;
  else if (strButton.Equals("enter")) buttonCode = XINPUT_IR_REMOTE_ENTER;
  else if (strButton.Equals("xbox")) buttonCode = XINPUT_IR_REMOTE_DISPLAY; // same as display
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
  if (strlen(szButton) == 1)
  { // single character
    buttonCode = (uint32_t)toupper(szButton[0]) | KEY_VKEY;
    // FIXME It is a printable character, printable should be ASCII not VKEY! Till now it works, but how (long)?
    // FIXME support unicode: additional parameter necessary since unicode can not be embedded into key/action-ID.
  }
  else
  { // for keys such as return etc. etc.
    CStdString strKey = szButton;
    strKey.ToLower();

    if (strKey.Equals("return")) buttonCode = 0xF00D;
    else if (strKey.Equals("enter")) buttonCode = 0xF06C;
    else if (strKey.Equals("escape")) buttonCode = 0xF01B;
    else if (strKey.Equals("esc")) buttonCode = 0xF01B;
    else if (strKey.Equals("tab")) buttonCode = 0xF009;
    else if (strKey.Equals("space")) buttonCode = 0xF020;
    else if (strKey.Equals("left")) buttonCode = 0xF025;
    else if (strKey.Equals("right")) buttonCode = 0xF027;
    else if (strKey.Equals("up")) buttonCode = 0xF026;
    else if (strKey.Equals("down")) buttonCode = 0xF028;
    else if (strKey.Equals("insert")) buttonCode = 0xF02D;
    else if (strKey.Equals("delete")) buttonCode = 0xF02E;
    else if (strKey.Equals("home")) buttonCode = 0xF024;
    else if (strKey.Equals("end")) buttonCode = 0xF023;
    else if (strKey.Equals("f1")) buttonCode = 0xF070;
    else if (strKey.Equals("f2")) buttonCode = 0xF071;
    else if (strKey.Equals("f3")) buttonCode = 0xF072;
    else if (strKey.Equals("f4")) buttonCode = 0xF073;
    else if (strKey.Equals("f5")) buttonCode = 0xF074;
    else if (strKey.Equals("f6")) buttonCode = 0xF075;
    else if (strKey.Equals("f7")) buttonCode = 0xF076;
    else if (strKey.Equals("f8")) buttonCode = 0xF077;
    else if (strKey.Equals("f9")) buttonCode = 0xF078;
    else if (strKey.Equals("f10")) buttonCode = 0xF079;
    else if (strKey.Equals("f11")) buttonCode = 0xF07A;
    else if (strKey.Equals("f12")) buttonCode = 0xF07B;
    else if (strKey.Equals("numpadzero") || strKey.Equals("zero")) buttonCode = 0xF060;
    else if (strKey.Equals("numpadone") || strKey.Equals("one")) buttonCode = 0xF061;
    else if (strKey.Equals("numpadtwo") || strKey.Equals("two")) buttonCode = 0xF062;
    else if (strKey.Equals("numpadthree") || strKey.Equals("three")) buttonCode = 0xF063;
    else if (strKey.Equals("numpadfour") || strKey.Equals("four")) buttonCode = 0xF064;
    else if (strKey.Equals("numpadfive") || strKey.Equals("five")) buttonCode = 0xF065;
    else if (strKey.Equals("numpadsix") || strKey.Equals("six")) buttonCode = 0xF066;
    else if (strKey.Equals("numpadseven") || strKey.Equals("seven")) buttonCode = 0xF067;
    else if (strKey.Equals("numpadeight") || strKey.Equals("eight")) buttonCode = 0xF068;
    else if (strKey.Equals("numpadnine") || strKey.Equals("nine")) buttonCode = 0xF069;
    else if (strKey.Equals("numpadtimes")) buttonCode = 0xF06A;
    else if (strKey.Equals("numpadplus")) buttonCode = 0xF06B;
    else if (strKey.Equals("numpadminus")) buttonCode = 0xF06D;
    else if (strKey.Equals("numpadperiod")) buttonCode = 0xF06E;
    else if (strKey.Equals("numpaddivide")) buttonCode = 0xF06F;
    else if (strKey.Equals("pageup")) buttonCode = 0xF021;
    else if (strKey.Equals("pagedown")) buttonCode = 0xF022;
    else if (strKey.Equals("printscreen")) buttonCode = 0xF02A;
    else if (strKey.Equals("backspace")) buttonCode = 0xF008;
    else if (strKey.Equals("menu")) buttonCode = 0xF05D;
    else if (strKey.Equals("pause")) buttonCode = 0xF013;
    else if (strKey.Equals("leftshift")) buttonCode = 0xF0A0;
    else if (strKey.Equals("rightshift")) buttonCode = 0xF0A1;
    else if (strKey.Equals("leftctrl")) buttonCode = 0xF0A2;
    else if (strKey.Equals("rightctrl")) buttonCode = 0xF0A3;
    else if (strKey.Equals("leftalt")) buttonCode = 0xF0A4;
    else if (strKey.Equals("rightalt")) buttonCode = 0xF0A5;
    else if (strKey.Equals("leftwindows")) buttonCode = 0xF05B;
    else if (strKey.Equals("rightwindows")) buttonCode = 0xF05C;
    else if (strKey.Equals("capslock")) buttonCode = 0xF020;
    else if (strKey.Equals("numlock")) buttonCode = 0xF090;
    else if (strKey.Equals("scrolllock")) buttonCode = 0xF091;
    else if (strKey.Equals("semicolon") || strKey.Equals("colon")) buttonCode = 0xF0BA;
    else if (strKey.Equals("equals") || strKey.Equals("plus")) buttonCode = 0xF0BB;
    else if (strKey.Equals("comma") || strKey.Equals("lessthan")) buttonCode = 0xF0BC;
    else if (strKey.Equals("minus") || strKey.Equals("underline")) buttonCode = 0xF0BD;
    else if (strKey.Equals("period") || strKey.Equals("greaterthan")) buttonCode = 0xF0BE;
    else if (strKey.Equals("forwardslash") || strKey.Equals("questionmark")) buttonCode = 0xF0BF;
    else if (strKey.Equals("leftquote") || strKey.Equals("tilde")) buttonCode = 0xF0C0;
    else if (strKey.Equals("opensquarebracket") || strKey.Equals("openbrace")) buttonCode = 0xF0EB;
    else if (strKey.Equals("backslash") || strKey.Equals("pipe")) buttonCode = 0xF0EC;
    else if (strKey.Equals("closesquarebracket") || strKey.Equals("closebrace")) buttonCode = 0xF0ED;
    else if (strKey.Equals("quote") || strKey.Equals("doublequote")) buttonCode = 0xF0EE;
    else if (strKey.Equals("launch_mail")) buttonCode = 0xF0B4;
    else if (strKey.Equals("browser_home")) buttonCode = 0xF0AC;
    else if (strKey.Equals("browser_favorites")) buttonCode = 0xF0AB;
    else if (strKey.Equals("browser_refresh")) buttonCode = 0xF0A8;
    else if (strKey.Equals("browser_search")) buttonCode = 0xF0AA;
    else if (strKey.Equals("browser_forward")) buttonCode = 0xF0A7;
    else if (strKey.Equals("launch_app1_pc_icon")) buttonCode = 0xF0B6;
    else if (strKey.Equals("launch_media_select")) buttonCode = 0xF0B5;
    else if (strKey.Equals("launch_file_browser")) buttonCode = 0xF0B8;
    else if (strKey.Equals("launch_media_center")) buttonCode = 0xF0B9;
    else if (strKey.Equals("play_pause")) buttonCode = 0xF0B3;
    else if (strKey.Equals("stop")) buttonCode = 0xF0B2;
    else if (strKey.Equals("volume_up")) buttonCode = 0xF0AF;
    else if (strKey.Equals("volume_mute")) buttonCode = 0xF0AD;
    else if (strKey.Equals("volume_down")) buttonCode = 0xF0AE;
    else if (strKey.Equals("prev_track")) buttonCode = 0xF0B1;
    else if (strKey.Equals("next_track")) buttonCode = 0xF0B0;
    else
      CLog::Log(LOGERROR, "Keyboard Translator: Can't find button %s", strKey.c_str());
  }
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

void CButtonTranslator::Clear()
{
  translatorMap.clear();
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  lircRemotesMap.clear();
#endif

#if defined(HAS_SDL_JOYSTICK) || defined(HAS_EVENT_SERVER)
  m_joystickButtonMap.clear();
  m_joystickAxisMap.clear();
  m_joystickHatMap.clear();
#endif
}
