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
#include "input/Key.h"
#include "cores/DSPlayer/Utils/DSFilterEnumerator.h"
#include "utils/XMLUtils.h"
#include "Filters/RendererSettings.h"
#include "PixelShaderList.h"

#define SETTING_RULE_SAVE                     "rule.save"
#define SETTING_RULE_ADD                      "rule.add"
#define SETTING_RULE_DEL                      "rule.del"

using namespace std;

CGUIDialogDSRules::CGUIDialogDSRules()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_DSRULES, "VideoOSDSettings.xml")
{
  m_dsmanager = CGUIDialogDSManager::Get();
  m_allowchange = true;
}


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
      CSetting *setting;
      if (!m_dsmanager->GetisNew())
        setting = GetSetting(SETTING_RULE_SAVE);
      else
        setting = GetSetting(SETTING_RULE_ADD);

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

  SetHeading(60001);
}



void CGUIDialogDSRules::SetVisible(CStdString id, bool visible, ConfigType subType, bool isChild /* = false */)
{
  CSetting *setting = m_settingsManager->GetSetting(id);
  if (setting->IsVisible() && visible)
    return;
  setting->SetVisible(visible);
  setting->SetEnabled(visible);
  if (!isChild)
    m_settingsManager->SetString(id, "[null]");
  else
  { 
    if (subType == SPINNERATTRSHADER)
      m_settingsManager->SetString(id, "prescale");
    else
      m_settingsManager->SetString(id, "");
  }
}

void CGUIDialogDSRules::HideUnused()
{
  if (!m_allowchange)
    return;

  m_allowchange = false;

  HideUnused(EXTRAFILTER, EDITATTREXTRA);
  HideUnused(SHADER, EDITATTRSHADER);
  HideUnused(SHADER, SPINNERATTRSHADER);

  m_allowchange = true;
}

void CGUIDialogDSRules::HideUnused(ConfigType type, ConfigType subType)
{
  int count = 0;
  bool show;
  bool isMadvr = (CSettings::GetInstance().GetString(CSettings::SETTING_DSPLAYER_VIDEORENDERER) == "madVR");

  std::vector<DSConfigList *>::iterator it;
  for (it = m_ruleList.begin(); it != m_ruleList.end(); ++it)
  {
    if ((*it)->m_configType == type)
    {
      if ((*it)->m_value == "[null]")
        count++;

      show = (count > 1 && (*it)->m_value == "[null]");
      SetVisible((*it)->m_setting.c_str(), !show, subType);

      show = (count > 0 && (*it)->m_value == "[null]");
      std::vector<DSConfigList *>::iterator itchild;
      for (itchild = m_ruleList.begin(); itchild != m_ruleList.end(); ++itchild)
      {
        if ((*itchild)->m_configType == subType && (*it)->m_subNode == (*itchild)->m_subNode)
        {
          if (!isMadvr && subType == SPINNERATTRSHADER)
            SetVisible((*itchild)->m_setting.c_str(), false, subType, true);
          else
            SetVisible((*itchild)->m_setting.c_str(), !show, subType, true);
        }
      }
    }
  }
}

bool CGUIDialogDSRules::NodeHasAttr(TiXmlElement *pNode, CStdString attr)
{
  if (pNode)
  {
    CStdString value = "";
    value = pNode->Attribute(attr.c_str());
    return (value != "");
  }

  return false;
}

