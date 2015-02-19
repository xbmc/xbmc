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

#include "GUIDialogDSRules.h"
#include "Application.h"
#include "URL.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/File.h"
#include "guilib/LocalizeStrings.h"
#include "profiles/ProfilesManager.h"
#include "settings/Settings.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "utils/LangCodeExpander.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/Key.h"
#include "cores/DSPlayer/Utils/DSFilterEnumerator.h"
#include "utils/XMLUtils.h"
#include "Filters/RendererSettings.h"
#include "PixelShaderList.h"

#define SETTING_RULE_SAVE                     "rule.save"
#define SETTING_RULE_ADD                      "rule.add"
#define SETTING_RULE_DEL                      "rule.del"

using namespace std;

DSRulesList::DSRulesList(RuleType type) :

settingRule(""),
strRuleName(""),
strRuleAttr(""),
strRuleValue(""),
ruleLabel(0),
subNode(0),
filler(NULL),
m_ruleType(type)
{
}

CGUIDialogDSRules::CGUIDialogDSRules()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_DSRULES, "VideoOSDSettings.xml")
{ }


CGUIDialogDSRules::~CGUIDialogDSRules()
{ }

CGUIDialogDSRules *CGUIDialogDSRules::m_pSingleton = NULL;

CGUIDialogDSRules* CGUIDialogDSRules::Get()
{
  return (m_pSingleton) ? m_pSingleton : (m_pSingleton = new CGUIDialogDSRules());
}

void CGUIDialogDSRules::OnInitWindow()
{
  CGUIDialogSettingsManualBase::OnInitWindow();
  HideUnused();
  isEdited = false;
}

void CGUIDialogDSRules::OnDeinitWindow(int nextWindowID)
{
  CGUIDialogSettingsManualBase::OnDeinitWindow(nextWindowID);
  ShowDSRulesList();
}

bool CGUIDialogDSRules::OnBack(int actionID)
{
  if (isEdited)
  {
    if (CGUIDialogYesNo::ShowAndGetInput(61001, 61002, 0, 0))
    { 
      CSetting *setting = GetSetting(SETTING_RULE_SAVE);
      OnSettingAction(setting);
    }
  }

  return CGUIDialogSettingsManualBase::OnBack(actionID);
}

void CGUIDialogDSRules::Save()
{

}
void CGUIDialogDSRules::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  SET_CONTROL_LABEL(0,60001);
}

void CGUIDialogDSRules::SetNewRule(bool b)
{
  m_newrule = b;
}

bool CGUIDialogDSRules::GetNewRule()
{
  return m_newrule;
}

void CGUIDialogDSRules::SetRuleIndex(int index)
{
  m_ruleIndex = index;
}

int CGUIDialogDSRules::GetRuleIndex()
{
  return m_ruleIndex;
}

bool CGUIDialogDSRules::compare_by_word(const DynamicStringSettingOption& lhs, const DynamicStringSettingOption& rhs)
{
  CStdString strLine1 = lhs.first;
  CStdString strLine2 = rhs.first;
  StringUtils::ToLower(strLine1);
  StringUtils::ToLower(strLine2);
  return strcmp(strLine1.c_str(), strLine2.c_str()) < 0;
}

void CGUIDialogDSRules::InitRules(RuleType type, CStdString settingRule, int ruleLabel, CStdString strRuleAttr /* = "" */, CStdString strRuleName /*= "" */, StringSettingOptionsFiller filler /* = NULL */, int subNode /* = 0 */)
{
  DSRulesList* ruleList = new DSRulesList(type);
  ruleList->settingRule = settingRule;
  ruleList->strRuleAttr = strRuleAttr;
  ruleList->strRuleName = strRuleName;
  ruleList->filler = filler;
  ruleList->ruleLabel = ruleLabel;
  ruleList->subNode = subNode;
  m_ruleList.push_back(ruleList);
}

