/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingsManager.h"

#include "ServiceBroker.h"
#include "Setting.h"
#include "SettingDefinitions.h"
#include "SettingSection.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include <algorithm>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <unordered_set>
#include <utility>

const uint32_t CSettingsManager::Version = 2;
const uint32_t CSettingsManager::MinimumSupportedVersion = 0;

bool ParseSettingIdentifier(const std::string& settingId, std::string& categoryTag, std::string& settingTag)
{
  static const std::string Separator = ".";

  if (settingId.empty())
    return false;

  std::vector<std::string> parts = StringUtils::Split(settingId, Separator);
  if (parts.empty() || parts.at(0).empty())
    return false;

  if (parts.size() == 1)
  {
    settingTag = parts.at(0);
    return true;
  }

  // get the category tag and remove it from the parts
  categoryTag = parts.at(0);
  parts.erase(parts.begin());

  // put together the setting tag
  settingTag = StringUtils::Join(parts, Separator);

  return true;
}

CSettingsManager::CSettingsManager()
  : m_logger(CServiceBroker::GetLogging().GetLogger("CSettingsManager"))
{
}

CSettingsManager::~CSettingsManager()
{
  // first clear all registered settings handler and subsettings
  // implementations because we can't be sure that they are still valid
  m_settingsHandlers.clear();
  m_settingCreators.clear();
  m_settingControlCreators.clear();

  Clear();
}

uint32_t CSettingsManager::ParseVersion(const TiXmlElement* root) const
{
  // try to get and check the version
  uint32_t version = 0;
  root->QueryUnsignedAttribute(SETTING_XML_ROOT_VERSION, &version);

  return version;
}

bool CSettingsManager::Initialize(const TiXmlElement *root)
{
  std::unique_lock lock(m_critical);
  std::unique_lock settingsLock(m_settingsCritical);
  if (m_initialized || !root)
    return false;

  if (!StringUtils::EqualsNoCase(root->ValueStr(), SETTING_XML_ROOT))
  {
    m_logger->error(StringUtils::Format(
        "error reading settings definition: doesn't contain <{}> tag", SETTING_XML_ROOT));
    return false;
  }

  // try to get and check the version
  uint32_t version = ParseVersion(root);
  if (version == 0)
    m_logger->warn(StringUtils::Format("missing {} attribute", SETTING_XML_ROOT_VERSION));

  if (MinimumSupportedVersion >= version+1)
  {
    m_logger->error("unable to read setting definitions from version {} (minimum version: {})",
                    version, MinimumSupportedVersion);
    return false;
  }
  if (version > Version)
  {
    m_logger->error("unable to read setting definitions from version {} (current version: {})",
                    version, Version);
    return false;
  }

  const TiXmlNode* sectionNode = root->FirstChild(SETTING_XML_ELM_SECTION);
  while (sectionNode)
  {
    std::string sectionId;
    if (CSettingSection::DeserializeIdentification(sectionNode, sectionId))
    {
      SettingSectionPtr section = nullptr;
      const auto itSection = m_sections.find(sectionId);
      bool update = (itSection != m_sections.end());
      if (!update)
        section = std::make_shared<CSettingSection>(sectionId, this);
      else
        section = itSection->second;

      if (section->Deserialize(sectionNode, update))
        AddSection(section);
      else
      {
        m_logger->warn("unable to read section \"{}\"", sectionId);
      }
    }

    sectionNode = sectionNode->NextSibling(SETTING_XML_ELM_SECTION);
  }

  return true;
}

bool CSettingsManager::Load(const TiXmlElement* root,
                            bool& updated,
                            bool triggerEvents /* = true */,
                            LoadedSettings* loadedSettings /* = nullptr */)
{
  std::shared_lock lock(m_critical);
  std::unique_lock settingsLock(m_settingsCritical);
  if (m_loaded || !root)
    return false;

  if (triggerEvents && !OnSettingsLoading())
    return false;

  // try to get and check the version
  uint32_t version = ParseVersion(root);
  if (version == 0)
    m_logger->warn("missing {} attribute", SETTING_XML_ROOT_VERSION);

  if (MinimumSupportedVersion >= version+1)
  {
    m_logger->error("unable to read setting values from version {} (minimum version: {})", version,
                    MinimumSupportedVersion);
    return false;
  }
  if (version > Version)
  {
    m_logger->error("unable to read setting values from version {} (current version: {})", version,
                    Version);
    return false;
  }

  if (!Deserialize(root, updated, loadedSettings))
    return false;

  if (triggerEvents)
    OnSettingsLoaded();

  return true;
}

bool CSettingsManager::Save(
  const ISettingsValueSerializer* serializer, std::string& serializedValues) const
{
  if (!serializer)
    return false;

  std::shared_lock lock(m_critical);
  std::shared_lock settingsLock(m_settingsCritical);
  if (!m_initialized)
    return false;

  if (!OnSettingsSaving())
    return false;

  serializedValues = serializer->SerializeValues(this);

  OnSettingsSaved();

  return true;
}

void CSettingsManager::Unload()
{
  std::unique_lock lock(m_settingsCritical);
  if (!m_loaded)
    return;

  // needs to be set before calling CSetting::Reset() to avoid calls to
  // OnSettingChanging() and OnSettingChanged()
  m_loaded = false;

  for (const auto& [_, setting] : m_settings)
    setting.setting->Reset();

  OnSettingsUnloaded();
}

