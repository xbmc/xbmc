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

#include "GUIDialogDSPlayercoreFactory.h"
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
#include "dialogs/GUIDialogKaiToast.h"
#include "input/Key.h"
#include "utils/XMLUtils.h"
#include "Filters/RendererSettings.h"
#include "PixelShaderList.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"


#define SETTING_RULE_SAVE                   "dsplayercore.save"
#define SETTING_RULE_ADD                    "dsplayercore.add"
#define SETTING_RULE_DEL                    "dsplayercore.del"

using namespace std;

CGUIDialogDSPlayercoreFactory::CGUIDialogDSPlayercoreFactory()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_DSPLAYERCORE, "VideoOSDSettings.xml")
{
  m_dsmanager = CGUIDialogDSManager::Get();
}


CGUIDialogDSPlayercoreFactory::~CGUIDialogDSPlayercoreFactory()
{ }

CGUIDialogDSPlayercoreFactory *CGUIDialogDSPlayercoreFactory::m_pSingleton = NULL;

CGUIDialogDSPlayercoreFactory* CGUIDialogDSPlayercoreFactory::Get()
{
  return (m_pSingleton) ? m_pSingleton : (m_pSingleton = new CGUIDialogDSPlayercoreFactory());
}

void CGUIDialogDSPlayercoreFactory::OnInitWindow()
{
  CGUIDialogSettingsManualBase::OnInitWindow();
  isEdited = false;
}

void CGUIDialogDSPlayercoreFactory::OnDeinitWindow(int nextWindowID)
{
  CGUIDialogSettingsManualBase::OnDeinitWindow(nextWindowID);
  ShowDSPlayercoreFactory();
}

bool CGUIDialogDSPlayercoreFactory::OnBack(int actionID)
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

void CGUIDialogDSPlayercoreFactory::Save()
{

}
void CGUIDialogDSPlayercoreFactory::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  SetHeading(66001);
}

void CGUIDialogDSPlayercoreFactory::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  CSettingCategory *category = AddCategory("dsplayercoresettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogDSPlayercoreFactory: unable to setup settings");
    return;
  }

  // get all necessary setting groups
  CSettingGroup *group = AddGroup(category);
  if (group == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogDSPlayercoreFactory: unable to setup settings");
    return;
  }

  CSettingGroup *groupSave = AddGroup(category);
  if (groupSave == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogDSPlayercoreFactory: unable to setup settings");
    return;
  }

  if (m_ruleList.size() == 0)
  {
    m_dsmanager->InitConfig(m_ruleList, EDITATTR, "dsplayrule.name", 60002, "name");
    m_dsmanager->InitConfig(m_ruleList, EDITATTR, "dsplayrule.filetypes", 60003, "filetypes");
    m_dsmanager->InitConfig(m_ruleList, EDITATTR, "dsplayrule.filename", 60004, "filename");
    m_dsmanager->InitConfig(m_ruleList, EDITATTR, "dsplayrule.protocols", 60005, "protocols");
    m_dsmanager->InitConfig(m_ruleList, EDITATTR, "dsplayrule.videoresolution", 60019, "videoresolution");
    m_dsmanager->InitConfig(m_ruleList, EDITATTR, "dsplayrule.videocodec", 60020, "videocodec");
    m_dsmanager->InitConfig(m_ruleList, EDITATTR, "dsplayrule.videoaspect", 66010, "videoaspect");
    m_dsmanager->InitConfig(m_ruleList, EDITATTR, "dsplayrule.audiochannels", 60021, "audiochannels");
    m_dsmanager->InitConfig(m_ruleList, EDITATTR, "dsplayrule.audiocodec", 60022, "audiocodec");
    m_dsmanager->InitConfig(m_ruleList, EDITATTR, "dsplayrule.mimetypes", 66011, "mimetypes");

    m_dsmanager->InitConfig(m_ruleList, BOOLATTR, "dsplayrule.internetstream", 66003, "internetstream");
    m_dsmanager->InitConfig(m_ruleList, BOOLATTR, "dsplayrule.remote", 66004, "remote");
    m_dsmanager->InitConfig(m_ruleList, BOOLATTR, "dsplayrule.audio", 66005, "audio");
    m_dsmanager->InitConfig(m_ruleList, BOOLATTR, "dsplayrule.video", 66006, "video");
    m_dsmanager->InitConfig(m_ruleList, BOOLATTR, "dsplayrule.dvd", 66007, "dvd");
    m_dsmanager->InitConfig(m_ruleList, BOOLATTR, "dsplayrule.dvdimage", 66008, "dvdimage");
    m_dsmanager->InitConfig(m_ruleList, BOOLATTR, "dsplayrule.dvdfile", 66009, "dvdfile");
  }

  // Reset Button value
  m_dsmanager->ResetValue(m_ruleList);

  // Load userdata Playercorefactory.xml
  if (!m_dsmanager->GetisNew())
  {
    TiXmlElement *pRules;
    m_dsmanager->LoadDsXML(PLAYERCOREFACTORY, pRules);

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
      }
    }
  }

  // Stamp Button
  std::vector<DSConfigList *>::iterator it;

  for (it = m_ruleList.begin(); it != m_ruleList.end(); ++it)
  {
    if ((*it)->m_configType == EDITATTR)
      AddEdit(group, (*it)->m_setting, (*it)->m_label, 0, (*it)->m_value, true);

    if ((*it)->m_configType == BOOLATTR)
      AddToggle(group, (*it)->m_setting, (*it)->m_label, 0, (*it)->GetBoolValue());
  }

  if (m_dsmanager->GetisNew())
    AddButton(groupSave, SETTING_RULE_ADD, 60015, 0);
  else
  {
    AddButton(groupSave, SETTING_RULE_SAVE, 60016, 0);
    AddButton(groupSave, SETTING_RULE_DEL, 60017, 0);
  }
}

