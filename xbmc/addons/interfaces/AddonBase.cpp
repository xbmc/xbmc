/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonBase.h"

#include "GUIUserMessages.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/settings/AddonSettings.h"
#include "addons/settings/GUIDialogAddonSettings.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

// "C" interface addon callback handle classes
#include "AudioEngine.h"
#include "Filesystem.h"
#include "General.h"
#include "Network.h"
#include "gui/General.h"

namespace ADDON
{

std::vector<ADDON_GET_INTERFACE_FN> Interface_Base::s_registeredInterfaces;

bool Interface_Base::InitInterface(CAddonDll* addon,
                                   AddonGlobalInterface& addonInterface,
                                   KODI_HANDLE firstKodiInstance)
{
  addonInterface = {0};

  addonInterface.libBasePath =
      strdup(CSpecialProtocol::TranslatePath("special://xbmcbinaddons").c_str());
  addonInterface.addonBase = nullptr;
  addonInterface.globalSingleInstance = nullptr;
  addonInterface.firstKodiInstance = firstKodiInstance;

  // Create function list from kodi to addon, generated with malloc to have
  // compatible with other versions
  addonInterface.toKodi = new AddonToKodiFuncTable_Addon();
  addonInterface.toKodi->kodiBase = addon;
  addonInterface.toKodi->get_addon_path = get_addon_path;
  addonInterface.toKodi->get_base_user_path = get_base_user_path;
  addonInterface.toKodi->addon_log_msg = addon_log_msg;
  addonInterface.toKodi->get_setting_bool = get_setting_bool;
  addonInterface.toKodi->get_setting_int = get_setting_int;
  addonInterface.toKodi->get_setting_float = get_setting_float;
  addonInterface.toKodi->get_setting_string = get_setting_string;
  addonInterface.toKodi->set_setting_bool = set_setting_bool;
  addonInterface.toKodi->set_setting_int = set_setting_int;
  addonInterface.toKodi->set_setting_float = set_setting_float;
  addonInterface.toKodi->set_setting_string = set_setting_string;
  addonInterface.toKodi->free_string = free_string;
  addonInterface.toKodi->free_string_array = free_string_array;

  // Related parts becomes set from addon headers, make here to nullptr to allow
  // checks for right set of them
  addonInterface.toAddon = new KodiToAddonFuncTable_Addon();

  // Init the other interfaces
  Interface_General::Init(&addonInterface);
  Interface_AudioEngine::Init(&addonInterface);
  Interface_Filesystem::Init(&addonInterface);
  Interface_Network::Init(&addonInterface);
  Interface_GUIGeneral::Init(&addonInterface);

  addonInterface.toKodi->get_interface = get_interface;

  return true;
}

void Interface_Base::DeInitInterface(AddonGlobalInterface& addonInterface)
{
  Interface_GUIGeneral::DeInit(&addonInterface);
  Interface_Network::DeInit(&addonInterface);
  Interface_Filesystem::DeInit(&addonInterface);
  Interface_AudioEngine::DeInit(&addonInterface);
  Interface_General::DeInit(&addonInterface);

  if (addonInterface.libBasePath)
    free(const_cast<char*>(addonInterface.libBasePath));

  delete addonInterface.toKodi;
  delete addonInterface.toAddon;
  addonInterface = {0};
}

void Interface_Base::RegisterInterface(ADDON_GET_INTERFACE_FN fn)
{
  s_registeredInterfaces.push_back(fn);
}

bool Interface_Base::UpdateSettingInActiveDialog(CAddonDll* addon,
                                                 const char* id,
                                                 const std::string& value)
{
  if (!CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_DIALOG_ADDON_SETTINGS))
    return false;

  CGUIDialogAddonSettings* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogAddonSettings>(
          WINDOW_DIALOG_ADDON_SETTINGS);
  if (dialog->GetCurrentAddonID() != addon->ID())
    return false;

