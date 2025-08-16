/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonBase.h"

#include "GUIUserMessages.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/gui/GUIDialogAddonSettings.h"
#include "addons/settings/AddonSettings.h"
#include "application/Application.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "settings/lib/Setting.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

// "C" interface addon callback handle classes
#include "AudioEngine.h"
#include "Filesystem.h"
#include "General.h"
#include "Network.h"
#include "gui/General.h"

namespace ADDON
{

std::vector<AddonGetInterface> Interface_Base::s_registeredInterfaces;

bool Interface_Base::InitInterface(CAddonDll* addon,
                                   AddonGlobalInterface& addonInterface,
                                   KODI_ADDON_INSTANCE_STRUCT* firstKodiInstance)
{
  addonInterface = {};

  addonInterface.addonBase = nullptr;
  addonInterface.globalSingleInstance = nullptr;
  addonInterface.firstKodiInstance = firstKodiInstance;

  // Create function list from kodi to addon, generated with malloc to have
  // compatible with other versions
  addonInterface.toKodi = new AddonToKodiFuncTable_Addon();
  addonInterface.toKodi->kodiBase = addon;
  addonInterface.toKodi->addon_log_msg = addon_log_msg;
  addonInterface.toKodi->free_string = free_string;
  addonInterface.toKodi->free_string_array = free_string_array;

  addonInterface.toKodi->kodi_addon = new AddonToKodiFuncTable_kodi_addon();
  addonInterface.toKodi->kodi_addon->get_addon_path = get_addon_path;
  addonInterface.toKodi->kodi_addon->get_lib_path = get_lib_path;
  addonInterface.toKodi->kodi_addon->get_user_path = get_user_path;
  addonInterface.toKodi->kodi_addon->get_temp_path = get_temp_path;
  addonInterface.toKodi->kodi_addon->get_localized_string = get_localized_string;
  addonInterface.toKodi->kodi_addon->open_settings_dialog = open_settings_dialog;
  addonInterface.toKodi->kodi_addon->is_setting_using_default = is_setting_using_default;
  addonInterface.toKodi->kodi_addon->get_setting_bool = get_setting_bool;
  addonInterface.toKodi->kodi_addon->get_setting_int = get_setting_int;
  addonInterface.toKodi->kodi_addon->get_setting_float = get_setting_float;
  addonInterface.toKodi->kodi_addon->get_setting_string = get_setting_string;
  addonInterface.toKodi->kodi_addon->set_setting_bool = set_setting_bool;
  addonInterface.toKodi->kodi_addon->set_setting_int = set_setting_int;
  addonInterface.toKodi->kodi_addon->set_setting_float = set_setting_float;
  addonInterface.toKodi->kodi_addon->set_setting_string = set_setting_string;
  addonInterface.toKodi->kodi_addon->get_addon_info = get_addon_info;
  addonInterface.toKodi->kodi_addon->get_type_version = get_type_version;
  addonInterface.toKodi->kodi_addon->get_interface = get_interface;

  // Related parts becomes set from addon headers, make here to nullptr to allow
  // checks for right set of them
  addonInterface.toAddon = new KodiToAddonFuncTable_Addon();

  // Init the other interfaces
  Interface_General::Init(&addonInterface);
  Interface_AudioEngine::Init(&addonInterface);
  Interface_Filesystem::Init(&addonInterface);
  Interface_Network::Init(&addonInterface);
  Interface_GUIGeneral::Init(&addonInterface);

  return true;
}

void Interface_Base::DeInitInterface(AddonGlobalInterface& addonInterface)
{
  Interface_GUIGeneral::DeInit(&addonInterface);
  Interface_Network::DeInit(&addonInterface);
  Interface_Filesystem::DeInit(&addonInterface);
  Interface_AudioEngine::DeInit(&addonInterface);
  Interface_General::DeInit(&addonInterface);

  if (addonInterface.toKodi)
    delete addonInterface.toKodi->kodi_addon;
  delete addonInterface.toKodi;
  delete addonInterface.toAddon;
  addonInterface = {};
}

void Interface_Base::RegisterInterface(const AddonGetInterface& fn)
{
  s_registeredInterfaces.emplace_back(fn);
}

bool Interface_Base::UpdateSettingInActiveDialog(const CAddonDll* addon,
                                                 AddonInstanceId instanceId,
                                                 const char* id,
                                                 const std::string& value)
{
  if (!CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_DIALOG_ADDON_SETTINGS))
    return false;

