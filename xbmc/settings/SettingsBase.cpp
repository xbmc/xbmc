/*
 *      Copyright (C) 2016 Team XBMC
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

#include "SettingsBase.h"
#include "settings/SettingUtils.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML.h"

#define SETTINGS_XML_ROOT   "settings"

CSettingsBase::CSettingsBase()
  : m_initialized(false)
  , m_settingsManager(new CSettingsManager())
{ }

CSettingsBase::~CSettingsBase()
{
  Uninitialize();

  delete m_settingsManager;
}

bool CSettingsBase::Initialize()
{
  CSingleLock lock(m_critical);
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

bool CSettingsBase::LoadValuesFromXml(const CXBMCTinyXML& xml)
{
  const TiXmlElement* xmlRoot = xml.RootElement();
  if (xmlRoot == nullptr || xmlRoot->ValueStr() != SETTINGS_XML_ROOT)
    return false;

  bool updated = false;
  return m_settingsManager->Load(xmlRoot, updated);
}

bool CSettingsBase::LoadValuesFromXml(const TiXmlElement* root, bool hide /* = false */)
{
  if (root == nullptr)
    return false;

  std::map<std::string, CSetting*>* loadedSettings = nullptr;
  if (hide)
    loadedSettings = new std::map<std::string, CSetting*>();

  bool updated;
  // only trigger settings events if hiding is disabled
  bool success = m_settingsManager->Load(root, updated, !hide, loadedSettings);
  // if necessary hide all the loaded settings
  if (success && hide && loadedSettings != nullptr)
  {
    for(std::map<std::string, CSetting*>::const_iterator setting = loadedSettings->begin(); setting != loadedSettings->end(); ++setting)
      setting->second->SetVisible(false);
  }
  delete loadedSettings;

  return success;
}

void CSettingsBase::SetLoaded()
{
  m_settingsManager->SetLoaded();
}

bool CSettingsBase::IsLoaded() const
{
  CSingleLock lock(m_critical);
  return m_settingsManager->IsLoaded();
}

bool CSettingsBase::SaveValuesToXml(CXBMCTinyXML& xml) const
{
  TiXmlElement rootElement(SETTINGS_XML_ROOT);
  TiXmlNode* xmlRoot = xml.InsertEndChild(rootElement);
  if (xmlRoot == nullptr)
    return false;

  return m_settingsManager->Save(xmlRoot);
}

void CSettingsBase::Unload()
{
  CSingleLock lock(m_critical);
  m_settingsManager->Unload();
}

void CSettingsBase::Uninitialize()
{
  CSingleLock lock(m_critical);
  if (!m_initialized)
    return;

  // unregister setting option fillers
  UninitializeOptionFillers();
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

void CSettingsBase::RegisterCallback(ISettingCallback* callback, const std::set<std::string>& settingList)
{
  CSingleLock lock(m_critical);
  m_settingsManager->RegisterCallback(callback, settingList);
}

void CSettingsBase::UnregisterCallback(ISettingCallback* callback)
{
  CSingleLock lock(m_critical);
  m_settingsManager->UnregisterCallback(callback);
}

CSetting* CSettingsBase::GetSetting(const std::string& id) const
{
  CSingleLock lock(m_critical);
  if (id.empty())
    return nullptr;

  return m_settingsManager->GetSetting(id);
}

std::vector<CSettingSection*> CSettingsBase::GetSections() const
{
  CSingleLock lock(m_critical);
  return m_settingsManager->GetSections();
}

CSettingSection* CSettingsBase::GetSection(const std::string& section) const
{
  CSingleLock lock(m_critical);
  if (section.empty())
    return nullptr;

  return m_settingsManager->GetSection(section);
}

bool CSettingsBase::GetBool(const std::string& id) const
{
  CSingleLock lock(m_critical);
  return m_settingsManager->GetBool(id);
}

bool CSettingsBase::SetBool(const std::string& id, bool value)
{
  CSingleLock lock(m_critical);
  return m_settingsManager->SetBool(id, value);
}

bool CSettingsBase::ToggleBool(const std::string& id)
{
  CSingleLock lock(m_critical);
  return m_settingsManager->ToggleBool(id);
}

int CSettingsBase::GetInt(const std::string& id) const
{
  CSingleLock lock(m_critical);
  return m_settingsManager->GetInt(id);
}

bool CSettingsBase::SetInt(const std::string& id, int value)
{
  CSingleLock lock(m_critical);
  return m_settingsManager->SetInt(id, value);
}

double CSettingsBase::GetNumber(const std::string& id) const
{
  CSingleLock lock(m_critical);
  return m_settingsManager->GetNumber(id);
}

bool CSettingsBase::SetNumber(const std::string& id, double value)
{
  CSingleLock lock(m_critical);
  return m_settingsManager->SetNumber(id, value);
}

std::string CSettingsBase::GetString(const std::string& id) const
{
  CSingleLock lock(m_critical);
  return m_settingsManager->GetString(id);
}

bool CSettingsBase::SetString(const std::string& id, const std::string& value)
{
  CSingleLock lock(m_critical);
  return m_settingsManager->SetString(id, value);
}

std::vector<CVariant> CSettingsBase::GetList(const std::string& id) const
{
  CSingleLock lock(m_critical);
  CSetting* setting = m_settingsManager->GetSetting(id);
  if (setting == nullptr || setting->GetType() != SettingTypeList)
    return std::vector<CVariant>();

  return CSettingUtils::GetList(static_cast<CSettingList*>(setting));
}

bool CSettingsBase::SetList(const std::string& id, const std::vector<CVariant>& value)
{
  CSingleLock lock(m_critical);
  CSetting* setting = m_settingsManager->GetSetting(id);
  if (setting == nullptr || setting->GetType() != SettingTypeList)
    return false;

  return CSettingUtils::SetList(static_cast<CSettingList*>(setting), value);
}

bool CSettingsBase::SetDefault(const std::string &id)
{
  CSingleLock lock(m_critical);
  return m_settingsManager->SetDefault(id);
}

void CSettingsBase::SetDefaults()
{
  CSingleLock lock(m_critical);
  m_settingsManager->SetDefaults();
}

bool CSettingsBase::InitializeDefinitionsFromXml(const CXBMCTinyXML& xml)
{
  const TiXmlElement* root = xml.RootElement();
  if (root == nullptr)
    return false;

  return m_settingsManager->Initialize(root);
}
