/*
 *      Copyright (C) 2017 Team XBMC
 *      http://kodi.tv
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
