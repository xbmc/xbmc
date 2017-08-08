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

#include "SettingsManager.h"

#include <algorithm>
#include <utility>

#include "SettingDefinitions.h"
#include "SettingSection.h"
#include "Setting.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

bool ParseSettingIdentifier(const std::string& settingId, std::string& categoryTag, std::string& settingTag)
{
  static const std::string Separator = ".";

  if (settingId.empty())
    return false;

  auto parts = StringUtils::Split(settingId, Separator);
  if (parts.size() < 1 || parts.at(0).empty())
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

CSettingsManager::~CSettingsManager()
{
  // first clear all registered settings handler and subsettings
  // implementations because we can't be sure that they are still valid
  m_settingsHandlers.clear();
  m_subSettings.clear();
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
  CExclusiveLock lock(m_critical);
  CExclusiveLock settingsLock(m_settingsCritical);
  if (m_initialized || root == nullptr)
    return false;

  if (!StringUtils::EqualsNoCase(root->ValueStr(), SETTING_XML_ROOT))
  {
    CLog::Log(LOGERROR, "CSettingsManager: error reading settings definition: doesn't contain <settings> tag");
    return false;
  }

  // try to get and check the version
  uint32_t version = ParseVersion(root);
  if (version == 0)
    CLog::Log(LOGWARNING, "CSettingsManager: missing %s attribute", SETTING_XML_ROOT_VERSION);

  if (MinimumSupportedVersion >= version+1)
  {
    CLog::Log(LOGERROR, "CSettingsManager: unable to read setting definitions from version %u (minimum version: %u)", version, MinimumSupportedVersion);
    return false;
  }
  if (version > Version)
  {
    CLog::Log(LOGERROR, "CSettingsManager: unable to read setting definitions from version %u (current version: %u)", version, Version);
    return false;
  }

  auto sectionNode = root->FirstChild(SETTING_XML_ELM_SECTION);
  while (sectionNode != nullptr)
  {
    std::string sectionId;
    if (CSettingSection::DeserializeIdentification(sectionNode, sectionId))
    {
      SettingSectionPtr section = nullptr;
      auto itSection = m_sections.find(sectionId);
      bool update = (itSection != m_sections.end());
      if (!update)
        section = std::make_shared<CSettingSection>(sectionId, this);
      else
        section = itSection->second;

      if (section->Deserialize(sectionNode, update))
        AddSection(section);
      else
      {
        CLog::Log(LOGWARNING, "CSettingsManager: unable to read section \"%s\"", sectionId.c_str());
      }
    }
      
    sectionNode = sectionNode->NextSibling(SETTING_XML_ELM_SECTION);
  }

  return true;
}

bool CSettingsManager::Load(const TiXmlElement *root, bool &updated, bool triggerEvents /* = true */, std::map<std::string, SettingPtr> *loadedSettings /* = nullptr */)
{
  CSharedLock lock(m_critical);
  CExclusiveLock settingsLock(m_settingsCritical);
  if (m_loaded || root == nullptr)
    return false;

  if (triggerEvents && !OnSettingsLoading())
    return false;

  // try to get and check the version
  uint32_t version = ParseVersion(root);
  if (version == 0)
    CLog::Log(LOGWARNING, "CSettingsManager: missing %s attribute", SETTING_XML_ROOT_VERSION);

  if (MinimumSupportedVersion >= version+1)
  {
    CLog::Log(LOGERROR, "CSettingsManager: unable to read setting values from version %u (minimum version: %u)", version, MinimumSupportedVersion);
    return false;
  }
  if (version > Version)
  {
    CLog::Log(LOGERROR, "CSettingsManager: unable to read setting values from version %u (current version: %u)", version, Version);
    return false;
  }

  if (!Deserialize(root, updated, loadedSettings))
    return false;

  bool ret = true;
  // load any ISubSettings implementations
  if (triggerEvents)
    ret = Load(root);

  if (triggerEvents)
    OnSettingsLoaded();

  return ret;
}

bool CSettingsManager::Save(TiXmlNode *root) const
{
  CSharedLock lock(m_critical);
  CSharedLock settingsLock(m_settingsCritical);
  if (!m_initialized || root == nullptr)
    return false;

  if (!OnSettingsSaving())
    return false;

  // save the current version
  auto rootElement = root->ToElement();
  if (rootElement == nullptr)
  {
    CLog::Log(LOGERROR, "CSettingsManager: failed to save settings");
    return false;
  }
  rootElement->SetAttribute(SETTING_XML_ROOT_VERSION, Version);

  if (!Serialize(root))
  {
    CLog::Log(LOGERROR, "CSettingsManager: failed to save settings");
    return false;
  }

  // save any ISubSettings implementations
  for (const auto& subSetting : m_subSettings)
  {
    if (!subSetting->Save(root))
      return false;
  }

  OnSettingsSaved();

  return true;
}

void CSettingsManager::Unload()
{
  CExclusiveLock lock(m_settingsCritical);
  if (!m_loaded)
    return;

  // needs to be set before calling CSetting::Reset() to avoid calls to
  // OnSettingChanging() and OnSettingChanged()
  m_loaded = false;

  for (auto& setting : m_settings)
    setting.second.setting->Reset();

  OnSettingsUnloaded();
}

void CSettingsManager::Clear()
{
  CExclusiveLock lock(m_critical);
  Unload();

  m_settings.clear();
  m_sections.clear();

  OnSettingsCleared();

  for (auto& subSetting : m_subSettings)
    subSetting->Clear();

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

  if (node == nullptr)
    return false;

  auto setting = GetSetting(settingId);
  if (setting == nullptr)
    return false;

  return LoadSetting(node, setting, updated);
}

void CSettingsManager::SetInitialized()
{
  CExclusiveLock lock(m_settingsCritical);
  if (m_initialized)
    return;

  m_initialized = true;

  // resolve any reference settings
  for (const auto& section : m_sections)
    ResolveReferenceSettings(section.second);

  // remove any incomplete settings
  CleanupIncompleteSettings();

  // figure out all the dependencies between settings
  for (const auto& setting : m_settings)
    ResolveSettingDependencies(setting.second);
}

void CSettingsManager::AddSection(SettingSectionPtr section)
{
  if (section == nullptr)
    return;

  CExclusiveLock lock(m_critical);
  CExclusiveLock settingsLock(m_settingsCritical);

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

bool CSettingsManager::AddSetting(std::shared_ptr<CSetting> setting, std::shared_ptr<CSettingSection> section,
  std::shared_ptr<CSettingCategory> category, std::shared_ptr<CSettingGroup> group)
{
  if (setting == nullptr || section == nullptr || category == nullptr || group == nullptr)
    return false;

  CExclusiveLock lock(m_critical);
  CExclusiveLock settingsLock(m_settingsCritical);

  // check if a setting with the given ID already exists
  if (FindSetting(setting->GetId()) != m_settings.end())
    return false;

  // if the given setting has not been added to the group yet, do it now
  auto settings = group->GetSettings();
  if (std::find(settings.begin(), settings.end(), setting) == settings.end())
    group->AddSetting(setting);

  // if the given group has not been added to the category yet, do it now
  auto groups = category->GetGroups();
  if (std::find(groups.begin(), groups.end(), group) == groups.end())
    category->AddGroup(group);

  // if the given category has not been added to the section yet, do it now
  auto categories = section->GetCategories();
  if (std::find(categories.begin(), categories.end(), category) == categories.end())
    section->AddCategory(category);

  // check if the given section exists and matches
  auto sectionPtr = GetSection(section->GetId());
  if (sectionPtr != nullptr && sectionPtr != section)
    return false;

  // if the section doesn't exist yet, add it
  if (sectionPtr == nullptr)
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

void CSettingsManager::RegisterCallback(ISettingCallback *callback, const std::set<std::string> &settingList)
{
  CExclusiveLock lock(m_settingsCritical);
  if (callback == nullptr)
    return;

  for (const auto& setting : settingList)
  {
    auto itSetting = FindSetting(setting);
    if (itSetting == m_settings.end())
    {
      if (m_initialized)
        continue;

      Setting tmpSetting = { nullptr };
      std::pair<SettingMap::iterator, bool> tmpIt = InsertSetting(setting, tmpSetting);
      itSetting = tmpIt.first;
    }

    itSetting->second.callbacks.insert(callback);
  }
}

void CSettingsManager::UnregisterCallback(ISettingCallback *callback)
{
  CExclusiveLock lock(m_settingsCritical);
  for (auto& setting : m_settings)
    setting.second.callbacks.erase(callback);
}

void CSettingsManager::RegisterSettingType(const std::string &settingType, ISettingCreator *settingCreator)
{
  CExclusiveLock lock(m_critical);
  if (settingType.empty() || settingCreator == nullptr)
    return;

  auto creatorIt = m_settingCreators.find(settingType);
  if (creatorIt == m_settingCreators.end())
    m_settingCreators.insert(make_pair(settingType, settingCreator));
}

void CSettingsManager::RegisterSettingControl(const std::string &controlType, ISettingControlCreator *settingControlCreator)
{
  if (controlType.empty() || settingControlCreator == nullptr)
    return;

  CExclusiveLock lock(m_critical);
  auto creatorIt = m_settingControlCreators.find(controlType);
  if (creatorIt == m_settingControlCreators.end())
    m_settingControlCreators.insert(make_pair(controlType, settingControlCreator));
}

void CSettingsManager::RegisterSettingsHandler(ISettingsHandler *settingsHandler)
{
  if (settingsHandler == nullptr)
    return;

  CExclusiveLock lock(m_critical);
  if (find(m_settingsHandlers.begin(), m_settingsHandlers.end(), settingsHandler) == m_settingsHandlers.end())
    m_settingsHandlers.push_back(settingsHandler);
}

void CSettingsManager::UnregisterSettingsHandler(ISettingsHandler *settingsHandler)
{
  if (settingsHandler == nullptr)
    return;

  CExclusiveLock lock(m_critical);
  auto it = std::find(m_settingsHandlers.begin(), m_settingsHandlers.end(), settingsHandler);
  if (it != m_settingsHandlers.end())
    m_settingsHandlers.erase(it);
}

void CSettingsManager::RegisterSubSettings(ISubSettings *subSettings)
{
  CExclusiveLock lock(m_critical);
  if (subSettings == nullptr)
    return;

  m_subSettings.insert(subSettings);
}

void CSettingsManager::UnregisterSubSettings(ISubSettings *subSettings)
{
  CExclusiveLock lock(m_critical);
  if (subSettings == nullptr)
    return;

  m_subSettings.erase(subSettings);
}

void CSettingsManager::RegisterSettingOptionsFiller(const std::string &identifier, IntegerSettingOptionsFiller optionsFiller)
{
  if (identifier.empty() || optionsFiller == nullptr)
    return;

  RegisterSettingOptionsFiller(identifier, reinterpret_cast<void*>(optionsFiller), SettingOptionsFillerType::Integer);
}

void CSettingsManager::RegisterSettingOptionsFiller(const std::string &identifier, StringSettingOptionsFiller optionsFiller)
{
  if (identifier.empty() || optionsFiller == nullptr)
    return;

  RegisterSettingOptionsFiller(identifier, reinterpret_cast<void*>(optionsFiller), SettingOptionsFillerType::String);
}

void CSettingsManager::UnregisterSettingOptionsFiller(const std::string &identifier)
{
  CExclusiveLock lock(m_critical);
  m_optionsFillers.erase(identifier);
}

void* CSettingsManager::GetSettingOptionsFiller(SettingConstPtr setting)
{
  CSharedLock lock(m_critical);
  if (setting == nullptr)
    return nullptr;

  // get the option filler's identifier
  std::string filler;
  if (setting->GetType() == SettingType::Integer)
    filler = std::static_pointer_cast<const CSettingInt>(setting)->GetOptionsFillerName();
  else if (setting->GetType() == SettingType::String)
    filler = std::static_pointer_cast<const CSettingString>(setting)->GetOptionsFillerName();

  if (filler.empty())
    return nullptr;

  // check if such an option filler is known
  auto fillerIt = m_optionsFillers.find(filler);
  if (fillerIt == m_optionsFillers.end())
    return nullptr;

  if (fillerIt->second.filler == nullptr)
    return nullptr;

  // make sure the option filler's type matches the setting's type
  switch (fillerIt->second.type)
  {
    case SettingOptionsFillerType::Integer:
    {
      if (setting->GetType() != SettingType::Integer)
        return nullptr;

      break;
    }
    
    case SettingOptionsFillerType::String:
    {
      if (setting->GetType() != SettingType::String)
        return nullptr;

      break;
    }

    default:
      return nullptr;
  }

  return fillerIt->second.filler;
}

SettingPtr CSettingsManager::GetSetting(const std::string &id) const
{
  CSharedLock lock(m_settingsCritical);
  if (id.empty())
    return nullptr;

  auto setting = FindSetting(id);
  if (setting != m_settings.end())
    return setting->second.setting;

  CLog::Log(LOGDEBUG, "CSettingsManager: requested setting (%s) was not found.", id.c_str());
  return nullptr;
}

SettingSectionList CSettingsManager::GetSections() const
{
  CSharedLock lock(m_critical);
  SettingSectionList sections;
  for (const auto& section : m_sections)
    sections.push_back(section.second);

  return sections;
}

SettingSectionPtr CSettingsManager::GetSection(std::string section) const
{
  CSharedLock lock(m_critical);
  if (section.empty())
    return nullptr;

  StringUtils::ToLower(section);

  auto sectionIt = m_sections.find(section);
  if (sectionIt != m_sections.end())
    return sectionIt->second;

  CLog::Log(LOGDEBUG, "CSettingsManager: requested setting section (%s) was not found.", section.c_str());
  return nullptr;
}

SettingDependencyMap CSettingsManager::GetDependencies(const std::string &id) const
{
  CSharedLock lock(m_settingsCritical);
  auto setting = FindSetting(id);
  if (setting == m_settings.end())
    return SettingDependencyMap();

  return setting->second.dependencies;
}

SettingDependencyMap CSettingsManager::GetDependencies(SettingConstPtr setting) const
{
  if (setting == nullptr)
    return SettingDependencyMap();

  return GetDependencies(setting->GetId());
}

bool CSettingsManager::GetBool(const std::string &id) const
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == nullptr || setting->GetType() != SettingType::Boolean)
    return false;

  return std::static_pointer_cast<CSettingBool>(setting)->GetValue();
}

bool CSettingsManager::SetBool(const std::string &id, bool value)
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == nullptr || setting->GetType() != SettingType::Boolean)
    return false;

  return std::static_pointer_cast<CSettingBool>(setting)->SetValue(value);
}

