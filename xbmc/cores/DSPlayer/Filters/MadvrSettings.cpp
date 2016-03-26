/*
 *      Copyright (C) 2005-2013 Team XBMC
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
// MadvrSettings.cpp: implementation of the CMadvrSettings class.
//
//////////////////////////////////////////////////////////////////////

#include "MadvrSettings.h"
#include "DSRendererCallback.h"
#include "utils/JSONVariantWriter.h"
#include "Utils/Log.h"
#include "cores/DSPlayer/Dialogs/GUIDialogDSManager.h"
#include "utils/StringUtils.h"
#include "utils/RegExp.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#ifndef countof
#define countof(array) (sizeof(array)/sizeof(array[0]))
#endif

CMadvrSettings::CMadvrSettings()
{
  m_Resolution = -1;
  m_TvShowName = "";
  m_madvrJsonAtStart = "";
  m_iSubSectionId = 0;
  m_bDebug = false;

  InitSettings();
}

void CMadvrSettings::InitSettings()
{
  //Load settings strutcture from dsplayer/madvrsettings.xml

  TiXmlElement *pSettings;

  CLog::Log(LOGNOTICE, "%s trying to load madvrSettings.xml from userdata folder", __FUNCTION__);
  CGUIDialogDSManager::Get()->LoadDsXML(MADVRSETTINGS, pSettings);

  if (pSettings == NULL)
  {
    CLog::Log(LOGNOTICE, "%s madvrSettings.xml not found in userdata folder", __FUNCTION__);
    CLog::Log(LOGNOTICE, "%s load madvrSettings.xml from home folder", __FUNCTION__);
    CGUIDialogDSManager::Get()->LoadDsXML(HOMEMADVRSETTINGS, pSettings);
  }

  if (pSettings)
  {
    AddProfiles(pSettings);
    AddSection(pSettings, MADVR_VIDEO_ROOT);
  }

  m_dbDefault = m_db;
}

void CMadvrSettings::AddProfiles(TiXmlNode *pNode)
{
  if (pNode == NULL)
    return;

  TiXmlElement *pProfile = pNode->FirstChildElement("dsprofile");
  while (pProfile)
  {
    std::string strPath;
    std::string strFolders;

    if (GetString(pProfile, "path", &strPath) && GetString(pProfile, "folder", &strFolders))
      m_profiles[strPath] = strFolders;

    pProfile = pProfile->NextSiblingElement("dsprofile");
  }
}

void CMadvrSettings::AddSection(TiXmlNode *pNode, int iSectionId)
{
  if (pNode == NULL)
    return;

  m_iSubSectionId = iSectionId * MADVR_SECTION_SUB;
  if (m_iSubSectionId == 0)
    m_iSubSectionId = MADVR_SECTION_ROOT;

  int iSubSectionId = m_iSubSectionId;

  int iGroupId = 0;
  TiXmlElement *pGroup = pNode->FirstChildElement("group");
  while (pGroup)
  {
    iGroupId++;
    TiXmlNode *pSettingNode = pGroup->FirstChildElement();
    while (pSettingNode)
    {
      std::string strNode = pSettingNode->ValueStr();
      if (strNode == "setting")
      {
        AddSetting(pSettingNode, iSectionId, iGroupId);
      }
      else if (strNode == "section")
      {
        iSubSectionId++;
        std::string strSection = StringUtils::Format("section%i", iSubSectionId);
        AddButton(pSettingNode, iSectionId, iGroupId, iSubSectionId, "button_section", strSection);
        AddSection(pSettingNode, iSubSectionId);
      }
      else if (strNode == "debug")
      {
        m_bDebug = true;
        AddButton(pSettingNode, iSectionId, iGroupId, 0, "button_debug", "debug");
      }

      pSettingNode = pSettingNode->NextSibling();
    }
    pGroup = pGroup->NextSiblingElement("group");
  }
}

void CMadvrSettings::AddButton(TiXmlNode *pNode, int iSectionId, int iGroupId, int iSubSectionId, const std::string &type, const std::string &name)
{
  TiXmlElement *pSetting = pNode->ToElement();

  CMadvrListSettings *button = new CMadvrListSettings();

  button->group = iGroupId;
  button->name = name;
  button->dialogId = NameToId(button->name);
  button->type = type;
  
  if (!GetInt(pSetting, "label",&button->label))
    CLog::Log(LOGERROR, "%s missing attritube (label) for button %s", __FUNCTION__, button->name.c_str());

  if (button->type == "button_section")
  {
    button->sectionId = iSubSectionId;
    CMadvrSection section;
    section.label = button->label;
    section.parentId = iSectionId;
    m_sections[iSubSectionId] = section;
  }
  else if (button->type == "button_debug")
  {
    if (!GetString(pSetting,"path",&button->value))
      CLog::Log(LOGERROR, "%s missing attritube (path) for button %s", __FUNCTION__, button->name.c_str());
  }

  m_gui[iSectionId].push_back(button);
}

void CMadvrSettings::AddSetting(TiXmlNode *pNode, int iSectionId, int iGroupId)
{
  TiXmlElement *pSetting = pNode->ToElement();

  CMadvrListSettings *setting = new CMadvrListSettings();
  CVariant default;

  setting->group = iGroupId;

  //GET NAME, TYPE
  if (!GetString(pSetting, "name", &setting->name)
    ||!GetString(pSetting, "type", &setting->type))
  {
    CLog::Log(LOGERROR, "%s missing attritube (name, type) for setting name=%s type=%s", __FUNCTION__, setting->name.c_str(), setting->type.c_str());
  }
  if (StringUtils::StartsWith(setting->type, "!"))
  {
    StringUtils::Replace(setting->type, "!", "");
    setting->negate = true;
  }  

  // GET VALUE, PARENT
  GetString(pSetting, "value", &setting->value);
  GetString(pSetting, "parent", &setting->parent);
  setting->parent = NameToId(setting->parent);
  setting->dialogId = NameToId(setting->name);

  // GET LABEL
  if (!GetInt(pSetting, "label", &setting->label))
    CLog::Log(LOGERROR, "%s missing attritube (label) for setting name=%s", __FUNCTION__, setting->name.c_str());

  // GET DEFAULT VALUE
  if (!GetVariant(pSetting, "default", setting->type, &default))
    CLog::Log(LOGERROR, "%s missing attritube (default) for setting name=%s", __FUNCTION__, setting->name.c_str());

  // GET DEPENDENCIES
  TiXmlElement *pDependencies = pSetting->FirstChildElement(SETTING_XML_ELM_DEPENDENCIES);
  if (pDependencies)
  { 
    TiXmlPrinter print;
    pDependencies->Accept(&print);
    setting->dependencies = DependenciesNameToId(print.CStr());
  }

  // GET OPTIONS
  if (setting->type.find("list_") != std::string::npos)
  {
    TiXmlElement *pOption = pSetting->FirstChildElement("option");
    while (pOption)
    { 
      int iLabel;
      CVariant value;
      if (!GetInt(pOption, "label", &iLabel))
        CLog::Log(LOGERROR, "%s missing attritube (label) for setting option name=%s", __FUNCTION__, setting->name.c_str());

      if (!GetVariant(pOption, "value", setting->type, &value))
        CLog::Log(LOGERROR, "%s missing attritube (value) for setting option name=%s", __FUNCTION__, setting->name.c_str());

      if (value.isInteger())
        setting->optionsInt.push_back(std::make_pair(iLabel, value.asInteger()));
      else
        setting->optionsString.push_back(std::make_pair(iLabel, value.asString()));

      pOption = pOption->NextSiblingElement("option");
    }   
  }
  //GET FLOAT
  else if (setting->type == "float")
  {
    setting->slider = new CMadvrSlider();

    if (!GetString(pSetting,"format",&setting->slider->format))
      CLog::Log(LOGERROR, "%s missing attribute (format) for setting name=%s", __FUNCTION__, setting->name.c_str());

    if ( !GetInt(pSetting, "parentlabel", &setting->slider->parentLabel)
      || !GetFloat(pSetting, "min", &setting->slider->min)
      || !GetFloat(pSetting, "max", &setting->slider->max)
      || !GetFloat(pSetting, "step", &setting->slider->step))
    {
      CLog::Log(LOGERROR, "%s missing attritube (parentLabel, min, max, step) for setting name=%s", __FUNCTION__, setting->name.c_str());
    }
  }

  // ADD GUI
  m_gui[iSectionId].push_back(setting);

  // ADD DATABASE
  m_db[setting->name] = default;
}

void CMadvrSettings::StoreSettingsAtStart()
{
  m_madvrJsonAtStart = CJSONVariantWriter::Write(m_db, true);
}

void CMadvrSettings::RestoreDefaultSettings()
{
  m_db = m_dbDefault;
}

bool CMadvrSettings::SettingsChanged()
{
  std::string strJson = CJSONVariantWriter::Write(m_db, true);
  return strJson != m_madvrJsonAtStart;
}

std::string CMadvrSettings::DependenciesNameToId(const std::string& dependencies)
{
  CRegExp reg(true);
  reg.RegComp("setting=\"([^\"]+)");
  char line[1024];
  std::string newDependencies;
  std::stringstream strStream(dependencies);

  while (strStream.getline(line, sizeof(line)))
  {
    std::string newLine = line;
    if (reg.RegFind(line) > -1)
    {
      std::string str = reg.GetMatch(1);
      StringUtils::Replace(newLine, str, NameToId(str));     
    }
    newDependencies = StringUtils::Format("%s%s\n", newDependencies.c_str(), newLine.c_str());
  }
  return newDependencies;
}

std::string CMadvrSettings::NameToId(const std::string &str)
{
  if (str.empty())
    return "";

  std::string sValue = StringUtils::Format("madvr.%s", str.c_str());
  StringUtils::ToLower(sValue);
  return sValue;
}

bool CMadvrSettings::GetVariant(TiXmlElement *pElement, const std::string &attr, const std::string &type, CVariant *variant)
{
  const char *str = pElement->Attribute(attr.c_str());
  if (str == NULL)
    return false;
  
  if (type == "list_int" || type == "list_boolint")
  {
    *variant = atoi(str);
  }
  else if (type == "bool" || type == "list_boolbool")
  {
    bool bValue = false;
    int iValue = -1;
    std::string strEnabled = str;
    StringUtils::ToLower(strEnabled);
    if (strEnabled == "false" || strEnabled == "0")
    {
      bValue = false;
      iValue = 0;
    }
    else if (strEnabled == "true" || strEnabled == "1")
    {
      bValue = true;
      iValue = 1;
    }
    type == "bool" ? *variant = bValue : *variant = iValue;
  }
  else if (type == "float")
  {
    *variant = (float)atof(str);
  }
  else
  {
    *variant = std::string(str);
  }

  return true;
}

bool CMadvrSettings::GetInt(TiXmlElement *pElement, const std::string &attr, int *iValue)
{
  const char *str = pElement->Attribute(attr.c_str());
  if (str == NULL)
  {
    *iValue = 0;
    return false;
  }

  *iValue = atoi(str);

  return true;
}

bool CMadvrSettings::GetFloat(TiXmlElement *pElement, const std::string &attr, float *fValue)
{
  const char *str = pElement->Attribute(attr.c_str());
  if (str == NULL)
  {
    *fValue = 0.0f;
    return false;
  }

  *fValue = (float)atof(str);

  return true;
}

bool CMadvrSettings::GetString(TiXmlElement *pElement, const std::string &attr, std::string *sValue)
{
  const char *str = pElement->Attribute(attr.c_str());
  if (str == NULL)
  {
    *sValue = "";
    return false;
  }

  *sValue = std::string(str);

  return true;
}