  const auto* dialog{
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogAddonSettings>(
          WINDOW_DIALOG_ADDON_SETTINGS)};
  if (dialog->GetCurrentAddonID() != addon->ID())
    return false;

  CGUIMessage message(GUI_MSG_SETTING_UPDATED, 0, 0);
  std::vector<std::string> params;
  params.emplace_back(id);
  params.push_back(value);
  message.SetStringParams(params);
  message.SetParam1(instanceId);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(message,
                                                                 WINDOW_DIALOG_ADDON_SETTINGS);

  return true;
}

/*!
 * @brief Addon to Kodi basic callbacks below
 *
 * The amount of functions here are hold so minimal as possible. Only parts
 * where needed on nearly every add-on (e.g. addon_log_msg) are to add there.
 *
 * More specific parts like e.g. to open files should be added to a separate
 * part.
 */
//@{

void Interface_Base::addon_log_msg(const KODI_ADDON_BACKEND_HDL hdl,
                                   const int addonLogLevel,
                                   const char* strMessage)
{
  const auto* addon = static_cast<const CAddonDll*>(hdl);
  if (!addon)
  {
    CLog::Log(LOGERROR, "addon_log_msg(...) called with empty kodi instance pointer");
    return;
  }

  int logLevel = LOGNONE;
  switch (addonLogLevel)
  {
    case ADDON_LOG_DEBUG:
      logLevel = LOGDEBUG;
      break;
    case ADDON_LOG_INFO:
      logLevel = LOGINFO;
      break;
    case ADDON_LOG_WARNING:
      logLevel = LOGWARNING;
      break;
    case ADDON_LOG_ERROR:
      logLevel = LOGERROR;
      break;
    case ADDON_LOG_FATAL:
      logLevel = LOGFATAL;
      break;
    default:
      logLevel = LOGDEBUG;
      break;
  }

  CLog::Log(addon->ID(), logLevel, "{}", strMessage);
}

char* Interface_Base::get_type_version(const KODI_ADDON_BACKEND_HDL /*hdl*/, int type)
{
  return strdup(kodi::addon::GetTypeVersion(type));
}

char* Interface_Base::get_addon_path(const KODI_ADDON_BACKEND_HDL hdl)
{
  const auto* addon = static_cast<const CAddonDll*>(hdl);
  if (!addon)
  {
    CLog::Log(LOGERROR, "get_addon_path(...) called with empty kodi instance pointer");
    return nullptr;
  }

  return strdup(CSpecialProtocol::TranslatePath(addon->Path()).c_str());
}

char* Interface_Base::get_lib_path(const KODI_ADDON_BACKEND_HDL hdl)
{
  const auto* addon = static_cast<const CAddonDll*>(hdl);
  if (!addon)
  {
    CLog::Log(LOGERROR, "get_lib_path(...) called with empty kodi instance pointer");
    return nullptr;
  }

  return strdup(CSpecialProtocol::TranslatePath("special://xbmcbinaddons/" + addon->ID()).c_str());
}

char* Interface_Base::get_user_path(const KODI_ADDON_BACKEND_HDL hdl)
{
  const auto* addon = static_cast<const CAddonDll*>(hdl);
  if (!addon)
  {
    CLog::Log(LOGERROR, "get_user_path(...) called with empty kodi instance pointer");
    return nullptr;
  }

  return strdup(CSpecialProtocol::TranslatePath(addon->Profile()).c_str());
}

char* Interface_Base::get_temp_path(const KODI_ADDON_BACKEND_HDL hdl)
{
  const auto* addon = static_cast<const CAddonDll*>(hdl);
  if (!addon)
  {
    CLog::Log(LOGERROR, "get_temp_path(...) called with empty kodi instance pointer");
    return nullptr;
  }

  std::string tempPath =
      URIUtils::AddFileToFolder(CServiceBroker::GetAddonMgr().GetTempAddonBasePath(), addon->ID());
  tempPath += "-temp";
  XFILE::CDirectory::Create(tempPath);

  return strdup(CSpecialProtocol::TranslatePath(tempPath).c_str());
}

