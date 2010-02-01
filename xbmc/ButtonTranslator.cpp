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
#include "utils/Builtins.h"
#include "ButtonTranslator.h"
#include "Util.h"
#include "Settings.h"
#include "SkinInfo.h"
#include "Key.h"
#include "File.h"
#include "Directory.h"
#include "FileItem.h"
#include "StringUtils.h"
#include "utils/log.h"
#include "tinyXML/tinyxml.h"

using namespace std;
using namespace XFILE;

extern CStdString g_LoadErrorStr;

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
    g_LoadErrorStr.Format("Error loading keymaps from: %s or %s or %s", DIRS_TO_CHECK[0], DIRS_TO_CHECK[1], DIRS_TO_CHECK[2]);
    return false;
  }

#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
#ifdef _LINUX
#define REMOTEMAP "Lircmap.xml"
#else
#define REMOTEMAP "IRSSmap.xml"
#endif
  CStdString lircmapPath;
  CUtil::AddFileToFolder("special://xbmc/system/", REMOTEMAP, lircmapPath);
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
  {
    CLog::Log(LOGERROR, "CButtonTranslator::Load - unable to load remote map %s", REMOTEMAP);
    // don't return false - it is to only indicate a fatal error (which this is not)
  }
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
          windowID = TranslateWindowString(szWindow);
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
    g_LoadErrorStr.Format("%s, Line %d\n%s", lircmapPath.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false; // This is so people who don't have the file won't fail, just warn
  }

  TiXmlElement* pRoot = xmlDoc.RootElement();
  CStdString strValue = pRoot->Value();
  if (strValue != REMOTEMAPTAG)
  {
    g_LoadErrorStr.Format("%sl Doesn't contain <%s>", lircmapPath.c_str(), REMOTEMAPTAG);
    return false;
  }

  // run through our window groups
  TiXmlNode* pRemote = pRoot->FirstChild();
  while (pRemote)
  {
    const char *szRemote = pRemote->Value();
    if (szRemote)
    {
      TiXmlAttribute* pAttr = pRemote->ToElement()->FirstAttribute();
      const char* szDeviceName = pAttr->Value();
      MapRemote(pRemote, szDeviceName);
    }
    pRemote = pRemote->NextSibling();
  }

  return true;
}

void CButtonTranslator::MapRemote(TiXmlNode *pRemote, const char* szDevice)
{
  lircButtonMap buttons;
  map<CStdString, lircButtonMap>::iterator it = lircRemotesMap.find(szDevice);
  if (it != lircRemotesMap.end())
  {
    buttons = it->second;
    lircRemotesMap.erase(it);
  }

  TiXmlElement *pButton = pRemote->FirstChildElement();
  while (pButton)
  {
    if (pButton->FirstChild() && pButton->FirstChild()->Value())
      buttons[pButton->FirstChild()->Value()] = pButton->Value();
    pButton = pButton->NextSiblingElement();
  }

  lircRemotesMap[szDevice] = buttons;
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
  {
    joyname = pJoy->Attribute("name");
  }
  else
  {
    CLog::Log(LOGNOTICE, "No Joystick name specified, loading default map");
  }

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
            {
              axisMap[-id] = string(szAction);
            }
            else if (limit==1)
            {
              axisMap[id] = string(szAction);
            }
            else if (limit==0)
            {
              axisMap[id|0xFFFF0000] = string(szAction);
            }
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
            if (position.compare("up")==0)
            {
              hatMap[(SDL_HAT_UP<<16)|hatID] = string(szAction);
            }
            else if (position.compare("down")==0)
            {
              hatMap[(SDL_HAT_DOWN<<16)|hatID] = string(szAction);
            }
            else if (position.compare("right")==0)
            {
              hatMap[(SDL_HAT_RIGHT<<16)|hatID] = string(szAction);
            }
            else if (position.compare("left")==0)
            {
              hatMap[(SDL_HAT_LEFT<<16)|hatID] = string(szAction);
            }
            else
            {
              CLog::Log(LOGERROR, "Error in joystick map, invalid position specified %s for axis %d", position.c_str(), id);
            }
          }
        }
        else
        {
          CLog::Log(LOGERROR, "Error reading joystick map element, unknown button type: %s", szType);
        }
      }
      else if (strcmpi(szType, "altname")==0)
      {
        joynames.push_back(string(szAction));
      }
      else
      {
        CLog::Log(LOGERROR, "Error reading joystick map element, Invalid id: %d", id);
      }
    }
    else
    {
      CLog::Log(LOGERROR, "Error reading joystick map element, skipping");
    }
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

  return;
}