void CGUIDialogDSPlayercoreFactory::OnSettingChanged(const CSetting *setting)
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
}

void CGUIDialogDSPlayercoreFactory::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  // Load userdata playercorefactory.xml
  TiXmlElement *pRules;
  m_dsmanager->LoadDsXML(PLAYERCOREFACTORY, pRules, true);
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

    m_dsmanager->SaveDsXML(PLAYERCOREFACTORY);

    CPlayerCoreFactory::Get().OnSettingsLoaded();

    CGUIDialogDSPlayercoreFactory::Close();
  }

  // Add & Save Rule
  if (settingId == SETTING_RULE_ADD || settingId == SETTING_RULE_SAVE)
  {
    TiXmlElement pRule("rule");

    std::vector<DSConfigList *>::iterator it;
    for (it = m_ruleList.begin(); it != m_ruleList.end(); ++it)
    {
      if ((*it)->m_configType == EDITATTR && (*it)->m_value != "")
        pRule.SetAttribute((*it)->m_attr.c_str(), (*it)->m_value.c_str());

      if ((*it)->m_configType == BOOLATTR && (*it)->m_value != "false")
        pRule.SetAttribute((*it)->m_attr.c_str(), (*it)->m_value.c_str());
    }

    pRule.SetAttribute("player", "DVDPlayer");

    // SAVE
    if (settingId == SETTING_RULE_SAVE)
    {
      TiXmlElement *oldRule = m_dsmanager->KeepSelectedNode(pRules, "rule");
      pRules->ReplaceChild(oldRule, pRule);
    }

    if (settingId == SETTING_RULE_ADD)
      pRules->InsertEndChild(pRule);

    isEdited = false;
    m_dsmanager->SaveDsXML(PLAYERCOREFACTORY);

    CPlayerCoreFactory::Get().OnSettingsLoaded();

    CGUIDialogDSPlayercoreFactory::Close();
  }
}

int CGUIDialogDSPlayercoreFactory::ShowDSPlayercoreFactory()
{
  // Load userdata PlayercoreFactory.xml
  TiXmlElement *pRules;
  Get()->m_dsmanager->LoadDsXML(PLAYERCOREFACTORY, pRules, true);
  if (!pRules)
    return -1;

  int selected;
  int count = 0;

  CGUIDialogSelect *pDlg = (CGUIDialogSelect *)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (!pDlg)
    return -1;

  pDlg->SetHeading(66001);

  std::vector<std::string> vecAttr
  {
    "name", "filename", "filetypes", "protocols",
    "videocodec", "videoresolution", "videoaspect", "audiocodec", "audiochannels",
    "internetstream", "remote", "audio", "video", "dvd", "dvdimage", "dvdfile", "mimetypes"
  };

  CStdString value;
  CStdString strLabel;

  TiXmlElement *pRule = pRules->FirstChildElement("rule");
  while (pRule)
  {
    strLabel = "Rule: ";
    value = pRule->Attribute("name");
    if (value != "")
      strLabel.Format("%s%s ", strLabel.c_str(), value.c_str());

    else
    {
      int countAttr = 0;
      std::vector<std::string>::iterator it;
      for (it = vecAttr.begin(); it != vecAttr.end(); ++it)
      {

        value = pRule->Attribute((*it).c_str());
        if (value != "")
        {
          strLabel.Format("%s %s=%s ", strLabel.c_str(), (*it).c_str(), value.c_str());
          countAttr++;
        }
        if (countAttr > 1)
          break;
      }
    }

    // Load only dvdplayer rules
    value = pRule->Attribute("player");
    if (value.ToLower() == "dvdplayer")
      pDlg->Add(strLabel);

    pRule = pRule->NextSiblingElement("rule");
    count++;
  }

  pDlg->Add(g_localizeStrings.Get(66002).c_str());
  pDlg->Open();

  selected = pDlg->GetSelectedLabel();
  Get()->m_dsmanager->SetisNew(selected == count);

  Get()->m_dsmanager->SetConfigIndex(selected);

  if (selected > -1) g_windowManager.ActivateWindow(WINDOW_DIALOG_DSPLAYERCORE);

  return selected;
}


