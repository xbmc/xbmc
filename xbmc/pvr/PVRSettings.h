#pragma once
/*
 *      Copyright (C) 2015 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <map>
#include <string>
#include <utility>

#include "settings/lib/ISettingCallback.h"
#include "settings/lib/Setting.h"

namespace PVR
{
  class CPVRSettings : private ISettingCallback
  {
  public:
    CPVRSettings(const std::set<std::string> & settingNames);
    virtual ~CPVRSettings();

    // ISettingCallback implementation
    void OnSettingChanged(const CSetting *setting) override;

    bool GetBoolValue(const std::string &settingName) const;
    int GetIntValue(const std::string &settingName) const;
    std::string GetStringValue(const std::string &settingName) const;

    // settings value filler for start/end recording margin time for PVR timers.
    static void MarginTimeFiller(
      const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);

  private:
    CPVRSettings(const CPVRSettings&) = delete;
    CPVRSettings& operator=(CPVRSettings const&) = delete;

    void Init(const std::set<std::string> &settingNames);

    CCriticalSection m_critSection;
    std::map<std::string, SettingPtr> m_settings;
  };
}
