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

class CSettingVisibilityCondition : public CSettingConditionItem
{
public:
  CSettingVisibilityCondition(CSettingsManager *settingsManager = NULL)
    : CSettingConditionItem(settingsManager)
  { }
  virtual ~CSettingVisibilityCondition() { }

  virtual bool Check() const;
};

class CSettingVisibilityConditionCombination : public CSettingConditionCombination
{
public:
  CSettingVisibilityConditionCombination(CSettingsManager *settingsManager = NULL)
    : CSettingConditionCombination(settingsManager)
  { }
  virtual ~CSettingVisibilityConditionCombination() { }

  virtual bool Check() const;

private:
  virtual CBooleanLogicOperation* newOperation() { return new CSettingVisibilityConditionCombination(m_settingsManager); }
  virtual CBooleanLogicValue* newValue() { return new CSettingVisibilityCondition(m_settingsManager); }
};

class CSettingVisibility : public CSettingCondition
{
public:
  CSettingVisibility(CSettingsManager *settingsManager = NULL);
  virtual ~CSettingVisibility() { }
};