bool CSettingsManager::ToggleBool(const std::string &id)
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == nullptr || setting->GetType() != SettingType::Boolean)
    return false;

  return SetBool(id, !std::static_pointer_cast<CSettingBool>(setting)->GetValue());
}

int CSettingsManager::GetInt(const std::string &id) const
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == nullptr || setting->GetType() != SettingType::Integer)
    return 0;

  return std::static_pointer_cast<CSettingInt>(setting)->GetValue();
}

bool CSettingsManager::SetInt(const std::string &id, int value)
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == nullptr || setting->GetType() != SettingType::Integer)
    return false;

  return std::static_pointer_cast<CSettingInt>(setting)->SetValue(value);
}

double CSettingsManager::GetNumber(const std::string &id) const
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == nullptr || setting->GetType() != SettingType::Number)
    return 0.0;

  return std::static_pointer_cast<CSettingNumber>(setting)->GetValue();
}

bool CSettingsManager::SetNumber(const std::string &id, double value)
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == nullptr || setting->GetType() != SettingType::Number)
    return false;

  return std::static_pointer_cast<CSettingNumber>(setting)->SetValue(value);
}

std::string CSettingsManager::GetString(const std::string &id) const
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == nullptr || setting->GetType() != SettingType::String)
    return "";

  return std::static_pointer_cast<CSettingString>(setting)->GetValue();
}