void CSettingsManager::Clear()
{
  std::unique_lock lock(m_critical);
  Unload();

  m_settings.clear();
  m_sections.clear();

  OnSettingsCleared();

  m_initialized = false;
}

bool CSettingsManager::LoadSetting(const TiXmlNode *node, const std::string &settingId)
{
  bool updated = false;
  return LoadSetting(node, settingId, updated);
}

bool CSettingsManager::LoadSetting(const TiXmlNode *node, const std::string &settingId, bool &updated)
{
  updated = false;

  if (!node)
    return false;

  const SettingPtr setting = GetSetting(settingId);
  if (!setting)
    return false;

  return LoadSetting(node, setting, updated);
}

void CSettingsManager::SetInitialized()
{
  std::unique_lock lock(m_settingsCritical);
  if (m_initialized)
    return;

  m_initialized = true;

  // resolve any reference settings
  for (const auto& [_, section] : m_sections)
    ResolveReferenceSettings(section);

  // remove any incomplete settings
  CleanupIncompleteSettings();

  // figure out all the dependencies between settings
  for (const auto& [_, setting] : m_settings)
    ResolveSettingDependencies(setting);
}

void CSettingsManager::AddSection(const SettingSectionPtr& section)
{
  if (!section)
    return;

  std::unique_lock lock(m_critical);
  std::unique_lock settingsLock(m_settingsCritical);

  section->CheckRequirements();
  m_sections[section->GetId()] = section;

  // get all settings and add them to the settings map
  std::set<SettingPtr> newSettings;
  for (const auto& category : section->GetCategories())
  {
    category->CheckRequirements();
    for (auto& group : category->GetGroups())
    {
      group->CheckRequirements();
      for (const auto& setting : group->GetSettings())
      {
        AddSetting(setting);

        newSettings.insert(setting);
      }
    }
  }

  if (m_initialized && !newSettings.empty())
  {
    // resolve any reference settings in the new section
    ResolveReferenceSettings(section);

    // cleanup any newly added incomplete settings
    CleanupIncompleteSettings();

    // resolve any dependencies for the newly added settings
    for (const auto& setting : newSettings)
      ResolveSettingDependencies(setting);
  }
}

bool CSettingsManager::AddSetting(const std::shared_ptr<CSetting>& setting,
                                  const std::shared_ptr<CSettingSection>& section,
                                  const std::shared_ptr<CSettingCategory>& category,
                                  const std::shared_ptr<CSettingGroup>& group)
{
  if (!setting || !section || !category || !group)
    return false;

  std::unique_lock lock(m_critical);
  std::unique_lock settingsLock(m_settingsCritical);

  // check if a setting with the given ID already exists
  if (FindSetting(setting->GetId()) != m_settings.end())
    return false;

  // if the given setting has not been added to the group yet, do it now
  const SettingList& settings = group->GetSettings();
  if (std::ranges::find(settings, setting) == settings.end())
    group->AddSetting(setting);

  // if the given group has not been added to the category yet, do it now
  const SettingGroupList& groups = category->GetGroups();
  if (std::ranges::find(groups, group) == groups.end())
    category->AddGroup(group);

  // if the given category has not been added to the section yet, do it now
  const SettingCategoryList& categories = section->GetCategories();
  if (std::ranges::find(categories, category) == categories.end())
    section->AddCategory(category);

  // check if the given section exists and matches
  const std::shared_ptr<const CSettingSection> sectionPtr = GetSection(section->GetId());
  if (sectionPtr && sectionPtr != section)
    return false;

  // if the section doesn't exist yet, add it
  if (!sectionPtr)
    AddSection(section);
  else
  {
    // add the setting
    AddSetting(setting);

    if (m_initialized)
    {
      // cleanup any newly added incomplete setting
      CleanupIncompleteSettings();

      // resolve any dependencies for the newly added setting
      ResolveSettingDependencies(setting);
    }
  }

  return true;
}

void CSettingsManager::RegisterCallback(ISettingCallback* callback,
                                        const SettingsContainer& settingList)
{
  std::unique_lock lock(m_settingsCritical);
  if (!callback)
    return;

  for (const auto& setting : settingList)
  {
    auto itSetting = FindSetting(setting);
    if (itSetting == m_settings.end())
    {
      if (m_initialized)
        continue;

      Setting tmpSetting = {};
      const auto [it, _] = InsertSetting(setting, tmpSetting);
      itSetting = it;
    }

    itSetting->second.callbacks.insert(callback);
  }
}

void CSettingsManager::UnregisterCallback(ISettingCallback *callback)
{
  std::unique_lock lock(m_settingsCritical);
  for (auto& [_, setting] : m_settings)
    setting.callbacks.erase(callback);
}

void CSettingsManager::RegisterSettingType(const std::string &settingType, ISettingCreator *settingCreator)
{
  std::unique_lock lock(m_critical);
  if (settingType.empty() || !settingCreator)
    return;

  if (!m_settingCreators.contains(settingType))
    m_settingCreators.try_emplace(settingType, settingCreator);
}

