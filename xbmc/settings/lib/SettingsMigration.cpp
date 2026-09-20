/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "settings/lib/SettingsMigration.h"

#include "settings/lib/SettingsMigrationSteps.h"
#include "utils/log.h"

#include <algorithm>
#include <stdexcept>

// Version when the migration system was added. Early exit for upgrades to that version or lower
constexpr int VERSION_BEFORE_MIGRATION_SYSTEM = 2;

using namespace KODI::SETTINGS;

CSettingsMigration::CSettingsMigration() : CSettingsMigration(BuildMigrationSteps())
{
}

CSettingsMigration::CSettingsMigration(MigrationStepList steps)
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
