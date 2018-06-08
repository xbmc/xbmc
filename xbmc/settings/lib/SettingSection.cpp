/*
 *      Copyright (C) 2013 Team XBMC
 *      http://kodi.tv
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

#include <algorithm>

#include "SettingSection.h"
#include "SettingDefinitions.h"
#include "SettingsManager.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

template<class T> void addISetting(const TiXmlNode *node, const T &item, std::vector<T> &items)
{
  if (node != nullptr)
  {
    auto element = node->ToElement();
    if (element != nullptr)
    {
      // check if there is a "before" or "after" attribute to place the setting at a specific position
      int position = -1; // -1 => end, 0 => before, 1 => after
      auto positionId = element->Attribute(SETTING_XML_ATTR_BEFORE);
      if (positionId != nullptr && strlen(positionId) > 0)
        position = 0;
      else if ((positionId = element->Attribute(SETTING_XML_ATTR_AFTER)) != nullptr && strlen(positionId) > 0)
        position = 1;

      if (positionId != nullptr && strlen(positionId) > 0 && position >= 0)
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

CSettingGroup::CSettingGroup(const std::string &id, CSettingsManager *settingsManager /* = nullptr */)
  : ISetting(id, settingsManager)
{ }

bool CSettingGroup::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  // handle <visible> conditions
  if (!ISetting::Deserialize(node, update))
    return false;

  auto controlElement = node->FirstChildElement(SETTING_XML_ELM_CONTROL);
  if (controlElement != nullptr)
  {
    auto controlType = controlElement->Attribute(SETTING_XML_ATTR_TYPE);
    if (controlType == nullptr || strlen(controlType) <= 0)
    {
      CLog::Log(LOGERROR, "CSettingGroup: unable to read control type");
      return false;
    }

    m_control = m_settingsManager->CreateControl(controlType);
    if (m_control == nullptr)
    {
      CLog::Log(LOGERROR, "CSettingGroup: unable to create new control \"%s\"", controlType);
      return false;
    }

    if (!m_control->Deserialize(controlElement))
    {
      CLog::Log(LOGWARNING, "CSettingGroup: unable to read control \"%s\"", controlType);
      m_control.reset();
    }
  }

  auto settingElement = node->FirstChildElement(SETTING_XML_ELM_SETTING);
  while (settingElement != nullptr)
  {
    std::string settingId;
    if (CSettingCategory::DeserializeIdentification(settingElement, settingId))
    {
      auto settingIt = std::find_if(m_settings.begin(), m_settings.end(),
        [&settingId](const SettingPtr& setting)
        {
          return setting->GetId() == settingId;
        });

      SettingPtr setting;
      if (settingIt != m_settings.end())
        setting = *settingIt;

      update = (setting != nullptr);
      if (!update)
      {
        auto settingType = settingElement->Attribute(SETTING_XML_ATTR_TYPE);
        if (settingType == nullptr || strlen(settingType) <= 0)
        {
          CLog::Log(LOGERROR, "CSettingGroup: unable to read setting type of \"%s\"", settingId.c_str());
          return false;
        }

        setting = m_settingsManager->CreateSetting(settingType, settingId, m_settingsManager);
        if (setting == nullptr)
          CLog::Log(LOGERROR, "CSettingGroup: unknown setting type \"%s\" of \"%s\"", settingType, settingId.c_str());
      }

      if (setting == nullptr)
        CLog::Log(LOGERROR, "CSettingGroup: unable to create new setting \"%s\"", settingId.c_str());
      else if (!setting->Deserialize(settingElement, update))
        CLog::Log(LOGWARNING, "CSettingGroup: unable to read setting \"%s\"", settingId.c_str());
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
  for (const auto& setting : m_settings)
  {
    if (setting->GetLevel() <= level && setting->MeetsRequirements())
      settings.push_back(setting);
  }

  return settings;
}

void CSettingGroup::AddSetting(SettingPtr setting)
{
  addISetting(nullptr, setting, m_settings);
}

void CSettingGroup::AddSettings(const SettingList &settings)
{
  for (const auto& setting : settings)
    addISetting(nullptr, setting, m_settings);
}

bool CSettingGroup::ReplaceSetting(std::shared_ptr<const CSetting> currentSetting, std::shared_ptr<CSetting> newSetting)
{
  for (auto itSetting = m_settings.begin(); itSetting != m_settings.end(); ++itSetting)
  {
    if (*itSetting == currentSetting)
    {
      if (newSetting == nullptr)
        m_settings.erase(itSetting);
      else
        *itSetting = newSetting;

      return true;
    }
  }

  return false;
}

CSettingCategory::CSettingCategory(const std::string &id, CSettingsManager *settingsManager /* = nullptr */)
  : ISetting(id, settingsManager),
    m_accessCondition(settingsManager)
{ }

bool CSettingCategory::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  // handle <visible> conditions
  if (!ISetting::Deserialize(node, update))
    return false;

  auto accessNode = node->FirstChild(SETTING_XML_ELM_ACCESS);
  if (accessNode != nullptr && !m_accessCondition.Deserialize(accessNode))
    return false;

  auto groupNode = node->FirstChild(SETTING_XML_ELM_GROUP);
  while (groupNode != nullptr)
  {
    std::string groupId;
    if (CSettingGroup::DeserializeIdentification(groupNode, groupId))
    {
      auto groupIt = std::find_if(m_groups.begin(), m_groups.end(),
        [&groupId](const SettingGroupPtr& group)
      {
        return group->GetId() == groupId;
      });

      SettingGroupPtr group;
      if (groupIt != m_groups.end())
        group = *groupIt;

      update = (group != nullptr);
      if (!update)
        group = std::make_shared<CSettingGroup>(groupId, m_settingsManager);

      if (group->Deserialize(groupNode, update))
      {
        if (!update)
          addISetting(groupNode, group, m_groups);
      }
      else
        CLog::Log(LOGWARNING, "CSettingCategory: unable to read group \"%s\"", groupId.c_str());
    }

    groupNode = groupNode->NextSibling(SETTING_XML_ELM_GROUP);
  }

  return true;
}

