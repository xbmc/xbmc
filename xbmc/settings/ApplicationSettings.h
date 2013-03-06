#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
 *      http://www.xbmc.org
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

#include <string>

#include "settings/ISubSettings.h"

#define VOLUME_MINIMUM 0.0f        // -60dB
#define VOLUME_MAXIMUM 1.0f        // 0dB

#define VOLUME_DYNAMIC_RANGE 90.0f // 60dB
#define VOLUME_CONTROL_STEPS 90    // 90 steps

class TiXmlNode;

class CApplicationSettings : public ISubSettings
{
public:
  static CApplicationSettings& Get();

  virtual bool Load(const TiXmlNode *settings);
  virtual bool Save(TiXmlNode *settings) const;
  virtual void Clear();

  const std::string& GetLogFolder() const { return m_logFolder; }
  void  SetLogFolder(const std::string logFolder) { m_logFolder = logFolder; }

  int   GetTotalSystemUpTime() const { return m_iSystemTimeTotalUp; }
  void  SetTotalSystemUpTime(int totalSystemUpTime) { m_iSystemTimeTotalUp = totalSystemUpTime; }
  const std::string& GetUserAgent() const { return m_userAgent; }

  bool AutoUpdateAddons() const { return m_bAddonAutoUpdate; }
  void SetAutoUpdateAddons(bool autoUpdate) { m_bAddonAutoUpdate = autoUpdate; }
  bool ShowAddonNotifications() const { return m_bAddonNotifications; }
  void SetShowAddonNotifications(bool notifications) { m_bAddonNotifications = notifications; }
  bool FilterForeignLanguageAddons() const { return m_bAddonForeignFilter; }
  void SetFilterForeignLanguageAddons(bool filter) { m_bAddonForeignFilter = filter; }

  float GetVolumeLevel() const { return m_fVolumeLevel; }
  void  SetVolumeLevel(float volumeLevel) { m_fVolumeLevel = volumeLevel; }
  bool  GetMute() const { return m_bMute; }
  void  SetMute(bool mute) { m_bMute = mute; }

protected:
  CApplicationSettings();
  CApplicationSettings(const CApplicationSettings&);
  CApplicationSettings const& operator=(CApplicationSettings const&);
  virtual ~CApplicationSettings();

private:
  std::string m_logFolder;
  float m_fVolumeLevel; // float 0.0 - 1.0 range
  bool m_bMute;
  int m_iSystemTimeTotalUp; // uptime in minutes!
  std::string m_userAgent;

  bool m_bAddonAutoUpdate;
  bool m_bAddonNotifications;
  bool m_bAddonForeignFilter;
};