  CGUIMessage message(GUI_MSG_SETTING_UPDATED, 0, 0);
  std::vector<std::string> params;
  params.emplace_back(id);
  params.push_back(value);
  message.SetStringParams(params);
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

char* Interface_Base::get_addon_path(void* kodiBase)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr)
  {
    CLog::Log(LOGERROR, "get_addon_path(...) called with empty kodi instance pointer");
    return nullptr;
  }

  return strdup(CSpecialProtocol::TranslatePath(addon->Path()).c_str());
}

char* Interface_Base::get_base_user_path(void* kodiBase)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr)
  {
    CLog::Log(LOGERROR, "get_base_user_path(...) called with empty kodi instance pointer");
    return nullptr;
  }

  return strdup(CSpecialProtocol::TranslatePath(addon->Profile()).c_str());
}

void Interface_Base::addon_log_msg(void* kodiBase, const int addonLogLevel, const char* strMessage)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr)
  {
    CLog::Log(LOGERROR, "addon_log_msg(...) called with empty kodi instance pointer");
    return;
  }

  int logLevel = LOGNONE;
  switch (addonLogLevel)
  {
    case ADDON_LOG_FATAL:
      logLevel = LOGFATAL;
      break;
    case ADDON_LOG_SEVERE:
      logLevel = LOGSEVERE;
      break;
    case ADDON_LOG_ERROR:
      logLevel = LOGERROR;
      break;
    case ADDON_LOG_WARNING:
      logLevel = LOGWARNING;
      break;
    case ADDON_LOG_NOTICE:
      logLevel = LOGNOTICE;
      break;
    case ADDON_LOG_INFO:
      logLevel = LOGINFO;
      break;
    case ADDON_LOG_DEBUG:
      logLevel = LOGDEBUG;
      break;
    default:
      break;
  }

  CLog::Log(logLevel, "AddOnLog: {}: {}", addon->Name(), strMessage);
}

bool Interface_Base::get_setting_bool(void* kodiBase, const char* id, bool* value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || id == nullptr || value == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - invalid data (addon='{}', id='{}', value='{}')",
              __func__, kodiBase, static_cast<const void*>(id), static_cast<void*>(value));

    return false;
  }

  if (!addon->ReloadSettings())
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - could't get settings for add-on '{}'", __func__,
              addon->Name());
    return false;
  }

  auto setting = addon->GetSettings()->GetSetting(id);
  if (setting == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - can't find setting '{}' in '{}'", __func__, id,
              addon->Name());
    return false;
  }

  if (setting->GetType() != SettingType::Boolean)
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - setting '{}' is not a boolean in '{}'", __func__, id,
              addon->Name());
    return false;
  }

  *value = std::static_pointer_cast<CSettingBool>(setting)->GetValue();
  return true;
}

bool Interface_Base::get_setting_int(void* kodiBase, const char* id, int* value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || id == nullptr || value == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - invalid data (addon='{}', id='{}', value='{}')",
              __func__, kodiBase, static_cast<const void*>(id), static_cast<void*>(value));

    return false;
  }

  if (!addon->ReloadSettings())
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - could't get settings for add-on '{}'", __func__,
              addon->Name());
    return false;
  }

  auto setting = addon->GetSettings()->GetSetting(id);
  if (setting == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - can't find setting '{}' in '{}'", __func__, id,
              addon->Name());
    return false;
  }

  if (setting->GetType() != SettingType::Integer && setting->GetType() != SettingType::Number)
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - setting '{}' is not a integer in '{}'", __func__, id,
              addon->Name());
    return false;
  }

  if (setting->GetType() == SettingType::Integer)
    *value = std::static_pointer_cast<CSettingInt>(setting)->GetValue();
  else
    *value = static_cast<int>(std::static_pointer_cast<CSettingNumber>(setting)->GetValue());
  return true;
}

