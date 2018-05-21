#pragma once
/*
 *      Copyright (C) 2012-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "guilib/guiinfo/GUIInfoProvider.h"

#include "utils/Temperature.h"

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{

class CGUIInfo;

class CSystemGUIInfo : public CGUIInfoProvider
{
public:
  CSystemGUIInfo();
  ~CSystemGUIInfo() override = default;

  // KODI::GUILIB::GUIINFO::IGUIInfoProvider implementation
  bool InitCurrentItem(CFileItem *item) override;
  bool GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const override;
  bool GetInt(int& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const override;
  bool GetBool(bool& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const override;

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
} // namespace GUILIB
} // namespace KODI