char* Interface_Base::get_localized_string(const KODI_ADDON_BACKEND_HDL hdl, long label_id)
{
  const auto* addon = static_cast<const CAddonDll*>(hdl);
  if (!addon)
  {
    CLog::Log(LOGERROR, "get_localized_string(...) called with empty kodi instance pointer");
    return nullptr;
  }

  if (g_application.m_bStop)
    return nullptr;

  std::string label =
      g_localizeStrings.GetAddonString(addon->ID(), static_cast<uint32_t>(label_id));
  if (label.empty())
    label = g_localizeStrings.Get(static_cast<uint32_t>(label_id));
  char* buffer = strdup(label.c_str());
  return buffer;
}

char* Interface_Base::get_addon_info(const KODI_ADDON_BACKEND_HDL hdl, const char* id)
{
  const auto* addon = static_cast<const CAddonDll*>(hdl);
  if (!addon || !id)
  {
    CLog::Log(LOGERROR, "get_addon_info(...) called with empty pointer");
    return nullptr;
  }

  std::string str;
  if (StringUtils::CompareNoCase(id, "author") == 0)
    str = addon->Author();
  else if (StringUtils::CompareNoCase(id, "changelog") == 0)
    str = addon->ChangeLog();
  else if (StringUtils::CompareNoCase(id, "description") == 0)
    str = addon->Description();
  else if (StringUtils::CompareNoCase(id, "disclaimer") == 0)
    str = addon->Disclaimer();
  else if (StringUtils::CompareNoCase(id, "fanart") == 0)
    str = addon->FanArt();
  else if (StringUtils::CompareNoCase(id, "icon") == 0)
    str = addon->Icon();
  else if (StringUtils::CompareNoCase(id, "id") == 0)
    str = addon->ID();
  else if (StringUtils::CompareNoCase(id, "name") == 0)
    str = addon->Name();
  else if (StringUtils::CompareNoCase(id, "path") == 0)
    str = addon->Path();
  else if (StringUtils::CompareNoCase(id, "profile") == 0)
    str = addon->Profile();
  else if (StringUtils::CompareNoCase(id, "summary") == 0)
    str = addon->Summary();
  else if (StringUtils::CompareNoCase(id, "type") == 0)
    str = ADDON::CAddonInfo::TranslateType(addon->Type());
  else if (StringUtils::CompareNoCase(id, "version") == 0)
    str = addon->Version().asString();
  else
  {
    CLog::LogF(LOGERROR, "Add-on '{}' requests invalid id '{}'", addon->Name(), id);
    return nullptr;
  }

  char* buffer = strdup(str.c_str());
  return buffer;
}

bool Interface_Base::open_settings_dialog(const KODI_ADDON_BACKEND_HDL hdl)
{
  const auto* addon = static_cast<const CAddonDll*>(hdl);
  if (!addon)
  {
    CLog::Log(LOGERROR, "open_settings_dialog(...) called with empty kodi instance pointer");
    return false;
  }

  // show settings dialog
  AddonPtr addonInfo;
  if (!CServiceBroker::GetAddonMgr().GetAddon(addon->ID(), addonInfo, OnlyEnabled::CHOICE_YES))
  {
    CLog::LogF(LOGERROR, "Could not get addon information for '{}'", addon->ID());
    return false;
  }

  return CGUIDialogAddonSettings::ShowForAddon(addonInfo);
}

bool Interface_Base::is_setting_using_default(const KODI_ADDON_BACKEND_HDL hdl, const char* id)
{
  auto* addon = static_cast<CAddonDll*>(hdl);
  if (!addon || !id)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', id='{}')", hdl, static_cast<const void*>(id));

    return false;
  }

  if (!addon->HasSettings())
  {
    CLog::LogF(LOGERROR, "Couldn't get settings for add-on '{}'", addon->Name());
    return false;
  }

  auto setting = addon->GetSettings()->GetSetting(id);
  if (setting == nullptr)
  {
    CLog::LogF(LOGERROR, "Can't find setting '{}' in '{}'", id, addon->Name());
    return false;
  }

  return setting->IsDefault();
}

