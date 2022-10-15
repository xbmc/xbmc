/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SkinSettings.h"

#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "addons/Skin.h"
#include "guilib/GUIComponent.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include <memory>
#include <mutex>
#include <string>

#define XML_SKINSETTINGS  "skinsettings"

CSkinSettings::CSkinSettings()
{
  Clear();
}

CSkinSettings::~CSkinSettings() = default;

CSkinSettings& CSkinSettings::GetInstance()
{
  static CSkinSettings sSkinSettings;
  return sSkinSettings;
}

int CSkinSettings::TranslateString(const std::string &setting)
{
  return g_SkinInfo->TranslateString(setting);
}

const std::string& CSkinSettings::GetString(int setting) const
{
  return g_SkinInfo->GetString(setting);
}

void CSkinSettings::SetString(int setting, const std::string &label)
{
  g_SkinInfo->SetString(setting, label);
}

int CSkinSettings::TranslateBool(const std::string &setting)
{
  return g_SkinInfo->TranslateBool(setting);
}

bool CSkinSettings::GetBool(int setting) const
{
  return g_SkinInfo->GetBool(setting);
}

int CSkinSettings::GetInt(int setting) const
{
  return g_SkinInfo->GetInt(setting);
}

void CSkinSettings::SetBool(int setting, bool set)
{
  g_SkinInfo->SetBool(setting, set);
}

void CSkinSettings::Reset(const std::string &setting)
{
  g_SkinInfo->Reset(setting);
}

std::set<ADDON::CSkinSettingPtr> CSkinSettings::GetSettings() const
{
  return g_SkinInfo->GetSkinSettings();
}

ADDON::CSkinSettingPtr CSkinSettings::GetSetting(const std::string& settingId)
{
  return g_SkinInfo->GetSkinSetting(settingId);
}

std::shared_ptr<const ADDON::CSkinSetting> CSkinSettings::GetSetting(
    const std::string& settingId) const
{
  return g_SkinInfo->GetSkinSetting(settingId);
}

void CSkinSettings::Reset()
{
  g_SkinInfo->Reset();

  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();
  infoMgr.ResetCache();
  infoMgr.GetInfoProviders().GetGUIControlsInfoProvider().ResetContainerMovingCache();
}

bool CSkinSettings::Load(const TiXmlNode *settings)
{
  if (settings == nullptr)
    return false;

  const TiXmlElement *rootElement = settings->FirstChildElement(XML_SKINSETTINGS);

  // return true in the case skinsettings is missing. It just means that
  // it's been migrated and it's not an error
  if (rootElement == nullptr)
  {
    CLog::Log(LOGDEBUG, "CSkinSettings: no <skinsettings> tag found");
    return true;
  }

  std::unique_lock<CCriticalSection> lock(m_critical);
  m_settings.clear();
  m_settings = ADDON::CSkinInfo::ParseSettings(rootElement);

  return true;
}

bool CSkinSettings::Save(TiXmlNode *settings) const
{
  if (settings == nullptr)
    return false;

  // nothing to do here because skin settings saving has been migrated to CSkinInfo

  return true;
}

void CSkinSettings::Clear()
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  m_settings.clear();
}

void CSkinSettings::MigrateSettings(const std::shared_ptr<ADDON::CSkinInfo>& skin)
{
  if (skin == nullptr)
    return;

  std::unique_lock<CCriticalSection> lock(m_critical);

  bool settingsMigrated = false;
  const std::string& skinId = skin->ID();
  std::set<ADDON::CSkinSettingPtr> settingsCopy(m_settings.begin(), m_settings.end());
  for (const auto& setting : settingsCopy)
  {
    if (!StringUtils::StartsWith(setting->name, skinId + "."))
      continue;

    std::string settingName = setting->name.substr(skinId.size() + 1);

    if (setting->GetType() == "string")
    {
      int settingNumber = skin->TranslateString(settingName);
      if (settingNumber >= 0)
        skin->SetString(settingNumber, std::dynamic_pointer_cast<ADDON::CSkinSettingString>(setting)->value);
    }
    else if (setting->GetType() == "bool")
    {
      int settingNumber = skin->TranslateBool(settingName);
      if (settingNumber >= 0)
        skin->SetBool(settingNumber, std::dynamic_pointer_cast<ADDON::CSkinSettingBool>(setting)->value);
    }

    m_settings.erase(setting);
    settingsMigrated = true;
  }

  if (settingsMigrated)
  {
    // save the skin's settings
    skin->SaveSettings();

    // save the guisettings.xml
    CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
  }
}

