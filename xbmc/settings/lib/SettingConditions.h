/*
 *      Copyright (C) 2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include <map>
#include <set>
#include <string>
#include <utility>

#include "SettingDefinitions.h"
#include "utils/BooleanLogic.h"

class CSettingsManager;
class CSetting;

using SettingConditionCheck = bool (*)(const std::string &condition, const std::string &value, std::shared_ptr<const CSetting> setting, void *data);

class ISettingCondition
{
public:
  explicit ISettingCondition(CSettingsManager *settingsManager)
    : m_settingsManager(settingsManager)
  { }
  virtual ~ISettingCondition() = default;

  virtual bool Check() const = 0;

protected:
  CSettingsManager *m_settingsManager;
};

class CSettingConditionItem : public CBooleanLogicValue, public ISettingCondition
{
public:
  explicit CSettingConditionItem(CSettingsManager *settingsManager = nullptr)
    : ISettingCondition(settingsManager)
  { }
  ~CSettingConditionItem() override = default;

  bool Deserialize(const TiXmlNode *node) override;
  const char* GetTag() const override { return SETTING_XML_ELM_CONDITION; }
  bool Check() const override;

protected:
  std::string m_name;
  std::string m_setting;
};

class CSettingConditionCombination : public CBooleanLogicOperation, public ISettingCondition
{
public:
  explicit CSettingConditionCombination(CSettingsManager *settingsManager = nullptr)
    : ISettingCondition(settingsManager)
  { }
  ~CSettingConditionCombination() override = default;

  bool Check() const override;

private:
  CBooleanLogicOperation* newOperation() override { return new CSettingConditionCombination(m_settingsManager); }
  CBooleanLogicValue* newValue() override { return new CSettingConditionItem(m_settingsManager); }
};

class CSettingCondition : public CBooleanLogic, public ISettingCondition
{
public:
  explicit CSettingCondition(CSettingsManager *settingsManager = nullptr);
  ~CSettingCondition() override = default;

  bool Check() const override;
};

class CSettingConditionsManager
{
public:
  CSettingConditionsManager() = default;
  CSettingConditionsManager(const CSettingConditionsManager&) = delete;
  CSettingConditionsManager const& operator=(CSettingConditionsManager const&) = delete;
  virtual ~CSettingConditionsManager() = default;

  void AddCondition(std::string condition);
  void AddCondition(std::string identifier, SettingConditionCheck condition, void *data = nullptr);

  bool Check(std::string condition, const std::string &value = "", std::shared_ptr<const CSetting> setting = std::shared_ptr<const CSetting>()) const;

private:
  using SettingConditionPair = std::pair<std::string, std::pair<SettingConditionCheck, void*>>;
  using SettingConditionMap = std::map<std::string, std::pair<SettingConditionCheck, void*>>;

  SettingConditionMap m_conditions;
  std::set<std::string> m_defines;
};
