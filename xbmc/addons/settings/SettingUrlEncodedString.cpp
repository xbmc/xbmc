/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingUrlEncodedString.h"

#include "URL.h"
#include "settings/lib/SettingsManager.h"

namespace ADDON
{

CSettingUrlEncodedString::CSettingUrlEncodedString(
    const std::string& id, CSettingsManager* settingsManager /* = nullptr */)
  : CSettingString(id, settingsManager)
{ }

CSettingUrlEncodedString::CSettingUrlEncodedString(
    const std::string& id,
    int label,
    const std::string& value,
    CSettingsManager* settingsManager /* = nullptr */)
  : CSettingString(id, label, value, settingsManager)
{ }

CSettingUrlEncodedString::CSettingUrlEncodedString(const std::string &id, const CSettingUrlEncodedString &setting)
  : CSettingString(id, setting)
{ }

std::string CSettingUrlEncodedString::GetDecodedValue() const
{
  return CURL::Decode(CSettingString::GetValue());
}

bool CSettingUrlEncodedString::SetDecodedValue(const std::string &decodedValue)
{
  return CSettingString::SetValue(CURL::Encode(decodedValue));
}

} /* namespace ADDON */
