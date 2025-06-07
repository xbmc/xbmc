/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SettingConditions.h"

#include <set>
#include <string>

class CSettingCategoryAccessCondition : public CSettingConditionItem
{
public:
  using CSettingConditionItem::CSettingConditionItem;
  ~CSettingCategoryAccessCondition() override = default;

  bool Check() const override;
};

class CSettingCategoryAccessConditionCombination : public CSettingConditionCombination
{
public:
  using CSettingConditionCombination::CSettingConditionCombination;
  ~CSettingCategoryAccessConditionCombination() override = default;

  bool Check() const override;

private:
  CBooleanLogicOperation* newOperation() override { return new CSettingCategoryAccessConditionCombination(m_settingsManager); }
  CBooleanLogicValue* newValue() override { return new CSettingCategoryAccessCondition(m_settingsManager); }
};

class CSettingCategoryAccess : public CSettingCondition
{
public:
  explicit CSettingCategoryAccess(CSettingsManager *settingsManager = nullptr);
  ~CSettingCategoryAccess() override = default;
};