void CSettingsManager::RegisterSettingControl(const std::string &controlType, ISettingControlCreator *settingControlCreator)
{
  if (controlType.empty() || !settingControlCreator)
    return;

  std::unique_lock lock(m_critical);
  if (!m_settingControlCreators.contains(controlType))
    m_settingControlCreators.try_emplace(controlType, settingControlCreator);
}

void CSettingsManager::RegisterSettingsHandler(ISettingsHandler *settingsHandler, bool bFront /* = false */)
{
  if (!settingsHandler)
    return;

  std::unique_lock lock(m_critical);
  if (std::ranges::find(m_settingsHandlers, settingsHandler) == m_settingsHandlers.end())
  {
    if (bFront)
      m_settingsHandlers.insert(m_settingsHandlers.begin(), settingsHandler);
    else
      m_settingsHandlers.emplace_back(settingsHandler);
  }
}

void CSettingsManager::UnregisterSettingsHandler(ISettingsHandler *settingsHandler)
{
  if (!settingsHandler)
    return;

  std::unique_lock lock(m_critical);
  const auto it = std::ranges::find(m_settingsHandlers, settingsHandler);
  if (it != m_settingsHandlers.end())
    m_settingsHandlers.erase(it);
}

void CSettingsManager::RegisterSettingOptionsFiller(
    const std::string& identifier, const IntegerSettingOptionsFiller& optionsFiller)
{
  if (identifier.empty() || !optionsFiller)
    return;

  std::unique_lock lock(m_critical);
  if (m_optionsFillers.contains(identifier))
    return;

  m_optionsFillers.try_emplace(identifier, optionsFiller);
}

void CSettingsManager::RegisterSettingOptionsFiller(const std::string& identifier,
                                                    const StringSettingOptionsFiller& optionsFiller)
{
  if (identifier.empty() || !optionsFiller)
    return;

  std::unique_lock lock(m_critical);
  if (m_optionsFillers.contains(identifier))
    return;

  m_optionsFillers.try_emplace(identifier, optionsFiller);
}

void CSettingsManager::UnregisterSettingOptionsFiller(const std::string &identifier)
{
  std::unique_lock lock(m_critical);
  m_optionsFillers.erase(identifier);
}

SettingOptionsFiller CSettingsManager::GetSettingOptionsFiller(
    const std::shared_ptr<const CSetting>& setting)
{
  std::shared_lock lock(m_critical);
  if (!setting)
    return {};

  // get the option filler's identifier
  std::string filler;
  if (setting->GetType() == SettingType::Integer)
    filler = std::static_pointer_cast<const CSettingInt>(setting)->GetOptionsFillerName();
  else if (setting->GetType() == SettingType::String)
    filler = std::static_pointer_cast<const CSettingString>(setting)->GetOptionsFillerName();

  if (filler.empty())
    return {};

  // check if such an option filler is known
  const auto fillerIt = m_optionsFillers.find(filler);
  if (fillerIt == m_optionsFillers.end())
    return {};

  // make sure the option filler's type matches the setting's type
  switch (fillerIt->second.type)
  {
    case SettingOptionsFillerType::Integer:
    {
      if (setting->GetType() != SettingType::Integer)
        return {};

      if (!fillerIt->second.intFiller)
        return {};

      break;
    }

    case SettingOptionsFillerType::String:
    {
      if (setting->GetType() != SettingType::String)
        return {};

      if (!fillerIt->second.stringFiller)
        return {};

      break;
    }

    default:
      return {};
  }

  return fillerIt->second;
}

bool CSettingsManager::HasSettings() const
{
  return !m_settings.empty();
}

SettingPtr CSettingsManager::GetSetting(const std::string &id) const
{
  std::shared_lock lock(m_settingsCritical);
  if (id.empty())
    return nullptr;

  const auto setting = FindSetting(id);
  if (setting != m_settings.end())
  {
    if (setting->second.setting->IsReference())
      return GetSetting(setting->second.setting->GetReferencedId());
    return setting->second.setting;
  }

  if (m_loaded)
    m_logger->debug("requested setting ({}) was not found.", id);
  return nullptr;
}

SettingSectionList CSettingsManager::GetSections() const
{
  std::shared_lock lock(m_critical);
  SettingSectionList sections;
  for (const auto& [_, section] : m_sections)
    sections.emplace_back(section);

  return sections;
}

SettingSectionPtr CSettingsManager::GetSection(std::string section) const
{
  std::shared_lock lock(m_critical);
  if (section.empty())
    return nullptr;

  StringUtils::ToLower(section);

  const auto sectionIt = m_sections.find(section);
  if (sectionIt != m_sections.end())
    return sectionIt->second;

  m_logger->debug("requested setting section ({}) was not found.", section);
  return nullptr;
}

SettingDependencyMap CSettingsManager::GetDependencies(const std::string &id) const
{
  std::shared_lock lock(m_settingsCritical);
  const auto setting = FindSetting(id);
  if (setting == m_settings.end())
    return SettingDependencyMap();

  return setting->second.dependencies;
}

SettingDependencyMap CSettingsManager::GetDependencies(const SettingConstPtr& setting) const
{
  if (!setting)
    return SettingDependencyMap();

  return GetDependencies(setting->GetId());
}

bool CSettingsManager::GetBool(const std::string &id) const
{
  std::shared_lock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (!setting || setting->GetType() != SettingType::Boolean)
    return false;

  return std::static_pointer_cast<CSettingBool>(setting)->GetValue();
}

