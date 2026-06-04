/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingsMigration.h"

#include "Setting.h"
#include "SettingsManager.h"
#include "settings/SettingsValueXmlSerializer.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <memory>
#include <string>

constexpr int VERSION_LEGACY = 2; // Last version before this migration system was added

CSettingsMigration::SettingConversionResult CSettingsMigration::ConvertSettingBoolToInt(
    TiXmlElement* root,
    std::string_view oldSettingId,
    std::string_view newSettingId,
    const SettingBoolToIntMapping& mapping)
{
  // Skip conversion in case the new setting already exists
  if (TiXmlElement* elem = CSettingsManager::LocateSetting(root, newSettingId); elem != nullptr)
  {
    CLog::Log(
        LOGWARNING,
        "Settings conversion: unexpectedly found the new setting \"{}\" - skipping conversion.",
        newSettingId);
    return SettingConversionResult::ALREADY_EXISTS;
  }

  if (TiXmlElement* elem = CSettingsManager::LocateSetting(root, oldSettingId); elem != nullptr)
  {
    //! maybe future @todo: strategy - read/write settings without dependency on CSetting* classes?
    auto oldSetting = std::make_shared<CSettingBool>(oldSettingId, nullptr);
    const std::string oldValue = elem->FirstChild() ? elem->FirstChild()->ValueStr() : "";

    if (!oldSetting->FromString(oldValue))
    {
      CLog::Log(LOGWARNING,
                "Settings conversion: unable to load the value of the old setting \"{}\": \"{}\". "
                "The new setting \"{}\" will have its default value.",
                oldSettingId, oldValue, newSettingId);
      return SettingConversionResult::INVALID;
    }

    // Map to int setting values
    const int newValue = oldSetting->GetValue() ? mapping.m_true : mapping.m_false;

    // Prepare a new setting
    auto newSetting = std::make_shared<CSettingInt>(newSettingId, nullptr);
    newSetting->SetDefault(mapping.m_default);
    newSetting->SetValue(newValue);

    // Add the new setting and remove the old one
    // The new setting doesn't have to be in the same place in the file as the old one.
    CSettingsValueXmlSerializer::SerializeSetting(root, newSetting);
    XMLUtils::RemoveNode(elem);

    CLog::LogF(LOGDEBUG,
               "Successful conversion of old setting \"{}\" / \"{}\" to new "
               "setting \"{}\" / \"{}\".",
               oldSettingId, oldValue, newSettingId, newValue);
    return SettingConversionResult::CONVERTED;
  }

  CLog::LogF(LOGDEBUG,
             "Old setting \"{}\" not found. The new setting \"{}\" will have "
             "its default value.",
             oldSettingId, newSettingId);

  return SettingConversionResult::NOT_PRESENT;
}

bool CSettingsMigration::UpdateXMLSettings(TiXmlElement* root,
                                           int currentVersion,
                                           int targetVersion,
                                           bool& updated)
{
  if (targetVersion > VERSION_LEGACY && currentVersion < targetVersion)
  {
    //! @todo would be nice to include the filename or something to identify more precisely in the log
    CLog::LogF(LOGDEBUG, "upgrading settings from version {} to {}", currentVersion, targetVersion);
    return Upgrade(root, currentVersion, updated);
  }

  return true;
}

bool CSettingsMigration::Upgrade(TiXmlElement* root, int currentVersion, bool& updated)
{
  // Return values of the conversions are ignored because they have no influence.
  // Setting upgrades failures are non-blocking, unlike database upgrades. In the worst case, the
  // new setting will have a default value instead of a converted old setting.
  // The conversion functions are responsible of the problems logging.

  return true;
}