void CGUIDialogDSRules::SetVisible(CStdString id, bool visible, bool isChild /* = false */)
{
  CSetting *setting = m_settingsManager->GetSetting(id);
  if (setting->IsVisible() && visible)
    return;
  setting->SetVisible(visible);
  m_settingsManager->SetString(id, "aaa");
  if (!isChild) 
    m_settingsManager->SetString(id, "[null]");
  else
    m_settingsManager->SetString(id, "");
}

void CGUIDialogDSRules::HideUnused()
{
  std::vector<DSRulesList *>::iterator it;
  int countExtra = 0;
  int countShader = 0;
  bool showExtra;
  bool showShader;

  if (!m_allowchange)
    return;

  m_allowchange = false;

  for (it = m_ruleList.begin(); it != m_ruleList.end(); ++it)
  {
    if ((*it)->m_ruleType == EXTRAFILTER)
    {
      if ((*it)->strRuleValue == "[null]")
        countExtra++;

      showExtra = (countExtra > 1 && (*it)->strRuleValue == "[null]");
      SetVisible((*it)->settingRule.c_str(), !showExtra);

      showExtra = (countExtra > 0 && (*it)->strRuleValue == "[null]");
      std::vector<DSRulesList *>::iterator itchild;
      for (itchild = m_ruleList.begin(); itchild != m_ruleList.end(); ++itchild)
      {
        if ((*itchild)->m_ruleType == EDITATTREXTRA && (*it)->subNode == (*itchild)->subNode)
        {
          SetVisible((*itchild)->settingRule.c_str(), !showExtra, true);
        }
      }
    }

    if ((*it)->m_ruleType == SHADER)
    {
      if ((*it)->strRuleValue == "[null]")
        countShader++;

      showShader = (countShader > 1 && (*it)->strRuleValue == "[null]");
      SetVisible((*it)->settingRule.c_str(), !showShader);

      showShader = (countShader > 0 && (*it)->strRuleValue == "[null]");
      std::vector<DSRulesList *>::iterator itchild;
      for (itchild = m_ruleList.begin(); itchild != m_ruleList.end(); ++itchild)
      {
        if ((*itchild)->m_ruleType == EDITATTRSHADER && (*it)->subNode == (*itchild)->subNode)
        {
          SetVisible((*itchild)->settingRule.c_str(), !showShader, true);
        }
      }
    }
  }
  m_allowchange = true;
}

void CGUIDialogDSRules::ResetValue()
{
  std::vector<DSRulesList *>::iterator it;
  for (it = m_ruleList.begin(); it != m_ruleList.end(); ++it)
  {
    if ((*it)->m_ruleType == EDITATTR || (*it)->m_ruleType == EDITATTREXTRA || (*it)->m_ruleType == EDITATTRSHADER)
      (*it)->strRuleValue = "";
    if ((*it)->m_ruleType == SPINNERATTR)
      (*it)->strRuleValue = "[null]";
    if ((*it)->m_ruleType == FILTER)
      (*it)->strRuleValue = "[null]";
    if ((*it)->m_ruleType == EXTRAFILTER || (*it)->m_ruleType == SHADER)
      (*it)->strRuleValue = "[null]";
  }
}

