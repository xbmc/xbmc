/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingsMigrationStep.h"
#include "utils/XBMCTinyXML.h"

#include <algorithm>
#include <memory>
#include <string_view>
#include <vector>

class CSettingsMigration
{
public:
  // The constructors throw for invalid lists of steps
  CSettingsMigration();
  CSettingsMigration(KODI::SETTINGS::MigrationStepList steps);

  /*!
   * \brief Upgrade the settings contained in \p root from \p fromVersion to \p targetVersion.
   *        Multiple steps will be executed automatically when \p fromVersion is more than one
   *        version away from \p targetVersion.
   * \param[in,out] root Settings XML representation.
   * \param[in] fromVersion Version to upgrade from.
   * \param[in] targetVersion Version to upgrade to.
   * \return False for catastrophic failure (for example \p root left in an inconsistent state due
   *         to out of memory. A setting that doesn't convert properly due to an invalid value is
   *         not a catastrophic failure: the situation is logged and the setting receives its
   *         default value). True otherwise.
   */
  bool UpdateXMLSettings(TiXmlElement* root, int currentVersion, int targetVersion);

private:
  KODI::SETTINGS::MigrationStepList m_steps; // steps sorted by TargetVersion() in constructor
};