void CGUIDialogDSRules::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  CSettingCategory *category = AddCategory("dsrulesettings", -1);
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

  if (m_ruleList.size() == 0)
  {
    // RULE
    m_dsmanager->InitConfig(m_ruleList, EDITATTR, "rules.name", 60002, "name");
    m_dsmanager->InitConfig(m_ruleList, EDITATTR, "rules.filetypes", 60003, "filetypes");
    m_dsmanager->InitConfig(m_ruleList, EDITATTR, "rules.filename", 60004, "filename");
    m_dsmanager->InitConfig(m_ruleList, EDITATTR, "rules.protocols", 60005, "protocols");
    m_dsmanager->InitConfig(m_ruleList, BOOLATTR, "rules.url", 60006, "url");

    // FILTER
    m_dsmanager->InitConfig(m_ruleList, FILTER, "rules.source", 60007, "filter", "source", m_dsmanager->AllFiltersConfigOptionFiller);
    m_dsmanager->InitConfig(m_ruleList, FILTER, "rules.splitter", 60008, "filter", "splitter", m_dsmanager->AllFiltersConfigOptionFiller);
    m_dsmanager->InitConfig(m_ruleList, FILTER, "rules.video", 60009, "filter", "video", m_dsmanager->AllFiltersConfigOptionFiller);
    m_dsmanager->InitConfig(m_ruleList, FILTER, "rules.audio", 60010, "filter", "audio", m_dsmanager->AllFiltersConfigOptionFiller);
    m_dsmanager->InitConfig(m_ruleList, FILTER, "rules.subs", 60011, "filter", "subs", m_dsmanager->AllFiltersConfigOptionFiller);

    // EXTRAFILTER
    m_dsmanager->InitConfig(m_ruleList, EXTRAFILTER, "rules.extra0", 60012, "filter", "extra", m_dsmanager->AllFiltersConfigOptionFiller, 0, "extra");
    m_dsmanager->InitConfig(m_ruleList, EDITATTREXTRA, "rules.videores0", 60019, "videoresolution", "extra", 0, 0, "extra");
    m_dsmanager->InitConfig(m_ruleList, EDITATTREXTRA, "rules.videocodec0", 60020, "videocodec", "extra", 0, 0, "extra");
    m_dsmanager->InitConfig(m_ruleList, EDITATTREXTRA, "rules.audiochans0", 60021, "audiochannels", "extra", 0, 0, "extra");
    m_dsmanager->InitConfig(m_ruleList, EDITATTREXTRA, "rules.audiocodec0", 60022, "audiocodec", "extra", 0, 0, "extra");

    m_dsmanager->InitConfig(m_ruleList, EXTRAFILTER, "rules.extra1", 60012, "filter", "extra", m_dsmanager->AllFiltersConfigOptionFiller, 1, "extra");
    m_dsmanager->InitConfig(m_ruleList, EDITATTREXTRA, "rules.videores1", 60019, "videoresolution", "extra", 0, 1, "extra");
    m_dsmanager->InitConfig(m_ruleList, EDITATTREXTRA, "rules.videocodec1", 60020, "videocodec", "extra", 0, 1, "extra");
    m_dsmanager->InitConfig(m_ruleList, EDITATTREXTRA, "rules.audiochans1", 60021, "audiochannels", "extra", 0, 1, "extra");
    m_dsmanager->InitConfig(m_ruleList, EDITATTREXTRA, "rules.audiocodec1", 60022, "audiocodec", "extra", 0, 1, "extra");

    m_dsmanager->InitConfig(m_ruleList, EXTRAFILTER, "rules.extra2", 60012, "filter", "extra", m_dsmanager->AllFiltersConfigOptionFiller, 2, "extra");
    m_dsmanager->InitConfig(m_ruleList, EDITATTREXTRA, "rules.videores2", 60019, "videoresolution", "extra", 0, 2, "extra");
    m_dsmanager->InitConfig(m_ruleList, EDITATTREXTRA, "rules.videocodec2", 60020, "videocodec", "extra", 0, 2, "extra");
    m_dsmanager->InitConfig(m_ruleList, EDITATTREXTRA, "rules.audiochans2", 60021, "audiochannels", "extra", 0, 2, "extra");
    m_dsmanager->InitConfig(m_ruleList, EDITATTREXTRA, "rules.audiocodec2", 60022, "audiocodec", "extra", 0, 2, "extra");

    // SHADER
    m_dsmanager->InitConfig(m_ruleList, SHADER, "rules.shader0", 60013, "id", "shader", m_dsmanager->ShadersOptionFiller, 0, "shaders");
    m_dsmanager->InitConfig(m_ruleList, SPINNERATTRSHADER, "rules.shprepost0", 60023, "stage", "shader", m_dsmanager->ShadersScaleOptionFiller, 0, "shaders");
    m_dsmanager->InitConfig(m_ruleList, EDITATTRSHADER, "rules.shvideores0", 60019, "videoresolution", "shader", 0, 0, "shaders");
    m_dsmanager->InitConfig(m_ruleList, EDITATTRSHADER, "rules.shvideocodec0", 60020, "videocodec", "shader", 0, 0, "shaders");

    m_dsmanager->InitConfig(m_ruleList, SHADER, "rules.shader1", 60013, "id", "shader", m_dsmanager->ShadersOptionFiller, 1, "shaders");
    m_dsmanager->InitConfig(m_ruleList, SPINNERATTRSHADER, "rules.shprepost1", 60023, "stage", "shader", m_dsmanager->ShadersScaleOptionFiller, 1, "shaders");
    m_dsmanager->InitConfig(m_ruleList, EDITATTRSHADER, "rules.shvideores1", 60019, "videoresolution", "shader", 0, 1, "shaders");
    m_dsmanager->InitConfig(m_ruleList, EDITATTRSHADER, "rules.shvideocodec1", 60020, "videocodec", "shader", 0, 1, "shaders");

    m_dsmanager->InitConfig(m_ruleList, SHADER, "rules.shader2", 60013, "id", "shader", m_dsmanager->ShadersOptionFiller, 2, "shaders");
    m_dsmanager->InitConfig(m_ruleList, SPINNERATTRSHADER, "rules.shprepost2", 60023, "stage", "shader", m_dsmanager->ShadersScaleOptionFiller, 2, "shaders");
    m_dsmanager->InitConfig(m_ruleList, EDITATTRSHADER, "rules.shvideores2", 60019, "videoresolution", "shader", 0, 2, "shaders");
    m_dsmanager->InitConfig(m_ruleList, EDITATTRSHADER, "rules.shvideocodec2", 60020, "videocodec", "shader", 0, 2, "shaders");
  }

  // Reset Button value
  m_dsmanager->ResetValue(m_ruleList);

  // Load userdata Mediaseconfig.xml
  if (!(m_dsmanager->GetisNew()))
  {
    TiXmlElement *pRules;
    m_dsmanager->LoadDsXML(MEDIASCONFIG, pRules);

    if (pRules)
    {
      TiXmlElement *pRule = m_dsmanager->KeepSelectedNode(pRules, "rule");

      std::vector<DSConfigList *>::iterator it;
      for (it = m_ruleList.begin(); it != m_ruleList.end(); ++it)
      {
        if ((*it)->m_configType == EDITATTR)
          (*it)->m_value = pRule->Attribute((*it)->m_attr.c_str());

        if ((*it)->m_configType == BOOLATTR)
        {
          (*it)->m_value = pRule->Attribute((*it)->m_attr.c_str());
          if ((*it)->m_value == "")
            (*it)->m_value = "false";
        }

        if ((*it)->m_configType == FILTER)
        {
          TiXmlElement *pFilter = pRule->FirstChildElement((*it)->m_nodeName.c_str());
          if (pFilter)
            (*it)->m_value = pFilter->Attribute((*it)->m_attr.c_str());
        }

        if ((*it)->m_configType == EXTRAFILTER
          || (*it)->m_configType == SHADER
          || (*it)->m_configType == EDITATTREXTRA
          || (*it)->m_configType == SPINNERATTRSHADER
          || (*it)->m_configType == EDITATTRSHADER)
        {
          TiXmlElement *pFilter;

          pFilter = pRule->FirstChildElement((*it)->m_nodeName.c_str());

          if (pFilter && NodeHasAttr(pFilter, (*it)->m_attr))
          {
            if ((*it)->m_subNode == 0)
              (*it)->m_value = pFilter->Attribute((*it)->m_attr.c_str());

            continue;
          }

          pFilter = pRule->FirstChildElement((*it)->m_nodeList.c_str());
          if (pFilter)
          {
            int countsize = 0;
            TiXmlElement *pSubExtra = pFilter->FirstChildElement((*it)->m_nodeName.c_str());
            while (pSubExtra)
            {
              if ((*it)->m_subNode == countsize)
                (*it)->m_value = pSubExtra->Attribute((*it)->m_attr.c_str());

              pSubExtra = pSubExtra->NextSiblingElement((*it)->m_nodeName.c_str());
              countsize++;
            }
          }
        }
      }
    }
  }

  // Stamp Button
  std::vector<DSConfigList *>::iterator it;
  CSettingGroup *groupTmp;

  for (it = m_ruleList.begin(); it != m_ruleList.end(); ++it)
  {
    if ((*it)->m_configType == EDITATTR
      || (*it)->m_configType == EDITATTREXTRA
      || (*it)->m_configType == EDITATTRSHADER)
    {
      if ((*it)->m_attr == "name")
        groupTmp = groupName;
      else
        groupTmp = groupRule;

      if ((*it)->m_configType == EDITATTREXTRA || (*it)->m_configType == EDITATTRSHADER)
        groupTmp = groupExtra;

      AddEdit(groupTmp, (*it)->m_setting, (*it)->m_label, 0, (*it)->m_value.c_str(), true);
    }
    if ((*it)->m_configType == SPINNERATTRSHADER)
      AddSpinner(groupExtra, (*it)->m_setting, (*it)->m_label, 0, (*it)->m_value, (*it)->m_filler);

    if ((*it)->m_configType == BOOLATTR)
      AddToggle(groupRule, (*it)->m_setting, (*it)->m_label, 0, (*it)->GetBoolValue());

    if ((*it)->m_configType == FILTER)
      AddList(groupFilter, (*it)->m_setting, (*it)->m_label, 0, (*it)->m_value, (*it)->m_filler, (*it)->m_label);

    if ((*it)->m_configType == EXTRAFILTER || (*it)->m_configType == SHADER)
      AddList(groupExtra, (*it)->m_setting, (*it)->m_label, 0, (*it)->m_value, (*it)->m_filler, (*it)->m_label);
  }


  if (m_dsmanager->GetisNew())
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

  std::vector<DSConfigList *>::iterator it;
  for (it = m_ruleList.begin(); it != m_ruleList.end(); ++it)
  {
    if (settingId == (*it)->m_setting)
    {
      if ((*it)->m_configType != BOOLATTR)
        (*it)->m_value = static_cast<std::string>(static_cast<const CSettingString*>(setting)->GetValue());
      else
        (*it)->SetBoolValue(static_cast<bool>(static_cast<const CSettingBool*>(setting)->GetValue()));
    }
  }
  HideUnused();
}

