/*
 *      Copyright (C) 2013 Team XBMC
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

#include "SettingSection.h"
#include "SettingDefinitions.h"
#include "SettingsManager.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

template<class T> void addISetting(const TiXmlNode *node, const T &item, std::vector<T> &items)
{
  if (node != NULL)
  {
    const TiXmlElement *element = node->ToElement();
    if (element != NULL)
    {
      // check if there is a "before" or "after" attribute to place the setting at a specific position
      int position = -1; // -1 => end, 0 => before, 1 => after
      const char *positionId = element->Attribute(SETTING_XML_ATTR_BEFORE);
      if (positionId != NULL && strlen(positionId) > 0)
        position = 0;
      else if ((positionId = element->Attribute(SETTING_XML_ATTR_AFTER)) != NULL && strlen(positionId) > 0)
        position = 1;

      if (positionId != NULL && strlen(positionId) > 0 && position >= 0)
      {
        for (typename std::vector<T>::iterator it = items.begin(); it != items.end(); ++it)
        {
          if (!StringUtils::EqualsNoCase((*it)->GetId(), positionId))
            continue;

          typename std::vector<T>::iterator positionIt = it;
          if (position == 1)
            ++positionIt;

          items.insert(positionIt, item);
          return;
        }
      }
    }
  }

  items.push_back(item);
}

CSettingGroup::CSettingGroup(const std::string &id, CSettingsManager *settingsManager /* = NULL */)
  : ISetting(id, settingsManager)
  , m_control(NULL)
{ }

CSettingGroup::~CSettingGroup()
{
  for (SettingList::const_iterator setting = m_settings.begin(); setting != m_settings.end(); ++setting)
    delete *setting;
  m_settings.clear();
  if (m_control)
    delete m_control;
}

bool CSettingGroup::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  // handle <visible> conditions
  if (!ISetting::Deserialize(node, update))
    return false;

  const TiXmlElement *controlElement = node->FirstChildElement(SETTING_XML_ELM_CONTROL);
  if (controlElement != NULL)
  {
    const char* controlType = controlElement->Attribute(SETTING_XML_ATTR_TYPE);
    if (controlType == NULL || strlen(controlType) <= 0)
    {
      CLog::Log(LOGERROR, "CSettingGroup: unable to read control type");
      return false;
    }

    if (m_control != NULL)
      delete m_control;

    m_control = m_settingsManager->CreateControl(controlType);
    if (m_control == NULL)
    {
      CLog::Log(LOGERROR, "CSettingGroup: unable to create new control \"%s\"", controlType);
      return false;
    }

    if (!m_control->Deserialize(controlElement))
    {
      CLog::Log(LOGWARNING, "CSettingGroup: unable to read control \"%s\"", controlType);
      delete m_control;
      m_control = NULL;
    }
  }

  const TiXmlElement *settingElement = node->FirstChildElement(SETTING_XML_ELM_SETTING);
  while (settingElement != NULL)
  {
    std::string settingId;
    if (CSettingCategory::DeserializeIdentification(settingElement, settingId))
    {
      CSetting *setting = NULL;
      for (SettingList::iterator itSetting = m_settings.begin(); itSetting != m_settings.end(); ++itSetting)
      {
        if ((*itSetting)->GetId() == settingId)
        {
          setting = *itSetting;
          break;
        }
      }
      
      update = (setting != NULL);
      if (!update)
      {
        const char* settingType = settingElement->Attribute(SETTING_XML_ATTR_TYPE);
        if (settingType == NULL || strlen(settingType) <= 0)
        {
          CLog::Log(LOGERROR, "CSettingGroup: unable to read setting type of \"%s\"", settingId.c_str());
          return false;
        }

        setting = m_settingsManager->CreateSetting(settingType, settingId, m_settingsManager);
        if (setting == NULL)
          CLog::Log(LOGERROR, "CSettingGroup: unknown setting type \"%s\" of \"%s\"", settingType, settingId.c_str());
      }
      
      if (setting == NULL)
        CLog::Log(LOGERROR, "CSettingGroup: unable to create new setting \"%s\"", settingId.c_str());
      else if (!setting->Deserialize(settingElement, update))
      {
        CLog::Log(LOGWARNING, "CSettingGroup: unable to read setting \"%s\"", settingId.c_str());
        if (!update)
          delete setting;
      }
      else if (!update)
        addISetting(settingElement, setting, m_settings);
    }
      
    settingElement = settingElement->NextSiblingElement(SETTING_XML_ELM_SETTING);
  }
    
  return true;
}

SettingList CSettingGroup::GetSettings(SettingLevel level) const
{
  SettingList settings;

  for (SettingList::const_iterator it = m_settings.begin(); it != m_settings.end(); ++it)
  {
    if ((*it)->GetLevel() <= level && (*it)->MeetsRequirements())
      settings.push_back(*it);
  }

  return settings;
}

void CSettingGroup::AddSetting(CSetting *setting)
{
  addISetting(NULL, setting, m_settings);
}

void CSettingGroup::AddSettings(const SettingList &settings)
{
  for (SettingList::const_iterator itSetting = settings.begin(); itSetting != settings.end(); ++itSetting)
    addISetting(NULL, *itSetting, m_settings);
}

