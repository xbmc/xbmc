/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/guiinfo/GUIInfoProvider.h"
#include "utils/GpuInfo.h"
#include "utils/Temperature.h"

#include <memory>

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

  float GetFPS() const { return m_fps; }
  void UpdateFPS();

private:
  std::string GetSystemHeatInfo(int info) const;

  std::unique_ptr<CGPUInfo> m_gpuInfo;

  static const int SYSTEM_HEAT_UPDATE_INTERVAL = 60000;

  mutable unsigned int m_lastSysHeatInfoTime;
  mutable CTemperature m_gpuTemp;
  mutable CTemperature m_cpuTemp;
  int m_fanSpeed = 0;
  float m_fps = 0.0;
  unsigned int m_frameCounter = 0;
  unsigned int m_lastFPSTime = 0;
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