bool Interface_Base::get_setting_bool(const KODI_ADDON_BACKEND_HDL hdl, const char* id, bool* value)
{
  auto* addon = static_cast<CAddonDll*>(hdl);
  if (!addon || !id || !value)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', id='{}', value='{}')", hdl,
               static_cast<const void*>(id), static_cast<void*>(value));

    return false;
  }

  if (!addon->HasSettings())
  {
    CLog::LogF(LOGERROR, "Couldn't get settings for add-on '{}'", addon->Name());
    return false;
  }

  auto setting = addon->GetSettings()->GetSetting(id);
  if (setting == nullptr)
  {
    CLog::LogF(LOGERROR, "Can't find setting '{}' in '{}'", id, addon->Name());
    return false;
  }

  if (setting->GetType() != SettingType::Boolean)
  {
    CLog::LogF(LOGERROR, "Setting '{}' is not a boolean in '{}'", id, addon->Name());
    return false;
  }

  *value = std::static_pointer_cast<CSettingBool>(setting)->GetValue();
  return true;
}

bool Interface_Base::get_setting_int(const KODI_ADDON_BACKEND_HDL hdl, const char* id, int* value)
{
  auto* addon = static_cast<CAddonDll*>(hdl);
  if (!addon || !id || !value)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', id='{}', value='{}')", hdl,
               static_cast<const void*>(id), static_cast<void*>(value));

    return false;
  }

  if (!addon->HasSettings())
  {
    CLog::LogF(LOGERROR, "Couldn't get settings for add-on '{}'", addon->Name());
    return false;
  }

  auto setting = addon->GetSettings()->GetSetting(id);
  if (setting == nullptr)
  {
    CLog::LogF(LOGERROR, "Can't find setting '{}' in '{}'", id, addon->Name());
    return false;
  }

  if (setting->GetType() != SettingType::Integer && setting->GetType() != SettingType::Number)
  {
    CLog::LogF(LOGERROR, "ISetting '{}' is not a integer in '{}'", id, addon->Name());
    return false;
  }

  if (setting->GetType() == SettingType::Integer)
    *value = std::static_pointer_cast<CSettingInt>(setting)->GetValue();
  else
    *value = static_cast<int>(std::static_pointer_cast<CSettingNumber>(setting)->GetValue());
  return true;
}

bool Interface_Base::get_setting_float(const KODI_ADDON_BACKEND_HDL hdl,
                                       const char* id,
                                       float* value)
{
  auto* addon = static_cast<CAddonDll*>(hdl);
  if (!addon || !id || !value)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', id='{}', value='{}')", hdl,
               static_cast<const void*>(id), static_cast<void*>(value));

    return false;
  }

  if (!addon->HasSettings())
  {
    CLog::LogF(LOGERROR, "Couldn't get settings for add-on '{}'", addon->Name());
    return false;
  }

  auto setting = addon->GetSettings()->GetSetting(id);
  if (setting == nullptr)
  {
    CLog::LogF(LOGERROR, "Can't find setting '{}' in '{}'", id, addon->Name());
    return false;
  }

  if (setting->GetType() != SettingType::Number)
  {
    CLog::LogF(LOGERROR, "Setting '{}' is not a number in '{}'", id, addon->Name());
    return false;
  }

  *value = static_cast<float>(std::static_pointer_cast<CSettingNumber>(setting)->GetValue());
  return true;
}

bool Interface_Base::get_setting_string(const KODI_ADDON_BACKEND_HDL hdl,
                                        const char* id,
                                        char** value)
{
  auto* addon = static_cast<CAddonDll*>(hdl);
  if (!addon || !id || !value)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', id='{}', value='{}')", hdl,
               static_cast<const void*>(id), static_cast<void*>(value));

    return false;
  }

  if (!addon->HasSettings())
  {
    CLog::LogF(LOGERROR, "Couldn't get settings for add-on '{}'", addon->Name());
    return false;
  }

  auto setting = addon->GetSettings()->GetSetting(id);
  if (setting == nullptr)
  {
    CLog::LogF(LOGERROR, "Can't find setting '{}' in '{}'", id, addon->Name());
    return false;
  }

  if (setting->GetType() != SettingType::String)
  {
    CLog::LogF(LOGERROR, "Setting '{}' is not a string in '{}'", id, addon->Name());
    return false;
  }

  *value = strdup(std::static_pointer_cast<CSettingString>(setting)->GetValue().c_str());
  return true;
}

