#include "stdafx.h"
#include "ButtonTranslator.h"
#include "util.h"

CButtonTranslator g_buttonTranslator;
extern CStdString g_LoadErrorStr;

CButtonTranslator::CButtonTranslator()
{}

CButtonTranslator::~CButtonTranslator()
{}

bool CButtonTranslator::Load()
{
  // load our xml file, and fill up our mapping tables
  TiXmlDocument xmlDoc;

  CLog::Log(LOGINFO, "Loading Q:\\keymap.xml");
  // Load the config file
  if (!xmlDoc.LoadFile("Q:\\keymap.xml"))
  {
    g_LoadErrorStr.Format("Q:\\keymap.xml, Line %d\n%s", xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement* pRoot = xmlDoc.RootElement();
  CStdString strValue = pRoot->Value();
  if ( strValue != "keymap")
  {
    g_LoadErrorStr.Format("Q:\\keymap.xml Doesn't contain <keymap>");
    return false;
  }
  // do the global actions
  TiXmlElement* pWindow = pRoot->FirstChildElement("global");
  if (pWindow)
  {
    buttonMap map;
    TiXmlNode* pAction = pWindow->FirstChild("action");
    //OutputDebugString("GLOBAL:\n");
    while (pAction)
    {
      WORD wID = 0;    // action identity
      CStdString strID;
      // check for an <execute> tag first...
      TiXmlNode* pExecute = pAction->FirstChild("execute");
      if (pExecute && pExecute->FirstChild())
      {
        strID = pExecute->FirstChild()->Value();
        wID = ACTION_BUILT_IN_FUNCTION;
      }
      else
      {
        TiXmlNode* pNode = pAction->FirstChild("id");
        if (pNode)
        {
          if (pNode->FirstChild())
          {
            strID = pNode->FirstChild()->Value();
            if (CUtil::IsBuiltIn(strID))
              wID = ACTION_BUILT_IN_FUNCTION;
            else
              wID = (WORD)atol(strID);
          }
        }
      }
      if (wID > 0)
      { // valid id, get the buttons associated with this action...
        MapAction(wID, strID, pAction->FirstChild(), map);
      }
      pAction = pAction->NextSibling();
    }
    // add our map to our table
    if (map.size() > 0)
      translatorMap.insert(pair<WORD, buttonMap>( -1, map));
  }
  // Now do the window specific mappings (if any)
  pWindow = pWindow->NextSiblingElement("window");
  while (pWindow)
  {
    buttonMap map;
    TiXmlNode* pID = pWindow->FirstChild("id");
    WORD wWindowID = WINDOW_INVALID;
    if (pID)
    {
      if (pID->FirstChild())
        wWindowID = WINDOW_HOME + (WORD)atol(pID->FirstChild()->Value());
    }
    if (wWindowID != WINDOW_INVALID)
    {
      TiXmlNode* pAction = pWindow->FirstChild("action");
      while (pAction)
      {
        CStdString strID;
        WORD wID = 0;    // action identity
        // check for an <execute> tag first...
        TiXmlNode* pExecute = pAction->FirstChild("execute");
        if (pExecute && pExecute->FirstChild())
        {
          strID = pExecute->FirstChild()->Value();
          wID = ACTION_BUILT_IN_FUNCTION;
        }
        else
        {
          TiXmlNode* pNode = pAction->FirstChild("id");
          if (pNode)
          {
            if (pNode->FirstChild())
            {
              strID = pNode->FirstChild()->Value();
              if (CUtil::IsBuiltIn(strID))
                wID = ACTION_BUILT_IN_FUNCTION;
              else
                wID = (WORD)atol(strID);
            }
          }
        }
        if (wID > 0)
        { // valid id, get the buttons associated with this action...
          MapAction(wID, strID, pAction->FirstChild(), map);
        }
        pAction = pAction->NextSibling();
      }
      // add our map to our table
      if (map.size() > 0)
        translatorMap.insert(pair<WORD, buttonMap>(wWindowID, map));
    }
    pWindow = pWindow->NextSiblingElement();
  }
  // Done!
  return true;
}

void CButtonTranslator::GetAction(WORD wWindow, const CKey &key, CAction &action)
{
  CStdString strAction;
  // try to get the action from the current window
  WORD wAction = GetActionCode(wWindow, key, strAction);
  // if it's invalid, try to get it from the global map
  if (wAction == 0)
    wAction = GetActionCode( -1, key, strAction);
  // Now fill our action structure
  action.wID = wAction;
  action.strAction = strAction;
  action.fAmount1 = 1; // digital button (could change this for repeat acceleration)
  action.fAmount2 = 0;
  action.m_dwButtonCode = key.GetButtonCode();
  // get the action amounts of the analog buttons
  if (key.GetButtonCode() == KEY_BUTTON_LEFT_ANALOG_TRIGGER)
  {
    action.fAmount1 = (float)key.GetLeftTrigger() / 255.0f;
  }
  else if (key.GetButtonCode() == KEY_BUTTON_RIGHT_ANALOG_TRIGGER)
  {
    action.fAmount1 = (float)key.GetRightTrigger() / 255.0f;
  }
  else if (key.GetButtonCode() == KEY_BUTTON_LEFT_THUMB_STICK)
  {
    action.fAmount1 = key.GetLeftThumbX();
    action.fAmount2 = key.GetLeftThumbY();
  }
  else if (key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK)
  {
    action.fAmount1 = key.GetRightThumbX();
    action.fAmount2 = key.GetRightThumbY();
  }
  else if (key.GetButtonCode() == KEY_BUTTON_LEFT_THUMB_STICK_UP || key.GetButtonCode() == KEY_BUTTON_LEFT_THUMB_STICK_DOWN)
  {
    action.fAmount1 = key.GetLeftThumbY();
  }
  else if (key.GetButtonCode() == KEY_BUTTON_LEFT_THUMB_STICK_LEFT || key.GetButtonCode() == KEY_BUTTON_LEFT_THUMB_STICK_RIGHT)
  {
    action.fAmount1 = key.GetLeftThumbX();
  }
  else if (key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK_UP || key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK_DOWN)
  {
    action.fAmount1 = key.GetRightThumbY();
  }
  else if (key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK_LEFT || key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT)
  {
    action.fAmount1 = key.GetRightThumbX();
  }
}

WORD CButtonTranslator::GetActionCode(WORD wWindow, const CKey &key, CStdString &strAction)
{
  WORD wKey = (WORD)key.GetButtonCode();
  map<WORD, buttonMap>::iterator it = translatorMap.find(wWindow);
  if (it == translatorMap.end())
    return 0;
  buttonMap::iterator it2 = (*it).second.find(wKey);
  WORD wAction = 0;
  while (it2 != (*it).second.end())
  {
    wAction = (*it2).second.wID;
    strAction = (*it2).second.strID;
    it2 = (*it).second.end();
  }
  return wAction;
}

void CButtonTranslator::MapAction(WORD wAction, const CStdString &strAction, TiXmlNode *pNode, buttonMap &map)
{
  CStdString strNode, strButton;
  WORD wButtonCode;
  while (pNode)
  {
    strNode = pNode->Value();
    wButtonCode = 0;
    if (strNode == "gamepad")
    {
      if (pNode->FirstChild())
        strButton = pNode->FirstChild()->Value();
      else strButton = "";
      if (strButton == "A") wButtonCode = KEY_BUTTON_A;
      else if (strButton == "B") wButtonCode = KEY_BUTTON_B;
      else if (strButton == "X") wButtonCode = KEY_BUTTON_X;
      else if (strButton == "Y") wButtonCode = KEY_BUTTON_Y;
      else if (strButton == "white") wButtonCode = KEY_BUTTON_WHITE;
      else if (strButton == "black") wButtonCode = KEY_BUTTON_BLACK;
      else if (strButton == "start") wButtonCode = KEY_BUTTON_START;
      else if (strButton == "back") wButtonCode = KEY_BUTTON_BACK;
      else if (strButton == "leftthumbbutton") wButtonCode = KEY_BUTTON_LEFT_THUMB_BUTTON;
      else if (strButton == "rightthumbbutton") wButtonCode = KEY_BUTTON_RIGHT_THUMB_BUTTON;
      else if (strButton == "leftthumbstick") wButtonCode = KEY_BUTTON_LEFT_THUMB_STICK;
      else if (strButton == "leftthumbstickup") wButtonCode = KEY_BUTTON_LEFT_THUMB_STICK_UP;
      else if (strButton == "leftthumbstickdown") wButtonCode = KEY_BUTTON_LEFT_THUMB_STICK_DOWN;
      else if (strButton == "leftthumbstickleft") wButtonCode = KEY_BUTTON_LEFT_THUMB_STICK_LEFT;
      else if (strButton == "leftthumbstickright") wButtonCode = KEY_BUTTON_LEFT_THUMB_STICK_RIGHT;
      else if (strButton == "rightthumbstick") wButtonCode = KEY_BUTTON_RIGHT_THUMB_STICK;
      else if (strButton == "rightthumbstickup") wButtonCode = KEY_BUTTON_RIGHT_THUMB_STICK_UP;
      else if (strButton == "rightthumbstickdown") wButtonCode = KEY_BUTTON_RIGHT_THUMB_STICK_DOWN;
      else if (strButton == "rightthumbstickleft") wButtonCode = KEY_BUTTON_RIGHT_THUMB_STICK_LEFT;
      else if (strButton == "rightthumbstickright") wButtonCode = KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT;
      else if (strButton == "lefttrigger") wButtonCode = KEY_BUTTON_LEFT_TRIGGER;
      else if (strButton == "righttrigger") wButtonCode = KEY_BUTTON_RIGHT_TRIGGER;
      else if (strButton == "leftanalogtrigger") wButtonCode = KEY_BUTTON_LEFT_ANALOG_TRIGGER;
      else if (strButton == "rightanalogtrigger") wButtonCode = KEY_BUTTON_RIGHT_ANALOG_TRIGGER;
      else if (strButton == "dpadleft") wButtonCode = KEY_BUTTON_DPAD_LEFT;
      else if (strButton == "dpadright") wButtonCode = KEY_BUTTON_DPAD_RIGHT;
      else if (strButton == "dpadup") wButtonCode = KEY_BUTTON_DPAD_UP;
      else if (strButton == "dpaddown") wButtonCode = KEY_BUTTON_DPAD_DOWN;
    }
    else if (strNode == "remote")
    {
      if (pNode->FirstChild())
        strButton = pNode->FirstChild()->Value();
      else
        strButton = "";
      if (strButton == "left") wButtonCode = XINPUT_IR_REMOTE_LEFT;
      else if (strButton == "right") wButtonCode = XINPUT_IR_REMOTE_RIGHT;
      else if (strButton == "up") wButtonCode = XINPUT_IR_REMOTE_UP;
      else if (strButton == "down") wButtonCode = XINPUT_IR_REMOTE_DOWN;
      else if (strButton == "select") wButtonCode = XINPUT_IR_REMOTE_SELECT;
      else if (strButton == "back") wButtonCode = XINPUT_IR_REMOTE_BACK;
      else if (strButton == "menu") wButtonCode = XINPUT_IR_REMOTE_MENU;
      else if (strButton == "info") wButtonCode = XINPUT_IR_REMOTE_INFO;
      else if (strButton == "display") wButtonCode = XINPUT_IR_REMOTE_DISPLAY;
      else if (strButton == "title") wButtonCode = XINPUT_IR_REMOTE_TITLE;
      else if (strButton == "play") wButtonCode = XINPUT_IR_REMOTE_PLAY;
      else if (strButton == "pause") wButtonCode = XINPUT_IR_REMOTE_PAUSE;
      else if (strButton == "reverse") wButtonCode = XINPUT_IR_REMOTE_REVERSE;
      else if (strButton == "forward") wButtonCode = XINPUT_IR_REMOTE_FORWARD;
      else if (strButton == "skipplus") wButtonCode = XINPUT_IR_REMOTE_SKIP_PLUS;
      else if (strButton == "skipminus") wButtonCode = XINPUT_IR_REMOTE_SKIP_MINUS;
      else if (strButton == "stop") wButtonCode = XINPUT_IR_REMOTE_STOP;
      else if (strButton == "0") wButtonCode = XINPUT_IR_REMOTE_0;
      else if (strButton == "1") wButtonCode = XINPUT_IR_REMOTE_1;
      else if (strButton == "2") wButtonCode = XINPUT_IR_REMOTE_2;
      else if (strButton == "3") wButtonCode = XINPUT_IR_REMOTE_3;
      else if (strButton == "4") wButtonCode = XINPUT_IR_REMOTE_4;
      else if (strButton == "5") wButtonCode = XINPUT_IR_REMOTE_5;
      else if (strButton == "6") wButtonCode = XINPUT_IR_REMOTE_6;
      else if (strButton == "7") wButtonCode = XINPUT_IR_REMOTE_7;
      else if (strButton == "8") wButtonCode = XINPUT_IR_REMOTE_8;
      else if (strButton == "9") wButtonCode = XINPUT_IR_REMOTE_9;
      // additional keys from the media center extender for xbox remote
      else if (strButton == "power") wButtonCode = XINPUT_IR_REMOTE_POWER;
      else if (strButton == "mytv") wButtonCode = XINPUT_IR_REMOTE_MY_TV;
      else if (strButton == "mymusic") wButtonCode = XINPUT_IR_REMOTE_MY_MUSIC;
      else if (strButton == "mypictures") wButtonCode = XINPUT_IR_REMOTE_MY_PICTURES;
      else if (strButton == "myvideo") wButtonCode = XINPUT_IR_REMOTE_MY_VIDEOS;
      else if (strButton == "record") wButtonCode = XINPUT_IR_REMOTE_RECORD;
      else if (strButton == "start") wButtonCode = XINPUT_IR_REMOTE_START;
      else if (strButton == "volumeplus") wButtonCode = XINPUT_IR_REMOTE_VOLUME_PLUS;
      else if (strButton == "volumeminus") wButtonCode = XINPUT_IR_REMOTE_VOLUME_MINUS;
      else if (strButton == "channelplus") wButtonCode = XINPUT_IR_REMOTE_CHANNEL_PLUS;
      else if (strButton == "channelminus") wButtonCode = XINPUT_IR_REMOTE_CHANNEL_MINUS;
      else if (strButton == "pageplus") wButtonCode = XINPUT_IR_REMOTE_CHANNEL_PLUS;
      else if (strButton == "pageminus") wButtonCode = XINPUT_IR_REMOTE_CHANNEL_MINUS;
      else if (strButton == "mute") wButtonCode = XINPUT_IR_REMOTE_MUTE;
      else if (strButton == "recorededtv") wButtonCode = XINPUT_IR_REMOTE_RECORDED_TV;
      else if (strButton == "guide") wButtonCode = XINPUT_IR_REMOTE_TITLE;   // same as title
      else if (strButton == "livetv") wButtonCode = XINPUT_IR_REMOTE_LIVE_TV;
      else if (strButton == "star") wButtonCode = XINPUT_IR_REMOTE_STAR;
      else if (strButton == "hash") wButtonCode = XINPUT_IR_REMOTE_HASH;
      else if (strButton == "clear") wButtonCode = XINPUT_IR_REMOTE_CLEAR;
      else if (strButton == "enter") wButtonCode = XINPUT_IR_REMOTE_SELECT;  // same as select
      else if (strButton == "xbox") wButtonCode = XINPUT_IR_REMOTE_DISPLAY; // same as display
    }
    else if (strNode == "remotecode")
    {
      // Button Code is 255 - OBC (Original Button Code) of the button
      if (pNode->FirstChild())
      {
        wButtonCode = 255 - (WORD)atol(pNode->FirstChild()->Value());
        if (wButtonCode > 255) wButtonCode = 0;
      }
    }
    else if (strNode == "keyboard")
    {
      if (pNode->FirstChild())
        strButton = pNode->FirstChild()->Value();
      else
        strButton = "";
      if (strButton.size() == 1)
      { // single character
        char ch = strButton.ToUpper()[0];
        if (ch == ';' || ch == ':') wButtonCode = 0xF0BA;
        else if (ch == '=' || ch == '+') wButtonCode = 0xF0BB;
        else if (ch == ',' || ch == '<') wButtonCode = 0xF0BC;
        else if (ch == '-' || ch == '_') wButtonCode = 0xF0BD;
        else if (ch == '.' || ch == '>') wButtonCode = 0xF0BE;
        else if (ch == '/' || ch == '?') wButtonCode = 0xF0BF;
        else if (ch == '`' || ch == '~') wButtonCode = 0xF0C0;
        else if (ch == '[' || ch == '{') wButtonCode = 0xF0EB;
        else if (ch == '\\' || ch == '|') wButtonCode = 0xF0EC;
        else if (ch == ']' || ch == '}') wButtonCode = 0xF0ED;
        else if (ch == '\'' || ch == '"') wButtonCode = 0xF0EE;
        else wButtonCode = (WORD)ch | KEY_VKEY;
      }
      else
      { // for keys such as return etc. etc.
        CStdString strKey = strButton.ToLower();
        if (strKey == "return") wButtonCode = 0xF00D;
        else if (strKey == "enter") wButtonCode = 0xF06C;
        else if (strKey == "escape") wButtonCode = 0xF01B;
        else if (strKey == "esc") wButtonCode = 0xF01B;
        else if (strKey == "tab") wButtonCode = 0xF009;
        else if (strKey == "space") wButtonCode = 0xF020;
        else if (strKey == "left") wButtonCode = 0xF025;
        else if (strKey == "right") wButtonCode = 0xF027;
        else if (strKey == "up") wButtonCode = 0xF026;
        else if (strKey == "down") wButtonCode = 0xF028;
        else if (strKey == "insert") wButtonCode = 0xF02D;
        else if (strKey == "delete") wButtonCode = 0xF02E;
        else if (strKey == "home") wButtonCode = 0xF024;
        else if (strKey == "end") wButtonCode = 0xF023;
        else if (strKey == "f1") wButtonCode = 0xF070;
        else if (strKey == "f2") wButtonCode = 0xF071;
        else if (strKey == "f3") wButtonCode = 0xF072;
        else if (strKey == "f4") wButtonCode = 0xF073;
        else if (strKey == "f5") wButtonCode = 0xF074;
        else if (strKey == "f6") wButtonCode = 0xF075;
        else if (strKey == "f7") wButtonCode = 0xF076;
        else if (strKey == "f8") wButtonCode = 0xF077;
        else if (strKey == "f9") wButtonCode = 0xF078;
        else if (strKey == "f10") wButtonCode = 0xF079;
        else if (strKey == "f11") wButtonCode = 0xF07A;
        else if (strKey == "f12") wButtonCode = 0xF07B;
        else if (strKey == "numpad0") wButtonCode = 0xF060;
        else if (strKey == "numpad1") wButtonCode = 0xF061;
        else if (strKey == "numpad2") wButtonCode = 0xF062;
        else if (strKey == "numpad3") wButtonCode = 0xF063;
        else if (strKey == "numpad4") wButtonCode = 0xF064;
        else if (strKey == "numpad5") wButtonCode = 0xF065;
        else if (strKey == "numpad6") wButtonCode = 0xF066;
        else if (strKey == "numpad7") wButtonCode = 0xF067;
        else if (strKey == "numpad8") wButtonCode = 0xF068;
        else if (strKey == "numpad9") wButtonCode = 0xF069;
        else if (strKey == "numpad*") wButtonCode = 0xF06A;
        else if (strKey == "numpad+") wButtonCode = 0xF06B;
        else if (strKey == "numpad-") wButtonCode = 0xF06D;
        else if (strKey == "numpad.") wButtonCode = 0xF06E;
        else if (strKey == "numpad/") wButtonCode = 0xF06F;
        else if (strKey == "pageup") wButtonCode = 0xF021;
        else if (strKey == "pagedown") wButtonCode = 0xF022;
        else if (strKey == "printscreen") wButtonCode = 0xF02C;
        else if (strKey == "backspace") wButtonCode = 0xF008;
        else if (strKey == "menu") wButtonCode = 0xF05D;
        else if (strKey == "pause") wButtonCode = 0xF013;
        else if (strKey == "home") wButtonCode = 0xF024;
        else if (strKey == "end") wButtonCode = 0xF023;
        else if (strKey == "insert") wButtonCode = 0xF02D;
        else if (strKey == "leftshift") wButtonCode = 0xF0A0;
        else if (strKey == "rightshift") wButtonCode = 0xF0A1;
        else if (strKey == "leftctrl") wButtonCode = 0xF0A2;
        else if (strKey == "rightctrl") wButtonCode = 0xF0A3;
        else if (strKey == "leftalt") wButtonCode = 0xF0A4;
        else if (strKey == "rightalt") wButtonCode = 0xF0A5;
        else if (strKey == "leftwindows") wButtonCode = 0xF05B;
        else if (strKey == "rightwindows") wButtonCode = 0xF05C;
        else if (strKey == "capslock") wButtonCode = 0xF020;
        else if (strKey == "numlock") wButtonCode = 0xF090;
        else if (strKey == "scrolllock") wButtonCode = 0xF091;
      }
    }
    // check we have a valid button code
    if (wButtonCode > 0)
    {
      // check to see if we've already got this (button,action) pair defined
      buttonMap::iterator it = map.find(wButtonCode);
      if (it == map.end() || (*it).second.wID != wAction)
      {
        //char szTmp[128];
        //sprintf(szTmp,"  action:%i button:%i\n", wAction,wButtonCode);
        //OutputDebugString(szTmp);
        CButtonAction button;
        button.wID = wAction;
        button.strID = strAction;
        map.insert(pair<WORD, CButtonAction>(wButtonCode, button));
      }
    }
    pNode = pNode->NextSibling();
  }
}
