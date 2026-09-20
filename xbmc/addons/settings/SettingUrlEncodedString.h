/*
 *  Copyright (C) 2017-2018, 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/Setting.h"

class CSettingsManager;

namespace ADDON
{
  class CSettingUrlEncodedString : public CSettingString
  {
  public:
    explicit CSettingUrlEncodedString(std::string_view id,
                                      CSettingsManager* settingsManager = nullptr);
    CSettingUrlEncodedString(std::string_view id,
                             int label,
                             const std::string& value,
                             CSettingsManager* settingsManager = nullptr);
    CSettingUrlEncodedString(std::string_view id, const CSettingUrlEncodedString& setting);
    ~CSettingUrlEncodedString() override = default;

    SettingPtr Clone(std::string_view id) const override
    {
      return std::make_shared<CSettingUrlEncodedString>(id, *this);
    }

    std::string GetDecodedValue() const;
    bool SetDecodedValue(const std::string& decodedValue);
  };
}