bool Interface_Base::get_setting_float(void* kodiBase, const char* id, float* value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || id == nullptr || value == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - invalid data (addon='{}', id='{}', value='{}')",
              __func__, kodiBase, static_cast<const void*>(id), static_cast<void*>(value));

    return false;
  }

  if (!addon->ReloadSettings())
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - could't get settings for add-on '{}'", __func__,
              addon->Name());
    return false;
  }

  auto setting = addon->GetSettings()->GetSetting(id);
  if (setting == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - can't find setting '{}' in '{}'", __func__, id,
              addon->Name());
    return false;
  }

  if (setting->GetType() != SettingType::Number)
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - setting '{}' is not a number in '{}'", __func__, id,
              addon->Name());
    return false;
  }

  *value = static_cast<float>(std::static_pointer_cast<CSettingNumber>(setting)->GetValue());
  return true;
}

bool Interface_Base::get_setting_string(void* kodiBase, const char* id, char** value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || id == nullptr || value == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - invalid data (addon='{}', id='{}', value='{}')",
              __func__, kodiBase, static_cast<const void*>(id), static_cast<void*>(value));

    return false;
  }

  if (!addon->ReloadSettings())
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - could't get settings for add-on '{}'", __func__,
              addon->Name());
    return false;
  }

  auto setting = addon->GetSettings()->GetSetting(id);
  if (setting == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - can't find setting '{}' in '{}'", __func__, id,
              addon->Name());
    return false;
  }

  if (setting->GetType() != SettingType::String)
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - setting '{}' is not a string in '{}'", __func__, id,
              addon->Name());
    return false;
  }

  *value = strdup(std::static_pointer_cast<CSettingString>(setting)->GetValue().c_str());
  return true;
}

bool Interface_Base::set_setting_bool(void* kodiBase, const char* id, bool value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || id == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - invalid data (addon='{}', id='{}')", __func__,
              kodiBase, static_cast<const void*>(id));

    return false;
  }

  if (Interface_Base::UpdateSettingInActiveDialog(addon, id, value ? "true" : "false"))
    return true;

  if (!addon->UpdateSettingBool(id, value))
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - invalid setting type", __func__);
    return false;
  }

  addon->SaveSettings();

  return true;
}

bool Interface_Base::set_setting_int(void* kodiBase, const char* id, int value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || id == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - invalid data (addon='{}', id='{}')", __func__,
              kodiBase, static_cast<const void*>(id));

    return false;
  }

  if (Interface_Base::UpdateSettingInActiveDialog(addon, id, StringUtils::Format("%d", value)))
    return true;

  if (!addon->UpdateSettingInt(id, value))
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - invalid setting type", __func__);
    return false;
  }

  addon->SaveSettings();

  return true;
}

bool Interface_Base::set_setting_float(void* kodiBase, const char* id, float value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || id == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - invalid data (addon='{}', id='{}')", __func__,
              kodiBase, static_cast<const void*>(id));

    return false;
  }

  if (Interface_Base::UpdateSettingInActiveDialog(addon, id, StringUtils::Format("%f", value)))
    return true;

  if (!addon->UpdateSettingNumber(id, value))
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - invalid setting type", __func__);
    return false;
  }

  addon->SaveSettings();

  return true;
}

bool Interface_Base::set_setting_string(void* kodiBase, const char* id, const char* value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || id == nullptr || value == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - invalid data (addon='{}', id='{}', value='{}')",
              __func__, kodiBase, static_cast<const void*>(id), static_cast<const void*>(value));

    return false;
  }

  if (Interface_Base::UpdateSettingInActiveDialog(addon, id, value))
    return true;

  if (!addon->UpdateSettingString(id, value))
  {
    CLog::Log(LOGERROR, "Interface_Base::{} - invalid setting type", __func__);
    return false;
  }

  addon->SaveSettings();

  return true;
}

void Interface_Base::free_string(void* kodiBase, char* str)
{
  if (str)
    free(str);
}

void Interface_Base::free_string_array(void* kodiBase, char** arr, int numElements)
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

void* Interface_Base::get_interface(void* kodiBase, const char* name, const char* version)
{
  if (!name || !version)
    return nullptr;

  void* retval(nullptr);

  for (auto fn : s_registeredInterfaces)
    if ((retval = fn(name, version)))
      break;

  return retval;
}

//@}

} // namespace ADDON
