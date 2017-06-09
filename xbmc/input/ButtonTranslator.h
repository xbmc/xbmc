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

#ifndef BUTTON_TRANSLATOR_H
#define BUTTON_TRANSLATOR_H

#pragma once

#include <map>
#include <string>
#include "system.h" // for HAS_EVENT_SERVER

#ifdef HAS_EVENT_SERVER
#include "network/EventClient.h"
#endif

class CKey;
class CAction;
class TiXmlNode;
class CRegExp;
class CCustomControllerTranslator;
class CIRTranslator;
class CTouchTranslator;

struct CButtonAction
{
  unsigned int id;
  std::string strID; // needed for "ActivateWindow()" type actions
  unsigned int holdtimeMs;
};
///
/// singleton class to map from buttons to actions
/// Warning: _not_ threadsafe!
class CButtonTranslator
{
#ifdef HAS_EVENT_SERVER
  friend class EVENTCLIENT::CEventButtonState;
#endif

private:
  //private construction, and no assignments; use the provided singleton methods
  CButtonTranslator();
  CButtonTranslator(const CButtonTranslator&);
  CButtonTranslator const& operator=(CButtonTranslator const&);
  virtual ~CButtonTranslator();

public:
  ///access to singleton
  static CButtonTranslator& GetInstance();

  // Add/remove a HID device with custom mappings
  void AddDevice(std::string& strDevice);
  void RemoveDevice(std::string& strDevice);

  /// loads Keymap.xml
  bool Load(bool AlwaysLoad = false);
  /// clears the maps
  void Clear();

  /*! \brief Finds out if a longpress mapping exists for this key
   \param window id of the current window
   \param key to search a mapping for
   \return true if a longpress mapping exists
   */
  bool HasLongpressMapping(int window, const CKey &key);

  /*! \brief Get the "holdtime" parameter if specified for this key
   \param window id of the current window
   \param key to search a mapping for
   \return the holdtime in ms, or 0 if no holdtime was specified
   */
  unsigned int GetHoldTimeMs(int window, const CKey &key, bool fallback = true);

  /*! \brief Obtain the action configured for a given window and key
   \param window the window id
   \param key the key to query the action for
   \param fallback if no action is directly configured for the given window, obtain the action from fallback window, if exists or from global config as last resort
   \return the action matching the key
   */
  CAction GetAction(int window, const CKey &key, bool fallback = true);

  /*! \brief Obtain the global action configured for a given key
   \param key the key to query the action for
   \return the global action
   */
  CAction GetGlobalAction(const CKey &key);

  int TranslateLircRemoteString(const char* szDevice, const char *szButton);

  bool TranslateCustomControllerString(int windowId, const std::string& controllerName, int buttonId, int& action, std::string& strAction);

  bool TranslateTouchAction(int window, int touchAction, int touchPointers, int &action, std::string &actionString);

private:
  typedef std::multimap<uint32_t, CButtonAction> buttonMap; // our button map to fill in

  // m_translatorMap contains all mappings i.e. m_BaseMap + HID device mappings
  std::map<int, buttonMap> m_translatorMap;
  // m_deviceList contains the list of connected HID devices
  std::list<std::string> m_deviceList;

  int GetActionCode(int window, int action);
  int GetActionCode(int window, const CKey &key, std::string &strAction) const;

  static uint32_t TranslateGamepadString(const char *szButton);
  static uint32_t TranslateJoystickCommand(const TiXmlElement *pButton, const std::string& controllerId, unsigned int& holdtimeMs);

  static uint32_t TranslateKeyboardString(const char *szButton);
  static uint32_t TranslateKeyboardButton(TiXmlElement *pButton);

  static uint32_t TranslateMouseCommand(TiXmlElement *pButton);

  static uint32_t TranslateAppCommand(const char *szButton);

  void MapWindowActions(TiXmlNode *pWindow, int wWindowID);
  void MapAction(uint32_t buttonCode, const char *szAction, unsigned int holdtimeMs, buttonMap &map);

  bool LoadKeymap(const std::string &keymapPath);

  bool m_Loaded;

  std::unique_ptr<CCustomControllerTranslator> m_customControllerTranslator;
  std::unique_ptr<CIRTranslator> m_irTranslator;
  std::unique_ptr<CTouchTranslator> m_touchTranslator;
};

#endif

