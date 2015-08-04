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

#include "GUIDialogDSFilters.h"
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
#include "cores/DSPlayer/Utils/DSFilterEnumerator.h"
#include "utils/XMLUtils.h"
#include "Filters/RendererSettings.h"
#include "PixelShaderList.h"

#define SETTING_FILTER_SAVE                   "dsfilters.save"
#define SETTING_FILTER_ADD                    "dsfilters.add"
#define SETTING_FILTER_DEL                    "dsfilters.del"

using namespace std;

CGUIDialogDSFilters::CGUIDialogDSFilters()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_DSFILTERS, "VideoOSDSettings.xml")
{
  m_dsmanager = CGUIDialogDSManager::Get();
}


CGUIDialogDSFilters::~CGUIDialogDSFilters()
{ }

CGUIDialogDSFilters *CGUIDialogDSFilters::m_pSingleton = NULL;

CGUIDialogDSFilters* CGUIDialogDSFilters::Get()
{
  return (m_pSingleton) ? m_pSingleton : (m_pSingleton = new CGUIDialogDSFilters());
}

void CGUIDialogDSFilters::OnInitWindow()
{
  CGUIDialogSettingsManualBase::OnInitWindow();
  isEdited = false;
}

void CGUIDialogDSFilters::OnDeinitWindow(int nextWindowID)
{
  CGUIDialogSettingsManualBase::OnDeinitWindow(nextWindowID);
  ShowDSFiltersList();
}

bool CGUIDialogDSFilters::OnBack(int actionID)
{
  if (isEdited)
  {
    if (CGUIDialogYesNo::ShowAndGetInput(61001, 61002, 0, 0))
    {
      CSetting *setting;
      if (!m_dsmanager->GetisNew())
        setting = GetSetting(SETTING_FILTER_SAVE);
      else
        setting = GetSetting(SETTING_FILTER_ADD);

      OnSettingAction(setting);

      if (isEdited)
        return false;
    }
  }

  return CGUIDialogSettingsManualBase::OnBack(actionID);
}

void CGUIDialogDSFilters::Save()
{

}
void CGUIDialogDSFilters::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  SetHeading(65001);
}

void CGUIDialogDSFilters::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  CSettingCategory *category = AddCategory("dsfiltersettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogDSFilters: unable to setup settings");
    return;
  }

  // get all necessary setting groups
  CSettingGroup *group = AddGroup(category);
  if (group == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogDSFilters: unable to setup settings");
    return;
  }

  CSettingGroup *groupSystem = AddGroup(category);
  if (groupSystem == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogDSFilters: unable to setup settings");
    return;
  }

  CSettingGroup *groupSave = AddGroup(category);
  if (groupSave == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogDSFilters: unable to setup settings");
    return;
  }

  // Init variables
  CStdString strGuid;

  if (m_filterList.size() == 0)
  {
    m_dsmanager->InitConfig(m_filterList, OSDGUID, "dsfilters.osdname", 65003, "", "osdname");
    m_dsmanager->InitConfig(m_filterList, EDITATTR, "dsfilters.name", 65004, "name");
    m_dsmanager->InitConfig(m_filterList, FILTER, "dsfilters.type", 65005, "type", "", TypeOptionFiller);
    m_dsmanager->InitConfig(m_filterList, OSDGUID, "dsfilters.guid", 65006, "", "guid");
    m_dsmanager->InitConfig(m_filterList, FILTERSYSTEM, "dsfilters.systemfilter", 65010, "", "", m_dsmanager->DSFilterOptionFiller);
  }

  // Reset Button value
  m_dsmanager->ResetValue(m_filterList);

  // Load userdata Filteseconfig.xml
  if (!m_dsmanager->GetisNew())
  {
    TiXmlElement *pFilters;
    m_dsmanager->LoadDsXML(FILTERSCONFIG, pFilters);

    if (pFilters)
    {
      TiXmlElement *pFilter = m_dsmanager->KeepSelectedNode(pFilters, "filter");

      std::vector<DSConfigList *>::iterator it;
      for (it = m_filterList.begin(); it != m_filterList.end(); ++it)
      {
        if ((*it)->m_configType == EDITATTR || (*it)->m_configType == FILTER)
          (*it)->m_value = pFilter->Attribute((*it)->m_attr.c_str());

        if ((*it)->m_configType == OSDGUID) {
          XMLUtils::GetString(pFilter, (*it)->m_nodeName.c_str(), strGuid);
          (*it)->m_value = strGuid;
        }
        if ((*it)->m_configType == FILTERSYSTEM)
          (*it)->m_value = strGuid;
      }
    }
  }

  // Stamp Button
  std::vector<DSConfigList *>::iterator it;

  for (it = m_filterList.begin(); it != m_filterList.end(); ++it)
  {
    if ((*it)->m_configType == EDITATTR || (*it)->m_configType == OSDGUID)
      AddEdit(group, (*it)->m_setting, (*it)->m_label, 0, (*it)->m_value, true);

    if ((*it)->m_configType == FILTER)
      AddList(group, (*it)->m_setting, (*it)->m_label, 0, (*it)->m_value, (*it)->m_filler, (*it)->m_label);

    if ((*it)->m_configType == FILTERSYSTEM)
      AddList(groupSystem, (*it)->m_setting, (*it)->m_label, 0, (*it)->m_value, (*it)->m_filler, (*it)->m_label);
  }

  if (m_dsmanager->GetisNew())
    AddButton(groupSave, SETTING_FILTER_ADD, 65007, 0);
  else
  {
    AddButton(groupSave, SETTING_FILTER_SAVE, 65008, 0);
    AddButton(groupSave, SETTING_FILTER_DEL, 65009, 0);
  }
}