bool CSettingsManager::SetBool(const std::string &id, bool value)
{
  std::shared_lock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (!setting || setting->GetType() != SettingType::Boolean)
    return false;

  return std::static_pointer_cast<CSettingBool>(setting)->SetValue(value);
}

bool CSettingsManager::ToggleBool(const std::string &id)
{
  std::shared_lock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (!setting || setting->GetType() != SettingType::Boolean)
    return false;

  return SetBool(id, !std::static_pointer_cast<CSettingBool>(setting)->GetValue());
}

int CSettingsManager::GetInt(const std::string &id) const
{
  std::shared_lock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (!setting || setting->GetType() != SettingType::Integer)
    return 0;

  return std::static_pointer_cast<CSettingInt>(setting)->GetValue();
}

bool CSettingsManager::SetInt(const std::string &id, int value)
{
  std::shared_lock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (!setting || setting->GetType() != SettingType::Integer)
    return false;

  return std::static_pointer_cast<CSettingInt>(setting)->SetValue(value);
}

double CSettingsManager::GetNumber(const std::string &id) const
{
  std::shared_lock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (!setting || setting->GetType() != SettingType::Number)
    return 0.0;

  return std::static_pointer_cast<CSettingNumber>(setting)->GetValue();
}

bool CSettingsManager::SetNumber(const std::string &id, double value)
{
  std::shared_lock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (!setting || setting->GetType() != SettingType::Number)
    return false;

  return std::static_pointer_cast<CSettingNumber>(setting)->SetValue(value);
}

std::string CSettingsManager::GetString(const std::string &id) const
{
  std::shared_lock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (!setting || setting->GetType() != SettingType::String)
    return "";

  return std::static_pointer_cast<CSettingString>(setting)->GetValue();
}

bool CSettingsManager::SetString(const std::string &id, const std::string &value)
{
  std::shared_lock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (!setting || setting->GetType() != SettingType::String)
    return false;

  return std::static_pointer_cast<CSettingString>(setting)->SetValue(value);
}

std::vector< std::shared_ptr<CSetting> > CSettingsManager::GetList(const std::string &id) const
{
  std::shared_lock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (!setting || setting->GetType() != SettingType::List)
    return std::vector< std::shared_ptr<CSetting> >();

  return std::static_pointer_cast<CSettingList>(setting)->GetValue();
}

bool CSettingsManager::SetList(const std::string &id, const std::vector< std::shared_ptr<CSetting> > &value)
{
  std::shared_lock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (!setting || setting->GetType() != SettingType::List)
    return false;

  return std::static_pointer_cast<CSettingList>(setting)->SetValue(value);
}

bool CSettingsManager::SetDefault(const std::string &id)
{
  std::shared_lock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (!setting)
    return false;

  setting->Reset();
  return true;
}

void CSettingsManager::SetDefaults()
{
  std::shared_lock lock(m_settingsCritical);
  for (const auto& [_, setting] : m_settings)
    setting.setting->Reset();
}

void CSettingsManager::AddCondition(const std::string &condition)
{
  std::unique_lock lock(m_critical);
  if (condition.empty())
    return;

  m_conditions.AddCondition(condition);
}

void CSettingsManager::AddDynamicCondition(const std::string& identifier,
                                           const SettingConditionCheck& condition)
{
  std::unique_lock lock(m_critical);
  if (identifier.empty() || !condition)
    return;

  m_conditions.AddDynamicCondition(identifier, condition);
}

void CSettingsManager::RemoveDynamicCondition(const std::string &identifier)
{
  std::unique_lock lock(m_critical);
  if (identifier.empty())
    return;

  m_conditions.RemoveDynamicCondition(identifier);
}

bool CSettingsManager::Serialize(TiXmlNode *parent) const
{
  if (!parent)
    return false;

  std::shared_lock lock(m_settingsCritical);

  for (const auto& [_, setting] : m_settings)
  {
    if (setting.setting->IsReference() || setting.setting->GetType() == SettingType::Action)
      continue;

    TiXmlElement settingElement(SETTING_XML_ELM_SETTING);
    settingElement.SetAttribute(SETTING_XML_ATTR_ID, setting.setting->GetId());

    // add the default attribute
    if (setting.setting->IsDefault())
      settingElement.SetAttribute(SETTING_XML_ELM_DEFAULT, "true");

    // add the value
    const TiXmlText value(setting.setting->ToString());
    settingElement.InsertEndChild(value);

    if (!parent->InsertEndChild(settingElement))
    {
      m_logger->warn(StringUtils::Format("unable to write <{} id=\"{}\"> tag",
                                         SETTING_XML_ELM_SETTING, setting.setting->GetId()));
      continue;
    }
  }

  return true;
}

bool CSettingsManager::Deserialize(const TiXmlNode* node,
                                   bool& updated,
                                   LoadedSettings* loadedSettings /* = nullptr */)
{
  updated = false;

  if (!node)
    return false;

  std::shared_lock lock(m_settingsCritical);

  // TODO: ideally this would be done by going through all <setting> elements
  // in node but as long as we have to support the v1- format that's not possible
  for (const auto& [settingname, setting] : m_settings)
  {
    bool settingUpdated = false;
    if (LoadSetting(node, setting.setting, settingUpdated))
    {
      updated |= settingUpdated;
      if (loadedSettings)
        loadedSettings->try_emplace(settingname, setting.setting);
    }
  }

  return true;
}

