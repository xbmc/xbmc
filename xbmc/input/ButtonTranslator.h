/*
 *      Copyright (C) 2005-2017 Team Kodi
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
#pragma once

#include <map>
#include <set>
#include <string>

#include "system.h" // for HAS_EVENT_SERVER
#include "Action.h"

#ifdef HAS_EVENT_SERVER
#include "network/EventClient.h"
#endif

class CKey;
class TiXmlNode;
class CCustomControllerTranslator;
class CIRTranslator;
class CTouchTranslator;
class IKeymapEnvironment;
class IWindowKeymap;

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
  CButtonTranslator(const CButtonTranslator&) = delete;
  CButtonTranslator const& operator=(CButtonTranslator const&) = delete;
  virtual ~CButtonTranslator() = default;

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

  int TranslateLircRemoteString(const std::string &szDevice, const std::string &szButton);

  bool TranslateCustomControllerString(int windowId, const std::string& controllerName, int buttonId, int& action, std::string& strAction);

  bool TranslateTouchAction(int window, int touchAction, int touchPointers, int &action, std::string &actionString);

  const IKeymapEnvironment *KeymapEnvironment() const { return m_keymapEnvironment.get(); }

  const IWindowKeymap *JoystickKeymap(const std::string &controllerId) const;

  const std::vector<std::string> &GetControllers() const { return m_controllerIds; }

private:
  struct CButtonAction
  {
    unsigned int id;
    std::string strID; // needed for "ActivateWindow()" type actions
  };

  typedef std::multimap<uint32_t, CButtonAction> buttonMap; // our button map to fill in

  // m_translatorMap contains all mappings i.e. m_BaseMap + HID device mappings
  std::map<int, buttonMap> m_translatorMap;

  // m_deviceList contains the list of connected HID devices
  std::set<std::string> m_deviceList;

  unsigned int GetActionCode(int window, const CKey &key, std::string &strAction) const;

  void MapWindowActions(const TiXmlNode *pWindow, int wWindowID);
  void MapAction(uint32_t buttonCode, const std::string &szAction, buttonMap &map);

  bool LoadKeymap(const std::string &keymapPath);

  std::unique_ptr<IKeymapEnvironment> m_keymapEnvironment;
  std::unique_ptr<CCustomControllerTranslator> m_customControllerTranslator;
  std::unique_ptr<CIRTranslator> m_irTranslator;
  std::unique_ptr<CTouchTranslator> m_touchTranslator;
  std::map<std::string, std::unique_ptr<IWindowKeymap>> m_joystickKeymaps; // Controller ID -> keymap
  std::vector<std::string> m_controllerIds;
};