CSettingCategory::CSettingCategory(const std::string &id, CSettingsManager *settingsManager /* = NULL */)
  : ISetting(id, settingsManager),
    m_accessCondition(settingsManager)
{ }

CSettingCategory::~CSettingCategory()
{
  for (SettingGroupList::const_iterator it = m_groups.begin(); it != m_groups.end(); ++it)
    delete *it;

  m_groups.clear();
}

bool CSettingCategory::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  // handle <visible> conditions
  if (!ISetting::Deserialize(node, update))
    return false;

  const TiXmlNode *accessNode = node->FirstChild(SETTING_XML_ELM_ACCESS);
  if (accessNode != NULL && !m_accessCondition.Deserialize(accessNode))
    return false;
    
  const TiXmlNode *groupNode = node->FirstChildElement(SETTING_XML_ELM_GROUP);
  while (groupNode != NULL)
  {
    std::string groupId;
    if (CSettingGroup::DeserializeIdentification(groupNode, groupId))
    {
      CSettingGroup *group = NULL;
      for (SettingGroupList::iterator itGroup = m_groups.begin(); itGroup != m_groups.end(); ++itGroup)
      {
        if ((*itGroup)->GetId() == groupId)
        {
          group = *itGroup;
          break;
        }
      }
      
      update = (group != NULL);
      if (!update)
        group = new CSettingGroup(groupId, m_settingsManager);

      if (group->Deserialize(groupNode, update))
      {
        if (!update)
          addISetting(groupNode, group, m_groups);
      }
      else
      {
        CLog::Log(LOGWARNING, "CSettingCategory: unable to read group \"%s\"", groupId.c_str());
        if (!update)
          delete group;
      }
    }
      
    groupNode = groupNode->NextSibling(SETTING_XML_ELM_GROUP);
  }
    
  return true;
}

SettingGroupList CSettingCategory::GetGroups(SettingLevel level) const
{
  SettingGroupList groups;

  for (SettingGroupList::const_iterator it = m_groups.begin(); it != m_groups.end(); ++it)
  {
    if ((*it)->MeetsRequirements() && (*it)->IsVisible() && (*it)->GetSettings(level).size() > 0)
      groups.push_back(*it);
  }

  return groups;
}

bool CSettingCategory::CanAccess() const
{
  return m_accessCondition.Check();
}

void CSettingCategory::AddGroup(CSettingGroup *group)
{
  addISetting(NULL, group, m_groups);
}

void CSettingCategory::AddGroups(const SettingGroupList &groups)
{
  for (SettingGroupList::const_iterator itGroup = groups.begin(); itGroup != groups.end(); ++itGroup)
    addISetting(NULL, *itGroup, m_groups);
}

CSettingSection::CSettingSection(const std::string &id, CSettingsManager *settingsManager /* = NULL */)
  : ISetting(id, settingsManager)
{ }

CSettingSection::~CSettingSection()
{
  for (SettingCategoryList::const_iterator it = m_categories.begin(); it != m_categories.end(); ++it)
    delete *it;

  m_categories.clear();
}
  
bool CSettingSection::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  // handle <visible> conditions
  if (!ISetting::Deserialize(node, update))
    return false;
    
  const TiXmlNode *categoryNode = node->FirstChild(SETTING_XML_ELM_CATEGORY);
  while (categoryNode != NULL)
  {
    std::string categoryId;
    if (CSettingCategory::DeserializeIdentification(categoryNode, categoryId))
    {
      CSettingCategory *category = NULL;
      for (SettingCategoryList::iterator itCategory = m_categories.begin(); itCategory != m_categories.end(); ++itCategory)
      {
        if ((*itCategory)->GetId() == categoryId)
        {
          category = *itCategory;
          break;
        }
      }
      
      update = (category != NULL);
      if (!update)
        category = new CSettingCategory(categoryId, m_settingsManager);

      if (category->Deserialize(categoryNode, update))
      {
        if (!update)
          addISetting(categoryNode, category, m_categories);
      }
      else
      {
        CLog::Log(LOGWARNING, "CSettingSection: unable to read category \"%s\"", categoryId.c_str());
        if (!update)
          delete category;
      }
    }
      
    categoryNode = categoryNode->NextSibling(SETTING_XML_ELM_CATEGORY);
  }
    
  return true;
}

SettingCategoryList CSettingSection::GetCategories(SettingLevel level) const
{
  SettingCategoryList categories;

  for (SettingCategoryList::const_iterator it = m_categories.begin(); it != m_categories.end(); ++it)
  {
    if ((*it)->MeetsRequirements() && (*it)->IsVisible() && (*it)->GetGroups(level).size() > 0)
      categories.push_back(*it);
  }

  return categories;
}

void CSettingSection::AddCategory(CSettingCategory *category)
{
  addISetting(NULL, category, m_categories);
}

void CSettingSection::AddCategories(const SettingCategoryList &categories)
{
  for (SettingCategoryList::const_iterator itCategory = categories.begin(); itCategory != categories.end(); ++itCategory)
    addISetting(NULL, *itCategory, m_categories);
}
