#ifndef BUTTON_TRANSLATOR_H
#define BUTTON_TRANSLATOR_H

#include <map>
#include "Key.h"
#include "tinyXML/tinyxml.h" 
#ifdef HAS_EVENT_SERVER
#include "utils/EventClient.h"
#endif
#pragma once

#ifdef HAS_EVENT_SERVER
#include "utils/EventClient.h"
#endif

struct CButtonAction
{
  WORD wID;
  CStdString strID; // needed for "XBMC.ActivateWindow()" type actions
};
// class to map from buttons to actions
class CButtonTranslator
{
#ifdef HAS_EVENT_SERVER
  friend class EVENTCLIENT::CEventButtonState;
#endif

public:
  CButtonTranslator();
  virtual ~CButtonTranslator();

  bool Load();
  void Clear();
  void GetAction(WORD wWindow, const CKey &key, CAction &action);
  WORD TranslateWindowString(const char *szWindow);
  bool TranslateActionString(const char *szAction, WORD &wAction);
#ifdef HAS_LIRC
  WORD TranslateLircRemoteString(const char* szDevice, const char *szButton);
#endif
#if defined(HAS_SDL_JOYSTICK) || defined(HAS_EVENT_SERVER)
  bool TranslateJoystickString(WORD wWindow, const char* szDevice, int id, 
                               bool axis, WORD& action, CStdString& strAction,
                               bool &fullrange);
#endif                          

private:
  typedef std::multimap<WORD, CButtonAction> buttonMap; // our button map to fill in
  std::map<WORD, buttonMap> translatorMap;       // mapping of windows to button maps
  WORD GetActionCode(WORD wWindow, const CKey &key, CStdString &strAction);
  
  WORD TranslateGamepadButton(TiXmlElement *pButton);
  WORD TranslateGamepadString(const char *szButton);
  
  WORD TranslateRemoteButton(TiXmlElement *pButton);
  WORD TranslateRemoteString(const char *szButton);
  
  WORD TranslateUniversalRemoteButton(TiXmlElement *pButton);
  WORD TranslateUniversalRemoteString(const char *szButton);
  
  WORD TranslateKeyboardString(const char *szButton);
  WORD TranslateKeyboardButton(TiXmlElement *pButton);
  
  void MapWindowActions(TiXmlNode *pWindow, WORD wWindowID);
  void MapAction(WORD wButtonCode, const char *szAction, buttonMap &map);

#ifdef HAS_LIRC
  bool LoadLircMap();
  void MapRemote(TiXmlNode *pRemote, const char* szDevice); 
  typedef std::map<CStdString, CStdString> lircButtonMap;
  std::map<CStdString, lircButtonMap> lircRemotesMap;
#endif

#if defined(HAS_SDL_JOYSTICK) || defined(HAS_EVENT_SERVER)
  void MapJoystickActions(WORD wWindowID, TiXmlNode *pJoystick); 

  typedef std::map<WORD, std::map<int, std::string> > JoystickMap; // <window, <button/axis, action> >
  std::map<std::string, JoystickMap> m_joystickButtonMap;      // <joy name, button map>
  std::map<std::string, JoystickMap> m_joystickAxisMap;        // <joy name, axis map>
#endif
};

extern CButtonTranslator g_buttonTranslator;

#endif

