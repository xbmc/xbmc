/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingRequirement.h"

#include "SettingsManager.h"

bool CSettingRequirementCondition::Check() const
{
  if (m_settingsManager == nullptr)
    return false;

  bool found = m_settingsManager->GetConditions().Check("IsDefined", m_value);
  if (m_negated)
    return !found;

  return found;
}

bool CSettingRequirementConditionCombination::Check() const
{
  if (m_operations.empty() && m_values.empty())
    return true;

  return CSettingConditionCombination::Check();
}

CSettingRequirement::CSettingRequirement(CSettingsManager *settingsManager /* = nullptr */)
  : CSettingCondition(settingsManager)
{
  m_operation = CBooleanLogicOperationPtr(new CSettingRequirementConditionCombination(m_settingsManager));
}