void CGUIDialogDSRules::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  CSettingCategory *category = AddCategory("dsfiltersettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogDSRules: unable to setup settings");
    return;
  }

  // get all necessary setting groups
  CSettingGroup *groupName = AddGroup(category);
  if (groupName == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogDSRules: unable to setup settings");
    return;
  }

  CSettingGroup *groupRule = AddGroup(category);
  if (groupRule == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogDSRules: unable to setup settings");
    return;
  }

  CSettingGroup *groupFilter = AddGroup(category);
  if (groupFilter == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogDSRules: unable to setup settings");
    return;
  }

  CSettingGroup *groupExtra = AddGroup(category);
  if (groupFilter == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogDSRules: unable to setup settings");
    return;
  }

  CSettingGroup *groupSave = AddGroup(category);
  if (groupFilter == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogDSRules: unable to setup settings");
    return;
  }

  // Save shader file in userdata folder
  g_dsSettings.pixelShaderList->SaveXML();
  
  // Init variables
  int selected = Get()->GetRuleIndex();
  int count = 0;

 if (m_ruleList.size() == 0)
  {
    // RULE
    InitRules(EDITATTR, "rules.name", 60002, "name");
    InitRules(EDITATTR, "rules.filetypes", 60003, "filetypes");
    InitRules(EDITATTR, "rules.filename", 60004, "filename");
    InitRules(EDITATTR, "rules.protocols", 60005, "protocols");
    InitRules(SPINNERATTR, "rules.url", 60006, "url", "", UrlOptionFiller);

    // FILTER
    InitRules(FILTER, "rules.source", 60007, "filter", "source", FiltersConfigOptionFiller);
    InitRules(FILTER, "rules.splitter", 60008, "filter", "splitter", FiltersConfigOptionFiller);
    InitRules(FILTER, "rules.video", 60009, "filter", "video", FiltersConfigOptionFiller);
    InitRules(FILTER, "rules.audio", 60010, "filter", "audio", FiltersConfigOptionFiller);
    InitRules(FILTER, "rules.subs", 60011, "filter", "subs", FiltersConfigOptionFiller);

    // EXTRAFILTER
    InitRules(EXTRAFILTER, "rules.extra0", 60012, "filter", "extra", FiltersConfigOptionFiller, 0);
    InitRules(EDITATTREXTRA, "rules.videores0", 60019, "videoresolution", "extra", 0 ,0);
    InitRules(EDITATTREXTRA, "rules.videocodec0", 60020, "videocodec", "extra", 0, 0);
    InitRules(EDITATTREXTRA, "rules.audiochans0", 60021, "audiochannels", "extra", 0, 0);
    InitRules(EDITATTREXTRA, "rules.audiocodec0", 60022, "audiocodec", "extra", 0, 0);

    InitRules(EXTRAFILTER, "rules.extra1", 60012, "filter", "extra", FiltersConfigOptionFiller, 1);
    InitRules(EDITATTREXTRA, "rules.videores1", 60019, "videoresolution", "extra", 0, 1);
    InitRules(EDITATTREXTRA, "rules.videocodec1", 60020, "videocodec", "extra", 0, 1);
    InitRules(EDITATTREXTRA, "rules.audiochans1", 60021, "audiochannels", "extra", 0, 1);
    InitRules(EDITATTREXTRA, "rules.audiocodec1", 60022, "audiocodec", "extra", 0, 1);

    InitRules(EXTRAFILTER, "rules.extra2", 60012, "filter", "extra", FiltersConfigOptionFiller, 2);
    InitRules(EDITATTREXTRA, "rules.videores2", 60019, "videoresolution", "extra", 0, 2);
    InitRules(EDITATTREXTRA, "rules.videocodec2", 60020, "videocodec", "extra", 0, 2);
    InitRules(EDITATTREXTRA, "rules.audiochans2", 60021, "audiochannels", "extra", 0, 2);
    InitRules(EDITATTREXTRA, "rules.audiocodec2", 60022, "audiocodec", "extra", 0, 2);

    // SHADER
    InitRules(SHADER, "rules.shader0", 60013, "id", "shader", ShadersOptionFiller, 0);
    InitRules(EDITATTRSHADER, "rules.shvideores0", 60019, "videoresolution", "shader", 0, 0);
    InitRules(EDITATTRSHADER, "rules.shvideocodec0", 60020, "videocodec", "shader", 0, 0);

    InitRules(SHADER, "rules.shader1", 60013, "id", "shader", ShadersOptionFiller, 1);
    InitRules(EDITATTRSHADER, "rules.shvideores1", 60019, "videoresolution", "shader", 0, 1);
    InitRules(EDITATTRSHADER, "rules.shvideocodec1", 60020, "videocodec", "shader", 0, 1);

    InitRules(SHADER, "rules.shader2", 60013, "id", "shader", ShadersOptionFiller, 2);
    InitRules(EDITATTRSHADER, "rules.shvideores2", 60019, "videoresolution", "shader", 0, 2);
    InitRules(EDITATTRSHADER, "rules.shvideocodec2", 60020, "videocodec", "shader", 0, 2);
  }

  // Reset Button value
  ResetValue();
  
  // Load userdata Mediaseconfig.xml
  if (!Get()->GetNewRule())
  {
    CXBMCTinyXML MediasConfigXML;
    CStdString xmlFile;
    TiXmlElement *pRules;
    LoadDsXML(&MediasConfigXML, MEDIASCONFIG, pRules, xmlFile, true);

    if (pRules)
    {
      TiXmlElement *pRule = pRules->FirstChildElement("rule");

      while (pRule)
      {
        if (count == selected)
        {
          std::vector<DSRulesList *>::iterator it;
          for (it = m_ruleList.begin(); it != m_ruleList.end(); ++it)
          {                   
            if ((*it)->m_ruleType == EDITATTR || (*it)->m_ruleType == SPINNERATTR)
              (*it)->strRuleValue = pRule->Attribute((*it)->strRuleAttr.c_str());     

            if ((*it)->m_ruleType == FILTER)
            {
              TiXmlElement *pFilter = pRule->FirstChildElement((*it)->strRuleName.c_str());
              if (pFilter)
                (*it)->strRuleValue = pFilter->Attribute((*it)->strRuleAttr.c_str());             
            }

            if ((*it)->m_ruleType == EXTRAFILTER || (*it)->m_ruleType == SHADER || (*it)->m_ruleType == EDITATTREXTRA || (*it)->m_ruleType == EDITATTRSHADER)
            {             
              TiXmlElement *pFilter = pRule->FirstChildElement((*it)->strRuleName.c_str());

              if (pFilter)
              {
                if ((*it)->subNode == 0)
                  (*it)->strRuleValue = pFilter->Attribute((*it)->strRuleAttr.c_str());

                if ((*it)->strRuleValue == "" || (*it)->subNode > 0)
                {
                  int countsize = 0;
                  TiXmlElement *pSubExtra = pFilter->FirstChildElement((*it)->strRuleName.c_str());
                  while (pSubExtra)
                  {
                    if ((*it)->subNode == countsize)
                      (*it)->strRuleValue = pSubExtra->Attribute((*it)->strRuleAttr.c_str());

                    pSubExtra = pSubExtra->NextSiblingElement((*it)->strRuleName.c_str());
                    countsize++;
                  }
                }
              }
            }
          }
          break;
        }
        pRule = pRule->NextSiblingElement("rule");
        count++;
      }
    }
  }

  // Stamp Button

  CStdString setting_rule;
  std::vector<DSRulesList *>::iterator it;
  CSettingGroup *groupTmp;

  for (it = m_ruleList.begin(); it != m_ruleList.end(); ++it)
  {
    if ((*it)->m_ruleType == EDITATTR || (*it)->m_ruleType == EDITATTREXTRA || (*it)->m_ruleType == EDITATTRSHADER)
    {
      if ((*it)->strRuleAttr == "name")
        groupTmp = groupName;
      else
        groupTmp = groupRule;

      if ((*it)->m_ruleType == EDITATTREXTRA || (*it)->m_ruleType == EDITATTRSHADER)
        groupTmp = groupExtra;

      AddEdit(groupTmp, (*it)->settingRule, (*it)->ruleLabel, 0, (*it)->strRuleValue.c_str(), true);
    } 
    if ((*it)->m_ruleType == SPINNERATTR)
      AddSpinner(groupRule, (*it)->settingRule, (*it)->ruleLabel, 0, (*it)->strRuleValue, (*it)->filler);

    if ((*it)->m_ruleType == FILTER)
      AddList(groupFilter, (*it)->settingRule, (*it)->ruleLabel, 0, (*it)->strRuleValue, (*it)->filler, (*it)->ruleLabel);

    if ((*it)->m_ruleType == EXTRAFILTER || (*it)->m_ruleType == SHADER)
      AddList(groupExtra, (*it)->settingRule, (*it)->ruleLabel, 0, (*it)->strRuleValue, (*it)->filler, (*it)->ruleLabel);
  }

  if (Get()->GetNewRule())
    AddButton(groupSave, SETTING_RULE_ADD, 60015, 0);  
  else
  { 
    AddButton(groupSave, SETTING_RULE_SAVE, 60016, 0);
    AddButton(groupSave, SETTING_RULE_DEL, 60017, 0);
  }
}

