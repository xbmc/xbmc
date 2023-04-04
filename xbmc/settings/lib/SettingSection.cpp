/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingSection.h"

#include "ServiceBroker.h"
#include "SettingDefinitions.h"
#include "SettingsManager.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include <algorithm>

template<class T>
void addISetting(const TiXmlNode* node, const T& item, std::vector<T>& items, bool toBegin = false)
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

  if (!toBegin)
    items.emplace_back(item);
  else
    items.insert(items.begin(), item);
}

Logger CSettingGroup::s_logger;

CSettingGroup::CSettingGroup(const std::string& id,
                             CSettingsManager* settingsManager /* = nullptr */)
  : ISetting(id, settingsManager)
{
  if (s_logger == nullptr)
    s_logger = CServiceBroker::GetLogging().GetLogger("CSettingGroup");
}

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
      s_logger->error("unable to read control type");
      return false;
    }

    m_control = m_settingsManager->CreateControl(controlType);
    if (m_control == nullptr)
    {
      s_logger->error("unable to create new control \"{}\"", controlType);
      return false;
    }

    if (!m_control->Deserialize(controlElement))
    {
      s_logger->warn("unable to read control \"{}\"", controlType);
      m_control.reset();
    }
  }

  auto settingElement = node->FirstChildElement(SETTING_XML_ELM_SETTING);
  while (settingElement != nullptr)
  {
    std::string settingId;
    bool isReference;
    if (CSetting::DeserializeIdentification(settingElement, settingId, isReference))
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
          s_logger->error("unable to read setting type of \"{}\"", settingId);
          return false;
        }

        setting = m_settingsManager->CreateSetting(settingType, settingId, m_settingsManager);
        if (setting == nullptr)
          s_logger->error("unknown setting type \"{}\" of \"{}\"", settingType, settingId);
      }

      if (setting == nullptr)
        s_logger->error("unable to create new setting \"{}\"", settingId);
      else
      {
        if (!setting->Deserialize(settingElement, update))
          s_logger->warn("unable to read setting \"{}\"", settingId);
        else
        {
          // if the setting is a reference turn it into one
          if (isReference)
            setting->MakeReference();

          if (!update)
            addISetting(settingElement, setting, m_settings);
        }
      }
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

bool CSettingGroup::ContainsVisibleSettings(const SettingLevel level) const
{
  return std::any_of(m_settings.begin(), m_settings.end(), [&level](const SettingPtr& setting) {
    return setting->GetLevel() <= level && setting->MeetsRequirements() && setting->IsVisible();
  });
}

void CSettingGroup::AddSetting(const SettingPtr& setting)
{
  addISetting(nullptr, setting, m_settings);
}

void CSettingGroup::AddSettings(const SettingList &settings)
{
  for (const auto& setting : settings)
    addISetting(nullptr, setting, m_settings);
}

bool CSettingGroup::ReplaceSetting(const std::shared_ptr<const CSetting>& currentSetting,
                                   const std::shared_ptr<CSetting>& newSetting)
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

Logger CSettingCategory::s_logger;

CSettingCategory::CSettingCategory(const std::string& id,
                                   CSettingsManager* settingsManager /* = nullptr */)
  : ISetting(id, settingsManager),
    m_accessCondition(settingsManager)
{
  if (s_logger == nullptr)
    s_logger = CServiceBroker::GetLogging().GetLogger("CSettingCategory");
}

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
        s_logger->warn("unable to read group \"{}\"", groupId);
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
    if (group->MeetsRequirements() && group->IsVisible() && group->ContainsVisibleSettings(level))
      groups.push_back(group);
  }

  return groups;
}

bool CSettingCategory::CanAccess() const
{
  return m_accessCondition.Check();
}

void CSettingCategory::AddGroup(const SettingGroupPtr& group)
{
  addISetting(nullptr, group, m_groups, false);
}

void CSettingCategory::AddGroupToFront(const SettingGroupPtr& group)
{
  addISetting(nullptr, group, m_groups, true);
}

void CSettingCategory::AddGroups(const SettingGroupList &groups)
{
  for (const auto& group : groups)
    addISetting(nullptr, group, m_groups);
}

Logger CSettingSection::s_logger;

CSettingSection::CSettingSection(const std::string& id,
                                 CSettingsManager* settingsManager /* = nullptr */)
  : ISetting(id, settingsManager)
{
  if (s_logger == nullptr)
    s_logger = CServiceBroker::GetLogging().GetLogger("CSettingSection");
}

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
        s_logger->warn("unable to read category \"{}\"", categoryId);
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

void CSettingSection::AddCategory(const SettingCategoryPtr& category)
{
  addISetting(nullptr, category, m_categories);
}

void CSettingSection::AddCategories(const SettingCategoryList &categories)
{
  for (const auto& category : categories)
    addISetting(nullptr, category, m_categories);
}
