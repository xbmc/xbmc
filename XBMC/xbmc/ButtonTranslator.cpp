
#include "stdafx.h"
#include "ButtonTranslator.h"
#include "XBIRRemote.h"
#include "../guilib/Key.h"
#include "utils/log.h"
#include "util.h"

CButtonTranslator g_buttonTranslator;
extern CStdString g_LoadErrorStr;

CButtonTranslator::CButtonTranslator()
{
}

CButtonTranslator::~CButtonTranslator()
{
}

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
	CStdString strValue=pRoot->Value();
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
		TiXmlNode* pAction =pWindow->FirstChild("action");
		//OutputDebugString("GLOBAL:\n");
		while (pAction)
		{
			TiXmlNode* pNode=pAction->FirstChild("id");
			WORD wID = 0;				// action identity
			CStdString strID;
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
			if (wID>0)
			{	// valid id, get the buttons associated with this action...
				MapAction(wID, strID, pAction->FirstChild(), map);
				pNode = pAction->FirstChild();
			}
			pAction = pAction->NextSibling();
		}
		// add our map to our table
		if (map.size()>0)
			translatorMap.insert(pair<WORD,buttonMap>(-1,map));
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
        wWindowID = WINDOW_HOME+(WORD)atol(pID->FirstChild()->Value());
    }
		if (wWindowID!=WINDOW_INVALID)
		{
			TiXmlNode* pAction =pWindow->FirstChild("action");
			while (pAction)
			{
				TiXmlNode* pNode=pAction->FirstChild("id");
				WORD wID = 0;				// action identity
				CStdString strID;
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
				if (wID>0)
				{	// valid id, get the buttons associated with this action...
					MapAction(wID, strID, pAction->FirstChild(), map);
					pNode = pAction->FirstChild();
				}
				pAction = pAction->NextSibling();
			}
			// add our map to our table
			if (map.size()>0)
				translatorMap.insert(pair<WORD,buttonMap>(wWindowID,map));
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
		wAction = GetActionCode(-1, key, strAction);
	// Now fill our action structure
	action.wID = wAction;
	action.strAction = strAction;
	action.fAmount1 = 1;	// digital button (could change this for repeat acceleration)
	action.fAmount2 = 0;
	action.m_dwButtonCode = key.GetButtonCode();
	// get the action amounts of the analog buttons
	if (key.GetButtonCode() == KEY_BUTTON_LEFT_ANALOG_TRIGGER)
	{
		action.fAmount1 = (float)key.GetLeftTrigger()/255.0f;
	}
	if (key.GetButtonCode() == KEY_BUTTON_RIGHT_ANALOG_TRIGGER)
	{
		action.fAmount1 = (float)key.GetRightTrigger()/255.0f;
	}
	if (key.GetButtonCode() == KEY_BUTTON_LEFT_THUMB_STICK)
	{
		action.fAmount1 = key.GetLeftThumbX();
		action.fAmount2 = key.GetLeftThumbY();
	}
	if (key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK)
	{
		action.fAmount1 = key.GetRightThumbX();
		action.fAmount2 = key.GetRightThumbY();
	}
	if (key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK_UP || key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK_DOWN)
	{
		action.fAmount1 = key.GetRightThumbY();
	}
	if (key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK_LEFT || key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT)
	{
		action.fAmount1 = key.GetRightThumbX();
	}
}