void CGUIDialogDSRules::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  isEdited = true;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);
  
  const std::string &settingId = setting->GetId(); 

  std::vector<DSRulesList *>::iterator it;
  for (it = m_ruleList.begin(); it != m_ruleList.end(); ++it)
  {
    if (settingId == (*it)->settingRule)
    {
      (*it)->strRuleValue = static_cast<std::string>(static_cast<const CSettingString*>(setting)->GetValue());
    }
  }
  HideUnused();
}

void CGUIDialogDSRules::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  // Load userdata Mediaseconfig.xml
  CXBMCTinyXML MediasConfigXML;
  CStdString xmlFile;
  TiXmlElement *pRules;
  LoadDsXML(&MediasConfigXML, MEDIASCONFIG, pRules, xmlFile, true);
  if (!pRules)
    return;

  // Init variables
  CGUIDialogSettingsManualBase::OnSettingAction(setting);
  const std::string &settingId = setting->GetId();

  // Del Rule
  if (settingId == SETTING_RULE_DEL)
  {

    if (!CGUIDialogYesNo::ShowAndGetInput(60017, 60018, 0, 0))
      return;
    
    int selected = Get()->GetRuleIndex();
    int count = 0;
    TiXmlElement *pRule = pRules->FirstChildElement("rule");
    while (pRule)
    {
      if (count == selected)
      {
        pRules->RemoveChild(pRule);
        break;
      }
      pRule = pRule->NextSiblingElement("rule");
      count++;
    }

    MediasConfigXML.SaveFile(xmlFile);
    CGUIDialogDSRules::Close();
  }

  // Add New Rule
  if (settingId == SETTING_RULE_ADD)
  {
    TiXmlElement pRule("rule");

    std::vector<DSRulesList *>::iterator it;
    for (it = m_ruleList.begin(); it != m_ruleList.end(); ++it)
    {
      if ((*it)->m_ruleType == EDITATTR && (*it)->strRuleValue != "")
          pRule.SetAttribute((*it)->strRuleAttr.c_str(), (*it)->strRuleValue.c_str());

      if ((*it)->m_ruleType == SPINNERATTR && (*it)->strRuleValue != "[null]" && (*it)->strRuleValue != "")
          pRule.SetAttribute((*it)->strRuleAttr.c_str(), (*it)->strRuleValue.c_str());

      if ((*it)->m_ruleType == FILTER && (*it)->strRuleValue != "[null]")
      {
        pRule.InsertEndChild(TiXmlElement((*it)->strRuleName.c_str()));
        TiXmlElement *pFilter = pRule.FirstChildElement((*it)->strRuleName.c_str());
        if (pFilter)
          pFilter->SetAttribute((*it)->strRuleAttr.c_str(), (*it)->strRuleValue.c_str());
      }

      if (((*it)->m_ruleType == EXTRAFILTER 
        || (*it)->m_ruleType == SHADER 
        || (*it)->m_ruleType == EDITATTREXTRA 
        || (*it)->m_ruleType == EDITATTRSHADER) 
        && (*it)->strRuleValue != "[null]" && (*it)->strRuleValue != "")
      {
        TiXmlElement *pExtra = pRule.FirstChildElement((*it)->strRuleName.c_str());
        if (!pExtra && (*it)->m_ruleType != EDITATTREXTRA && (*it)->m_ruleType != EDITATTRSHADER)
        {
          pRule.InsertEndChild(TiXmlElement((*it)->strRuleName.c_str()));
          pExtra = pRule.FirstChildElement((*it)->strRuleName.c_str());
        }
        if ((*it)->m_ruleType != EDITATTREXTRA && (*it)->m_ruleType != EDITATTRSHADER)
          pExtra->InsertEndChild(TiXmlElement((*it)->strRuleName.c_str()));
         
        TiXmlElement *pSubExtra = pExtra->FirstChildElement((*it)->strRuleName.c_str());

        int countsize = 0;
        while (pSubExtra)
        {
          if ((*it)->subNode == countsize)
            pSubExtra->SetAttribute((*it)->strRuleAttr.c_str(), (*it)->strRuleValue.c_str());

          pSubExtra = pSubExtra->NextSiblingElement((*it)->strRuleName.c_str());
          countsize++;
        }
      }
    }
    pRules->InsertEndChild(pRule);
    MediasConfigXML.SaveFile(xmlFile);
    CGUIDialogDSRules::Close();
  }

  // Save Modifications
  if (settingId == SETTING_RULE_SAVE)
  {

    int selected = Get()->GetRuleIndex();
    int count = 0;

    TiXmlElement *pRule = pRules->FirstChildElement("rule");
    while (pRule)
    {
      if (count == selected)
      {
        pRule->Clear();
        std::vector<DSRulesList *>::iterator it;
        for (it = m_ruleList.begin(); it != m_ruleList.end(); ++it)
        {
          if ((*it)->m_ruleType == EDITATTR)
          {
            if ((*it)->strRuleValue != "")
              pRule->SetAttribute((*it)->strRuleAttr.c_str(), (*it)->strRuleValue.c_str());
            else
              pRule->RemoveAttribute((*it)->strRuleAttr.c_str());
          }
          if ((*it)->m_ruleType == SPINNERATTR)
          {
            if ((*it)->strRuleValue != "[null]" && (*it)->strRuleValue != "")
              pRule->SetAttribute((*it)->strRuleAttr.c_str(), (*it)->strRuleValue.c_str());
            else
              pRule->RemoveAttribute((*it)->strRuleAttr.c_str());
          }

          if ((*it)->m_ruleType == FILTER)
          {
            if ((*it)->strRuleValue != "[null]" )
            {
              pRule->InsertEndChild(TiXmlElement((*it)->strRuleName.c_str()));
              TiXmlElement *pFilter = pRule->FirstChildElement((*it)->strRuleName.c_str());
              if (pFilter)
                pFilter->SetAttribute((*it)->strRuleAttr.c_str(), (*it)->strRuleValue.c_str());
            }
          }
          if (((*it)->m_ruleType == EXTRAFILTER 
            || (*it)->m_ruleType == SHADER 
            || (*it)->m_ruleType == EDITATTREXTRA 
            || (*it)->m_ruleType == EDITATTRSHADER) 
            && (*it)->strRuleValue != "[null]" && (*it)->strRuleValue != "")
          {

            TiXmlElement *pExtra = pRule->FirstChildElement((*it)->strRuleName.c_str());
            if (!pExtra && (*it)->m_ruleType != EDITATTREXTRA && (*it)->m_ruleType != EDITATTRSHADER)
            {
              pRule->InsertEndChild(TiXmlElement((*it)->strRuleName.c_str()));
              pExtra = pRule->FirstChildElement((*it)->strRuleName.c_str());
            }
            if ((*it)->m_ruleType != EDITATTREXTRA && (*it)->m_ruleType != EDITATTRSHADER)
              pExtra->InsertEndChild(TiXmlElement((*it)->strRuleName.c_str()));

            TiXmlElement *pSubExtra = pExtra->FirstChildElement((*it)->strRuleName.c_str());

            int countsize = 0;
            while (pSubExtra)
            {
              if ((*it)->subNode == countsize)
                pSubExtra->SetAttribute((*it)->strRuleAttr.c_str(), (*it)->strRuleValue.c_str());

              pSubExtra = pSubExtra->NextSiblingElement((*it)->strRuleName.c_str());
              countsize++;
            }
          }
        }
        break;
      }  
      pRule = pRule->NextSiblingElement("rule");
      count++;
    }
    isEdited = false;
    MediasConfigXML.SaveFile(xmlFile);
    CGUIDialogDSRules::Close();
  }
}