SettingGroupList CSettingCategory::GetGroups(SettingLevel level) const
{
  SettingGroupList groups;
  for (const auto& group : m_groups)
  {
    if (group->MeetsRequirements() && group->IsVisible() && group->GetSettings(level).size() > 0)
      groups.push_back(group);
  }

  return groups;
}

bool CSettingCategory::CanAccess() const
{
  return m_accessCondition.Check();
}

void CSettingCategory::AddGroup(SettingGroupPtr group)
{
  addISetting(nullptr, group, m_groups);
}

void CSettingCategory::AddGroups(const SettingGroupList &groups)
{
  for (const auto& group : groups)
    addISetting(nullptr, group, m_groups);
}

CSettingSection::CSettingSection(const std::string &id, CSettingsManager *settingsManager /* = nullptr */)
  : ISetting(id, settingsManager)
{ }

bool CSettingSection::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  // handle <visible> conditions
  if (!ISetting::Deserialize(node, update))
    return false;

  auto categoryNode = node->FirstChild(SETTING_XML_ELM_CATEGORY);
  while (categoryNode != nullptr)
  {
    std::string categoryId;
    if (CSettingCategory::DeserializeIdentification(categoryNode, categoryId))
    {
      auto categoryIt = std::find_if(m_categories.begin(), m_categories.end(),
        [&categoryId](const SettingCategoryPtr& category)
      {
        return category->GetId() == categoryId;
      });

      SettingCategoryPtr category;
      if (categoryIt != m_categories.end())
        category = *categoryIt;

      update = (category != nullptr);
      if (!update)
        category = std::make_shared<CSettingCategory>(categoryId, m_settingsManager);

      if (category->Deserialize(categoryNode, update))
      {
        if (!update)
          addISetting(categoryNode, category, m_categories);
      }
      else
        CLog::Log(LOGWARNING, "CSettingSection: unable to read category \"%s\"", categoryId.c_str());
    }

    categoryNode = categoryNode->NextSibling(SETTING_XML_ELM_CATEGORY);
  }

  return true;
}

SettingCategoryList CSettingSection::GetCategories(SettingLevel level) const
{
  SettingCategoryList categories;
  for (const auto& category : m_categories)
  {
    if (category->MeetsRequirements() && category->IsVisible() && category->GetGroups(level).size() > 0)
      categories.push_back(category);
  }

  return categories;
}

void CSettingSection::AddCategory(SettingCategoryPtr category)
{
  addISetting(nullptr, category, m_categories);
}

void CSettingSection::AddCategories(const SettingCategoryList &categories)
{
  for (const auto& category : categories)
    addISetting(nullptr, category, m_categories);
}
