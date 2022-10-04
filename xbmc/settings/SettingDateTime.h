/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/Setting.h"

class CDateTime;

class CSettingDate : public CSettingString
{
public:
  CSettingDate(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSettingDate(const std::string &id, int label, const std::string &value, CSettingsManager *settingsManager = NULL);
  CSettingDate(const std::string &id, const CSettingDate &setting);
  ~CSettingDate() override = default;

  SettingPtr Clone(const std::string &id) const override;

  bool CheckValidity(const std::string &value) const override;

  CDateTime GetDate() const;
  bool SetDate(const CDateTime& date);
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

  CDateTime GetTime() const;
  bool SetTime(const CDateTime& time);
};
