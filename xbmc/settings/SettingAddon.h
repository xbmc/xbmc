/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/Setting.h"
#include "addons/IAddon.h"

class CSettingAddon : public CSettingString
{
public:
  CSettingAddon(const std::string &id, CSettingsManager *settingsManager = nullptr);
  CSettingAddon(const std::string &id, int label, const std::string &value, CSettingsManager *settingsManager = nullptr);
  CSettingAddon(const std::string &id, const CSettingAddon &setting);
  ~CSettingAddon() override = default;

  SettingPtr Clone(const std::string &id) const override;

  bool Deserialize(const TiXmlNode *node, bool update = false) override;

  ADDON::TYPE GetAddonType() const { return m_addonType; }
  void SetAddonType(ADDON::TYPE addonType) { m_addonType = addonType; }

private:
  void copyaddontype(const CSettingAddon &setting);

  ADDON::TYPE m_addonType = ADDON::ADDON_UNKNOWN;
};