bool CSettingsManager::SetString(const std::string &id, const std::string &value)
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == nullptr || setting->GetType() != SettingType::String)
    return false;

  return std::static_pointer_cast<CSettingString>(setting)->SetValue(value);
}

std::vector< std::shared_ptr<CSetting> > CSettingsManager::GetList(const std::string &id) const
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == nullptr || setting->GetType() != SettingType::List)
    return std::vector< std::shared_ptr<CSetting> >();

  return std::static_pointer_cast<CSettingList>(setting)->GetValue();
}

bool CSettingsManager::SetList(const std::string &id, const std::vector< std::shared_ptr<CSetting> > &value)
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == nullptr || setting->GetType() != SettingType::List)
    return false;

  return std::static_pointer_cast<CSettingList>(setting)->SetValue(value);
}

bool CSettingsManager::SetDefault(const std::string &id)
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == nullptr)
    return false;

  setting->Reset();
  return true;
}

void CSettingsManager::SetDefaults()
{
  CSharedLock lock(m_settingsCritical);
  for (auto& setting : m_settings)
    setting.second.setting->Reset();
}

void CSettingsManager::AddCondition(const std::string &condition)
{
  CExclusiveLock lock(m_critical);
  if (condition.empty())
    return;

  m_conditions.AddCondition(condition);
}

