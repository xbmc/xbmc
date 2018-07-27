/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingCreator.h"

class CSettingCreator : public ISettingCreator
{
public:
  // implementation of ISettingCreator
  std::shared_ptr<CSetting> CreateSetting(const std::string &settingType, const std::string &settingId, CSettingsManager *settingsManager = nullptr) const override;

protected:
  CSettingCreator() = default;
  ~CSettingCreator() override = default;
};
