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

#include <list>
#include <set>
#include <string>

#include "SettingConditions.h"
#include "utils/BooleanLogic.h"

enum class SettingDependencyType {
  Unknown = 0,
  Enable,
  Update,
  Visible
};

enum class SettingDependencyOperator {
  Unknown = 0,
  Equals,
  LessThan,
  GreaterThan,
  Contains
};

enum class SettingDependencyTarget {
  Unknown = 0,
  Setting,
  Property
};

class CSettingDependencyCondition : public CSettingConditionItem
{
public:
  explicit CSettingDependencyCondition(CSettingsManager *settingsManager = nullptr);
  CSettingDependencyCondition(const std::string &setting, const std::string &value,
                              SettingDependencyOperator op, bool negated = false,
                              CSettingsManager *settingsManager = nullptr);
  CSettingDependencyCondition(const std::string &strProperty, const std::string &value,
                              const std::string &setting = "", bool negated = false,
                              CSettingsManager *settingsManager = nullptr);
  ~CSettingDependencyCondition() override = default;

  bool Deserialize(const TiXmlNode *node) override;
  bool Check() const override;
  
  const std::string& GetName() const { return m_name; }
  const std::string& GetSetting() const { return m_setting; }
  const SettingDependencyTarget GetTarget() const { return m_target; }
  const SettingDependencyOperator GetOperator() const { return m_operator; }

private:
  bool setTarget(const std::string &target);
  bool setOperator(const std::string &op);
  
  SettingDependencyTarget m_target = SettingDependencyTarget::Unknown;
  SettingDependencyOperator m_operator = SettingDependencyOperator::Equals;
};

using CSettingDependencyConditionPtr = std::shared_ptr<CSettingDependencyCondition>;

class CSettingDependencyConditionCombination;
using CSettingDependencyConditionCombinationPtr = std::shared_ptr<CSettingDependencyConditionCombination>;

class CSettingDependencyConditionCombination : public CSettingConditionCombination
{
public:
  explicit CSettingDependencyConditionCombination(CSettingsManager *settingsManager = nullptr)
    : CSettingConditionCombination(settingsManager)
  { }
  CSettingDependencyConditionCombination(BooleanLogicOperation op, CSettingsManager *settingsManager = nullptr)
    : CSettingConditionCombination(settingsManager)
  {
    SetOperation(op);
  }
  ~CSettingDependencyConditionCombination() override = default;

  bool Deserialize(const TiXmlNode *node) override;

  const std::set<std::string>& GetSettings() const { return m_settings; }

  CSettingDependencyConditionCombination* Add(CSettingDependencyConditionPtr condition);
  CSettingDependencyConditionCombination* Add(CSettingDependencyConditionCombinationPtr operation);

private:
  CBooleanLogicOperation* newOperation() override { return new CSettingDependencyConditionCombination(m_settingsManager); }
  CBooleanLogicValue* newValue() override { return new CSettingDependencyCondition(m_settingsManager); }

  std::set<std::string> m_settings;
};

class CSettingDependency : public CSettingCondition
{
public:
  explicit CSettingDependency(CSettingsManager *settingsManager = nullptr);
  CSettingDependency(SettingDependencyType type, CSettingsManager *settingsManager = nullptr);
  ~CSettingDependency() override = default;

  bool Deserialize(const TiXmlNode *node) override;

  SettingDependencyType GetType() const { return m_type; }
  std::set<std::string> GetSettings() const;

  CSettingDependencyConditionCombinationPtr And();
  CSettingDependencyConditionCombinationPtr Or();

private:
  bool setType(const std::string &type);

  SettingDependencyType m_type = SettingDependencyType::Unknown;
};

using SettingDependencies = std::list<CSettingDependency>;
using SettingDependencyMap = std::map<std::string, SettingDependencies>;
