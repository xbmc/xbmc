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

typedef enum {
  SettingDependencyTypeNone   = 0,
  SettingDependencyTypeEnable,
  SettingDependencyTypeUpdate,
  SettingDependencyTypeVisible
} SettingDependencyType;

typedef enum {
  SettingDependencyOperatorNone     = 0,
  SettingDependencyOperatorEquals,
  SettingDependencyOperatorContains
} SettingDependencyOperator;

typedef enum {
  SettingDependencyTargetNone     = 0,
  SettingDependencyTargetSetting,
  SettingDependencyTargetProperty
} SettingDependencyTarget;

class CSettingDependencyCondition : public CSettingConditionItem
{
public:
  CSettingDependencyCondition(CSettingsManager *settingsManager = NULL)
    : CSettingConditionItem(settingsManager),
      m_target(SettingDependencyTargetNone),  
      m_operator(SettingDependencyOperatorEquals)      
  { }
  virtual ~CSettingDependencyCondition() { }

  virtual bool Deserialize(const TiXmlNode *node);
  virtual bool Check() const;
  
  const std::string& GetName() const { return m_name; }
  const std::string& GetSetting() const { return m_setting; }
  const SettingDependencyTarget GetTarget() const { return m_target; }
  const SettingDependencyOperator GetOperator() const { return m_operator; }

private:
  bool setTarget(const std::string &target);
  bool setOperator(const std::string &op);
  
  SettingDependencyTarget m_target;
  SettingDependencyOperator m_operator;
};

class CSettingDependencyConditionCombination : public CSettingConditionCombination
{
public:
  CSettingDependencyConditionCombination(CSettingsManager *settingsManager = NULL)
    : CSettingConditionCombination(settingsManager)
  { }
  virtual ~CSettingDependencyConditionCombination() { }

  virtual bool Deserialize(const TiXmlNode *node);

  const std::set<std::string>& GetSettings() const { return m_settings; }

private:
  virtual CBooleanLogicOperation* newOperation() { return new CSettingDependencyConditionCombination(m_settingsManager); }
  virtual CBooleanLogicValue* newValue() { return new CSettingDependencyCondition(m_settingsManager); }

  std::set<std::string> m_settings;
};

class CSettingDependency : public CSettingCondition
{
public:
  CSettingDependency(CSettingsManager *settingsManager = NULL);
  virtual ~CSettingDependency() { }

  virtual bool Deserialize(const TiXmlNode *node);

  SettingDependencyType GetType() const { return m_type; }
  std::set<std::string> GetSettings() const;

private:
  bool setType(const std::string &type);

  SettingDependencyType m_type;
};

typedef std::list<CSettingDependency> SettingDependencies;
typedef std::map<std::string, SettingDependencies> SettingDependencyMap;
