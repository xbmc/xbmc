/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
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