void CGUIDialogDSFilters::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  isEdited = true;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);
  const std::string &settingId = setting->GetId();

  std::vector<DSConfigList *>::iterator it;
  for (it = m_filterList.begin(); it != m_filterList.end(); ++it)
  {
    if ((*it)->m_configType == EDITATTR
      || (*it)->m_configType == FILTER
      || (*it)->m_configType == OSDGUID)
    {
      if (settingId == (*it)->m_setting)
      {
        (*it)->m_value = static_cast<std::string>(static_cast<const CSettingString*>(setting)->GetValue());
      }
    }
    if ((*it)->m_configType == FILTERSYSTEM)
    {
      if (settingId == "dsfilters.systemfilter")
      {
        (*it)->m_value = static_cast<std::string>(static_cast<const CSettingString*>(setting)->GetValue());

        if ((*it)->m_value != "[null]")
        {
          CStdString strOSDName = GetFilterName((*it)->m_value);
          CStdString strFilterName = strOSDName;
          strFilterName.ToLower();
          strFilterName.Replace(" ", "_");

          m_settingsManager->SetString("dsfilters.guid", (*it)->m_value.c_str());
          m_settingsManager->SetString("dsfilters.osdname", strOSDName);
          m_settingsManager->SetString("dsfilters.name", strFilterName);
        }
      }
    }
  }
}