bool Interface_Base::set_setting_bool(const KODI_ADDON_BACKEND_HDL hdl, const char* id, bool value)
{
  auto* addon = static_cast<CAddonDll*>(hdl);
  if (!addon || !id || !value)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', id='{}')", hdl, static_cast<const void*>(id));

    return false;
  }

  if (Interface_Base::UpdateSettingInActiveDialog(addon, ADDON_SETTINGS_ID, id,
                                                  value ? "true" : "false"))
    return true;

  if (!addon->UpdateSettingBool(id, value))
  {
    CLog::LogF(LOGERROR, "Invalid setting type");
    return false;
  }

  addon->SaveSettings();

  return true;
}

bool Interface_Base::set_setting_int(const KODI_ADDON_BACKEND_HDL hdl, const char* id, int value)
{
  auto* addon = static_cast<CAddonDll*>(hdl);
  if (!addon || !id)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', id='{}')", hdl, static_cast<const void*>(id));

    return false;
  }

  if (Interface_Base::UpdateSettingInActiveDialog(addon, ADDON_SETTINGS_ID, id,
                                                  std::to_string(value)))
    return true;

  if (!addon->UpdateSettingInt(id, value))
  {
    CLog::LogF(LOGERROR, "Invalid setting type");
    return false;
  }

  addon->SaveSettings();

  return true;
}

bool Interface_Base::set_setting_float(const KODI_ADDON_BACKEND_HDL hdl,
                                       const char* id,
                                       float value)
{
  auto* addon = static_cast<CAddonDll*>(hdl);
  if (!addon || !id)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', id='{}')", hdl, static_cast<const void*>(id));

    return false;
  }

  if (Interface_Base::UpdateSettingInActiveDialog(addon, ADDON_SETTINGS_ID, id,
                                                  StringUtils::Format("{:f}", value)))
    return true;

  if (!addon->UpdateSettingNumber(id, static_cast<double>(value)))
  {
    CLog::LogF(LOGERROR, "Invalid setting type");
    return false;
  }

  addon->SaveSettings();

  return true;
}

bool Interface_Base::set_setting_string(const KODI_ADDON_BACKEND_HDL hdl,
                                        const char* id,
                                        const char* value)
{
  auto* addon = static_cast<CAddonDll*>(hdl);
  if (!addon || !id || !value)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', id='{}', value='{}')", hdl,
               static_cast<const void*>(id), static_cast<const void*>(value));

    return false;
  }

  if (Interface_Base::UpdateSettingInActiveDialog(addon, ADDON_SETTINGS_ID, id, value))
    return true;

  if (!addon->UpdateSettingString(id, value))
  {
    CLog::LogF(LOGERROR, "Invalid setting type");
    return false;
  }

  addon->SaveSettings();

  return true;
}

void Interface_Base::free_string(const KODI_ADDON_BACKEND_HDL /*hdl*/, char* str)
{
  if (str)
    free(str);
}

void Interface_Base::free_string_array(const KODI_ADDON_BACKEND_HDL /*hdl*/,
                                       char** arr,
                                       int numElements)
{
  if (arr)
  {
    for (int i = 0; i < numElements; ++i)
    {
      free(arr[i]);
    }
    free(arr);
  }
}

void* Interface_Base::get_interface(const KODI_ADDON_BACKEND_HDL /*hdl*/,
                                    const char* name,
                                    const char* version)
{
  if (!name || !version)
    return nullptr;

  void* retval(nullptr);

  for (const auto& fn : s_registeredInterfaces)
    if ((retval = fn(name, version)))
      break;

  return retval;
}

//@}

} // namespace ADDON
