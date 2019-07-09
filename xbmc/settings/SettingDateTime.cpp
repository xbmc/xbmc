/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingDateTime.h"

#include "XBDateTime.h"

CSettingDate::CSettingDate(const std::string &id, CSettingsManager *settingsManager /* = NULL */)
  : CSettingString(id, settingsManager)
{ }

CSettingDate::CSettingDate(const std::string &id, int label, const std::string &value, CSettingsManager *settingsManager /* = NULL */)
  : CSettingString(id, label, value, settingsManager)
{ }

CSettingDate::CSettingDate(const std::string &id, const CSettingDate &setting)
  : CSettingString(id, setting)
{ }

SettingPtr CSettingDate::Clone(const std::string &id) const
{
  return std::make_shared<CSettingDate>(id, *this);
}

bool CSettingDate::CheckValidity(const std::string &value) const
{
  CSharedLock lock(m_critical);

  if (!CSettingString::CheckValidity(value))
    return false;

  return CDateTime::FromDBDate(value).IsValid();
}

CSettingTime::CSettingTime(const std::string &id, CSettingsManager *settingsManager /* = NULL */)
  : CSettingString(id, settingsManager)
{ }

CSettingTime::CSettingTime(const std::string &id, int label, const std::string &value, CSettingsManager *settingsManager /* = NULL */)
  : CSettingString(id, label, value, settingsManager)
{ }

CSettingTime::CSettingTime(const std::string &id, const CSettingTime &setting)
  : CSettingString(id, setting)
{ }

SettingPtr CSettingTime::Clone(const std::string &id) const
{
  return std::make_shared<CSettingTime>(id, *this);
}

bool CSettingTime::CheckValidity(const std::string &value) const
{
  CSharedLock lock(m_critical);

  if (!CSettingString::CheckValidity(value))
    return false;

  return CDateTime::FromDBTime(value).IsValid();
}
