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
#include <stdexcept>
#include <string>

// Version when the migration system was added. Early exit for upgrades to that version or lower
constexpr int VERSION_BEFORE_MIGRATION_SYSTEM = 2;

namespace
{
class CSettingsMigrationToV3 : public ISettingsMigrationStep
{
public:
  int TargetVersion() const override { return 3; }
  bool Apply(TiXmlElement* root) override
  {
    constexpr std::string_view oldSettingId = "dvds.autorun";
    constexpr std::string_view newSettingId = "dvds.autoaction";

    return CSettingsMigration::SettingConversionResult::FAILURE !=
           CSettingsMigration::ConvertSettingBoolToInt(root, oldSettingId, newSettingId,
                                                       {.m_default = 0, .m_false = 0, .m_true = 1});
  }
};

CSettingsMigration::StepList BuildMigrationSteps()
{
  return {
      std::make_shared<CSettingsMigrationToV3>(),
  };
}
} // namespace

CSettingsMigration::CSettingsMigration() : CSettingsMigration(BuildMigrationSteps())
{
}

CSettingsMigration::CSettingsMigration(StepList steps)
{
  m_steps = std::move(steps);
  std::ranges::sort(m_steps, {}, &ISettingsMigrationStep::TargetVersion);

  if (m_steps.end() !=
      std::ranges::adjacent_find(m_steps, {}, &ISettingsMigrationStep::TargetVersion))
    throw std::invalid_argument("Multiple steps for the same target version are forbidden");
}

bool CSettingsMigration::UpdateXMLSettings(TiXmlElement* root,
                                           int currentVersion,
                                           int targetVersion)
{
  if (targetVersion > VERSION_BEFORE_MIGRATION_SYSTEM && currentVersion < targetVersion)
  {
    //! @todo would be nice to include the filename or something to identify more precisely in the log
    CLog::LogF(LOGDEBUG, "upgrading settings from version {} to {}", currentVersion, targetVersion);

    const int firstTarget = currentVersion + 1;
    auto itFirst =
        std::ranges::lower_bound(m_steps, firstTarget, {}, &ISettingsMigrationStep::TargetVersion);
    auto itLast = std::ranges::upper_bound(m_steps, targetVersion, {},
                                           &ISettingsMigrationStep::TargetVersion);

    if (itFirst == itLast)
    {
      CLog::LogF(LOGDEBUG, "no migration steps available from version {} to {}", currentVersion,
                 targetVersion);
      return true;
    }

    // All steps must succeed, stop on first error
    return std::none_of(itFirst, itLast, [root](const std::shared_ptr<ISettingsMigrationStep>& step)
                        { return !step->Apply(root); });
  }

  return true;
}

CSettingsMigration::SettingConversionResult CSettingsMigration::ConvertSettingBoolToInt(
    TiXmlElement* root,
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
