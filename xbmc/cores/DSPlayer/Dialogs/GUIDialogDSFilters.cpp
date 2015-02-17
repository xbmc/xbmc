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
#include "guilib/Key.h"
#include "cores/DSPlayer/Utils/DSFilterEnumerator.h"
#include "utils/XMLUtils.h"
#include "Filters/RendererSettings.h"
#include "PixelShaderList.h"

#define SETTING_FILTER_SAVE                   "dsfilters.save"
#define SETTING_FILTER_ADD                    "dsfilters.add"
#define SETTING_FILTER_DEL                    "dsfilters.del"

using namespace std;

DSFiltersList::DSFiltersList(FilterType type) :

settingFilter(""),
strFilterName(""),
strFilterAttr(""),
strFilterValue(""),
filterLabel(0),
filler(NULL),
m_filterType(type)
{
}

CGUIDialogDSFilters::CGUIDialogDSFilters()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_DSFILTERS, "VideoOSDSettings.xml")
{ }


CGUIDialogDSFilters::~CGUIDialogDSFilters()
{ }

CGUIDialogDSFilters *CGUIDialogDSFilters::m_pSingleton = NULL;

CGUIDialogDSFilters* CGUIDialogDSFilters::Get()
{
  return (m_pSingleton) ? m_pSingleton : (m_pSingleton = new CGUIDialogDSFilters());
}

void CGUIDialogDSFilters::OnDeinitWindow(int nextWindowID)
{
  CGUIDialogSettingsManualBase::OnDeinitWindow(nextWindowID);
  ShowDSFiltersList();

}

void CGUIDialogDSFilters::Save()
{

}
void CGUIDialogDSFilters::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  SetHeading(60001);
}

void CGUIDialogDSFilters::SetNewFilter(bool b)
{
  m_newfilter = b;
}

bool CGUIDialogDSFilters::GetNewFilter()
{
  return m_newfilter;
}

void CGUIDialogDSFilters::SetFilterIndex(int index)
{
  m_filterIndex = index;
}

int CGUIDialogDSFilters::GetFilterIndex()
{
  return m_filterIndex;
}

bool CGUIDialogDSFilters::compare_by_word(const DynamicStringSettingOption& lhs, const DynamicStringSettingOption& rhs)
{
  CStdString strLine1 = lhs.first;
  CStdString strLine2 = rhs.first;
  StringUtils::ToLower(strLine1);
  StringUtils::ToLower(strLine2);
  return strcmp(strLine1.c_str(), strLine2.c_str()) < 0;
}

void CGUIDialogDSFilters::InitFilters(FilterType type, CStdString settingFilter, int filterLabel, CStdString strFilterAttr /* = "" */, CStdString strFilterName /*= "" */, StringSettingOptionsFiller filler /* = NULL */)
{
  DSFiltersList* filterList = new DSFiltersList(type);
  filterList->settingFilter = settingFilter;
  filterList->strFilterAttr = strFilterAttr;
  filterList->strFilterName = strFilterName;
  filterList->filler = filler;
  filterList->filterLabel = filterLabel;
  m_filterList.push_back(filterList);
}

