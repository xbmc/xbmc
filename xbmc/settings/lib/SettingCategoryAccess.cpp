/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingCategoryAccess.h"
#include "SettingConditions.h"
#include "SettingsManager.h"

bool CSettingCategoryAccessCondition::Check() const
{
  if (m_value.empty())
    return true;

  if (m_settingsManager == nullptr)
    return false;

  bool found = m_settingsManager->GetConditions().Check(m_value, "true");
  if (m_negated)
    return !found;

  return found;
}

bool CSettingCategoryAccessConditionCombination::Check() const
{
  if (m_operations.empty() && m_values.empty())
    return true;

  return CSettingConditionCombination::Check();
}

CSettingCategoryAccess::CSettingCategoryAccess(CSettingsManager *settingsManager /* = nullptr */)
  : CSettingCondition(settingsManager)
{
  m_operation = CBooleanLogicOperationPtr(new CSettingCategoryAccessConditionCombination(m_settingsManager));
}