bool CSettingsManager::OnSettingChanging(const std::shared_ptr<const CSetting>& setting)
{
  if (!setting)
    return false;

  std::shared_lock settingsLock(m_settingsCritical);
  if (!m_loaded)
    return true;

  const auto settingIt = FindSetting(setting->GetId());
  if (settingIt == m_settings.end())
    return false;

  Setting settingData = settingIt->second;
  // now that we have a copy of the setting's data, we can leave the lock
  settingsLock.unlock();

  for (auto& callback : settingData.callbacks)
  {
    if (!callback->OnSettingChanging(setting))
      return false;
  }

  // if this is a reference setting apply the same change to the referenced setting
  if (setting->IsReference())
  {
    std::shared_lock lock(m_settingsCritical);
    const auto referencedSettingIt = FindSetting(setting->GetReferencedId());
    if (referencedSettingIt != m_settings.end())
    {
      Setting referencedSettingData = referencedSettingIt->second;
      // now that we have a copy of the setting's data, we can leave the lock
      lock.unlock();

      referencedSettingData.setting->FromString(setting->ToString());
    }
  }
  else if (!settingData.references.empty())
  {
    // if the changed setting is referenced by other settings apply the same change to the referencing settings
    std::unordered_set<SettingPtr> referenceSettings;
    std::shared_lock lock(m_settingsCritical);
    for (const auto& reference : settingData.references)
    {
      const auto referenceSettingIt = FindSetting(reference);
      if (referenceSettingIt != m_settings.end())
        referenceSettings.insert(referenceSettingIt->second.setting);
    }
    // now that we have a copy of the setting's data, we can leave the lock
    lock.unlock();

    for (auto& referenceSetting : referenceSettings)
      referenceSetting->FromString(setting->ToString());
  }

  return true;
}

void CSettingsManager::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  std::shared_lock lock(m_settingsCritical);
  if (!m_loaded || !setting)
    return;

  const auto settingIt = FindSetting(setting->GetId());
  if (settingIt == m_settings.end())
    return;

  Setting settingData = settingIt->second;
  // now that we have a copy of the setting's data, we can leave the lock
  lock.unlock();

  for (auto& callback : settingData.callbacks)
    callback->OnSettingChanged(setting);

  // now handle any settings which depend on the changed setting
  const SettingDependencyMap dependencies = GetDependencies(setting);
  for (const auto& [depname, deps] : dependencies)
  {
    for (const auto& dep : deps)
      UpdateSettingByDependency(depname, dep);
  }
}

void CSettingsManager::OnSettingAction(const std::shared_ptr<const CSetting>& setting)
{
  std::shared_lock lock(m_settingsCritical);
  if (!m_loaded || !setting)
    return;

  const auto settingIt = FindSetting(setting->GetId());
  if (settingIt == m_settings.end())
    return;

  Setting settingData = settingIt->second;
  // now that we have a copy of the setting's data, we can leave the lock
  lock.unlock();

  for (auto& callback : settingData.callbacks)
    callback->OnSettingAction(setting);
}

bool CSettingsManager::OnSettingUpdate(const SettingPtr& setting,
                                       const char* oldSettingId,
                                       const TiXmlNode* oldSettingNode)
{
  std::shared_lock lock(m_settingsCritical);
  if (!setting)
    return false;

  const auto settingIt = FindSetting(setting->GetId());
  if (settingIt == m_settings.end())
    return false;

  Setting settingData = settingIt->second;
  // now that we have a copy of the setting's data, we can leave the lock
  lock.unlock();

  bool ret = false;
  for (auto& callback : settingData.callbacks)
    ret |= callback->OnSettingUpdate(setting, oldSettingId, oldSettingNode);

  return ret;
}

void CSettingsManager::OnSettingPropertyChanged(const std::shared_ptr<const CSetting>& setting,
                                                const char* propertyName)
{
  std::shared_lock lock(m_settingsCritical);
  if (!m_loaded || !setting)
    return;

  const auto settingIt = FindSetting(setting->GetId());
  if (settingIt == m_settings.end())
    return;

  Setting settingData = settingIt->second;
  // now that we have a copy of the setting's data, we can leave the lock
  lock.unlock();

  for (auto& callback : settingData.callbacks)
    callback->OnSettingPropertyChanged(setting, propertyName);

  // check the changed property and if it may have an influence on the
  // children of the setting
  SettingDependencyType dependencyType = SettingDependencyType::Unknown;
  if (StringUtils::EqualsNoCase(propertyName, "enabled"))
    dependencyType = SettingDependencyType::Enable;
  else if (StringUtils::EqualsNoCase(propertyName, "visible"))
    dependencyType = SettingDependencyType::Visible;

  if (dependencyType != SettingDependencyType::Unknown)
  {
    for (const auto& child : settingIt->second.children)
      UpdateSettingByDependency(child, dependencyType);
  }
}

