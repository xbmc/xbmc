/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
  ~CAddonSystemSettings() override = default;

  const std::map<ADDON::TYPE, std::string> m_activeSettings;
};
};
