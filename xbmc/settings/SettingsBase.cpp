/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingsBase.h"

#include "settings/SettingUtils.h"
#include "settings/SettingsValueXmlSerializer.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML.h"

#include <mutex>

#define SETTINGS_XML_ROOT   "settings"

CSettingsBase::CSettingsBase()
  : m_settingsManager(new CSettingsManager())
{ }

CSettingsBase::~CSettingsBase()
{
  Uninitialize();

  delete m_settingsManager;
}

bool CSettingsBase::Initialize()
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  if (m_initialized)
    return false;

  // register custom setting types
  InitializeSettingTypes();
  // register custom setting controls
  InitializeControls();

  // option fillers and conditions need to be
  // initialized before the setting definitions
  InitializeOptionFillers();
  InitializeConditions();

  // load the settings definitions
  if (!InitializeDefinitions())
    return false;

  InitializeVisibility();
  InitializeDefaults();

  m_settingsManager->SetInitialized();

  InitializeISettingsHandlers();
  InitializeISubSettings();
  InitializeISettingCallbacks();

  m_initialized = true;

  return true;
}

bool CSettingsBase::IsInitialized() const
{
  return m_initialized && m_settingsManager->IsInitialized();
}

bool CSettingsBase::LoadValuesFromXml(const CXBMCTinyXML& xml, bool& updated)
{
  const TiXmlElement* xmlRoot = xml.RootElement();
  if (xmlRoot == nullptr || xmlRoot->ValueStr() != SETTINGS_XML_ROOT)
    return false;

  return m_settingsManager->Load(xmlRoot, updated);
}

bool CSettingsBase::LoadValuesFromXml(const TiXmlElement* root, bool& updated)
{
  if (root == nullptr)
    return false;

  return m_settingsManager->Load(root, updated);
}

bool CSettingsBase::LoadHiddenValuesFromXml(const TiXmlElement* root)
{
  if (root == nullptr)
    return false;

  std::map<std::string, std::shared_ptr<CSetting>> loadedSettings;

  bool updated;
  // don't trigger events for hidden settings
  bool success = m_settingsManager->Load(root, updated, false, &loadedSettings);
  if (success)
  {
    for(std::map<std::string, std::shared_ptr<CSetting>>::const_iterator setting = loadedSettings.begin(); setting != loadedSettings.end(); ++setting)
      setting->second->SetVisible(false);
  }

  return success;
}

void CSettingsBase::SetLoaded()
{
  m_settingsManager->SetLoaded();
}

bool CSettingsBase::IsLoaded() const
{
  return m_settingsManager->IsLoaded();
}

bool CSettingsBase::SaveValuesToXml(CXBMCTinyXML& xml) const
{
  std::string serializedSettings;
  auto xmlSerializer = std::make_unique<CSettingsValueXmlSerializer>();
  if (!m_settingsManager->Save(xmlSerializer.get(), serializedSettings))
    return false;

  if (!xml.Parse(serializedSettings))
    return false;

  return true;
}

void CSettingsBase::Unload()
{
  m_settingsManager->Unload();
}

void CSettingsBase::Uninitialize()
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  if (!m_initialized)
    return;

  // unregister setting option fillers
  UninitializeOptionFillers();

  // unregister setting conditions
  UninitializeConditions();

  // unregister ISettingCallback implementations
  UninitializeISettingCallbacks();

  // cleanup the settings manager
  m_settingsManager->Clear();

  // unregister ISubSettings implementations
  UninitializeISubSettings();
  // unregister ISettingsHandler implementations
  UninitializeISettingsHandlers();

  m_initialized = false;
}

void CSettingsBase::RegisterCallback(ISettingCallback* callback,
                                     const SettingsContainer& settingList)
{
  m_settingsManager->RegisterCallback(callback, settingList);
}

void CSettingsBase::UnregisterCallback(ISettingCallback* callback)
{
  m_settingsManager->UnregisterCallback(callback);
}

SettingPtr CSettingsBase::GetSetting(const std::string& id) const
{
  if (id.empty())
    return nullptr;

  return m_settingsManager->GetSetting(id);
}

std::vector<std::shared_ptr<CSettingSection>> CSettingsBase::GetSections() const
{
  return m_settingsManager->GetSections();
}

std::shared_ptr<CSettingSection> CSettingsBase::GetSection(const std::string& section) const
{
  if (section.empty())
    return nullptr;

  return m_settingsManager->GetSection(section);
}

bool CSettingsBase::GetBool(const std::string& id) const
{
  return m_settingsManager->GetBool(id);
}

bool CSettingsBase::SetBool(const std::string& id, bool value)
{
  return m_settingsManager->SetBool(id, value);
}

bool CSettingsBase::ToggleBool(const std::string& id)
{
  return m_settingsManager->ToggleBool(id);
}

int CSettingsBase::GetInt(const std::string& id) const
{
  return m_settingsManager->GetInt(id);
}

bool CSettingsBase::SetInt(const std::string& id, int value)
{
  return m_settingsManager->SetInt(id, value);
}

double CSettingsBase::GetNumber(const std::string& id) const
{
  return m_settingsManager->GetNumber(id);
}

bool CSettingsBase::SetNumber(const std::string& id, double value)
{
  return m_settingsManager->SetNumber(id, value);
}

std::string CSettingsBase::GetString(const std::string& id) const
{
  return m_settingsManager->GetString(id);
}

bool CSettingsBase::SetString(const std::string& id, const std::string& value)
{
  return m_settingsManager->SetString(id, value);
}

std::vector<CVariant> CSettingsBase::GetList(const std::string& id) const
{
  std::shared_ptr<CSetting> setting = m_settingsManager->GetSetting(id);
  if (setting == nullptr || setting->GetType() != SettingType::List)
    return std::vector<CVariant>();

  return CSettingUtils::GetList(std::static_pointer_cast<CSettingList>(setting));
}

bool CSettingsBase::SetList(const std::string& id, const std::vector<CVariant>& value)
{
  std::shared_ptr<CSetting> setting = m_settingsManager->GetSetting(id);
  if (setting == nullptr || setting->GetType() != SettingType::List)
    return false;

  return CSettingUtils::SetList(std::static_pointer_cast<CSettingList>(setting), value);
}

bool CSettingsBase::SetDefault(const std::string &id)
{
  return m_settingsManager->SetDefault(id);
}

void CSettingsBase::SetDefaults()
{
  m_settingsManager->SetDefaults();
}

bool CSettingsBase::InitializeDefinitionsFromXml(const CXBMCTinyXML& xml)
{
  const TiXmlElement* root = xml.RootElement();
  if (root == nullptr)
    return false;

  return m_settingsManager->Initialize(root);
}