SettingPtr CSettingsManager::CreateSetting(const std::string &settingType, const std::string &settingId, CSettingsManager *settingsManager /* = nullptr */) const
{
  if (StringUtils::EqualsNoCase(settingType, "boolean"))
    return std::make_shared<CSettingBool>(settingId, const_cast<CSettingsManager*>(this));
  else if (StringUtils::EqualsNoCase(settingType, "integer"))
    return std::make_shared<CSettingInt>(settingId, const_cast<CSettingsManager*>(this));
  else if (StringUtils::EqualsNoCase(settingType, "number"))
    return std::make_shared<CSettingNumber>(settingId, const_cast<CSettingsManager*>(this));
  else if (StringUtils::EqualsNoCase(settingType, "string"))
    return std::make_shared<CSettingString>(settingId, const_cast<CSettingsManager*>(this));
  else if (StringUtils::EqualsNoCase(settingType, "action"))
    return std::make_shared<CSettingAction>(settingId, const_cast<CSettingsManager*>(this));
  else if (settingType.size() > 6 &&
           StringUtils::StartsWith(settingType, "list[") &&
           StringUtils::EndsWith(settingType, "]"))
  {
    std::string elementType = StringUtils::Mid(settingType, 5, settingType.size() - 6);
    SettingPtr elementSetting = CreateSetting(elementType, settingId + ".definition", const_cast<CSettingsManager*>(this));
    if (elementSetting)
      return std::make_shared<CSettingList>(settingId, elementSetting, const_cast<CSettingsManager*>(this));
  }

  std::shared_lock lock(m_critical);
  const auto creator = m_settingCreators.find(settingType);
  if (creator != m_settingCreators.end())
    return creator->second->CreateSetting(settingType, settingId, const_cast<CSettingsManager*>(this));

  return nullptr;
}

std::shared_ptr<ISettingControl> CSettingsManager::CreateControl(const std::string &controlType) const
{
  if (controlType.empty())
    return nullptr;

  std::shared_lock lock(m_critical);
  const auto creator = m_settingControlCreators.find(controlType);
  if (creator != m_settingControlCreators.end() && creator->second)
    return creator->second->CreateControl(controlType);

  return nullptr;
}

bool CSettingsManager::OnSettingsLoading()
{
  std::shared_lock lock(m_critical);
  for (const auto& settingsHandler : m_settingsHandlers)
  {
    if (!settingsHandler->OnSettingsLoading())
      return false;
  }

  return true;
}

void CSettingsManager::OnSettingsUnloaded()
{
  std::shared_lock lock(m_critical);
  for (const auto& settingsHandler : m_settingsHandlers)
    settingsHandler->OnSettingsUnloaded();
}

void CSettingsManager::OnSettingsLoaded()
{
  std::shared_lock lock(m_critical);
  for (const auto& settingsHandler : m_settingsHandlers)
    settingsHandler->OnSettingsLoaded();
}

bool CSettingsManager::OnSettingsSaving() const
{
  std::shared_lock lock(m_critical);
  return std::ranges::all_of(m_settingsHandlers, [](const auto& settingsHandler)
                             { return settingsHandler->OnSettingsSaving(); });
}

void CSettingsManager::OnSettingsSaved() const
{
  std::shared_lock lock(m_critical);
  for (const auto& settingsHandler : m_settingsHandlers)
    settingsHandler->OnSettingsSaved();
}

void CSettingsManager::OnSettingsCleared()
{
  std::shared_lock lock(m_critical);
  for (const auto& settingsHandler : m_settingsHandlers)
    settingsHandler->OnSettingsCleared();
}

bool CSettingsManager::LoadSetting(const TiXmlNode* node, const SettingPtr& setting, bool& updated)
{
  updated = false;

  if (!node || !setting)
    return false;

  if (setting->GetType() == SettingType::Action)
    return false;

  std::string settingId = setting->GetId();
  if (setting->IsReference())
    settingId = setting->GetReferencedId();

  const TiXmlElement* settingElement = nullptr;
  // try to split the setting identifier into category and subsetting identifier (v1-)
  std::string categoryTag;
  std::string settingTag;
  if (ParseSettingIdentifier(settingId, categoryTag, settingTag))
  {
    const TiXmlNode* categoryNode = node;
    if (!categoryTag.empty())
      categoryNode = node->FirstChild(categoryTag);

    if (categoryNode)
      settingElement = categoryNode->FirstChildElement(settingTag);
  }

  if (!settingElement)
  {
    // check if the setting is stored using its full setting identifier (v2+)
    settingElement = node->FirstChildElement(SETTING_XML_ELM_SETTING);
    while (settingElement)
    {
      const char* id = settingElement->Attribute(SETTING_XML_ATTR_ID);
      if (id && settingId.compare(id) == 0)
        break;

      settingElement = settingElement->NextSiblingElement(SETTING_XML_ELM_SETTING);
    }
  }

  if (!settingElement)
    return false;

  // check if the default="true" attribute is set for the value
  const char* isDefaultAttribute = settingElement->Attribute(SETTING_XML_ELM_DEFAULT);
  const bool isDefault =
      isDefaultAttribute && StringUtils::EqualsNoCase(isDefaultAttribute, "true");

  if (!setting->FromString(settingElement->FirstChild() ? settingElement->FirstChild()->ValueStr()
                                                        : StringUtils::Empty))
  {
    m_logger->warn("unable to read value of setting \"{}\"", settingId);
    return false;
  }

  // check if we need to perform any update logic for the setting
  const std::set<CSettingUpdate>& updates = setting->GetUpdates();
  for (const auto& update : updates)
    updated |= UpdateSetting(node, setting, update);

  // the setting's value hasn't been updated and is the default value
  // so we can reset it to the default value (in case the default value has changed)
  if (!updated && isDefault)
    setting->Reset();

  return true;
}

