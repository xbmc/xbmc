/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_general.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_timers.h"
#include "pvr/PVRConstants.h" // PVR_CLIENT_INVALID_UID
#include "pvr/settings/PVRIntSettingDefinition.h"
#include "pvr/settings/PVRStringSettingDefinition.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class CVariant;

namespace PVR
{
class CPVRTimerSettingDefinition
{
public:
  static std::vector<std::shared_ptr<const CPVRTimerSettingDefinition>>
  CreateSettingDefinitionsList(int clientId,
                               unsigned int timerTypeId,
                               struct PVR_SETTING_DEFINITION** defs,
                               unsigned int settingDefsSize);

  CPVRTimerSettingDefinition(int clientId,
                             unsigned int timerTypeId,
                             const PVR_SETTING_DEFINITION& def);

  virtual ~CPVRTimerSettingDefinition() = default;

  bool operator==(const CPVRTimerSettingDefinition& right) const;
  bool operator!=(const CPVRTimerSettingDefinition& right) const;

  int GetClientId() const { return m_clientId; }
  unsigned int GetTimerTypeId() const { return m_timerTypeId; }
  unsigned int GetId() const { return m_id; }
  const std::string& GetName() const { return m_name; }
  PVR_SETTING_TYPE GetType() const { return m_type; }
  CVariant GetDefaultValue() const;

  bool IsIntDefinition() const { return m_type == PVR_SETTING_TYPE::INTEGER; }
  bool IsStringDefinition() const { return m_type == PVR_SETTING_TYPE::STRING; }
  const CPVRIntSettingDefinition& GetIntDefinition() const { return m_intDefinition; }
  const CPVRStringSettingDefinition& GetStringDefinition() const { return m_stringDefinition; }

  bool IsReadonlyForTimerState(PVR_TIMER_STATE timerState) const;

private:
  int m_clientId{PVR_CLIENT_INVALID_UID};
  unsigned int m_timerTypeId{PVR_TIMER_TYPE_NONE};
  unsigned int m_id{0};
  std::string m_name;
  PVR_SETTING_TYPE m_type{PVR_SETTING_TYPE::INTEGER};
  uint64_t m_readonlyConditions{PVR_SETTING_READONLY_CONDITION_NONE};
  CPVRIntSettingDefinition m_intDefinition;
  CPVRStringSettingDefinition m_stringDefinition;
};
} // namespace PVR
