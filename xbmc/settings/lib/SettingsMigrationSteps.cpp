/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingsMigrationSteps.h"

#include "SettingsMigrationPrimitive.h"

#include <array>
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

class CSettingsMigrationToV4 : public ISettingsMigrationStep
{
public:
  int TargetVersion() const override { return 4; }
  bool Apply(TiXmlElement* root) override
  {
    constexpr std::array<std::pair<std::string_view, std::string_view>, 2> settingIds{
        {// old, new
         {"musicplayer.replaygainpreamp", "musicplayer.replaygainpreampdb"},
         {"musicplayer.replaygainnogainpreamp", "musicplayer.replaygainnogainpreampdb"}}};
    bool ret = true;
    double defaultValue = 0.0;
    for (const auto& [oldSettingId, newSettingId] : settingIds)
      ret &=
          impl::ConvertSingleSetting<CSettingInt, CSettingNumber>(
              root, oldSettingId, newSettingId, [&defaultValue](int oldValue)
              { return std::pair{static_cast<double>(oldValue - 89), defaultValue}; }) != FAILURE;
    return ret;
  }
};

MigrationStepList BuildMigrationSteps()
{
  return {
      std::make_shared<CSettingsMigrationToV3>(),
      std::make_shared<CSettingsMigrationToV4>(),
  };
}

} // namespace KODI::SETTINGS
