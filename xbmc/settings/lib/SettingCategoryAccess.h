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

class CSettingCategoryAccessCondition : public CSettingConditionItem
{
public:
  CSettingCategoryAccessCondition(CSettingsManager *settingsManager = nullptr)
    : CSettingConditionItem(settingsManager)
  { }
  virtual ~CSettingCategoryAccessCondition() = default;

  virtual bool Check() const;
};

class CSettingCategoryAccessConditionCombination : public CSettingConditionCombination
{
public:
  CSettingCategoryAccessConditionCombination(CSettingsManager *settingsManager = nullptr)
    : CSettingConditionCombination(settingsManager)
  { }
  virtual ~CSettingCategoryAccessConditionCombination() = default;

  virtual bool Check() const;

private:
  virtual CBooleanLogicOperation* newOperation() { return new CSettingCategoryAccessConditionCombination(m_settingsManager); }
  virtual CBooleanLogicValue* newValue() { return new CSettingCategoryAccessCondition(m_settingsManager); }
};

class CSettingCategoryAccess : public CSettingCondition
{
public:
  CSettingCategoryAccess(CSettingsManager *settingsManager = nullptr);
  virtual ~CSettingCategoryAccess() = default;
};