void CSettingsManager::AddCondition(const std::string &identifier, SettingConditionCheck condition, void *data /*= nullptr*/)
{
  CExclusiveLock lock(m_critical);
  if (identifier.empty() || condition == nullptr)
    return;

  m_conditions.AddCondition(identifier, condition, data);
}
  
bool CSettingsManager::Serialize(TiXmlNode *parent) const
{
  if (parent == nullptr)
    return false;

  CSharedLock lock(m_settingsCritical);

  for (const auto& setting : m_settings)
  {
    if (setting.second.setting->GetType() == SettingType::Action)
      continue;

    TiXmlElement settingElement(SETTING_XML_ELM_SETTING);
    settingElement.SetAttribute(SETTING_XML_ATTR_ID, setting.second.setting->GetId());

    // add the default attribute
    if (setting.second.setting->IsDefault())
      settingElement.SetAttribute(SETTING_XML_ELM_DEFAULT, "true");

    // add the value
    TiXmlText value(setting.second.setting->ToString());
    settingElement.InsertEndChild(value);

    if (parent->InsertEndChild(settingElement) == nullptr)
    {
      CLog::Log(LOGWARNING, "CSetting: unable to write <" SETTING_XML_ELM_SETTING " id=\"%s\"> tag", setting.second.setting->GetId().c_str());
      continue;
    }
  }

  return true;
}
  
