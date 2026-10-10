/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/StringUtils.h"

#include <string_view>

/*!
 \ingroup settings
 \brief Levels which every setting is assigned to.
 */
enum class SettingLevel {
  Basic = 0,
  Standard,
  Advanced,
  Expert,
  Internal
};

/*!
 \ingroup settings
 \brief Names a setting level, or nullptr for Internal, which is not a level a viewer can be at
 */
inline const char* SettingLevelToString(SettingLevel level)
{
  switch (level)
  {
    case SettingLevel::Basic:
      return "basic";
    case SettingLevel::Standard:
      return "standard";
    case SettingLevel::Advanced:
      return "advanced";
    case SettingLevel::Expert:
      return "expert";
    default:
      return nullptr;
  }
}

/*!
 \ingroup settings
 \brief The level \p name names, ignoring case; Standard for any other name
 */
inline SettingLevel SettingLevelFromString(std::string_view name)
{
  if (StringUtils::EqualsNoCase(name, "basic"))
    return SettingLevel::Basic;
  if (StringUtils::EqualsNoCase(name, "advanced"))
    return SettingLevel::Advanced;
  if (StringUtils::EqualsNoCase(name, "expert"))
    return SettingLevel::Expert;

  return SettingLevel::Standard;
}