bool CSettingsManager::UpdateSetting(const TiXmlNode* node,
                                     const SettingPtr& setting,
                                     const CSettingUpdate& update)
{
  if (!node || !setting || update.GetType() == SettingUpdateType::Unknown)
    return false;

  bool updated = false;
  const char *oldSetting = nullptr;
  const TiXmlNode *oldSettingNode = nullptr;
  if (update.GetType() == SettingUpdateType::Rename)
  {
    if (update.GetValue().empty())
      return false;

    oldSetting = update.GetValue().c_str();
    std::string categoryTag;
    std::string settingTag;
    if (!ParseSettingIdentifier(oldSetting, categoryTag, settingTag))
      return false;

    const TiXmlNode* categoryNode = node;
    if (!categoryTag.empty())
    {
      categoryNode = node->FirstChild(categoryTag);
      if (!categoryNode)
        return false;
    }

    oldSettingNode = categoryNode->FirstChild(settingTag);
    if (!oldSettingNode)
      return false;

    if (setting->FromString(oldSettingNode->FirstChild() ? oldSettingNode->FirstChild()->ValueStr()
                                                         : StringUtils::Empty))
      updated = true;
    else
      m_logger->warn("unable to update \"{}\" through automatically renaming from \"{}\"",
                     setting->GetId(), oldSetting);
  }

  updated |= OnSettingUpdate(setting, oldSetting, oldSettingNode);
  return updated;
}

void CSettingsManager::UpdateSettingByDependency(const std::string &settingId, const CSettingDependency &dependency)
{
  UpdateSettingByDependency(settingId, dependency.GetType());
}

void CSettingsManager::UpdateSettingByDependency(const std::string &settingId, SettingDependencyType dependencyType)
{
  const auto settingIt = FindSetting(settingId);
  if (settingIt == m_settings.end())
    return;
  SettingPtr setting = settingIt->second.setting;
  if (!setting)
    return;

  switch (dependencyType)
  {
    case SettingDependencyType::Enable:
      // just trigger the property changed callback and a call to
      // CSetting::IsEnabled() will automatically determine the new
      // enabled state
      OnSettingPropertyChanged(setting, "enabled");
      break;

    case SettingDependencyType::Update:
    {
      SettingType type = setting->GetType();
      if (type == SettingType::Integer)
      {
        const auto settingInt = std::static_pointer_cast<CSettingInt>(setting);
        if (settingInt->GetOptionsType() == SettingOptionsType::Dynamic)
          settingInt->UpdateDynamicOptions();
      }
      else if (type == SettingType::String)
      {
        const auto settingString = std::static_pointer_cast<CSettingString>(setting);
        if (settingString->GetOptionsType() == SettingOptionsType::Dynamic)
          settingString->UpdateDynamicOptions();
      }
      // when a setting depends on another, it might need to refresh its visible/enable status
      // after been updated. E.g. if it depends on some complex setting condition
      RefreshVisibilityAndEnableStatus(setting);
      break;
    }

    case SettingDependencyType::Visible:
      // just trigger the property changed callback and a call to
      // CSetting::IsVisible() will automatically determine the new
      // visible state
      OnSettingPropertyChanged(setting, "visible");
      break;

    case SettingDependencyType::Unknown:
    default:
      break;
  }
}

void CSettingsManager::RefreshVisibilityAndEnableStatus(
    const std::shared_ptr<const CSetting>& setting)
{
  bool updateVisibility{false};
  bool updateEnableStatus{false};
  for (const auto& dep : setting->GetDependencies())
  {
    if (dep.GetType() == SettingDependencyType::Enable)
    {
      updateEnableStatus = true;
    }

    if (dep.GetType() == SettingDependencyType::Visible)
    {
      updateVisibility = true;
    }
  }

  if (updateVisibility)
  {
    OnSettingPropertyChanged(setting, "visible");
  }
  if (updateEnableStatus)
  {
    OnSettingPropertyChanged(setting, "enabled");
  }
}

void CSettingsManager::AddSetting(const std::shared_ptr<CSetting>& setting)
{
  setting->CheckRequirements();

  auto addedSetting = FindSetting(setting->GetId());
  if (addedSetting == m_settings.end())
  {
    Setting tmpSetting = {};
    const auto [it, _] = InsertSetting(setting->GetId(), tmpSetting);
    addedSetting = it;
  }

  if (!addedSetting->second.setting)
  {
    addedSetting->second.setting = setting;
    setting->SetCallback(this);
  }
}

