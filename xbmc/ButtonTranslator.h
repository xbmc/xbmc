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

class CKey;
class CAction;
class TiXmlNode;

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

  WORD TranslateGamepadString(const char *szButton);
  WORD TranslateRemoteString(const char *szButton);
  WORD TranslateUniversalRemoteString(const char *szButton);

  WORD TranslateKeyboardString(const char *szButton);
  WORD TranslateKeyboardButton(TiXmlElement *pButton);

  void MapWindowActions(TiXmlNode *pWindow, WORD wWindowID);
  void MapAction(WORD wButtonCode, const char *szAction, buttonMap &map);

  bool LoadKeymap(const CStdString &keymapPath);
#ifdef HAS_LIRC
  bool LoadLircMap(const CStdString &lircmapPath);
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