void CGUIDialogDSFilters::ResetValue()
{
  std::vector<DSFiltersList *>::iterator it;
  for (it = m_filterList.begin(); it != m_filterList.end(); ++it)
  {
    if ((*it)->m_filterType == EDITATTRFILTER || (*it)->m_filterType == OSDGUID)
      (*it)->strFilterValue = "";
    if ((*it)->m_filterType == SPINNERATTRFILTER)
      (*it)->strFilterValue = "[null]";
    if ((*it)->m_filterType == FILTERSYSTEM)
      (*it)->strFilterValue = "[null]";
  }
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
  int selected = Get()->GetFilterIndex();
  int count = 0;
  CStdString strGuid;

  if (m_filterList.size() == 0)
  {
    InitFilters(OSDGUID, "dsfilters.osdname", 65003, "","osdname");
    InitFilters(EDITATTRFILTER, "dsfilters.name", 65004, "name");
    InitFilters(SPINNERATTRFILTER, "dsfilters.type", 65005, "type", "", TypeOptionFiller);
    InitFilters(OSDGUID, "dsfilters.guid", 65006, "","guid");
    InitFilters(FILTERSYSTEM, "dsfilters.systemfilter", 65010, "", "", DSFilterOptionFiller);
  }

  // Reset Button value
  ResetValue();
  
  // Load userdata Filteseconfig.xml
  if (!Get()->GetNewFilter())
  {
    CXBMCTinyXML FiltersConfigXML;
    CStdString xmlFile;
    TiXmlElement *pFilters;
    LoadDsXML(&FiltersConfigXML, pFilters, xmlFile, true);

    if (pFilters)
    {
      TiXmlElement *pFilter = pFilters->FirstChildElement("filter");

      while (pFilter)
      {
        if (count == selected)
        {
          std::vector<DSFiltersList *>::iterator it;
          for (it = m_filterList.begin(); it != m_filterList.end(); ++it)
          {                   
            if ((*it)->m_filterType == EDITATTRFILTER || (*it)->m_filterType == SPINNERATTRFILTER)
              (*it)->strFilterValue = pFilter->Attribute((*it)->strFilterAttr.c_str());   
            if ((*it)->m_filterType == OSDGUID) {
              XMLUtils::GetString(pFilter, (*it)->strFilterName.c_str(), strGuid);
              (*it)->strFilterValue = strGuid;
            }          
            if ((*it)->m_filterType == FILTERSYSTEM)
              (*it)->strFilterValue = strGuid;
          }
        }
        pFilter = pFilter->NextSiblingElement("filter");
        count++;
      }
    }
  }

  // Stamp Button

  CStdString setting_filter;
  std::vector<DSFiltersList *>::iterator it;

  for (it = m_filterList.begin(); it != m_filterList.end(); ++it)
  {
    if ((*it)->m_filterType == EDITATTRFILTER || (*it)->m_filterType == OSDGUID)
      AddEdit(group, (*it)->settingFilter, (*it)->filterLabel, 0, (*it)->strFilterValue, true);

    if ((*it)->m_filterType == SPINNERATTRFILTER)
      AddSpinner(group, (*it)->settingFilter, (*it)->filterLabel, 0, (*it)->strFilterValue, (*it)->filler);

    if ((*it)->m_filterType == FILTERSYSTEM)
      AddList(groupSystem, (*it)->settingFilter, (*it)->filterLabel, 0, (*it)->strFilterValue, (*it)->filler, (*it)->filterLabel);
  }

  if (Get()->GetNewFilter())
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

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);
  const std::string &settingId = setting->GetId(); 

  std::vector<DSFiltersList *>::iterator it;
  for (it = m_filterList.begin(); it != m_filterList.end(); ++it)
  {
    if ((*it)->m_filterType == EDITATTRFILTER || (*it)->m_filterType == SPINNERATTRFILTER || (*it)->m_filterType == OSDGUID)
    { 
      if (settingId == (*it)->settingFilter)
      {
        (*it)->strFilterValue = static_cast<std::string>(static_cast<const CSettingString*>(setting)->GetValue());
      }
    }
    if ((*it)->m_filterType == FILTERSYSTEM)
    {
      if (settingId == "dsfilters.systemfilter")
      {
        (*it)->strFilterValue = static_cast<std::string>(static_cast<const CSettingString*>(setting)->GetValue());
        m_settingsManager->SetString("dsfilters.guid", (*it)->strFilterValue.c_str());
      }    
    }
  }
}

