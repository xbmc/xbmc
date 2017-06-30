#pragma once
/*
 *      Copyright (C) 2017 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "settings/lib/Setting.h"

namespace ADDON
{
  class CSettingUrlEncodedString : public CSettingString
  {
  public:
    CSettingUrlEncodedString(const std::string &id, CSettingsManager *settingsManager = NULL);
    CSettingUrlEncodedString(const std::string &id, int label, const std::string &value, CSettingsManager *settingsManager = NULL);
    CSettingUrlEncodedString(const std::string &id, const CSettingUrlEncodedString &setting);
    ~CSettingUrlEncodedString() override { }

    SettingPtr Clone(const std::string &id) const override { return std::make_shared<CSettingUrlEncodedString>(id, *this); }

    std::string GetDecodedValue() const;
    bool SetDecodedValue(const std::string& decodedValue);
  };
}
