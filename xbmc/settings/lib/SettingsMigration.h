/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/XBMCTinyXML.h"

#include <string_view>

class TestConversions;

class CSettingsMigration
{
public:
  CSettingsMigration() = delete;

  static bool UpdateXMLSettings(TiXmlElement* root,
                                int currentVersion,
                                int targetVersion,
                                bool& updated);

private:
  friend class TestConversions;

  enum class SettingConversionResult
  {
    NOT_PRESENT, // the old setting cannot be found
    INVALID, // the old setting has an invalid value for its data type
    CONVERTED, // successful conversion from old to new setting
    ALREADY_EXISTS, // the new setting is already present
  };

  struct SettingBoolToIntMapping
  {
    int m_default;
    int m_false;
    int m_true;
  };

  static SettingConversionResult ConvertSettingBoolToInt(TiXmlElement* root,
                                                         std::string_view oldSettingId,
                                                         std::string_view newSettingId,
                                                         const SettingBoolToIntMapping& mapping);

  static bool Upgrade(TiXmlElement* root, int currentVersion, bool& updated);
};
