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

#include "GUIDialogDSManager.h"
#include "cores/DSPlayer/Utils/DSFilterEnumerator.h"
#include "profiles/ProfilesManager.h"
#include "utils/log.h"

using namespace std;

DSConfigList::DSConfigList(ConfigType type) :

m_setting(""),
m_nodeName(""),
m_nodeList(""),
m_attr(""),
m_value(""),
m_label(0),
m_subNode(0),
m_filler(NULL),
m_configType(type)
{
}

CGUIDialogDSManager::CGUIDialogDSManager()
{

}

CGUIDialogDSManager *CGUIDialogDSManager::m_pSingleton = NULL;

CGUIDialogDSManager* CGUIDialogDSManager::Get()
{
  return (m_pSingleton) ? m_pSingleton : (m_pSingleton = new CGUIDialogDSManager());
}


void CGUIDialogDSManager::InitConfig(std::vector<DSConfigList *> &configList, ConfigType type, CStdString strSetting, int label, CStdString strAttr /* = "" */, CStdString strNodeName /*= "" */, StringSettingOptionsFiller filler /* = NULL */, int subNode /* = 0 */, CStdString strNodeList /* = "" */)
{
  DSConfigList* list = new DSConfigList(type);

  list->m_setting = strSetting;
  list->m_attr = strAttr;
  list->m_nodeName = strNodeName;
  list->m_nodeList = strNodeList;
  list->m_filler = filler;
  list->m_label = label;
  list->m_subNode = subNode;
  configList.push_back(list);
}

void CGUIDialogDSManager::ResetValue(std::vector<DSConfigList *> &configList)
{
  std::vector<DSConfigList *>::iterator it;
  for (it = configList.begin(); it != configList.end(); ++it)
  {
    if ((*it)->m_configType == EDITATTR
      || (*it)->m_configType == EDITATTREXTRA
      || (*it)->m_configType == EDITATTRSHADER
      || (*it)->m_configType == OSDGUID)
      (*it)->m_value = "";

    if ((*it)->m_configType == SPINNERATTR
      || (*it)->m_configType == FILTER
      || (*it)->m_configType == EXTRAFILTER
      || (*it)->m_configType == SHADER
      || (*it)->m_configType == FILTERSYSTEM)
      (*it)->m_value = "[null]";

    if ((*it)->m_configType == SPINNERATTRSHADER)
      (*it)->m_value = "preresize";

    if ((*it)->m_configType == BOOLATTR)
      (*it)->m_value = "false";
  }
}

void CGUIDialogDSManager::GetPath(xmlType type, CStdString &xmlFile, CStdString &xmlNode, CStdString &xmlRoot)
{
  if (type == MEDIASCONFIG)
  {
    xmlFile = CProfilesManager::GetInstance().GetUserDataItem("dsplayer/mediasconfig.xml");
    xmlRoot = "mediasconfig";
    xmlNode = "rules";
  }

  if (type == FILTERSCONFIG)
  {
    xmlFile = CProfilesManager::GetInstance().GetUserDataItem("dsplayer/filtersconfig.xml");
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
    xmlFile = CProfilesManager::GetInstance().GetUserDataItem("dsplayer/shaders.xml");
    xmlRoot = "shaders";
  }

  if (type == PLAYERCOREFACTORY)
  {
    xmlFile = CProfilesManager::GetInstance().GetUserDataItem("playercorefactory.xml");
    xmlRoot = "playercorefactory";
    xmlNode = "rules";
  }
}

void CGUIDialogDSManager::SaveDsXML(xmlType type)
{
  CStdString xmlFile, xmlNode, xmlRoot;
  GetPath(type, xmlFile, xmlNode, xmlRoot);

  m_XML.SaveFile(xmlFile);
}

bool CGUIDialogDSManager::FindPrepend(TiXmlElement* &pNode, CStdString xmlNode)
{
  bool isPrepend = false;
  while (pNode)
  {
    CStdString value;
    value = pNode->Attribute("action");
    if (value == "prepend")
    {
      isPrepend = true;
      break;
    }
    pNode = pNode->NextSiblingElement(xmlNode.c_str());
  }
  return isPrepend;
}

