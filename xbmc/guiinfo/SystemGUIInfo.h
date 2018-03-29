#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "guiinfo/GUIInfoProvider.h"

#include "utils/Temperature.h"

namespace GUIINFO
{

class GUIInfo;

class CSystemGUIInfo : public CGUIInfoProvider
{
public:
  CSystemGUIInfo();
  ~CSystemGUIInfo() override = default;

  // GUIINFO::IGUIInfoProvider implementation
  bool InitCurrentItem(CFileItem *item) override;
  bool GetLabel(std::string& value, const CFileItem *item, const GUIInfo &info, std::string *fallback) const override;
  bool GetInt(int& value, const CGUIListItem *item, const GUIInfo &info) const override;
  bool GetBool(bool& value, const CGUIListItem *item, const GUIInfo &info) const override;

  float GetFPS() const { return m_fps; };
  void UpdateFPS();

private:
  std::string GetSystemHeatInfo(int info) const;
  CTemperature GetGPUTemperature() const;

  static const int SYSTEM_HEAT_UPDATE_INTERVAL = 60000;

  mutable unsigned int m_lastSysHeatInfoTime;
  mutable CTemperature m_gpuTemp;
  mutable CTemperature m_cpuTemp;
  int m_fanSpeed;
  float m_fps;
  unsigned int m_frameCounter;
  unsigned int m_lastFPSTime;
};

} // namespace GUIINFO
