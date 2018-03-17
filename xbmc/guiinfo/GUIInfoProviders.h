/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include <string>
#include <vector>

class CFileItem;
class CGUIListItem;

namespace GUIINFO
{

class GUIInfo;
class IGUIInfoProvider;

class CGUIInfoProviders
{
public:
  CGUIInfoProviders() = default;
  virtual ~CGUIInfoProviders() = default;

  /*!
   * @brief Register a guiinfo provider.
   * @param provider The provider to register.
   */
  void RegisterProvider(IGUIInfoProvider *provider);

  /*!
   * @brief Unregister a guiinfo provider.
   * @param provider The provider to unregister.
   */
  void UnregisterProvider(IGUIInfoProvider *provider);

  /*!
   * @brief Get a GUIInfoManager label string from one of the registered providers.
   * @param value Will be filled with the requested value.
   * @param item The item to get the value for.
   * @param info The GUI info (label id + additional data).
   * @param fallback A fallback value. Can be nullptr.
   * @return True if the value was filled successfully by one of the providers, false otherwise.
   */
  bool GetLabel(std::string& value, const CFileItem *item, const GUIInfo &info, std::string *fallback) const;

  /*!
   * @brief Get a GUIInfoManager integer value from one of the registered providers.
   * @param value Will be filled with the requested value.
   * @param item The item to get the value for.
   * @param info The GUI info (label id + additional data).
   * @return True if the value was filled successfully by one of the providers, false otherwise.
   */
  bool GetInt(int& value, const CGUIListItem *item, const GUIInfo &info) const;

  /*!
   * @brief Get a GUIInfoManager bool value from one of the registered providers.
   * @param value Will be filled with the requested value.
   * @param item The item to get the value for.
   * @param info The GUI info (label id + additional data).
   * @return True if the value was filled successfully by one of the providers, false otherwise.
   */
  bool GetBool(bool& value, const CGUIListItem *item, const GUIINFO::GUIInfo &info) const;

private:
  std::vector<IGUIInfoProvider *> m_providers;
};

} // namespace GUIINFO
