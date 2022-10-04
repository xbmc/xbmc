/*
 *  Copyright (C) 2017-2018 Team Kodi
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
    CSettingUrlEncodedString(const std::string& id, CSettingsManager* settingsManager = nullptr);
    CSettingUrlEncodedString(const std::string& id,
                             int label,
                             const std::string& value,
                             CSettingsManager* settingsManager = nullptr);
    CSettingUrlEncodedString(const std::string &id, const CSettingUrlEncodedString &setting);
    ~CSettingUrlEncodedString() override = default;

    SettingPtr Clone(const std::string &id) const override { return std::make_shared<CSettingUrlEncodedString>(id, *this); }

    std::string GetDecodedValue() const;
    bool SetDecodedValue(const std::string& decodedValue);
  };
}
