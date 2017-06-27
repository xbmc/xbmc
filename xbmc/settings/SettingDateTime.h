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

#include "XBDateTime.h"
#include "settings/lib/Setting.h"

class CSettingDate : public CSettingString
{
public:
  CSettingDate(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSettingDate(const std::string &id, int label, const std::string &value, CSettingsManager *settingsManager = NULL);
  CSettingDate(const std::string &id, const CSettingDate &setting);
  ~CSettingDate() override = default;

  SettingPtr Clone(const std::string &id) const override;

  bool CheckValidity(const std::string &value) const override;

  CDateTime GetDate() const { return CDateTime::FromDBDate(GetValue()); }
  bool SetDate(const CDateTime& date) { return SetValue(date.GetAsDBDate()); }
};

class CSettingTime : public CSettingString
{
public:
  CSettingTime(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSettingTime(const std::string &id, int label, const std::string &value, CSettingsManager *settingsManager = NULL);
  CSettingTime(const std::string &id, const CSettingTime &setting);
  ~CSettingTime() override = default;

  SettingPtr Clone(const std::string &id) const override;

  bool CheckValidity(const std::string &value) const override;

  CDateTime GetTime() const { return CDateTime::FromDBTime(GetValue()); }
  bool SetTime(const CDateTime& time) { return SetValue(time.GetAsDBTime()); }
};
