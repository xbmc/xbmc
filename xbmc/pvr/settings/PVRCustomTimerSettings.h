/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_timers.h" // PVR_TIMER_STATE
#include "pvr/timers/PVRTimerInfoTag.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

class CSetting;
class CSettingGroup;
struct IntegerSettingOption;
struct StringSettingOption;

namespace PVR
{
class CPVRTimerSettingDefinition;
class CPVRTimerType;
class IPVRSettingsContainer;

class CPVRCustomTimerSettings
{
public:
  CPVRCustomTimerSettings(const CPVRTimerType& timerType,
                          const CPVRTimerInfoTag::CustomPropsMap& customProps,
                          const std::map<int, std::shared_ptr<CPVRTimerType>>& typeEntries);
  virtual ~CPVRCustomTimerSettings() = default;

  void SetTimerType(const CPVRTimerType& timerType);

  void AddSettings(IPVRSettingsContainer& settingsContainer,
                   const std::shared_ptr<CSettingGroup>& group);

  bool IsCustomSetting(const std::string& settingId) const;
  bool IsCustomIntSetting(const std::string& settingId) const;
  bool IsCustomStringSetting(const std::string& settingId) const;

  const CPVRTimerInfoTag::CustomPropsMap& GetProperties() const { return m_customProps; }

  bool UpdateIntProperty(const std::shared_ptr<const CSetting>& setting);
  bool UpdateStringProperty(const std::shared_ptr<const CSetting>& setting);

  std::string GetSettingsLabel(const std::string& settingId) const;

  bool IntSettingDefinitionsFiller(const std::string& settingId,
                                   std::vector<IntegerSettingOption>& list,
                                   int& current);
  bool StringSettingDefinitionsFiller(const std::string& settingId,
                                      std::vector<StringSettingOption>& list,
                                      std::string& current);

  bool IsSettingReadonlyForTimerState(const std::string& settingId,
                                      PVR_TIMER_STATE timerState) const;
  bool IsSettingSupportedForTimerType(const std::string& setting,
                                      const CPVRTimerType& timerType) const;

private:
  std::shared_ptr<const CPVRTimerSettingDefinition> GetSettingDefintion(
      const std::string& settingId) const;

  using CustomSettingDefinitionsVector = std::vector<
      std::pair<std::string,
                std::shared_ptr<const CPVRTimerSettingDefinition>>>; // setting id, setting def

  CustomSettingDefinitionsVector m_customSettingDefs;
  CPVRTimerInfoTag::CustomPropsMap m_customProps;
};
} // namespace PVR
