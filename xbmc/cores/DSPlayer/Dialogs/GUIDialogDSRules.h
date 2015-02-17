#pragma once

/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://xbmc.org
 *
 *      Copyright (C) 2014-2015 Aracnoz
 *      http://github.com/aracnoz/xbmc 
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

#include "settings/dialogs/GUIDialogSettingsManualBase.h"
#include "utils/stdstring.h"

enum RuleType {
  EDITATTR,
  SPINNERATTR,
  FILTER,
  EXTRAFILTER,
  SHADER
};

enum xmlType {
  MEDIASCONFIG,
  FILTERSCONFIG,
  HOMEFILTERSCONFIG,
  SHADERS
};

class DSRulesList
{
public:
  DSRulesList(RuleType type);

  CStdString strRuleAttr;
  CStdString strRuleName;
  CStdString strRuleValue;
  CStdString settingRule;
  std::vector<std::string> strVec;
  int ruleLabel;
  StringSettingOptionsFiller filler;
  RuleType m_ruleType;
};

class CGUIDialogDSRules : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogDSRules();
  virtual ~CGUIDialogDSRules();

  static CGUIDialogDSRules* Get();
  static void Destroy()
  {
    delete m_pSingleton;
    m_pSingleton = NULL;
  }

  static int ShowDSRulesList();
  void SetNewRule(bool b);
  bool GetNewRule();
  void SetRuleIndex(int index);
  int GetRuleIndex();

protected:

  // implementations of ISettingCallback
  virtual void OnSettingChanged(const CSetting *setting);
  virtual void OnSettingAction(const CSetting *setting);
  virtual void OnDeinitWindow(int nextWindowID);

  // specialization of CGUIDialogSettingsBase
  virtual bool AllowResettingSettings() const { return false; }
  virtual void Save();

  // specialization of CGUIDialogSettingsManualBase
  virtual void InitializeSettings();

  virtual void SetupView();

  static void UrlOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static void FiltersConfigOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static void ShadersOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static bool compare_by_word(const DynamicStringSettingOption& lhs, const DynamicStringSettingOption& rhs);
  static CGUIDialogDSRules* m_pSingleton;
  void LoadDsXML(CXBMCTinyXML *XML, xmlType type, TiXmlElement* &pNode, CStdString &xmlFile, bool forceCreate = false);
  void InitRules(RuleType type, CStdString settingRule, int ruleLabel, CStdString strRuleName = "", CStdString strRuleAttr = "", StringSettingOptionsFiller filler = NULL);
  void ResetValue();

  bool m_newrule;
  int m_ruleIndex;

  std::vector<DSRulesList *> m_ruleList;
};
