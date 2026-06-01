/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingsMigration.h"

#include "utils/log.h"

constexpr int VERSION_LEGACY = 2; // Last version before this migration system was added

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