int CGUIDialogDSRules::ShowDSRulesList()
{
  // Load userdata Mediaseconfig.xml
  CXBMCTinyXML MediasConfigXML;
  CStdString xmlFile;
  TiXmlElement *pRules;
  Get()->LoadDsXML(&MediasConfigXML, MEDIASCONFIG, pRules, xmlFile, true);
  if (!pRules)
    return -1;

  CStdString strName;
  CStdString strfileName;
  CStdString strfileTypes;
  CStdString strProtocols;
  CStdString strRule;
  int selected;
  int count = 0;

  CGUIDialogSelect *pDlg = (CGUIDialogSelect *)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (!pDlg)
    return -1;

  pDlg->SetHeading(60001);

  TiXmlElement *pRule = pRules->FirstChildElement("rule");
  while (pRule)
  {
    strName = pRule->Attribute("name");
    strfileTypes = pRule->Attribute("filetypes");
    strfileName = pRule->Attribute("filename");
    strProtocols = pRule->Attribute("protocols");

    if (strfileTypes != "")
      strfileTypes.Format("Filetypes=%s", strfileTypes);
    if (strfileName != "")
      strfileName.Format("Filename=%s", strfileName);
    if (strProtocols != "")
      strProtocols.Format("Protocols=%s", strProtocols);

    if (strName != "")
      strRule = strName;
    else
    {
      strRule.Format("%s %s %s", strfileTypes, strfileName, strProtocols);
      strRule.Trim();
    }

    strRule.Format("Rule:   %s", strRule);
    pDlg->Add(strRule);
    count++;
    pRule = pRule->NextSiblingElement("rule");
  }

  pDlg->Add(g_localizeStrings.Get(60014).c_str());

  pDlg->DoModal();
  selected = pDlg->GetSelectedLabel();
  if (selected == count)
   Get()->SetNewRule(true);
  else
    Get()->SetNewRule(false);
  Get()->SetRuleIndex(selected);

  if (selected > -1) g_windowManager.ActivateWindow(WINDOW_DIALOG_DSRULES);

  return selected;
}

