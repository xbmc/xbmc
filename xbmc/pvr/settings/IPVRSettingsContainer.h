/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

class CSettingGroup;

namespace PVR
{
class IPVRSettingsContainer
{
public:
  virtual ~IPVRSettingsContainer() = default;

  virtual void AddMultiIntSetting(const std::shared_ptr<CSettingGroup>& group,
                                  const std::string& settingName,
                                  int settingValue)
  {
  }
  virtual void AddSingleIntSetting(const std::shared_ptr<CSettingGroup>& group,
                                   const std::string& settingName,
                                   int settingValue,
                                   int minValue,
                                   int step,
                                   int maxValue)
  {
  }
  virtual void AddMultiStringSetting(const std::shared_ptr<CSettingGroup>& group,
                                     const std::string& settingName,
                                     const std::string& settingValue)
  {
  }
  virtual void AddSingleStringSetting(const std::shared_ptr<CSettingGroup>& group,
                                      const std::string& settingName,
                                      const std::string& settingValue,
                                      bool allowEmptyValue)
  {
  }
};
} // namespace PVR