bool CSettingsManager::Deserialize(const TiXmlNode *node, bool &updated, std::map<std::string, SettingPtr> *loadedSettings /* = nullptr */)
{
  updated = false;

  if (node == nullptr)
    return false;

  CSharedLock lock(m_settingsCritical);

  // TODO: ideally this would be done by going through all <setting> elements
  // in node but as long as we have to support the v1- format that's not possible
  for (auto& setting : m_settings)
  {
    bool settingUpdated = false;
    if (LoadSetting(node, setting.second.setting, settingUpdated))
    {
      updated |= settingUpdated;
      if (loadedSettings != nullptr)
        loadedSettings->insert(make_pair(setting.first, setting.second.setting));
    }
  }

  return true;
}

bool CSettingsManager::OnSettingChanging(std::shared_ptr<const CSetting> setting)
{
  if (setting == nullptr)
    return false;

  CSharedLock lock(m_settingsCritical);
  if (!m_loaded)
    return true;

  auto settingIt = FindSetting(setting->GetId());
  if (settingIt == m_settings.end())
    return false;

  Setting settingData = settingIt->second;
  // now that we have a copy of the setting's data, we can leave the lock
  lock.Leave();

  for (auto& callback : settingData.callbacks)
  {
    if (!callback->OnSettingChanging(setting))
      return false;
  }

  return true;
}
  
void CSettingsManager::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  CSharedLock lock(m_settingsCritical);
  if (!m_loaded || setting == nullptr)
    return;
    
  auto settingIt = FindSetting(setting->GetId());
  if (settingIt == m_settings.end())
    return;

  Setting settingData = settingIt->second;
  // now that we have a copy of the setting's data, we can leave the lock
  lock.Leave();

  for (auto& callback : settingData.callbacks)
    callback->OnSettingChanged(setting);

  // now handle any settings which depend on the changed setting
  auto dependencies = GetDependencies(setting);
  for (const auto& deps : dependencies)
  {
    for (const auto& dep : deps.second)
      UpdateSettingByDependency(deps.first, dep);
  }
}

