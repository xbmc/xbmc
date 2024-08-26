/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRTimerSettingDefinition.h"

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_general.h"
#include "utils/Variant.h"
#include "utils/log.h"

namespace PVR
{
std::vector<std::shared_ptr<const CPVRTimerSettingDefinition>> CPVRTimerSettingDefinition::
    CreateSettingDefinitionsList(int clientId,
                                 unsigned int timerTypeId,
                                 struct PVR_SETTING_DEFINITION** defs,
                                 unsigned int settingDefsSize)
{
  std::vector<std::shared_ptr<const CPVRTimerSettingDefinition>> defsList;
  if (defs && settingDefsSize > 0)
  {
    defsList.reserve(settingDefsSize);
    for (unsigned int i = 0; i < settingDefsSize; ++i)
    {
      const PVR_SETTING_DEFINITION* def{defs[i]};
      if (def)
      {
        defsList.emplace_back(
            std::make_shared<const CPVRTimerSettingDefinition>(clientId, timerTypeId, *def));
      }
    }
  }
  return defsList;
}

CPVRTimerSettingDefinition::CPVRTimerSettingDefinition(int clientId,
                                                       unsigned int timerTypeId,
                                                       const PVR_SETTING_DEFINITION& def)
  : m_clientId(clientId),
    m_timerTypeId(timerTypeId),
    m_id(def.iId),
    m_name(def.strName ? def.strName : ""),
    m_type(def.eType),
    m_readonlyConditions(def.iReadonlyConditions)
{
  if (def.intSettingDefinition)
    m_intDefinition = CPVRIntSettingDefinition{*def.intSettingDefinition};
  if (def.stringSettingDefinition)
    m_stringDefinition = CPVRStringSettingDefinition{*def.stringSettingDefinition};
}

bool CPVRTimerSettingDefinition::operator==(const CPVRTimerSettingDefinition& right) const
{
  return (m_id == right.m_id && m_name == right.m_name && m_type == right.m_type &&
          m_readonlyConditions == right.m_readonlyConditions &&
          m_intDefinition == right.m_intDefinition &&
          m_stringDefinition == right.m_stringDefinition);
}

bool CPVRTimerSettingDefinition::operator!=(const CPVRTimerSettingDefinition& right) const
{
  return !(*this == right);
}

CVariant CPVRTimerSettingDefinition::GetDefaultValue() const
{
  if (GetType() == PVR_SETTING_TYPE::INTEGER)
    return CVariant{GetIntDefinition().GetDefaultValue()};
  else if (GetType() == PVR_SETTING_TYPE::STRING)
    return CVariant{GetStringDefinition().GetDefaultValue()};

  CLog::LogF(LOGERROR, "Unknown setting type for custom property");
  return {};
}

bool CPVRTimerSettingDefinition::IsReadonlyForTimerState(PVR_TIMER_STATE timerState) const
{
  switch (timerState)
  {
    case PVR_TIMER_STATE_DISABLED:
      return m_readonlyConditions & PVR_SETTING_READONLY_CONDITION_TIMER_DISABLED;
    case PVR_TIMER_STATE_SCHEDULED:
    case PVR_TIMER_STATE_CONFLICT_OK:
    case PVR_TIMER_STATE_CONFLICT_NOK:
      return m_readonlyConditions & PVR_SETTING_READONLY_CONDITION_TIMER_SCHEDULED;
    case PVR_TIMER_STATE_RECORDING:
      return m_readonlyConditions & PVR_SETTING_READONLY_CONDITION_TIMER_RECORDING;
    case PVR_TIMER_STATE_COMPLETED:
    case PVR_TIMER_STATE_ABORTED:
    case PVR_TIMER_STATE_CANCELLED:
    case PVR_TIMER_STATE_ERROR:
      return m_readonlyConditions & PVR_SETTING_READONLY_CONDITION_TIMER_COMPLETED;
    default:
      CLog::LogF(LOGWARNING, "Unhandled timer state {}", timerState);
      break;
  }
  return false;
}
} // namespace PVR
