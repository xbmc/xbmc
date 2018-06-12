/*
 *      Copyright (C) 2014 Team XBMC
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

#pragma once

#include <set>
#include <string>

#include "settings/lib/SettingConditions.h"

class CProfile;
class CProfilesManager;

class CSettingConditions
{
public:
  static void Initialize(const CProfilesManager &profileManager);
  static void Deinitialize();

  static const CProfile& GetCurrentProfile();

  static const std::set<std::string>& GetSimpleConditions() { return m_simpleConditions; }
  static const std::map<std::string, SettingConditionCheck>& GetComplexConditions() { return m_complexConditions; }

  static bool Check(const std::string &condition, const std::string &value = "", std::shared_ptr<const CSetting> setting = NULL);

private:
  // Initialization parameters
  static const CProfilesManager *m_profileManager;

  static std::set<std::string> m_simpleConditions;
  static std::map<std::string, SettingConditionCheck> m_complexConditions;
};
