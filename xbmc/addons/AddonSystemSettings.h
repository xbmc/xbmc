#pragma once
/*
 *      Copyright (C) 2015 Team Kodi
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

#include "addons/IAddon.h"
#include "settings/lib/ISettingCallback.h"
#include <functional>
#include <string>

namespace ADDON
{

const int AUTO_UPDATES_ON = 0;
const int AUTO_UPDATES_NOTIFY = 1;
const int AUTO_UPDATES_NEVER = 2;

class CAddonSystemSettings : public ISettingCallback
{
public:
  static CAddonSystemSettings& GetInstance();
  void OnSettingAction(std::shared_ptr<const CSetting> setting) override;
  void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;

  bool GetActive(const TYPE& type, AddonPtr& addon);
  bool SetActive(const TYPE& type, const std::string& addonID);
  bool IsActive(const IAddon& addon);

  /*!
   * Attempt to unset addon as active. Returns true if addon is no longer active,
   * false if it could not be unset (e.g. if the addon is the default)
   */
  bool UnsetActive(const AddonPtr& addon);

  /*!
   * Check compatibility of installed addons and attempt to migrate.
   *
   * @param onMigrate Called when a long running migration task takes place.
   * @return list of addons that was modified.
   */
  std::vector<std::string> MigrateAddons(std::function<void(void)> onMigrate);

private:
  CAddonSystemSettings();
  CAddonSystemSettings(const CAddonSystemSettings&) = default;
  CAddonSystemSettings& operator=(const CAddonSystemSettings&) = default;
  CAddonSystemSettings(CAddonSystemSettings&&);
  CAddonSystemSettings& operator=(CAddonSystemSettings&&);
  virtual ~CAddonSystemSettings() = default;

  const std::map<ADDON::TYPE, std::string> m_activeSettings;
};
};