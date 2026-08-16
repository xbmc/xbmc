/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingsMigrationSteps.h"

#include "SettingsMigrationPrimitive.h"

#include <string_view>

namespace KODI::SETTINGS
{

using enum SettingConversionResult;

class CSettingsMigrationToV3 : public ISettingsMigrationStep
{
public:
  int TargetVersion() const override { return 3; }
  bool Apply(TiXmlElement* root) override
  {
    constexpr std::string_view oldSettingId = "dvds.autorun";
    constexpr std::string_view newSettingId = "dvds.autoaction";

    return FAILURE != ConvertSettingBoolToInt(root, oldSettingId, newSettingId,
                                              {.m_default = 0, .m_false = 0, .m_true = 1});
  }
};

MigrationStepList BuildMigrationSteps()
{
  return {
      std::make_shared<CSettingsMigrationToV3>(),
  };
}

} // namespace KODI::SETTINGS
