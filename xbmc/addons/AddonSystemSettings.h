/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingCallback.h"

#include <map>
#include <memory>
#include <string>

namespace ADDON
{

const int AUTO_UPDATES_ON = 0;
const int AUTO_UPDATES_NOTIFY = 1;
const int AUTO_UPDATES_NEVER = 2;

enum class AddonRepoUpdateMode
{
  OFFICIAL_ONLY = 0,
  ANY_REPOSITORY = 1
};

enum class AddonType;

class CAddonInfo;
using AddonInfoPtr = std::shared_ptr<CAddonInfo>;

class IAddon;
using AddonPtr = std::shared_ptr<IAddon>;

class CAddonSystemSettings : public ISettingCallback
{
public:
  static CAddonSystemSettings& GetInstance();
  void OnSettingAction(const std::shared_ptr<const CSetting>& setting) override;
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  bool GetActive(AddonType type, AddonPtr& addon);
  bool SetActive(AddonType type, const std::string& addonID);
  bool IsActive(const IAddon& addon);

  /*!
   * Gets Kodi addon auto update mode
   *
   * @return the autoupdate mode value
  */
  int GetAddonAutoUpdateMode() const;


  /*!
   * Gets Kodi preferred addon repository update mode
   *
   * @return the preferred mode value
   */
  AddonRepoUpdateMode GetAddonRepoUpdateMode() const;

  /*!
   * Attempt to unset addon as active. Returns true if addon is no longer active,
   * false if it could not be unset (e.g. if the addon is the default)
   */
  bool UnsetActive(const AddonInfoPtr& addon);

private:
  CAddonSystemSettings();
  CAddonSystemSettings(const CAddonSystemSettings&) = delete;
  CAddonSystemSettings& operator=(const CAddonSystemSettings&) = delete;
  ~CAddonSystemSettings() override = default;

  const std::map<AddonType, std::string> m_activeSettings;
};
};
