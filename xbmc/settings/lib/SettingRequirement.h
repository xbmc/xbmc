#pragma once
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

#include <set>
#include <string>

#include "SettingConditions.h"

class CSettingRequirementCondition : public CSettingConditionItem
{
public:
  CSettingRequirementCondition(CSettingsManager *settingsManager = NULL)
    : CSettingConditionItem(settingsManager)
  { }
  virtual ~CSettingRequirementCondition() { }

  virtual bool Check() const;
};

class CSettingRequirementConditionCombination : public CSettingConditionCombination
{
public:
  CSettingRequirementConditionCombination(CSettingsManager *settingsManager = NULL)
    : CSettingConditionCombination(settingsManager)
  { }
  virtual ~CSettingRequirementConditionCombination() { }

  virtual bool Check() const;

private:
  virtual CBooleanLogicOperation* newOperation() { return new CSettingRequirementConditionCombination(m_settingsManager); }
  virtual CBooleanLogicValue* newValue() { return new CSettingRequirementCondition(m_settingsManager); }
};

class CSettingRequirement : public CSettingCondition
{
public:
  CSettingRequirement(CSettingsManager *settingsManager = NULL);
  virtual ~CSettingRequirement() { }
};
