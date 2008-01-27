#ifndef BUTTON_TRANSLATOR_H
#define BUTTON_TRANSLATOR_H

#include <map>
#include "Key.h"
#include "tinyXML/tinyxml.h" 

#pragma once

struct CButtonAction
{
  WORD wID;
  CStdString strID; // needed for "XBMC.ActivateWindow()" type actions
};
// class to map from buttons to actions
class CButtonTranslator
{
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
#ifdef HAS_SDL_JOYSTICK
  bool TranslateJoystickString(WORD wWindow, const char* szDevice, int id, 
                               bool axis, WORD& action, CStdString& strAction,
                               bool &fullrange);
#endif                          

private:
  typedef multimap<WORD, CButtonAction> buttonMap; // our button map to fill in
  map<WORD, buttonMap> translatorMap;       // mapping of windows to button maps
  WORD GetActionCode(WORD wWindow, const CKey &key, CStdString &strAction);
  WORD TranslateGamepadString(const char *szButton);
  WORD TranslateRemoteString(const char *szButton);
  WORD TranslateUniversalRemoteString(const char *szButton);
  WORD TranslateKeyboardString(const char *szButton);
  void MapWindowActions(TiXmlNode *pWindow, WORD wWindowID);
  void MapAction(WORD wButtonCode, const char *szAction, buttonMap &map);

#ifdef HAS_LIRC
  bool LoadLircMap();
  void MapRemote(TiXmlNode *pRemote, const char* szDevice); 
  typedef map<CStdString, CStdString> lircButtonMap;
  map<CStdString, lircButtonMap> lircRemotesMap;
#endif

#ifdef HAS_SDL_JOYSTICK
  void MapJoystickActions(WORD wWindowID, TiXmlNode *pJoystick); 

  typedef map<WORD, map<int, string> > JoystickMap; // <window, <button/axis, action> >
  map<string, JoystickMap> m_joystickButtonMap;      // <joy name, button map>
  map<string, JoystickMap> m_joystickAxisMap;        // <joy name, axis map>
#endif

#ifdef HAS_CWIID
  WORD TranslateWiiremoteString(const char *szButton);
  void MapWiiremoteAction(WORD wButtonCode, const char *szAction, buttonMap &map);
#endif

};

extern CButtonTranslator g_buttonTranslator;

#endif
