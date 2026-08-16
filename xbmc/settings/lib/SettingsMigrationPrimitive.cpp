/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingsMigrationPrimitive.h"

#include "Setting.h"
#include "settings/SettingsValueXmlSerializer.h"
#include "settings/lib/SettingsManager.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <memory>
#include <stdexcept>
#include <string>

namespace KODI::SETTINGS
{
SettingConversionResult ConvertSettingBoolToInt(TiXmlElement* root,
                                                std::string_view oldSettingId,
                                                std::string_view newSettingId,
                                                const SettingBoolToIntMapping& mapping)
{
  if (TiXmlElement* elem = CSettingsManager::LocateSetting(root, oldSettingId); elem != nullptr)
  {
    //! maybe future @todo: strategy - read/write settings without dependency on CSetting* classes?
    auto oldSetting = std::make_shared<CSettingBool>(oldSettingId, nullptr);
    if (oldSetting == nullptr)
      return SettingConversionResult::FAILURE;

    const std::string oldValue = elem->FirstChild() ? elem->FirstChild()->ValueStr() : "";

    if (!oldSetting->FromString(oldValue))
    {
      CLog::Log(LOGWARNING,
                "Settings conversion: unable to load the value of the old "
                "setting \"{}\": \"{}\". "
                "The new setting \"{}\" will have its default value.",
                oldSettingId, oldValue, newSettingId);
      return SettingConversionResult::INVALID;
    }

    // Map to int setting values
    const int newValue = oldSetting->GetValue() ? mapping.m_true : mapping.m_false;

    // Prepare a new setting
    auto newSetting = std::make_shared<CSettingInt>(newSettingId, nullptr);
    if (newSetting == nullptr)
      return SettingConversionResult::FAILURE;

    newSetting->SetDefault(mapping.m_default);
    newSetting->SetValue(newValue);

    // Add the new setting and remove the old one
    // The new setting doesn't have to be in the same place in the file as the old one.
    if (!CSettingsValueXmlSerializer::SerializeSetting(root, newSetting) ||
        !XMLUtils::RemoveNode(elem))
      return SettingConversionResult::FAILURE;

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
} // namespace KODI::SETTINGS