void CGUIDialogDSRules::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  // Load userdata Mediaseconfig.xml
  TiXmlElement *pRules;
  m_dsmanager->LoadDsXML(MEDIASCONFIG, pRules, true);
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

    TiXmlElement *oldRule = m_dsmanager->KeepSelectedNode(pRules, "rule");
    pRules->RemoveChild(oldRule);

    m_dsmanager->SaveDsXML(MEDIASCONFIG);
    CGUIDialogDSRules::Close();
  }

  // Add & Save Rule
  if (settingId == SETTING_RULE_SAVE || settingId == SETTING_RULE_ADD)
  {

    bool isMadvr = (CSettings::GetInstance().GetString(CSettings::SETTING_DSPLAYER_VIDEORENDERER) == "madVR");
    TiXmlElement pRule("rule");

    std::vector<DSConfigList *>::iterator it;
    for (it = m_ruleList.begin(); it != m_ruleList.end(); ++it)
    {
      if ((*it)->m_configType == EDITATTR && (*it)->m_value != "")
        pRule.SetAttribute((*it)->m_attr.c_str(), (*it)->m_value.c_str());

      if ((*it)->m_configType == BOOLATTR && (*it)->m_value != "false")
        pRule.SetAttribute((*it)->m_attr.c_str(), (*it)->m_value.c_str());

      if ((*it)->m_configType == FILTER && (*it)->m_value != "[null]")
      {
        pRule.InsertEndChild(TiXmlElement((*it)->m_nodeName.c_str()));
        TiXmlElement *pFilter = pRule.FirstChildElement((*it)->m_nodeName.c_str());
        if (pFilter)
          pFilter->SetAttribute((*it)->m_attr.c_str(), (*it)->m_value.c_str());
      }

      if (((*it)->m_configType == EXTRAFILTER
        || (*it)->m_configType == SHADER
        || (*it)->m_configType == EDITATTREXTRA
        || (*it)->m_configType == EDITATTRSHADER
        || (*it)->m_configType == SPINNERATTRSHADER)
        && (*it)->m_value != "[null]" && (*it)->m_value != "")
      {

        if (!isMadvr && (*it)->m_configType == SPINNERATTRSHADER)
          continue;

        TiXmlElement *pExtra = pRule.FirstChildElement((*it)->m_nodeList.c_str());
        if (!pExtra && (*it)->m_configType != EDITATTREXTRA && (*it)->m_configType != EDITATTRSHADER && (*it)->m_configType != SPINNERATTRSHADER)
        {
          pRule.InsertEndChild(TiXmlElement((*it)->m_nodeList.c_str()));
          pExtra = pRule.FirstChildElement((*it)->m_nodeList.c_str());
        }
        if ((*it)->m_configType != EDITATTREXTRA && (*it)->m_configType != EDITATTRSHADER && (*it)->m_configType != SPINNERATTRSHADER)
          pExtra->InsertEndChild(TiXmlElement((*it)->m_nodeName.c_str()));

        if (!pExtra)
          continue;

        TiXmlElement *pSubExtra = pExtra->FirstChildElement((*it)->m_nodeName.c_str());

        int countsize = 0;
        while (pSubExtra)
        {
          if ((*it)->m_subNode == countsize)
            pSubExtra->SetAttribute((*it)->m_attr.c_str(), (*it)->m_value.c_str());

          pSubExtra = pSubExtra->NextSiblingElement((*it)->m_nodeName.c_str());
          countsize++;
        }
      }
    }

    // SAVE
    if (settingId == SETTING_RULE_SAVE)
    {
      TiXmlElement *oldRule = m_dsmanager->KeepSelectedNode(pRules, "rule");
      pRules->ReplaceChild(oldRule, pRule);
    }

    if (settingId == SETTING_RULE_ADD)
      pRules->InsertEndChild(pRule);

    isEdited = false;
    m_dsmanager->SaveDsXML(MEDIASCONFIG);
    CGUIDialogDSRules::Close();
  }
}

int CGUIDialogDSRules::ShowDSRulesList()
{
  // Load userdata Mediaseconfig.xml
  TiXmlElement *pRules;
  Get()->m_dsmanager->LoadDsXML(MEDIASCONFIG, pRules, true);
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

  pDlg->Open();
  selected = pDlg->GetSelectedLabel();

  Get()->m_dsmanager->SetisNew(selected == count);

  Get()->m_dsmanager->SetConfigIndex(selected);

  if (selected > -1) g_windowManager.ActivateWindow(WINDOW_DIALOG_DSRULES);

  return selected;
}