WORD CButtonTranslator::GetActionCode(WORD wWindow, const CKey &key, CStdString &strAction)
{
	WORD wKey = (WORD)key.GetButtonCode();
	map<WORD,buttonMap>::iterator it = translatorMap.find(wWindow);
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
		if (strNode=="gamepad")
		{
      if (pNode->FirstChild())
			  strButton = pNode->FirstChild()->Value();
      else strButton ="";
			if (strButton == "A")						wButtonCode = KEY_BUTTON_A;
			if (strButton == "B")						wButtonCode = KEY_BUTTON_B;
			if (strButton == "X")						wButtonCode = KEY_BUTTON_X;
			if (strButton == "Y")						wButtonCode = KEY_BUTTON_Y;
			if (strButton == "white")					wButtonCode = KEY_BUTTON_WHITE;
			if (strButton == "black")					wButtonCode = KEY_BUTTON_BLACK;
			if (strButton == "start")					wButtonCode = KEY_BUTTON_START;
			if (strButton == "back")					wButtonCode = KEY_BUTTON_BACK;
			if (strButton == "leftthumbbutton")			wButtonCode = KEY_BUTTON_LEFT_THUMB_BUTTON;
			if (strButton == "rightthumbbutton")		wButtonCode = KEY_BUTTON_RIGHT_THUMB_BUTTON;
			if (strButton == "leftthumbstick")			wButtonCode = KEY_BUTTON_LEFT_THUMB_STICK;
			if (strButton == "rightthumbstick")			wButtonCode = KEY_BUTTON_RIGHT_THUMB_STICK;
			if (strButton == "rightthumbstickup")		wButtonCode = KEY_BUTTON_RIGHT_THUMB_STICK_UP;
			if (strButton == "rightthumbstickdown")		wButtonCode = KEY_BUTTON_RIGHT_THUMB_STICK_DOWN;
			if (strButton == "rightthumbstickleft")		wButtonCode = KEY_BUTTON_RIGHT_THUMB_STICK_LEFT;
			if (strButton == "rightthumbstickright")	wButtonCode = KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT;
			if (strButton == "lefttrigger")				wButtonCode = KEY_BUTTON_LEFT_TRIGGER;
			if (strButton == "righttrigger")			wButtonCode = KEY_BUTTON_RIGHT_TRIGGER;
			if (strButton == "leftanalogtrigger") wButtonCode = KEY_BUTTON_LEFT_ANALOG_TRIGGER;
			if (strButton == "rightanalogtrigger") wButtonCode = KEY_BUTTON_RIGHT_ANALOG_TRIGGER;
			if (strButton == "dpadleft")				wButtonCode = KEY_BUTTON_DPAD_LEFT;
			if (strButton == "dpadright")				wButtonCode = KEY_BUTTON_DPAD_RIGHT;
			if (strButton == "dpadup")					wButtonCode = KEY_BUTTON_DPAD_UP;
			if (strButton == "dpaddown")				wButtonCode = KEY_BUTTON_DPAD_DOWN;
		}
		if (strNode=="remote")
		{
      if (pNode->FirstChild())
			  strButton = pNode->FirstChild()->Value();
      else
        strButton="";
			if (strButton == "left")				wButtonCode = XINPUT_IR_REMOTE_LEFT;
			if (strButton == "right")				wButtonCode = XINPUT_IR_REMOTE_RIGHT;
			if (strButton == "up")					wButtonCode = XINPUT_IR_REMOTE_UP;
			if (strButton == "down")				wButtonCode = XINPUT_IR_REMOTE_DOWN;
			if (strButton == "select")				wButtonCode = XINPUT_IR_REMOTE_SELECT;
			if (strButton == "back")				wButtonCode = XINPUT_IR_REMOTE_BACK;
			if (strButton == "menu")				wButtonCode = XINPUT_IR_REMOTE_MENU;
			if (strButton == "info")				wButtonCode = XINPUT_IR_REMOTE_INFO;
			if (strButton == "display")				wButtonCode = XINPUT_IR_REMOTE_DISPLAY;
			if (strButton == "title")				wButtonCode = XINPUT_IR_REMOTE_TITLE;
			if (strButton == "play")				wButtonCode = XINPUT_IR_REMOTE_PLAY;
			if (strButton == "pause")				wButtonCode = XINPUT_IR_REMOTE_PAUSE;
			if (strButton == "reverse")				wButtonCode = XINPUT_IR_REMOTE_REVERSE;
			if (strButton == "forward")				wButtonCode = XINPUT_IR_REMOTE_FORWARD;
			if (strButton == "skipplus")			wButtonCode = XINPUT_IR_REMOTE_SKIP_PLUS;
			if (strButton == "skipminus")			wButtonCode = XINPUT_IR_REMOTE_SKIP_MINUS;
			if (strButton == "stop")				wButtonCode = XINPUT_IR_REMOTE_STOP;
			if (strButton == "0")					wButtonCode = XINPUT_IR_REMOTE_0;
			if (strButton == "1")					wButtonCode = XINPUT_IR_REMOTE_1;
			if (strButton == "2")					wButtonCode = XINPUT_IR_REMOTE_2;
			if (strButton == "3")					wButtonCode = XINPUT_IR_REMOTE_3;
			if (strButton == "4")					wButtonCode = XINPUT_IR_REMOTE_4;
			if (strButton == "5")					wButtonCode = XINPUT_IR_REMOTE_5;
			if (strButton == "6")					wButtonCode = XINPUT_IR_REMOTE_6;
			if (strButton == "7")					wButtonCode = XINPUT_IR_REMOTE_7;
			if (strButton == "8")					wButtonCode = XINPUT_IR_REMOTE_8;
			if (strButton == "9")					wButtonCode = XINPUT_IR_REMOTE_9;
		}
		if (strNode=="remotecode")
		{
			// Button Code is 255 - OBC (Original Button Code) of the button
			if (pNode->FirstChild())
			{
				wButtonCode = 255-(WORD)atol(pNode->FirstChild()->Value());
				if (wButtonCode > 255) wButtonCode = 0;
			}
		}
		if (strNode=="keyboard")
		{
      if (pNode->FirstChild())
			  strButton = pNode->FirstChild()->Value();
      else
        strButton="";
			if (strButton.size() == 1)
			{	// single character
				char ch = strButton.ToUpper()[0];
				if			(ch == ';' || ch == ':')	wButtonCode = 0xF0BA;
				else if (ch == '=' || ch == '+')	wButtonCode = 0xF0BB;
				else if (ch == ',' || ch == '<')	wButtonCode = 0xF0BC;
				else if (ch == '-' || ch == '_')	wButtonCode = 0xF0BD;
				else if (ch == '.' || ch == '>')	wButtonCode = 0xF0BE;
				else if (ch == '/' || ch == '?')	wButtonCode = 0xF0BF;
				else if (ch == '`' || ch == '~')	wButtonCode = 0xF0C0;
				else if (ch == '[' || ch == '{')	wButtonCode = 0xF0EB;
				else if (ch == '\\'|| ch == '|')	wButtonCode = 0xF0EC;
				else if (ch == ']' || ch == '}')	wButtonCode = 0xF0ED;
				else if (ch == '\''|| ch == '"')	wButtonCode = 0xF0EE;
				else wButtonCode = (WORD)ch | KEY_VKEY;
			}
			else
			{	// for keys such as return etc. etc.
				CStdString strKey = strButton.ToLower();
				if (strKey == "return")		wButtonCode = 0xF00D;
				if (strKey == "enter")		wButtonCode = 0xF06C;
				if (strKey == "escape")		wButtonCode = 0xF01B;
				if (strKey == "esc")			wButtonCode = 0xF01B;
				if (strKey == "tab")			wButtonCode = 0xF009;
				if (strKey == "space")		wButtonCode = 0xF020;
				if (strKey == "left")			wButtonCode = 0xF025;
				if (strKey == "right")		wButtonCode = 0xF027;
				if (strKey == "up")				wButtonCode = 0xF026;
				if (strKey == "down")			wButtonCode = 0xF028;
				if (strKey == "insert")		wButtonCode = 0xF02D;
				if (strKey == "delete")		wButtonCode = 0xF02E;
				if (strKey == "home")			wButtonCode = 0xF024;
				if (strKey == "end")			wButtonCode = 0xF023;
				if (strKey == "f1")				wButtonCode = 0xF070;
				if (strKey == "f2")				wButtonCode = 0xF071;
				if (strKey == "f3")				wButtonCode = 0xF072;
				if (strKey == "f4")				wButtonCode = 0xF073;
				if (strKey == "f5")				wButtonCode = 0xF074;
				if (strKey == "f6")				wButtonCode = 0xF075;
				if (strKey == "f7")				wButtonCode = 0xF076;
				if (strKey == "f8")				wButtonCode = 0xF077;
				if (strKey == "f9")				wButtonCode = 0xF078;
				if (strKey == "f10")			wButtonCode = 0xF079;
				if (strKey == "f11")			wButtonCode = 0xF07A;
				if (strKey == "f12")			wButtonCode = 0xF07B;
				if (strKey == "numpad0")	wButtonCode = 0xF060;
				if (strKey == "numpad1")	wButtonCode = 0xF061;
				if (strKey == "numpad2")	wButtonCode = 0xF062;
				if (strKey == "numpad3")	wButtonCode = 0xF063;
				if (strKey == "numpad4")	wButtonCode = 0xF064;
				if (strKey == "numpad5")	wButtonCode = 0xF065;
				if (strKey == "numpad6")	wButtonCode = 0xF066;
				if (strKey == "numpad7")	wButtonCode = 0xF067;
				if (strKey == "numpad8")	wButtonCode = 0xF068;
				if (strKey == "numpad9")	wButtonCode = 0xF069;
				if (strKey == "numpad*")	wButtonCode = 0xF06A;
				if (strKey == "numpad+")	wButtonCode = 0xF06B;
				if (strKey == "numpad-")	wButtonCode = 0xF06D;
				if (strKey == "numpad.")	wButtonCode = 0xF06E;
				if (strKey == "numpad/")	wButtonCode = 0xF06F;
				if (strKey == "pageup")				wButtonCode = 0xF021;
				if (strKey == "pagedown")			wButtonCode = 0xF022;
				if (strKey == "printscreen")	wButtonCode = 0xF02C;
				if (strKey == "backspace")		wButtonCode = 0xF008;
				if (strKey == "menu")					wButtonCode = 0xF05D;
				if (strKey == "pause")				wButtonCode = 0xF013;
				if (strKey == "home")					wButtonCode = 0xF024;
				if (strKey == "end")					wButtonCode = 0xF023;
				if (strKey == "insert")				wButtonCode = 0xF02D;
				if (strKey == "leftshift")		wButtonCode = 0xF0A0;
				if (strKey == "rightshift")		wButtonCode = 0xF0A1;
				if (strKey == "leftctrl")			wButtonCode = 0xF0A2;
				if (strKey == "rightctrl")		wButtonCode = 0xF0A3;
				if (strKey == "leftalt")			wButtonCode = 0xF0A4;
				if (strKey == "rightalt")			wButtonCode = 0xF0A5;
				if (strKey == "leftwindows")	wButtonCode = 0xF05B;
				if (strKey == "rightwindows")	wButtonCode = 0xF05C;
				if (strKey == "capslock")			wButtonCode = 0xF020;
				if (strKey == "numlock")			wButtonCode = 0xF090;
				if (strKey == "scrolllock")		wButtonCode = 0xF091;
			}
		}
		// check we have a valid button code
		if (wButtonCode>0)
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
				map.insert(pair<WORD, CButtonAction>(wButtonCode,button));
			}
		}
		pNode = pNode->NextSibling();
	}
}