bool CButtonTranslator::TranslateJoystickString(int window, const char* szDevice, int id, short inputType, int& action, CStdString& strAction, bool &fullrange)
{
  bool found = false;

  map<string, JoystickMap>::iterator it;
  map<string, JoystickMap> *jmap;

  fullrange = false;
  if (inputType == JACTIVE_AXIS)
  {
    jmap = &m_joystickAxisMap;
  }
  else if (inputType == JACTIVE_BUTTON)
  {
    jmap = &m_joystickButtonMap;
  }
  else if (inputType == JACTIVE_HAT)
  {
  	jmap = &m_joystickHatMap;
  }
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
  {
    return TranslateActionString(strAction.c_str(), action);
  }

  return false;
}
#endif

void CButtonTranslator::GetAction(int window, const CKey &key, CAction &action, bool fallback)
{
  CStdString strAction;
  // try to get the action from the current window
  int actionID = GetActionCode(window, key, strAction);
  // if it's invalid, try to get it from the global map
  if (actionID == 0 && fallback)
    actionID = GetActionCode( -1, key, strAction);
  // Now fill our action structure
  action.actionId = actionID;
  action.strAction = strAction;
  action.amount1 = 1; // digital button (could change this for repeat acceleration)
  action.amount2 = 0;
  action.repeat = key.GetRepeat();
  action.buttonCode = key.GetButtonCode();
  action.holdTime = key.GetHeld();
  // get the action amounts of the analog buttons
  if (key.GetButtonCode() == KEY_BUTTON_LEFT_ANALOG_TRIGGER)
  {
    action.amount1 = (float)key.GetLeftTrigger() / 255.0f;
  }
  else if (key.GetButtonCode() == KEY_BUTTON_RIGHT_ANALOG_TRIGGER)
  {
    action.amount1 = (float)key.GetRightTrigger() / 255.0f;
  }
  else if (key.GetButtonCode() == KEY_BUTTON_LEFT_THUMB_STICK)
  {
    action.amount1 = key.GetLeftThumbX();
    action.amount2 = key.GetLeftThumbY();
  }
  else if (key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK)
  {
    action.amount1 = key.GetRightThumbX();
    action.amount2 = key.GetRightThumbY();
  }
  else if (key.GetButtonCode() == KEY_BUTTON_LEFT_THUMB_STICK_UP)
    action.amount1 = key.GetLeftThumbY();
  else if (key.GetButtonCode() == KEY_BUTTON_LEFT_THUMB_STICK_DOWN)
    action.amount1 = -key.GetLeftThumbY();
  else if (key.GetButtonCode() == KEY_BUTTON_LEFT_THUMB_STICK_LEFT)
    action.amount1 = -key.GetLeftThumbX();
  else if (key.GetButtonCode() == KEY_BUTTON_LEFT_THUMB_STICK_RIGHT)
    action.amount1 = key.GetLeftThumbX();
  else if (key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK_UP)
    action.amount1 = key.GetRightThumbY();
  else if (key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK_DOWN)
    action.amount1 = -key.GetRightThumbY();
  else if (key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK_LEFT)
    action.amount1 = -key.GetRightThumbX();
  else if (key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT)
    action.amount1 = key.GetRightThumbX();
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

void CButtonTranslator::MapWindowActions(TiXmlNode *pWindow, int windowID)
{
  if (!pWindow || windowID == WINDOW_INVALID) return;
  buttonMap map;
  std::map<int, buttonMap>::iterator it = translatorMap.find(windowID);
  if (it != translatorMap.end())
  {
    map = it->second;
    translatorMap.erase(it);
  }
  TiXmlNode* pDevice;
  if ((pDevice = pWindow->FirstChild("gamepad")) != NULL)
  { // map gamepad actions
    TiXmlElement *pButton = pDevice->FirstChildElement();
    while (pButton)
    {
      uint32_t buttonCode = TranslateGamepadString(pButton->Value());
      if (pButton->FirstChild())
        MapAction(buttonCode, pButton->FirstChild()->Value(), map);
      pButton = pButton->NextSiblingElement();
    }
  }
  if ((pDevice = pWindow->FirstChild("remote")) != NULL)
  { // map remote actions
    TiXmlElement *pButton = pDevice->FirstChildElement();
    while (pButton)
    {
      uint32_t buttonCode = TranslateRemoteString(pButton->Value());
      if (pButton->FirstChild())
        MapAction(buttonCode, pButton->FirstChild()->Value(), map);
      pButton = pButton->NextSiblingElement();
    }
  }
  if ((pDevice = pWindow->FirstChild("universalremote")) != NULL)
  { // map universal remote actions
    TiXmlElement *pButton = pDevice->FirstChildElement();
    while (pButton)
    {
      uint32_t buttonCode = TranslateUniversalRemoteString(pButton->Value());
      if (pButton->FirstChild())
        MapAction(buttonCode, pButton->FirstChild()->Value(), map);
      pButton = pButton->NextSiblingElement();
    }
  }
  if ((pDevice = pWindow->FirstChild("keyboard")) != NULL)
  { // map keyboard actions
    TiXmlElement *pButton = pDevice->FirstChildElement();
    while (pButton)
    {
      uint32_t buttonCode = TranslateKeyboardButton(pButton);
      if (pButton->FirstChild())
        MapAction(buttonCode, pButton->FirstChild()->Value(), map);
      pButton = pButton->NextSiblingElement();
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
  if (CBuiltins::HasCommand(strAction)) action = ACTION_BUILT_IN_FUNCTION;

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

int CButtonTranslator::TranslateWindowString(const char *szWindow)
{
  int windowID = WINDOW_INVALID;
  CStdString strWindow = szWindow;
  if (strWindow.IsEmpty()) return windowID;
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
      windowID = iWindow;
    else
      windowID = WINDOW_HOME + iWindow;
  }
  else if (strWindow.Equals("home")) windowID = WINDOW_HOME;
  else if (strWindow.Equals("programs")) windowID = WINDOW_PROGRAMS;
  else if (strWindow.Equals("pictures")) windowID = WINDOW_PICTURES;
  else if (strWindow.Equals("files") || strWindow.Equals("filemanager")) windowID = WINDOW_FILES;
  else if (strWindow.Equals("settings")) windowID = WINDOW_SETTINGS_MENU;
  else if (strWindow.Equals("music")) windowID = WINDOW_MUSIC;
  else if (strWindow.Equals("musicfiles")) windowID = WINDOW_MUSIC_FILES;
  else if (strWindow.Equals("musiclibrary")) windowID = WINDOW_MUSIC_NAV;
  else if (strWindow.Equals("musicplaylist")) windowID = WINDOW_MUSIC_PLAYLIST;
  else if (strWindow.Equals("musicplaylisteditor")) windowID = WINDOW_MUSIC_PLAYLIST_EDITOR;
  else if (strWindow.Equals("musicinformation")) windowID = WINDOW_MUSIC_INFO;
  else if (strWindow.Equals("video") || strWindow.Equals("videos")) windowID = WINDOW_VIDEOS;
  else if (strWindow.Equals("videofiles")) windowID = WINDOW_VIDEO_FILES;
  else if (strWindow.Equals("videolibrary")) windowID = WINDOW_VIDEO_NAV;
  else if (strWindow.Equals("videoplaylist")) windowID = WINDOW_VIDEO_PLAYLIST;
  else if (strWindow.Equals("systeminfo")) windowID = WINDOW_SYSTEM_INFORMATION;
  else if (strWindow.Equals("teletext")) windowID = WINDOW_DIALOG_OSD_TELETEXT;
  else if (strWindow.Equals("guicalibration")) windowID = WINDOW_SCREEN_CALIBRATION;
  else if (strWindow.Equals("screencalibration")) windowID = WINDOW_SCREEN_CALIBRATION;
  else if (strWindow.Equals("testpattern")) windowID = WINDOW_TEST_PATTERN;
  else if (strWindow.Equals("picturessettings")) windowID = WINDOW_SETTINGS_MYPICTURES;
  else if (strWindow.Equals("programssettings")) windowID = WINDOW_SETTINGS_MYPROGRAMS;
  else if (strWindow.Equals("weathersettings")) windowID = WINDOW_SETTINGS_MYWEATHER;
  else if (strWindow.Equals("musicsettings")) windowID = WINDOW_SETTINGS_MYMUSIC;
  else if (strWindow.Equals("systemsettings")) windowID = WINDOW_SETTINGS_SYSTEM;
  else if (strWindow.Equals("videossettings")) windowID = WINDOW_SETTINGS_MYVIDEOS;
  else if (strWindow.Equals("networksettings")) windowID = WINDOW_SETTINGS_NETWORK;
  else if (strWindow.Equals("appearancesettings")) windowID = WINDOW_SETTINGS_APPEARANCE;
  else if (strWindow.Equals("scripts")) windowID = WINDOW_SCRIPTS;
  else if (strWindow.Equals("profiles")) windowID = WINDOW_SETTINGS_PROFILES;
  else if (strWindow.Equals("yesnodialog")) windowID = WINDOW_DIALOG_YES_NO;
  else if (strWindow.Equals("progressdialog")) windowID = WINDOW_DIALOG_PROGRESS;
  else if (strWindow.Equals("virtualkeyboard")) windowID = WINDOW_DIALOG_KEYBOARD;
  else if (strWindow.Equals("volumebar")) windowID = WINDOW_DIALOG_VOLUME_BAR;
  else if (strWindow.Equals("submenu")) windowID = WINDOW_DIALOG_SUB_MENU;
  else if (strWindow.Equals("favourites")) windowID = WINDOW_DIALOG_FAVOURITES;
  else if (strWindow.Equals("contextmenu")) windowID = WINDOW_DIALOG_CONTEXT_MENU;
  else if (strWindow.Equals("infodialog")) windowID = WINDOW_DIALOG_KAI_TOAST;
  else if (strWindow.Equals("numericinput")) windowID = WINDOW_DIALOG_NUMERIC;
  else if (strWindow.Equals("gamepadinput")) windowID = WINDOW_DIALOG_GAMEPAD;
  else if (strWindow.Equals("shutdownmenu")) windowID = WINDOW_DIALOG_BUTTON_MENU;
  else if (strWindow.Equals("scandialog")) windowID = WINDOW_DIALOG_MUSIC_SCAN;
  else if (strWindow.Equals("mutebug")) windowID = WINDOW_DIALOG_MUTE_BUG;
  else if (strWindow.Equals("playercontrols")) windowID = WINDOW_DIALOG_PLAYER_CONTROLS;
  else if (strWindow.Equals("seekbar")) windowID = WINDOW_DIALOG_SEEK_BAR;
  else if (strWindow.Equals("musicosd")) windowID = WINDOW_DIALOG_MUSIC_OSD;
  else if (strWindow.Equals("visualisationsettings")) windowID = WINDOW_DIALOG_VIS_SETTINGS;
  else if (strWindow.Equals("visualisationpresetlist")) windowID = WINDOW_DIALOG_VIS_PRESET_LIST;
  else if (strWindow.Equals("osdvideosettings")) windowID = WINDOW_DIALOG_VIDEO_OSD_SETTINGS;
  else if (strWindow.Equals("osdaudiosettings")) windowID = WINDOW_DIALOG_AUDIO_OSD_SETTINGS;
  else if (strWindow.Equals("videobookmarks")) windowID = WINDOW_DIALOG_VIDEO_BOOKMARKS;
  else if (strWindow.Equals("profilesettings")) windowID = WINDOW_DIALOG_PROFILE_SETTINGS;
  else if (strWindow.Equals("locksettings")) windowID = WINDOW_DIALOG_LOCK_SETTINGS;
  else if (strWindow.Equals("contentsettings")) windowID = WINDOW_DIALOG_CONTENT_SETTINGS;
  else if (strWindow.Equals("networksetup")) windowID = WINDOW_DIALOG_NETWORK_SETUP;
  else if (strWindow.Equals("mediasource")) windowID = WINDOW_DIALOG_MEDIA_SOURCE;
  else if (strWindow.Equals("smartplaylisteditor")) windowID = WINDOW_DIALOG_SMART_PLAYLIST_EDITOR;
  else if (strWindow.Equals("smartplaylistrule")) windowID = WINDOW_DIALOG_SMART_PLAYLIST_RULE;
  else if (strWindow.Equals("selectdialog")) windowID = WINDOW_DIALOG_SELECT;
  else if (strWindow.Equals("okdialog")) windowID = WINDOW_DIALOG_OK;
  else if (strWindow.Equals("movieinformation")) windowID = WINDOW_VIDEO_INFO;
  else if (strWindow.Equals("scriptsdebuginfo")) windowID = WINDOW_SCRIPTS_INFO;
  else if (strWindow.Equals("fullscreenvideo")) windowID = WINDOW_FULLSCREEN_VIDEO;
  else if (strWindow.Equals("visualisation")) windowID = WINDOW_VISUALISATION;
  else if (strWindow.Equals("slideshow")) windowID = WINDOW_SLIDESHOW;
  else if (strWindow.Equals("filestackingdialog")) windowID = WINDOW_DIALOG_FILESTACKING;
  else if (strWindow.Equals("weather")) windowID = WINDOW_WEATHER;
  else if (strWindow.Equals("screensaver")) windowID = WINDOW_SCREENSAVER;
  else if (strWindow.Equals("videoosd")) windowID = WINDOW_OSD;
  else if (strWindow.Equals("videomenu")) windowID = WINDOW_VIDEO_MENU;
  else if (strWindow.Equals("filebrowser")) windowID = WINDOW_DIALOG_FILE_BROWSER;
  else if (strWindow.Equals("startup")) windowID = WINDOW_STARTUP;
  else if (strWindow.Equals("startwindow")) windowID = g_SkinInfo.GetStartWindow();
  else if (strWindow.Equals("loginscreen")) windowID = WINDOW_LOGIN_SCREEN;
  else if (strWindow.Equals("musicoverlay")) windowID = WINDOW_MUSIC_OVERLAY;
  else if (strWindow.Equals("videooverlay")) windowID = WINDOW_VIDEO_OVERLAY;
  else if (strWindow.Equals("pictureinfo")) windowID = WINDOW_DIALOG_PICTURE_INFO;
  else if (strWindow.Equals("pluginsettings")) windowID = WINDOW_DIALOG_PLUGIN_SETTINGS;
  else if (strWindow.Equals("fullscreeninfo")) windowID = WINDOW_DIALOG_FULLSCREEN_INFO;
  else if (strWindow.Equals("karaokeselector")) windowID = WINDOW_DIALOG_KARAOKE_SONGSELECT;
  else if (strWindow.Equals("karaokelargeselector")) windowID = WINDOW_DIALOG_KARAOKE_SELECTOR;
  else if (strWindow.Equals("sliderdialog")) windowID = WINDOW_DIALOG_SLIDER;
  else if (strWindow.Equals("songinformation")) windowID = WINDOW_DIALOG_SONG_INFO;
  else
    CLog::Log(LOGERROR, "Window Translator: Can't find window %s", strWindow.c_str());

  //CLog::Log(LOGDEBUG,"CButtonTranslator::TranslateWindowString(%s) returned Window ID (%i)", szWindow, windowID);
  return windowID;
}

uint32_t CButtonTranslator::TranslateGamepadString(const char *szButton)
{
  if (!szButton) return 0;
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
  if (!szButton) return 0;
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
  else if (strButton.Equals("enter")) buttonCode = XINPUT_IR_REMOTE_SELECT;  // same as select
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
  if (!szButton || strlen(szButton) < 4 || strnicmp(szButton, "obc", 3)) return 0;
  const char *szCode = szButton + 3;
  // Button Code is 255 - OBC (Original Button Code) of the button
  uint32_t buttonCode = 255 - atol(szCode);
  if (buttonCode > 255) buttonCode = 0;
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
    else if (strKey.Equals("launch_app1_pc_icon")) buttonCode = 0xF0B6;
    else if (strKey.Equals("launch_media_select")) buttonCode = 0xF0B5;
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

  if (!szButton) return 0;
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
  {
    button_id = TranslateKeyboardString(szButton);
  }

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