void CGUIDialogDSRules::LoadDsXML(CXBMCTinyXML *XML, xmlType type, TiXmlElement* &pNode, CStdString &xmlFile, bool forceCreate /*= false*/)
{
  CStdString xmlRoot, xmlNode;
  pNode = NULL;
  if (type == MEDIASCONFIG)
  {
    xmlFile = CProfilesManager::Get().GetUserDataItem("dsplayer/mediasconfig.xml");
    xmlRoot = "mediasconfig";
    xmlNode = "rules";
  }
  if (type == FILTERSCONFIG)
  {
    xmlFile = CProfilesManager::Get().GetUserDataItem("dsplayer/filtersconfig.xml");
    xmlRoot = "filtersconfig";
    xmlNode = "filters";
  }
  if (type == HOMEFILTERSCONFIG)
  {
    xmlFile = "special://xbmc/system/players/dsplayer/filtersconfig.xml";
    xmlRoot = "filtersconfig";
    xmlNode = "filters";
  }

  if (type == SHADERS)
  {
    xmlFile = CProfilesManager::Get().GetUserDataItem("dsplayer/shaders.xml");
    xmlRoot = "shaders";
  }


  if (!XML->LoadFile(xmlFile))
  {
    CLog::Log(LOGERROR, "%s Error loading %s, Line %d (%s)", __FUNCTION__, xmlFile.c_str(), XML->ErrorRow(), XML->ErrorDesc());
    if (!forceCreate)
      return;

    TiXmlElement pRoot(xmlRoot.c_str());
    pRoot.InsertEndChild(TiXmlElement(xmlNode.c_str()));
    XML->InsertEndChild(pRoot);
  }
  TiXmlElement *pConfig = XML->RootElement();
  if (!pConfig || strcmpi(pConfig->Value(), xmlRoot.c_str()) != 0)
  {
    CLog::Log(LOGERROR, "%s Error loading medias configuration, no <%s> node", __FUNCTION__, xmlRoot.c_str());
    return;
  }
  if (type != SHADERS)
    pNode = pConfig->FirstChildElement(xmlNode.c_str());
  else
    pNode = pConfig;
}

