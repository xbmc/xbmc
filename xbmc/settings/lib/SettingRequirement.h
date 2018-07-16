/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <set>
#include <string>

#include "SettingConditions.h"

class CSettingRequirementCondition : public CSettingConditionItem
{
public:
  explicit CSettingRequirementCondition(CSettingsManager *settingsManager = nullptr)
    : CSettingConditionItem(settingsManager)
  { }
  ~CSettingRequirementCondition() override = default;

  bool Check() const override;
};

class CSettingRequirementConditionCombination : public CSettingConditionCombination
{
public:
  explicit CSettingRequirementConditionCombination(CSettingsManager *settingsManager = nullptr)
    : CSettingConditionCombination(settingsManager)
  { }
  ~CSettingRequirementConditionCombination() override = default;

  bool Check() const override;

private:
  CBooleanLogicOperation* newOperation() override { return new CSettingRequirementConditionCombination(m_settingsManager); }
  CBooleanLogicValue* newValue() override { return new CSettingRequirementCondition(m_settingsManager); }
};

class CSettingRequirement : public CSettingCondition
{
public:
  explicit CSettingRequirement(CSettingsManager *settingsManager = nullptr);
  ~CSettingRequirement() override = default;
};
