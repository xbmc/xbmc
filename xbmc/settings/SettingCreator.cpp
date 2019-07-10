/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingCreator.h"

#include "settings/SettingAddon.h"
#include "settings/SettingDateTime.h"
#include "settings/SettingPath.h"
#include "utils/StringUtils.h"

std::shared_ptr<CSetting> CSettingCreator::CreateSetting(const std::string &settingType, const std::string &settingId, CSettingsManager *settingsManager /* = nullptr */) const
{
  if (StringUtils::EqualsNoCase(settingType, "addon"))
    return std::make_shared<CSettingAddon>(settingId, settingsManager);
  else if (StringUtils::EqualsNoCase(settingType, "path"))
    return std::make_shared<CSettingPath>(settingId, settingsManager);
  else if (StringUtils::EqualsNoCase(settingType, "date"))
    return std::make_shared<CSettingDate>(settingId, settingsManager);
  else if (StringUtils::EqualsNoCase(settingType, "time"))
    return std::make_shared<CSettingTime>(settingId, settingsManager);

  return nullptr;
}
