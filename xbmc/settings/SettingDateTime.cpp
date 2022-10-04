/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingDateTime.h"

#include "XBDateTime.h"
#include "utils/TimeUtils.h"

#include <shared_mutex>

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
  std::shared_lock<CSharedSection> lock(m_critical);

  if (!CSettingString::CheckValidity(value))
    return false;

  return CDateTime::FromDBDate(value).IsValid();
}

CDateTime CSettingDate::GetDate() const
{
  return CDateTime::FromDBDate(GetValue());
}

bool CSettingDate::SetDate(const CDateTime& date)
{
  return SetValue(date.GetAsDBDate());
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
  std::shared_lock<CSharedSection> lock(m_critical);

  if (!CSettingString::CheckValidity(value))
    return false;

  return CDateTime::FromDBTime(value).IsValid();
}

CDateTime CSettingTime::GetTime() const
{
  return CDateTime::FromDBTime(GetValue());
}

bool CSettingTime::SetTime(const CDateTime& time)
{
  return SetValue(CTimeUtils::WithoutSeconds(time.GetAsDBTime()));
}
