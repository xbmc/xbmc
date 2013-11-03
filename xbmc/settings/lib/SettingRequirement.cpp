/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
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

#include "SettingRequirement.h"
#include "SettingsManager.h"

bool CSettingRequirementCondition::Check() const
{
  if (m_settingsManager == NULL)
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

CSettingRequirement::CSettingRequirement(CSettingsManager *settingsManager /* = NULL */)
  : CSettingCondition(settingsManager)
{
  m_operation = CBooleanLogicOperationPtr(new CSettingRequirementConditionCombination(m_settingsManager));
}