void CGUIDialogDSRules::UrlOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  list.push_back(std::make_pair("[null]", "[null]"));
  list.push_back(std::make_pair("true", "true"));
  list.push_back(std::make_pair("false", "false"));
}

void CGUIDialogDSRules::FiltersConfigOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{

  list.push_back(std::make_pair("[null]", "[null]"));

  CXBMCTinyXML FiltersConfigXML;
  CStdString xmlFile;
  TiXmlElement *pFilters;

  // Load homedir filtersconfig.xml
  Get()->LoadDsXML(&FiltersConfigXML, HOMEFILTERSCONFIG, pFilters, xmlFile);

  CStdString strFilter;
  CStdString strFilterLabel;

  if (pFilters)
  {

    TiXmlElement *pFilter = pFilters->FirstChildElement("filter");
    while (pFilter)
    {
      TiXmlElement *pOsdname = pFilter->FirstChildElement("osdname");
      if (pOsdname)
      {
        XMLUtils::GetString(pFilter, "osdname", strFilterLabel);
        strFilter = pFilter->Attribute("name");
        strFilterLabel.Format("%s (%s)", strFilterLabel, strFilter);

        list.push_back(std::make_pair(strFilterLabel, strFilter));
      }
      pFilter = pFilter->NextSiblingElement("filter");
    }
  }

  std::sort(list.begin(), list.end(), compare_by_word);
  std::vector<DynamicStringSettingOption> tmp = list;

  // Load userdata Filterseconfig.xml
  Get()->LoadDsXML(&FiltersConfigXML, FILTERSCONFIG, pFilters, xmlFile);

  bool match = false;
  CStdString strTmp;

  if (pFilters)
  {

    TiXmlElement *pFilter = pFilters->FirstChildElement("filter");
    while (pFilter)
    {
      TiXmlElement *pOsdname = pFilter->FirstChildElement("osdname");
      if (pOsdname)
      {

        XMLUtils::GetString(pFilter, "osdname", strFilterLabel);
        strFilter = pFilter->Attribute("name");
        strFilterLabel.Format("%s (%s)", strFilterLabel, strFilter);

        std::vector<DynamicStringSettingOption>::const_iterator it;

        for (it = tmp.begin(); it != tmp.end(); ++it)
        {
          strTmp = it->second;
          if (strTmp == strFilter)
          {
            match = true;
            break;
          }
        }

        if (!match)
          list.push_back(std::make_pair(strFilterLabel, strFilter));
      }
      match = false;
      pFilter = pFilter->NextSiblingElement("filter");
    }
  }
  std::sort(list.begin(), list.end(), compare_by_word);
}

void CGUIDialogDSRules::ShadersOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{

  list.push_back(std::make_pair("[null]", "[null]"));

  CXBMCTinyXML ShadersXML;
  CStdString xmlFile;
  TiXmlElement *pShaders;

  // Load userdata shaders.xml
  Get()->LoadDsXML(&ShadersXML, SHADERS, pShaders, xmlFile);

  CStdString strShader;
  CStdString strShaderLabel;

  if (!pShaders)
    return;

  TiXmlElement *pShader = pShaders->FirstChildElement("shader");
  while (pShader)
  {
    strShaderLabel = pShader->Attribute("name");
    strShader = pShader->Attribute("id");
    strShaderLabel.Format("%s (%s)", strShaderLabel, strShader);

    list.push_back(std::make_pair(strShaderLabel, strShader));

    pShader = pShader->NextSiblingElement("shader");
  }
}





