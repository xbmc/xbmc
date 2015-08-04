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

#include <map>
#include <set>
#include <string>

#include "SettingDefinitions.h"
#include "utils/BooleanLogic.h"

class CSettingsManager;
class CSetting;

typedef bool (*SettingConditionCheck)(const std::string &condition, const std::string &value, const CSetting *setting, void *data);

class ISettingCondition
{
public:
  ISettingCondition(CSettingsManager *settingsManager)
    : m_settingsManager(settingsManager)
  { }
  virtual ~ISettingCondition() { }

  virtual bool Check() const = 0;

protected:
  CSettingsManager *m_settingsManager;
};

class CSettingConditionItem : public CBooleanLogicValue, public ISettingCondition
{
public:
  CSettingConditionItem(CSettingsManager *settingsManager = NULL)
    : ISettingCondition(settingsManager)
  { }
  virtual ~CSettingConditionItem() { }
  
  virtual bool Deserialize(const TiXmlNode *node);
  virtual const char* GetTag() const { return SETTING_XML_ELM_CONDITION; }
  virtual bool Check() const;

protected:
  std::string m_name;
  std::string m_setting;
};

class CSettingConditionCombination : public CBooleanLogicOperation, public ISettingCondition
{
public:
  CSettingConditionCombination(CSettingsManager *settingsManager = NULL)
    : ISettingCondition(settingsManager)
  { }
  virtual ~CSettingConditionCombination() { }

  virtual bool Check() const;

private:
  virtual CBooleanLogicOperation* newOperation() { return new CSettingConditionCombination(m_settingsManager); }
  virtual CBooleanLogicValue* newValue() { return new CSettingConditionItem(m_settingsManager); }
};

class CSettingCondition : public CBooleanLogic, public ISettingCondition
{
public:
  CSettingCondition(CSettingsManager *settingsManager = NULL);
  virtual ~CSettingCondition() { }

  virtual bool Check() const;
};

class CSettingConditionsManager
{
public:
  CSettingConditionsManager();
  virtual ~CSettingConditionsManager();

  void AddCondition(const std::string &condition);
  void AddCondition(const std::string &identifier, SettingConditionCheck condition, void *data = NULL);

  bool Check(const std::string &condition, const std::string &value = "", const CSetting *setting = NULL) const;

private:
  CSettingConditionsManager(const CSettingConditionsManager&);
  CSettingConditionsManager const& operator=(CSettingConditionsManager const&);
  
  typedef std::pair<std::string, std::pair<SettingConditionCheck, void*> > SettingConditionPair;
  typedef std::map<std::string, std::pair<SettingConditionCheck, void*> > SettingConditionMap;

  SettingConditionMap m_conditions;
  std::set<std::string> m_defines;
};