void CGUIDialogDSManager::LoadDsXML(xmlType type, TiXmlElement* &pNode, bool forceCreate /*= false*/)
{

  CStdString xmlFile, xmlNode, xmlRoot;
  GetPath(type, xmlFile, xmlNode, xmlRoot);

  pNode = NULL;

  m_XML.Clear();

  if (!m_XML.LoadFile(xmlFile))
  {
    CLog::Log(LOGERROR, "%s Error loading %s, Line %d (%s)", __FUNCTION__, xmlFile.c_str(), m_XML.ErrorRow(), m_XML.ErrorDesc());
    if (!forceCreate)
      return;

    CLog::Log(LOGDEBUG, "%s Creating loading %s, with root <%s> and first node <%s>", __FUNCTION__, xmlFile.c_str(), xmlRoot.c_str(), xmlNode.c_str());

    TiXmlElement pRoot(xmlRoot.c_str());
    if (type != PLAYERCOREFACTORY)
      pRoot.InsertEndChild(TiXmlElement(xmlNode.c_str()));
    m_XML.InsertEndChild(pRoot);
  }
  TiXmlElement *pConfig = m_XML.RootElement();
  if (!pConfig || strcmpi(pConfig->Value(), xmlRoot.c_str()) != 0)
  {
    CLog::Log(LOGERROR, "%s Error loading medias configuration, no <%s> node", __FUNCTION__, xmlRoot.c_str());
    return;
  }
  if (type == MEDIASCONFIG
    || type == FILTERSCONFIG
    || type == HOMEFILTERSCONFIG)
    pNode = pConfig->FirstChildElement(xmlNode.c_str());

  if (type == SHADERS)
    pNode = pConfig;

  if (type == PLAYERCOREFACTORY) {

    pNode = pConfig->FirstChildElement(xmlNode.c_str());

    if (!FindPrepend(pNode, xmlNode))
    {
      TiXmlElement pTmp(xmlNode.c_str());
      pTmp.SetAttribute("action", "prepend");
      pConfig->InsertEndChild(pTmp);

      pNode = pConfig->FirstChildElement(xmlNode.c_str());
      FindPrepend(pNode, xmlNode);
    }
  }
}

std::vector<DynamicStringSettingOption>CGUIDialogDSManager::GetFilterList(xmlType type)
{
  std::vector<DynamicStringSettingOption> list;
  list.push_back(std::make_pair("", "[null]"));

  // Load filtersconfig.xml
  TiXmlElement *pFilters;
  LoadDsXML(type, pFilters);

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
  return list;
}

void CGUIDialogDSManager::AllFiltersConfigOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{

  std::vector<DynamicStringSettingOption> listUserdata = Get()->GetFilterList(FILTERSCONFIG);
  std::vector<DynamicStringSettingOption> listHome = Get()->GetFilterList(HOMEFILTERSCONFIG);

  list.resize(listUserdata.size() + listHome.size());

  std::merge(listUserdata.begin(), listUserdata.end(), listHome.begin(), listHome.end(), list.begin());

  std::sort(list.begin(), list.end(), compare_by_word);
  list.erase(unique(list.begin(), list.end()), list.end());
}

void CGUIDialogDSManager::ShadersOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{

  list.push_back(std::make_pair("", "[null]"));

  TiXmlElement *pShaders;

  // Load userdata shaders.xml
  Get()->LoadDsXML(SHADERS, pShaders);

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

void CGUIDialogDSManager::ShadersScaleOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{

  list.push_back(std::make_pair("Pre-resize", "preresize"));
  list.push_back(std::make_pair("Post-resize", "postresize"));

}

void CGUIDialogDSManager::DSFilterOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  CDSFilterEnumerator p_dfilter;
  std::vector<DSFiltersInfo> filterList;
  p_dfilter.GetDSFilters(filterList);

  list.push_back(std::make_pair("", "[null]"));

  std::vector<DSFiltersInfo>::const_iterator iter = filterList.begin();

  for (int i = 1; iter != filterList.end(); i++)
  {
    DSFiltersInfo filter = *iter;
    list.push_back(std::make_pair(filter.lpstrName, filter.lpstrGuid));
    ++iter;
  }
}

void CGUIDialogDSManager::BoolOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  list.push_back(std::make_pair("[null]", "[null]"));
  list.push_back(std::make_pair("true", "true"));
  list.push_back(std::make_pair("false", "false"));
}

void CGUIDialogDSManager::PriorityOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  list.push_back(std::make_pair("", "[null]"));

  for (unsigned int i = 0; i < 10; i++)
  {
    CStdString sValue;
    sValue.Format("%i",i);
    list.push_back(std::make_pair(sValue, sValue));
  }
}

TiXmlElement* CGUIDialogDSManager::KeepSelectedNode(TiXmlElement* pNode, CStdString subNodeName)
{
  int selected = GetConfigIndex();
  int count = 0;

  TiXmlElement *pRule = pNode->FirstChildElement(subNodeName.c_str());
  while (pRule)
  {
    if (count == selected)
      break;

    pRule = pRule->NextSiblingElement(subNodeName.c_str());
    count++;
  }
  return pRule;
}

bool CGUIDialogDSManager::compare_by_word(const DynamicStringSettingOption& lhs, const DynamicStringSettingOption& rhs)
{
  CStdString strLine1 = lhs.first;
  CStdString strLine2 = rhs.first;
  StringUtils::ToLower(strLine1);
  StringUtils::ToLower(strLine2);
  return strcmp(strLine1.c_str(), strLine2.c_str()) < 0;
}

