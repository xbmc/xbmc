#include "ButtonTranslator.h"
#include "XBIRRemote.h"
#include "../guilib/Key.h"

CButtonTranslator g_buttonTranslator;

CButtonTranslator::CButtonTranslator()
{
}

CButtonTranslator::~CButtonTranslator()
{
}

void CButtonTranslator::Load()
{	
	// load our xml file, and fill up our mapping tables
	TiXmlDocument xmlDoc;

	OutputDebugString("Loading keymap.xml\n");
	// Load the config file
	if (!xmlDoc.LoadFile("Q:\\keymap.xml"))
	{
		OutputDebugString("Unable to load keymap.xml\n");
		return;
	}

	TiXmlElement* pRoot = xmlDoc.RootElement();
	CStdString strValue=pRoot->Value();
	if ( strValue != "keymap") return;
	// do the global actions
	TiXmlElement* pWindow = pRoot->FirstChildElement("global");
	if (pWindow)
	{
		buttonMap map;
		TiXmlNode* pAction =pWindow->FirstChild("action");
		OutputDebugString("GLOBAL:\n");
		while (pAction)
		{
			TiXmlNode* pNode=pAction->FirstChild("id");
			WORD wID = 0;				// action identity
			if (pNode) wID = (WORD)atol(pNode->FirstChild()->Value());
			if (wID>0)
			{	// valid id, get the buttons associated with this action...
				MapAction(wID, pAction->FirstChild(), map);
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
		WORD wWindowID = -1;
		if (pID) wWindowID = (WORD)atol(pID->FirstChild()->Value());
		if (wWindowID>=0)
		{
			char szTmp[128];
			sprintf(szTmp,"WINDOW:%i\n", wWindowID);
			OutputDebugString(szTmp);
			TiXmlNode* pAction =pWindow->FirstChild("action");
			while (pAction)
			{
				TiXmlNode* pNode=pAction->FirstChild("id");
				WORD wID = 0;				// action identity
				if (pNode) wID = (WORD)atol(pNode->FirstChild()->Value());
				if (wID>0)
				{	// valid id, get the buttons associated with this action...
					MapAction(wID, pAction->FirstChild(), map);
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
}

void CButtonTranslator::GetAction(WORD wWindow, const CKey &key, CAction &action)
{
	// try to get the action from the current window
	WORD wAction = GetActionCode(wWindow, key);
	// if it's invalid, try to get it from the global map
	if (wAction == 0)
		wAction = GetActionCode(-1, key);
	// Now fill our action structure
	action.wID = wAction;
	action.fAmount1 = 0;
	action.fAmount2 = 0;
	// get the action amounts of the analog buttons
	if (key.GetButtonCode() == KEY_BUTTON_LEFT_TRIGGER)
	{
		action.fAmount1 = key.GetLeftTrigger();
	}
	if (key.GetButtonCode() == KEY_BUTTON_RIGHT_TRIGGER)
	{
		action.fAmount1 = key.GetRightTrigger();
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
}

WORD CButtonTranslator::GetActionCode(WORD wWindow, const CKey &key)
{
	WORD wKey = (WORD)key.GetButtonCode();
	map<WORD,buttonMap>::iterator it = translatorMap.find(wWindow);
	if (it == translatorMap.end())
		return 0;
	buttonMap::iterator it2 = (*it).second.find(wKey);
	WORD wAction = 0;
	while (it2 != (*it).second.end())
	{
		wAction = (*it2).second;
		it2 = (*it).second.end();
	}
	return wAction;
}

void CButtonTranslator::MapAction(WORD wAction, TiXmlNode *pNode, buttonMap &map)
{
	CStdString strNode, strButton;
	WORD wButtonCode;
	while (pNode)
	{
		strNode = pNode->Value();
		wButtonCode = 0;
		if (strNode=="gamepad")
		{
			strButton = pNode->FirstChild()->Value();
			if (strButton == "A")					wButtonCode = KEY_BUTTON_A;
			if (strButton == "B")					wButtonCode = KEY_BUTTON_B;
			if (strButton == "X")					wButtonCode = KEY_BUTTON_X;
			if (strButton == "Y")					wButtonCode = KEY_BUTTON_Y;
			if (strButton == "white")				wButtonCode = KEY_BUTTON_WHITE;
			if (strButton == "black")				wButtonCode = KEY_BUTTON_BLACK;
			if (strButton == "start")				wButtonCode = KEY_BUTTON_START;
			if (strButton == "back")				wButtonCode = KEY_BUTTON_BACK;
			if (strButton == "leftthumbbutton")		wButtonCode = KEY_BUTTON_LEFT_THUMB_BUTTON;
			if (strButton == "rightthumbbutton")	wButtonCode = KEY_BUTTON_RIGHT_THUMB_BUTTON;
			if (strButton == "leftthumbstick")		wButtonCode = KEY_BUTTON_LEFT_THUMB_STICK;
			if (strButton == "rightthumbstick")		wButtonCode = KEY_BUTTON_RIGHT_THUMB_STICK;
			if (strButton == "lefttrigger")			wButtonCode = KEY_BUTTON_LEFT_TRIGGER;
			if (strButton == "righttrigger")		wButtonCode = KEY_BUTTON_RIGHT_TRIGGER;
			if (strButton == "dpadleft")			wButtonCode = KEY_BUTTON_DPAD_LEFT;
			if (strButton == "dpadright")			wButtonCode = KEY_BUTTON_DPAD_RIGHT;
			if (strButton == "dpadup")				wButtonCode = KEY_BUTTON_DPAD_UP;
			if (strButton == "dpaddown")			wButtonCode = KEY_BUTTON_DPAD_DOWN;
		}
		if (strNode=="remote")
		{
			strButton = pNode->FirstChild()->Value();
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
		// check we have a valid button code
		if (wButtonCode>0)
		{
			// check to see if we've already got this (button,action) pair defined
			buttonMap::iterator it = map.find(wButtonCode);
			if (it == map.end() || (*it).second != wAction)
			{
				char szTmp[128];
				sprintf(szTmp,"  action:%i button:%i\n", wAction,wButtonCode);
				OutputDebugString(szTmp);
				map.insert(pair<WORD, WORD>(wButtonCode,wAction));
			}
		}
		pNode = pNode->NextSibling();
	}
}