void CSettingsManager::ResolveReferenceSettings(const std::shared_ptr<CSettingSection>& section)
{
  struct GroupedReferenceSettings
  {
    SettingPtr referencedSetting;
    std::unordered_set<SettingPtr> referenceSettings;
  };
  std::map<std::string, GroupedReferenceSettings, std::less<>> groupedReferenceSettings;

  // collect and group all reference(d) settings
  const SettingCategoryList& categories = section->GetCategories();
  for (const auto& category : categories)
  {
    const SettingGroupList& groups = category->GetGroups();
    for (const auto& group : groups)
    {
      const SettingList& settings = group->GetSettings();
      for (const auto& setting : settings)
      {
        if (setting->IsReference())
        {
          const std::string& referencedSettingId = setting->GetReferencedId();
          auto itGroupedReferenceSetting = groupedReferenceSettings.find(referencedSettingId);
          if (itGroupedReferenceSetting == groupedReferenceSettings.end())
          {
            SettingPtr referencedSetting = nullptr;
            const auto itReferencedSetting = FindSetting(referencedSettingId);
            if (itReferencedSetting == m_settings.end())
            {
              m_logger->warn("missing referenced setting \"{}\"", referencedSettingId);
              continue;
            }

            GroupedReferenceSettings groupedReferenceSetting;
            groupedReferenceSetting.referencedSetting = itReferencedSetting->second.setting;

            itGroupedReferenceSetting =
                groupedReferenceSettings.try_emplace(referencedSettingId, groupedReferenceSetting)
                    .first;
          }

          itGroupedReferenceSetting->second.referenceSettings.insert(setting);
        }
      }
    }
  }

  if (groupedReferenceSettings.empty())
    return;

  // merge all reference settings into the referenced setting
  for (const auto& [settingname, setting] : groupedReferenceSettings)
  {
    const auto itReferencedSetting = FindSetting(settingname);
    if (itReferencedSetting == m_settings.end())
      continue;

    for (const auto& referenceSetting : setting.referenceSettings)
    {
      setting.referencedSetting->MergeDetails(*referenceSetting);
      itReferencedSetting->second.references.insert(referenceSetting->GetId());
    }
  }

  // resolve any reference settings
  for (const auto& category : categories)
  {
    const SettingGroupList& groups = category->GetGroups();
    for (const auto& group : groups)
    {
      const SettingList& settings = group->GetSettings();
      for (const auto& setting : settings)
      {
        if (setting->IsReference())
        {
          const std::string& referencedSettingId = setting->GetReferencedId();
          auto itGroupedReferenceSetting = groupedReferenceSettings.find(referencedSettingId);
          if (itGroupedReferenceSetting != groupedReferenceSettings.end())
          {
            const SettingPtr referencedSetting =
                itGroupedReferenceSetting->second.referencedSetting;

            // clone the referenced setting and copy the general properties of the reference setting
            const SettingPtr clonedReferencedSetting = referencedSetting->Clone(setting->GetId());
            clonedReferencedSetting->SetReferencedId(referencedSettingId);
            clonedReferencedSetting->MergeBasics(*setting);

            group->ReplaceSetting(setting, clonedReferencedSetting);

            // update the setting
            const auto itReferenceSetting = FindSetting(setting->GetId());
            if (itReferenceSetting != m_settings.end())
              itReferenceSetting->second.setting = clonedReferencedSetting;
          }
        }
      }
    }
  }
}

void CSettingsManager::CleanupIncompleteSettings()
{
  // remove any empty and reference settings
  for (auto setting = m_settings.begin(); setting != m_settings.end(); )
  {
    auto tmpIterator = setting++;
    if (!tmpIterator->second.setting)
    {
      m_logger->warn("removing empty setting \"{}\"", tmpIterator->first);
      m_settings.erase(tmpIterator);
    }
  }
}

void CSettingsManager::ResolveSettingDependencies(const std::shared_ptr<CSetting>& setting)
{
  if (!setting)
    return;

  ResolveSettingDependencies(FindSetting(setting->GetId())->second);
}

void CSettingsManager::ResolveSettingDependencies(const Setting& setting)
{
  if (!setting.setting)
    return;

  // if the setting has a parent setting, add it to its children
  const std::string& parentSettingId = setting.setting->GetParent();
  if (!parentSettingId.empty())
  {
    const auto itParentSetting = FindSetting(parentSettingId);
    if (itParentSetting != m_settings.end())
      itParentSetting->second.children.insert(setting.setting->GetId());
  }

  // handle all dependencies of the setting
  const SettingDependencies& dependencies = setting.setting->GetDependencies();
  for (const auto& deps : dependencies)
  {
    const SettingsContainer settingIds = deps.GetSettings();
    for (const auto& settingId : settingIds)
    {
      const auto settingIt = FindSetting(settingId);
      if (settingIt == m_settings.end())
        continue;

      bool newDep = true;
      SettingDependencies& settingDeps = settingIt->second.dependencies[setting.setting->GetId()];
      for (const auto& dep : settingDeps)
      {
        if (dep.GetType() == deps.GetType())
        {
          newDep = false;
          break;
        }
      }

      if (newDep)
        settingDeps.push_back(deps);
    }
  }
}

CSettingsManager::SettingMap::const_iterator CSettingsManager::FindSetting(std::string settingId) const
{
  StringUtils::ToLower(settingId);
  return m_settings.find(settingId);
}

CSettingsManager::SettingMap::iterator CSettingsManager::FindSetting(std::string settingId)
{
  StringUtils::ToLower(settingId);
  return m_settings.find(settingId);
}

std::pair<CSettingsManager::SettingMap::iterator, bool> CSettingsManager::InsertSetting(std::string settingId, const Setting& setting)
{
  StringUtils::ToLower(settingId);
  return m_settings.try_emplace(settingId, setting);
}