void CSettingsManager::OnSettingAction(std::shared_ptr<const CSetting> setting)
{
  CSharedLock lock(m_settingsCritical);
  if (!m_loaded || setting == nullptr)
    return;

  auto settingIt = FindSetting(setting->GetId());
  if (settingIt == m_settings.end())
    return;

  Setting settingData = settingIt->second;
  // now that we have a copy of the setting's data, we can leave the lock
  lock.Leave();

  for (auto& callback : settingData.callbacks)
    callback->OnSettingAction(setting);
}

bool CSettingsManager::OnSettingUpdate(SettingPtr setting, const char *oldSettingId, const TiXmlNode *oldSettingNode)
{
  CSharedLock lock(m_settingsCritical);
  if (setting == nullptr)
    return false;

  auto settingIt = FindSetting(setting->GetId());
  if (settingIt == m_settings.end())
    return false;

  Setting settingData = settingIt->second;
  // now that we have a copy of the setting's data, we can leave the lock
  lock.Leave();

  bool ret = false;
  for (auto& callback : settingData.callbacks)
    ret |= callback->OnSettingUpdate(setting, oldSettingId, oldSettingNode);

  return ret;
}

void CSettingsManager::OnSettingPropertyChanged(std::shared_ptr<const CSetting> setting, const char *propertyName)
{
  CSharedLock lock(m_settingsCritical);
  if (!m_loaded || setting == nullptr)
    return;

  auto settingIt = FindSetting(setting->GetId());
  if (settingIt == m_settings.end())
    return;

  Setting settingData = settingIt->second;
  // now that we have a copy of the setting's data, we can leave the lock
  lock.Leave();

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
  else if (StringUtils::EqualsNoCase(settingType, "reference"))
    return std::make_shared<CSettingReference>(settingId, const_cast<CSettingsManager*>(this));
  else if (settingType.size() > 6 &&
           StringUtils::StartsWith(settingType, "list[") &&
           StringUtils::EndsWith(settingType, "]"))
  {
    std::string elementType = StringUtils::Mid(settingType, 5, settingType.size() - 6);
    SettingPtr elementSetting = CreateSetting(elementType, settingId + ".definition", const_cast<CSettingsManager*>(this));
    if (elementSetting != nullptr)
      return std::make_shared<CSettingList>(settingId, elementSetting, const_cast<CSettingsManager*>(this));
  }

  CSharedLock lock(m_critical);
  auto creator = m_settingCreators.find(settingType);
  if (creator != m_settingCreators.end())
    return creator->second->CreateSetting(settingType, settingId, const_cast<CSettingsManager*>(this));

  return nullptr;
}

std::shared_ptr<ISettingControl> CSettingsManager::CreateControl(const std::string &controlType) const
{
  if (controlType.empty())
    return nullptr;

  CSharedLock lock(m_critical);
  auto creator = m_settingControlCreators.find(controlType);
  if (creator != m_settingControlCreators.end() && creator->second != nullptr)
    return creator->second->CreateControl(controlType);

  return nullptr;
}

bool CSettingsManager::OnSettingsLoading()
{
  CSharedLock lock(m_critical);
  for (const auto& settingsHandler : m_settingsHandlers)
  {
    if (!settingsHandler->OnSettingsLoading())
      return false;
  }

  return true;
}

void CSettingsManager::OnSettingsUnloaded()
{
  CSharedLock lock(m_critical);
  for (const auto& settingsHandler : m_settingsHandlers)
    settingsHandler->OnSettingsUnloaded();
}

void CSettingsManager::OnSettingsLoaded()
{
  CSharedLock lock(m_critical);
  for (const auto& settingsHandler : m_settingsHandlers)
    settingsHandler->OnSettingsLoaded();
}

bool CSettingsManager::OnSettingsSaving() const
{
  CSharedLock lock(m_critical);
  for (const auto& settingsHandler : m_settingsHandlers)
  {
    if (!settingsHandler->OnSettingsSaving())
      return false;
  }

  return true;
}

void CSettingsManager::OnSettingsSaved() const
{
  CSharedLock lock(m_critical);
  for (const auto& settingsHandler : m_settingsHandlers)
    settingsHandler->OnSettingsSaved();
}

void CSettingsManager::OnSettingsCleared()
{
  CSharedLock lock(m_critical);
  for (const auto& settingsHandler : m_settingsHandlers)
    settingsHandler->OnSettingsCleared();
}

