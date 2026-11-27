/*
 *  Copyright (C) 2022-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/guiinfo/GUIInfoProvider.h"

class CGUIInfoManager;

namespace KODI
{
namespace SMART_HOME
{
class IPowerMeterHUD;
class ISystemHealthHUD;
class IVehicleHUD;

class CSmartHomeGuiInfo : public GUILIB::GUIINFO::IGUIInfoProvider
{
public:
  CSmartHomeGuiInfo(CGUIInfoManager& infoManager,
                    ISystemHealthHUD& systemHealthHud,
                    IPowerMeterHUD& powerMeterHud,
                    IVehicleHUD& vehicleHud);
  ~CSmartHomeGuiInfo() override;

  void Initialize();
  void Deinitialize();

  // Implementation of IGUIInfoProvider
  bool InitCurrentItem(CFileItem* item) override { return false; }
  bool GetLabel(std::string& value,
                const CFileItem* item,
                int contextWindow,
                const GUILIB::GUIINFO::CGUIInfo& info,
                std::string* fallback) const override;
  bool GetFallbackLabel(std::string& value,
                        const CFileItem* item,
                        int contextWindow,
                        const GUILIB::GUIINFO::CGUIInfo& info,
                        std::string* fallback) override
  {
    return false;
  }
  bool GetInt(int& value,
              const CGUIListItem* item,
              int contextWindow,
              const GUILIB::GUIINFO::CGUIInfo& info) const override;
  bool GetBool(bool& value,
               const CGUIListItem* item,
               int contextWindow,
               const GUILIB::GUIINFO::CGUIInfo& info) const override;
  void UpdateAVInfo(const AudioStreamInfo& audioInfo,
                    const VideoStreamInfo& videoInfo,
                    const SubtitleStreamInfo& subtitleInfo) override
  {
  }

private:
  // Construction parameters
  CGUIInfoManager& m_infoManager;
  ISystemHealthHUD& m_systemHealthHud;
  IPowerMeterHUD& m_powerMeterHud;
  IVehicleHUD& m_vehicleHud;
};
} // namespace SMART_HOME
} // namespace KODI
