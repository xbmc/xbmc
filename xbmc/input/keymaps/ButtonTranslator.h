/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/actions/Action.h"
#include "network/EventClient.h"

#include <map>
#include <set>
#include <string>

class CKey;

namespace tinyxml2
{
class XMLNode;
}

namespace KODI
{
namespace KEYMAP
{
class CCustomControllerTranslator;
class CTouchTranslator;
class IKeyMapper;
class IWindowKeymap;

/*!
 * \ingroup keymap
 *
 * \brief Singleton class to map from buttons to actions
 *
 * Warning: _not_ threadsafe!
 */
class CButtonTranslator
{
  friend class EVENTCLIENT::CEventButtonState;

public:
  CButtonTranslator() = default;
  CButtonTranslator(const CButtonTranslator&) = delete;
  CButtonTranslator const& operator=(CButtonTranslator const&) = delete;
  virtual ~CButtonTranslator() = default;

  // Add/remove a HID device with custom mappings
  bool AddDevice(const std::string& strDevice);
  bool RemoveDevice(const std::string& strDevice);

  /// loads Keymap.xml
  bool Load();

  /// clears the maps
  void Clear();

  /*! \brief Finds out if a longpress mapping exists for this key
   \param window id of the current window
   \param key to search a mapping for
   \return true if a longpress mapping exists
   */
  bool HasLongpressMapping(int window, const CKey& key);

  /*! \brief Obtain the action configured for a given window and key
   \param window the window id
   \param key the key to query the action for
   \param fallback if no action is directly configured for the given window, obtain the action from
   fallback window, if exists or from global config as last resort
   \return the action matching the key
   */
  CAction GetAction(int window, const CKey& key, bool fallback = true);

  void RegisterMapper(const std::string& device, IKeyMapper* mapper);
  void UnregisterMapper(const IKeyMapper* mapper);

  static uint32_t TranslateString(const std::string& strMap, const std::string& strButton);

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

  unsigned int GetActionCode(int window, const CKey& key, std::string& strAction) const;

  void MapWindowActions(const tinyxml2::XMLNode* pWindow, int wWindowID);
  void MapAction(uint32_t buttonCode, const std::string& szAction, buttonMap& map);

  bool LoadKeymap(const std::string& keymapPath);

  bool HasLongpressMapping_Internal(int window, const CKey& key);

  std::map<std::string, IKeyMapper*> m_buttonMappers;
};
} // namespace KEYMAP
} // namespace KODI