bool CSettingsManager::Load(const TiXmlNode *settings)
{
  bool ok = true;
  CSharedLock lock(m_critical);
  for (const auto& subSetting : m_subSettings)
    ok &= subSetting->Load(settings);

  return ok;
}

bool CSettingsManager::LoadSetting(const TiXmlNode *node, SettingPtr setting, bool &updated)
{
  updated = false;

  if (node == nullptr || setting == nullptr)
    return false;

  if (setting->GetType() == SettingType::Action)
    return false;

  auto settingId = setting->GetId();

  const TiXmlElement* settingElement = nullptr;
  // try to split the setting identifier into category and subsetting identifer (v1-)
  std::string categoryTag, settingTag;
  if (ParseSettingIdentifier(settingId, categoryTag, settingTag))
  {
    auto categoryNode = node;
    if (!categoryTag.empty())
      categoryNode = node->FirstChild(categoryTag);

    if (categoryNode != nullptr)
      settingElement = categoryNode->FirstChildElement(settingTag);
  }

  if (settingElement == nullptr)
  {
    // check if the setting is stored using its full setting identifier (v2+)
    settingElement = node->FirstChildElement(SETTING_XML_ELM_SETTING);
    while (settingElement != nullptr)
    {
      const auto id = settingElement->Attribute(SETTING_XML_ATTR_ID);
      if (id != nullptr && settingId.compare(id) == 0)
        break;

      settingElement = settingElement->NextSiblingElement(SETTING_XML_ELM_SETTING);
    }
  } 

  if (settingElement == nullptr)
    return false;

  // check if the default="true" attribute is set for the value
  auto isDefaultAttribute = settingElement->Attribute(SETTING_XML_ELM_DEFAULT);
  bool isDefault = isDefaultAttribute != nullptr && StringUtils::EqualsNoCase(isDefaultAttribute, "true");

  if (!setting->FromString(settingElement->FirstChild() != nullptr ? settingElement->FirstChild()->ValueStr() : StringUtils::Empty))
  {
    CLog::Log(LOGWARNING, "CSettingsManager: unable to read value of setting \"%s\"", settingId.c_str());
    return false;
  }

  // check if we need to perform any update logic for the setting
  auto updates = setting->GetUpdates();
  for (const auto& update : updates)
    updated |= UpdateSetting(node, setting, update);

  // the setting's value hasn't been updated and is the default value
  // so we can reset it to the default value (in case the default value has changed)
  if (!updated && isDefault)
    setting->Reset();

  return true;
}

