/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonInstanceHandler.h"

#include "ServiceBroker.h"
#include "addons/AddonVersion.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/binary-addons/BinaryAddonManager.h"
#include "addons/interfaces/AddonBase.h"
#include "addons/kodi-dev-kit/include/kodi/AddonBase.h"
#include "addons/settings/AddonSettings.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/lib/Setting.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <mutex>

namespace ADDON
{

CCriticalSection IAddonInstanceHandler::m_cdSec;

IAddonInstanceHandler::IAddonInstanceHandler(
    ADDON_TYPE type,
    const AddonInfoPtr& addonInfo,
    AddonInstanceId instanceId /* = ADDON_INSTANCE_ID_UNUSED */,
    KODI_HANDLE parentInstance /* = nullptr*/,
    const std::string& uniqueWorkID /* = ""*/)
  : m_type(type), m_instanceId(instanceId), m_parentInstance(parentInstance), m_addonInfo(addonInfo)
{
  // if no special instance ID is given generate one from class pointer (is
  // faster as unique id and also safe enough for them).
  m_uniqueWorkID = !uniqueWorkID.empty() ? uniqueWorkID : StringUtils::Format("{}", fmt::ptr(this));
  m_addonBase = CServiceBroker::GetBinaryAddonManager().GetAddonBase(addonInfo, this, m_addon);

  m_info.number = m_instanceId; // @todo change within next big API change to "instance_id"
  m_info.id = m_uniqueWorkID.c_str(); // @todo change within next big API change to "unique_work_id"
  m_info.version = kodi::addon::GetTypeVersion(m_type);
  m_info.type = m_type;
  m_info.kodi = this;
  m_info.parent = m_parentInstance;
  m_info.first_instance = m_addon && !m_addon->Initialized();
  m_info.functions = &m_callbacks;
  m_info.functions->get_instance_user_path = get_instance_user_path;
  m_info.functions->is_instance_setting_using_default = is_instance_setting_using_default;
  m_info.functions->get_instance_setting_bool = get_instance_setting_bool;
  m_info.functions->get_instance_setting_int = get_instance_setting_int;
  m_info.functions->get_instance_setting_float = get_instance_setting_float;
  m_info.functions->get_instance_setting_string = get_instance_setting_string;
  m_info.functions->set_instance_setting_bool = set_instance_setting_bool;
  m_info.functions->set_instance_setting_int = set_instance_setting_int;
  m_info.functions->set_instance_setting_float = set_instance_setting_float;
  m_info.functions->set_instance_setting_string = set_instance_setting_string;
  m_ifc.info = &m_info;
  m_ifc.functions = &m_functions;
}

IAddonInstanceHandler::~IAddonInstanceHandler()
{
  CServiceBroker::GetBinaryAddonManager().ReleaseAddonBase(m_addonBase, this);
}

std::string IAddonInstanceHandler::ID() const
{
  return m_addon ? m_addon->ID() : "";
}

AddonInstanceId IAddonInstanceHandler::InstanceID() const
{
  return m_instanceId;
}

std::string IAddonInstanceHandler::Name() const
{
  return m_addon ? m_addon->Name() : "";
}

std::string IAddonInstanceHandler::Author() const
{
  return m_addon ? m_addon->Author() : "";
}

std::string IAddonInstanceHandler::Icon() const
{
  return m_addon ? m_addon->Icon() : "";
}

std::string IAddonInstanceHandler::Path() const
{
  return m_addon ? m_addon->Path() : "";
}

std::string IAddonInstanceHandler::Profile() const
{
  return m_addon ? m_addon->Profile() : "";
}

CAddonVersion IAddonInstanceHandler::Version() const
{
  return m_addon ? m_addon->Version() : CAddonVersion();
}

ADDON_STATUS IAddonInstanceHandler::CreateInstance()
{
  if (!m_addon)
    return ADDON_STATUS_UNKNOWN;

  std::unique_lock lock(m_cdSec);

  ADDON_STATUS status = m_addon->CreateInstance(&m_ifc);
  if (status != ADDON_STATUS_OK)
  {
    CLog::LogF(LOGERROR, "{} returned bad status \"{}\" during instance creation", m_addon->ID(),
               kodi::addon::TranslateAddonStatus(status));
  }
  return status;
}

void IAddonInstanceHandler::DestroyInstance()
{
  std::unique_lock lock(m_cdSec);
  if (m_addon)
    m_addon->DestroyInstance(&m_ifc);
}

std::shared_ptr<CSetting> IAddonInstanceHandler::GetSetting(const std::string& setting) const
{
  if (!m_addon->HasSettings(m_instanceId))
  {
    CLog::LogF(LOGERROR, "Couldn't get settings for add-on '{}'", Name());
    return nullptr;
  }

  auto value = m_addon->GetSettings(m_instanceId)->GetSetting(setting);
  if (value == nullptr)
  {
    CLog::LogF(LOGERROR, "Can't find setting '{}' in '{}'", setting, Name());
    return nullptr;
  }

  return value;
}

char* IAddonInstanceHandler::get_instance_user_path(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl)
{
  const auto* instance = static_cast<const IAddonInstanceHandler*>(hdl);
  if (!instance)
    return nullptr;

  const std::string path = CSpecialProtocol::TranslatePath(instance->m_addon->Profile());

  XFILE::CDirectory::Create(path);
  return strdup(path.c_str());
}

bool IAddonInstanceHandler::is_instance_setting_using_default(
    const KODI_ADDON_INSTANCE_BACKEND_HDL hdl, const char* id)
{
  const auto* instance = static_cast<const IAddonInstanceHandler*>(hdl);
  if (!instance || !id)
    return false;

  auto setting = instance->GetSetting(id);
  if (setting == nullptr)
    return false;

  return setting->IsDefault();
}

bool IAddonInstanceHandler::get_instance_setting_bool(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                                      const char* id,
                                                      bool* value)
{
  const auto* instance = static_cast<const IAddonInstanceHandler*>(hdl);
  if (!instance || !id || !value)
    return false;

  auto setting = instance->GetSetting(id);
  if (setting == nullptr)
    return false;

  if (setting->GetType() != SettingType::Boolean)
  {
    CLog::LogF(LOGERROR, "Setting '{}' is not a boolean in '{}'", id, instance->Name());
    return false;
  }

  *value = std::static_pointer_cast<CSettingBool>(setting)->GetValue();
  return true;
}

bool IAddonInstanceHandler::get_instance_setting_int(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                                     const char* id,
                                                     int* value)
{
  const auto* instance = static_cast<const IAddonInstanceHandler*>(hdl);
  if (!instance || !id || !value)
    return false;

  auto setting = instance->GetSetting(id);
  if (setting == nullptr)
    return false;

  if (setting->GetType() != SettingType::Integer && setting->GetType() != SettingType::Number)
  {
    CLog::LogF(LOGERROR, "Setting '{}' is not a integer in '{}'", id, instance->Name());
    return false;
  }

  if (setting->GetType() == SettingType::Integer)
    *value = std::static_pointer_cast<CSettingInt>(setting)->GetValue();
  else
    *value = static_cast<int>(std::static_pointer_cast<CSettingNumber>(setting)->GetValue());
  return true;
}

bool IAddonInstanceHandler::get_instance_setting_float(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                                       const char* id,
                                                       float* value)
{
  const auto* instance = static_cast<const IAddonInstanceHandler*>(hdl);
  if (!instance || !id || !value)
    return false;

  auto setting = instance->GetSetting(id);
  if (setting == nullptr)
    return false;

  if (setting->GetType() != SettingType::Number)
  {
    CLog::LogF(LOGERROR, "Setting '{}' is not a number in '{}'", id, instance->Name());
    return false;
  }

  *value = static_cast<float>(std::static_pointer_cast<CSettingNumber>(setting)->GetValue());
  return true;
}

bool IAddonInstanceHandler::get_instance_setting_string(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                                        const char* id,
                                                        char** value)
{
  const auto* instance = static_cast<const IAddonInstanceHandler*>(hdl);
  if (!instance || !id || !value)
    return false;

  auto setting = instance->GetSetting(id);
  if (setting == nullptr)
    return false;

  if (setting->GetType() != SettingType::String)
  {
    CLog::LogF(LOGERROR, "Setting '{}' is not a string in '{}'", id, instance->Name());
    return false;
  }

  *value = strdup(std::static_pointer_cast<CSettingString>(setting)->GetValue().c_str());
  return true;
}

bool IAddonInstanceHandler::set_instance_setting_bool(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                                      const char* id,
                                                      bool value)
{
  const auto* instance = static_cast<const IAddonInstanceHandler*>(hdl);
  if (!instance || !id)
    return false;

  if (Interface_Base::UpdateSettingInActiveDialog(instance->m_addon.get(), instance->m_instanceId,
                                                  id, value ? "true" : "false"))
    return true;

  if (!instance->m_addon->UpdateSettingBool(id, value, instance->m_instanceId))
  {
    CLog::LogF(LOGERROR, "Invalid setting type");
    return false;
  }

  instance->m_addon->SaveSettings(instance->m_instanceId);

  return true;
}

bool IAddonInstanceHandler::set_instance_setting_int(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                                     const char* id,
                                                     int value)
{
  const auto* instance = static_cast<const IAddonInstanceHandler*>(hdl);
  if (!instance || !id)
  {
    CLog::LogF(LOGERROR, "Invalid data (instance='{}', id='{}')", hdl,
               static_cast<const void*>(id));

    return false;
  }

  if (Interface_Base::UpdateSettingInActiveDialog(instance->m_addon.get(), instance->m_instanceId,
                                                  id, std::to_string(value)))
    return true;

  if (!instance->m_addon->UpdateSettingInt(id, value, instance->m_instanceId))
  {
    CLog::LogF(LOGERROR, "Invalid setting type");
    return false;
  }

  instance->m_addon->SaveSettings(instance->m_instanceId);

  return true;
}

bool IAddonInstanceHandler::set_instance_setting_float(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                                       const char* id,
                                                       float value)
{
  const auto* instance = static_cast<const IAddonInstanceHandler*>(hdl);
  if (!instance || !id)
  {
    CLog::LogF(LOGERROR, "Invalid data (instance='{}', id='{}')", hdl,
               static_cast<const void*>(id));

    return false;
  }

  if (Interface_Base::UpdateSettingInActiveDialog(instance->m_addon.get(), instance->m_instanceId,
                                                  id, StringUtils::Format("{:f}", value)))
    return true;

  if (!instance->m_addon->UpdateSettingNumber(id, static_cast<double>(value),
                                              instance->m_instanceId))
  {
    CLog::LogF(LOGERROR, "Invalid setting type");
    return false;
  }

  instance->m_addon->SaveSettings(instance->m_instanceId);

  return true;
}

bool IAddonInstanceHandler::set_instance_setting_string(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                                        const char* id,
                                                        const char* value)
{
  const auto* instance = static_cast<const IAddonInstanceHandler*>(hdl);
  if (!instance || !id || !value)
  {
    CLog::LogF(LOGERROR, "Invalid data (instance='{}', id='{}', value='{}')", hdl,
               static_cast<const void*>(id), static_cast<const void*>(value));

    return false;
  }

  if (Interface_Base::UpdateSettingInActiveDialog(instance->m_addon.get(), instance->m_instanceId,
                                                  id, value))
    return true;

  if (!instance->m_addon->UpdateSettingString(id, value, instance->m_instanceId))
  {
    CLog::LogF(LOGERROR, "Invalid setting type");
    return false;
  }

  instance->m_addon->SaveSettings(instance->m_instanceId);

  return true;
}

} /* namespace ADDON */