void CGUIDialogDSFilters::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  // Load userdata Filteseconfig.xml
  TiXmlElement *pFilters;
  m_dsmanager->LoadDsXML(FILTERSCONFIG, pFilters, true);
  if (!pFilters)
    return;

  // Init variables
  CGUIDialogSettingsManualBase::OnSettingAction(setting);
  const std::string &settingId = setting->GetId();

  // Del Filter
  if (settingId == SETTING_FILTER_DEL)
  {
    if (!CGUIDialogYesNo::ShowAndGetInput(65009, 65011, 0, 0))
      return;

    TiXmlElement *oldRule = m_dsmanager->KeepSelectedNode(pFilters, "filter");
    pFilters->RemoveChild(oldRule);

    m_dsmanager->SaveDsXML(FILTERSCONFIG);
    CGUIDialogDSFilters::Close();
  }

  // Add & Save Filter
  if (settingId == SETTING_FILTER_ADD || settingId == SETTING_FILTER_SAVE)
  {
    TiXmlElement pFilter("filter");

    std::vector<DSConfigList *>::iterator it;
    for (it = m_filterList.begin(); it != m_filterList.end(); ++it)
    {
      if ((*it)->m_value == "" || (*it)->m_value == "[null]")
      {
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(65001), g_localizeStrings.Get(65012), 2000, false, 300);
        return;
      }

      if ((*it)->m_configType == EDITATTR || (*it)->m_configType == FILTER)
        pFilter.SetAttribute((*it)->m_attr.c_str(), (*it)->m_value.c_str());

      if ((*it)->m_configType == OSDGUID)
      {
        TiXmlElement newElement((*it)->m_nodeName.c_str());
        TiXmlNode *pNewNode = pFilter.InsertEndChild(newElement);
        TiXmlText value((*it)->m_value.c_str());
        pNewNode->InsertEndChild(value);
      }
    }

    // SAVE
    if (settingId == SETTING_FILTER_SAVE)
    {
      TiXmlElement *oldFilter = m_dsmanager->KeepSelectedNode(pFilters, "filter");
      pFilters->ReplaceChild(oldFilter, pFilter);
    }

    if (settingId == SETTING_FILTER_ADD)
      pFilters->InsertEndChild(pFilter);

    isEdited = false;
    m_dsmanager->SaveDsXML(FILTERSCONFIG);
    CGUIDialogDSFilters::Close();
  }
}

int CGUIDialogDSFilters::ShowDSFiltersList()
{
  // Load userdata Filterseconfig.xml
  TiXmlElement *pFilters;
  Get()->m_dsmanager->LoadDsXML(FILTERSCONFIG, pFilters, true);
  if (!pFilters)
    return -1;

  int selected;
  int count = 0;

  CGUIDialogSelect *pDlg = (CGUIDialogSelect *)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (!pDlg)
    return -1;

  pDlg->SetHeading(65001);

  CStdString strFilter;
  CStdString strFilterLabel;

  TiXmlElement *pFilter = pFilters->FirstChildElement("filter");
  while (pFilter)
  {
    strFilterLabel = "";
    strFilter = "";

    TiXmlElement *pOsdname = pFilter->FirstChildElement("osdname");
    if (pOsdname)
      XMLUtils::GetString(pFilter, "osdname", strFilterLabel);

    strFilter = pFilter->Attribute("name");
    strFilterLabel.Format("%s (%s)", strFilterLabel, strFilter);
    pDlg->Add(strFilterLabel.c_str()); 
    count++;

    pFilter = pFilter->NextSiblingElement("filter");
  }

  pDlg->Add(g_localizeStrings.Get(65002).c_str());

  pDlg->Open();
  selected = pDlg->GetSelectedLabel();

  Get()->m_dsmanager->SetisNew(selected == count);

  Get()->m_dsmanager->SetConfigIndex(selected);

  if (selected > -1) g_windowManager.ActivateWindow(WINDOW_DIALOG_DSFILTERS);

  return selected;
}

void CGUIDialogDSFilters::TypeOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  list.push_back(std::make_pair("", "[null]"));
  list.push_back(std::make_pair("Source Filter (source)", "source"));
  list.push_back(std::make_pair("Splitter Filter (splitter)", "splitter"));
  list.push_back(std::make_pair("Video Decoder (videodec)", "videodec"));
  list.push_back(std::make_pair("Audio Decoder (audiodec)", "audiodec"));
  list.push_back(std::make_pair("Subtitles Filter (subs)", "subs"));
  list.push_back(std::make_pair("Extra Filter (extra)", "extra"));
}

CStdString CGUIDialogDSFilters::GetFilterName(CStdString guid)
{
  CDSFilterEnumerator p_dfilter;
  std::vector<DSFiltersInfo> filterList;
  p_dfilter.GetDSFilters(filterList);

  std::vector<DSFiltersInfo>::const_iterator iter = filterList.begin();

  for (int i = 1; iter != filterList.end(); i++)
  {
    DSFiltersInfo filter = *iter;
    if (guid == filter.lpstrGuid)
    {
      return filter.lpstrName;
    }
    ++iter;
  }
  return "";
}