bool CSettingsManager::UpdateSetting(const TiXmlNode *node, SettingPtr setting, const CSettingUpdate& update)
{
  if (node == nullptr || setting == nullptr || update.GetType() == SettingUpdateType::Unknown)
    return false;

  bool updated = false;
  const char *oldSetting = nullptr;
  const TiXmlNode *oldSettingNode = nullptr;
  if (update.GetType() == SettingUpdateType::Rename)
  {
    if (update.GetValue().empty())
      return false;

    oldSetting = update.GetValue().c_str();
    std::string categoryTag, settingTag;
    if (!ParseSettingIdentifier(oldSetting, categoryTag, settingTag))
      return false;

    auto categoryNode = node;
    if (!categoryTag.empty())
    {
      categoryNode = node->FirstChild(categoryTag);
      if (categoryNode == nullptr)
        return false;
    }

    oldSettingNode = categoryNode->FirstChild(settingTag);
    if (oldSettingNode == nullptr)
      return false;

    if (setting->FromString(oldSettingNode->FirstChild() != nullptr ? oldSettingNode->FirstChild()->ValueStr() : StringUtils::Empty))
      updated = true;
    else
      CLog::Log(LOGWARNING, "CSetting: unable to update \"%s\" through automatically renaming from \"%s\"", setting->GetId().c_str(), oldSetting);
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
  SettingPtr setting = GetSetting(settingId);
  if (setting == nullptr)
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
      SettingType type = (SettingType)setting->GetType();
      if (type == SettingType::Integer)
      {
        auto settingInt = std::static_pointer_cast<CSettingInt>(setting);
        if (settingInt->GetOptionsType() == SettingOptionsType::Dynamic)
          settingInt->UpdateDynamicOptions();
      }
      else if (type == SettingType::String)
      {
        auto settingString = std::static_pointer_cast<CSettingString>(setting);
        if (settingString->GetOptionsType() == SettingOptionsType::Dynamic)
          settingString->UpdateDynamicOptions();
      }
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

void CSettingsManager::AddSetting(std::shared_ptr<CSetting> setting)
{
  setting->CheckRequirements();

  auto addedSetting = FindSetting(setting->GetId());
  if (addedSetting == m_settings.end())
  {
    Setting tmpSetting = { nullptr };
    auto tmpIt = InsertSetting(setting->GetId(), tmpSetting);
    addedSetting = tmpIt.first;
  }

  if (addedSetting->second.setting == nullptr)
  {
    addedSetting->second.setting = setting;
    setting->SetCallback(this);
  }
}

void CSettingsManager::ResolveReferenceSettings(std::shared_ptr<CSettingSection> section)
{
  // resolve any reference settings
  auto categories = section->GetCategories();
  for (const auto& category : categories)
  {
    auto groups = category->GetGroups();
    for (auto& group : groups)
    {
      auto settings = group->GetSettings();
      SettingList referenceSettings;
      for (const auto& setting : settings)
      {
        if (setting->GetType() == SettingType::Reference)
          referenceSettings.push_back(setting);
      }

      for (const auto& referenceSetting : referenceSettings)
      {
        auto referencedSettingId = std::static_pointer_cast<const CSettingReference>(referenceSetting)->GetReferencedId();
        SettingPtr referencedSetting = nullptr;
        auto itReferencedSetting = FindSetting(referencedSettingId);
        if (itReferencedSetting == m_settings.end())
          CLog::Log(LOGWARNING, "CSettingsManager: missing referenced setting \"%s\"", referencedSettingId.c_str());
        else
        {
          referencedSetting = itReferencedSetting->second.setting;
          itReferencedSetting = FindSetting(referenceSetting->GetId());
          if (itReferencedSetting != m_settings.end())
            m_settings.erase(itReferencedSetting);
        }

        group->ReplaceSetting(referenceSetting, referencedSetting);
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
    if (tmpIterator->second.setting == nullptr)
    {
      CLog::Log(LOGWARNING, "CSettingsManager: removing empty setting \"%s\"", tmpIterator->first.c_str());
      m_settings.erase(tmpIterator);
    }
    else if (tmpIterator->second.setting->GetType() == SettingType::Reference)
    {
      CLog::Log(LOGWARNING, "CSettingsManager: removing missing reference setting \"%s\"", tmpIterator->first.c_str());
      m_settings.erase(tmpIterator);
    }
  }
}

void CSettingsManager::RegisterSettingOptionsFiller(const std::string &identifier, void *filler, SettingOptionsFillerType type)
{
  CExclusiveLock lock(m_critical);
  auto it = m_optionsFillers.find(identifier);
  if (it != m_optionsFillers.end())
    return;

  SettingOptionsFiller optionsFiller = { filler, type };
  m_optionsFillers.insert(make_pair(identifier, optionsFiller));
}

void CSettingsManager::ResolveSettingDependencies(std::shared_ptr<CSetting> setting)
{
  if (setting == nullptr)
    return;

  ResolveSettingDependencies(FindSetting(setting->GetId())->second);
}

void CSettingsManager::ResolveSettingDependencies(const Setting& setting)
{
  if (setting.setting == nullptr)
    return;

  // if the setting has a parent setting, add it to its children
  auto parentSettingId = setting.setting->GetParent();
  if (!parentSettingId.empty())
  {
    auto itParentSetting = FindSetting(parentSettingId);
    if (itParentSetting != m_settings.end())
      itParentSetting->second.children.insert(setting.setting->GetId());
  }

  // handle all dependencies of the setting
  const auto& dependencies = setting.setting->GetDependencies();
  for (const auto& deps : dependencies)
  {
    const auto settingIds = deps.GetSettings();
    for (const auto& settingId : settingIds)
    {
      auto settingIt = FindSetting(settingId);
      if (settingIt == m_settings.end())
        continue;

      bool newDep = true;
      auto& settingDeps = settingIt->second.dependencies[setting.setting->GetId()];
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
  return m_settings.insert(std::make_pair(settingId, setting));
}
