#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
 *      http://www.xbmc.org
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

#include "SettingConditions.h"

#include <vector>

#define SETTINGS_LEVEL_DEFAULT  SettingLevelStandard

#define XML_ELM_LEVEL           "level"
#define XML_ELM_LEVELS          "levels"

class TiXmlNode;

typedef enum {
  SettingLevelBasic  = 0,
  SettingLevelStandard,
  SettingLevelAdvanced,
  SettingLevelExpert,
  SettingLevelInternal
} SettingLevel;

class CSettingLevelCondition : public CSettingConditionItem
{
public:
  CSettingLevelCondition(CSettingsManager *settingsManager = NULL)
    : CSettingConditionItem(settingsManager)
  { }
  virtual ~CSettingLevelCondition() { }

  virtual bool Check() const;
};

class CSettingLevelConditionCombination : public CSettingConditionCombination
{
public:
  CSettingLevelConditionCombination(CSettingsManager *settingsManager = NULL)
    : CSettingConditionCombination(settingsManager)
  { }
  virtual ~CSettingLevelConditionCombination() { }

protected:
  virtual CBooleanLogicOperation* newOperation() { return new CSettingLevelConditionCombination(m_settingsManager); }
  virtual CBooleanLogicValue* newValue() { return new CSettingLevelCondition(m_settingsManager); }
};

class CLevelCondition : public CSettingCondition
{
public:
  CLevelCondition(CSettingsManager *settingsManager = NULL);
  CLevelCondition(const CLevelCondition &rhs);
  virtual ~CLevelCondition() { }

  virtual bool Deserialize(const TiXmlNode *node);

  SettingLevel GetLevel() const { return m_level; }

private:
  SettingLevel m_level;
};

class CSettingLevel
{
public:
  CSettingLevel(CSettingsManager *settingsManager = NULL);
  virtual ~CSettingLevel() { }

  virtual bool Deserialize(const TiXmlNode *node);

  /**
   * Alternative, simplified deserialization method, where only a default level
   * is specified and no level conditions are needed.
   */
  void SetLevel(SettingLevel level);

  /**
   * For complex levels with conditions, this applies the level conditions in
   * the order that the <level> tags appeared. If no <level> tags evaluate to
   * true, the default level is returned.
   */
  SettingLevel GetLevel() const;

private:
  SettingLevel m_default;
  std::vector<CLevelCondition> m_levelConditions;
  CSettingsManager *m_settingsManager;
};
