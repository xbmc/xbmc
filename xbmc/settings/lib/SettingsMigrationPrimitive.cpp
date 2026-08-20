/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingsMigrationPrimitive.h"

#include "Setting.h"

namespace KODI::SETTINGS
{
SettingConversionResult ConvertSettingBoolToInt(TiXmlElement* root,
                                                std::string_view oldSettingId,
                                                std::string_view newSettingId,
                                                const SettingBoolToIntMapping& mapping)
{
  return impl::ConvertSingleSetting<CSettingBool, CSettingInt>(
      root, oldSettingId, newSettingId,
      [&mapping](bool oldValue)
      {
        const int newValue = oldValue ? mapping.m_true : mapping.m_false;
        return std::pair{newValue, mapping.m_default};
      });
}
} // namespace KODI::SETTINGS