void CGUIDialogDSFilters::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  // Load userdata Filteseconfig.xml
  CXBMCTinyXML FiltersConfigXML;
  CStdString xmlFile;
  TiXmlElement *pFilters;
  LoadDsXML(&FiltersConfigXML, pFilters, xmlFile, true);
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
    
    int selected = Get()->GetFilterIndex();
    int count = 0;
    TiXmlElement *pFilter = pFilters->FirstChildElement("filter");
    while (pFilter)
    {
      if (count == selected)
      {
        pFilters->RemoveChild(pFilter);
        break;
      }
      pFilter = pFilter->NextSiblingElement("filter");
      count++;
    }

    FiltersConfigXML.SaveFile(xmlFile);
    CGUIDialogDSFilters::Close();
  }

  // Add New Filter
  if (settingId == SETTING_FILTER_ADD)
  {
    TiXmlElement pFilter("filter");

    std::vector<DSFiltersList *>::iterator it;
    for (it = m_filterList.begin(); it != m_filterList.end(); ++it)
    {
      if ((*it)->strFilterValue == "" || (*it)->strFilterValue == "[null]") 
      {
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(65001), g_localizeStrings.Get(65012), 2000, false, 300);
        return;
      }

      if ((*it)->m_filterType == EDITATTRFILTER || (*it)->m_filterType == SPINNERATTRFILTER)
          pFilter.SetAttribute((*it)->strFilterAttr.c_str(), (*it)->strFilterValue.c_str());

      if ((*it)->m_filterType == OSDGUID)
      { 
        TiXmlElement newElement((*it)->strFilterName.c_str());
        TiXmlNode *pNewNode = pFilter.InsertEndChild(newElement);
        TiXmlText value((*it)->strFilterValue.c_str());
        pNewNode->InsertEndChild(value);
      }
    }
    pFilters->InsertEndChild(pFilter);
    FiltersConfigXML.SaveFile(xmlFile);
    CGUIDialogDSFilters::Close();
  }

  // Save Modifications
  if (settingId == SETTING_FILTER_SAVE)
  {
    int selected = Get()->GetFilterIndex();
    int count = 0;

    TiXmlElement *pFilter = pFilters->FirstChildElement("filter");
    while (pFilter)
    {
      if (count == selected)
      {
        pFilter->Clear();
        std::vector<DSFiltersList *>::iterator it;
        for (it = m_filterList.begin(); it != m_filterList.end(); ++it)
        {
          if ((*it)->strFilterValue == "" || (*it)->strFilterValue == "[null]")
          {
            CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(65001), g_localizeStrings.Get(65012), 2000, false, 300);
            return;
          }

          if ((*it)->m_filterType == EDITATTRFILTER || (*it)->m_filterType == SPINNERATTRFILTER)
            pFilter->SetAttribute((*it)->strFilterAttr.c_str(), (*it)->strFilterValue.c_str());

          if ((*it)->m_filterType == OSDGUID)
          {
            TiXmlElement newElement((*it)->strFilterName.c_str());
            TiXmlNode *pNewNode = pFilter->InsertEndChild(newElement);
            TiXmlText value((*it)->strFilterValue.c_str());
            pNewNode->InsertEndChild(value);
          }
        }
      }
      pFilter = pFilter->NextSiblingElement("filter");
      count++;
    }
    FiltersConfigXML.SaveFile(xmlFile);
    CGUIDialogDSFilters::Close();
  }
}

int CGUIDialogDSFilters::ShowDSFiltersList()
{
  // Load userdata Filteseconfig.xml
  CXBMCTinyXML FiltesConfigXML;
  CStdString xmlFile;
  TiXmlElement *pFilters;
  Get()->LoadDsXML(&FiltesConfigXML, pFilters, xmlFile, true);
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
    TiXmlElement *pOsdname = pFilter->FirstChildElement("osdname");
    if (pOsdname)
    {
      XMLUtils::GetString(pFilter, "osdname", strFilterLabel);
      strFilter = pFilter->Attribute("name");
      strFilterLabel.Format("%s (%s)", strFilterLabel, strFilter);

      pDlg->Add(strFilterLabel.c_str());
      count++;
    }
    pFilter = pFilter->NextSiblingElement("filter");
  }

  pDlg->Add(g_localizeStrings.Get(65002).c_str());

  pDlg->DoModal();
  selected = pDlg->GetSelectedLabel();
  if (selected == count)
    Get()->SetNewFilter(true);
  else
    Get()->SetNewFilter(false);
  Get()->SetFilterIndex(selected);

  if (selected > -1) g_windowManager.ActivateWindow(WINDOW_DIALOG_DSFILTERS);

  return selected;
}

void CGUIDialogDSFilters::LoadDsXML(CXBMCTinyXML *XML, TiXmlElement* &pNode, CStdString &xmlFile, bool forceCreate /*= false*/)
{
  CStdString xmlRoot, xmlNode;
  pNode = NULL;

  xmlFile = CProfilesManager::Get().GetUserDataItem("dsplayer/filtersconfig.xml");
  xmlRoot = "filtersconfig";
  xmlNode = "filters";

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
  pNode = pConfig->FirstChildElement(xmlNode.c_str());
}

void CGUIDialogDSFilters::TypeOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  list.push_back(std::make_pair("[null]", "[null]"));
  list.push_back(std::make_pair("Source Filter (source)", "source"));
  list.push_back(std::make_pair("Splitter Filter (splitter)", "splitter"));
  list.push_back(std::make_pair("Video Decoder (videodec)", "videodec"));
  list.push_back(std::make_pair("Audio Decoder (audiodec)", "audiodec"));
  list.push_back(std::make_pair("Subtitles Filter (subs)", "subs"));
  list.push_back(std::make_pair("Extra Filter (extra)", "extra"));
}

void CGUIDialogDSFilters::DSFilterOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  CDSFilterEnumerator p_dfilter;
  std::vector<DSFiltersInfo> filterList;
  p_dfilter.GetDSFilters(filterList);

  std::vector<DSFiltersInfo>::const_iterator iter = filterList.begin();

  for (int i = 1; iter != filterList.end(); i++)
  {
    DSFiltersInfo filter = *iter;
    list.push_back(std::make_pair(filter.lpstrName, filter.lpstrGuid));
    ++iter;
  }
}




