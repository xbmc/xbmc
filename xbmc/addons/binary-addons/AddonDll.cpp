/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "AddonDll.h"

#include "addons/AddonStatusHandler.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "addons/settings/AddonSettings.h"
#include "addons/settings/GUIDialogAddonSettings.h"
#include "events/EventLog.h"
#include "events/NotificationEvent.h"
#include "guilib/GUIWindowManager.h"
#include "utils/URIUtils.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/Directory.h"
#include "messaging/helpers/DialogOKHelper.h" 
#include "settings/lib/SettingSection.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "Util.h"

// Global addon callback handle classes
#include "addons/interfaces/AudioEngine.h"
#include "addons/interfaces/Filesystem.h"
#include "addons/interfaces/General.h"
#include "addons/interfaces/Network.h"
#include "addons/interfaces/GUI/General.h"

using namespace KODI::MESSAGING;

namespace ADDON
{

CAddonDll::CAddonDll(CAddonInfo addonInfo, BinaryAddonBasePtr addonBase)
  : CAddon(std::move(addonInfo)),
    m_pHelpers(nullptr),
    m_binaryAddonBase(addonBase),
    m_pDll(nullptr),
    m_initialized(false),
    m_interface{0}
{
}

CAddonDll::CAddonDll(CAddonInfo addonInfo)
  : CAddon(std::move(addonInfo)),
    m_pHelpers(nullptr),
    m_binaryAddonBase(nullptr),
    m_pDll(nullptr),
    m_initialized(false),
    m_interface{0}
{
}

CAddonDll::~CAddonDll()
{
  if (m_initialized)
    Destroy();
}

std::string CAddonDll::GetDllPath(const std::string &libPath)
{
  std::string strFileName = libPath;
  std::string strLibName = URIUtils::GetFileName(strFileName);

  if (strLibName.empty())
    return "";

  /* Check if lib being loaded exists, else check in XBMC binary location */
#if defined(TARGET_ANDROID)
  // Android libs MUST live in this path, else multi-arch will break.
  // The usual soname requirements apply. no subdirs, and filename is ^lib.*\.so$
  if (!XFILE::CFile::Exists(strFileName))
  {
    std::string tempbin = getenv("XBMC_ANDROID_LIBS");
    strFileName = tempbin + "/" + strLibName;
  }
#endif

  if (!XFILE::CFile::Exists(strFileName))
  {
    std::string strAltFileName;

    std::string altbin = CSpecialProtocol::TranslatePath("special://xbmcaltbinaddons/");
    if (!altbin.empty())
    {
      strAltFileName = altbin + strLibName;
      if (!XFILE::CFile::Exists(strAltFileName))
      {
        std::string temp = CSpecialProtocol::TranslatePath("special://xbmc/addons/");
        strAltFileName = strFileName;
        strAltFileName.erase(0, temp.size());
        strAltFileName = altbin + strAltFileName;
      }
      CLog::Log(LOGDEBUG, "ADDON: Trying to load %s", strAltFileName.c_str());
    }

    if (XFILE::CFile::Exists(strAltFileName))
      strFileName = strAltFileName;
    else
    {
      std::string temp = CSpecialProtocol::TranslatePath("special://xbmc/");
      std::string tempbin = CSpecialProtocol::TranslatePath("special://xbmcbin/");
      strFileName.erase(0, temp.size());
      strFileName = tempbin + strFileName;
      if (!XFILE::CFile::Exists(strFileName))
      {
        CLog::Log(LOGERROR, "ADDON: Could not locate %s", strLibName.c_str());
        strFileName.clear();
      }
    }
  }

  return strFileName;
}

std::string CAddonDll::LibPath() const
{
  return GetDllPath(CAddon::LibPath());
}

bool CAddonDll::LoadDll()
{
  if (m_pDll)
    return true;

  std::string strFileName = LibPath();
  if (strFileName.empty())
    return false;

  /* Load the Dll */
  m_pDll = new DllAddon;
  m_pDll->SetFile(strFileName);
  m_pDll->EnableDelayedUnload(false);
  if (!m_pDll->Load())
  {
    delete m_pDll;
    m_pDll = NULL;

    std::string heading = StringUtils::Format("%s: %s", CAddonInfo::TranslateType(Type(), true).c_str(), Name().c_str());
    HELPERS::ShowOKDialogLines(CVariant{heading}, CVariant{24070}, CVariant{24071});

    return false;
  }

  return true;
}

ADDON_STATUS CAddonDll::Create(ADDON_TYPE type, void* funcTable, void* info)
{
  /* ensure that a previous instance is destroyed */
  Destroy();

  if (!funcTable)
    return ADDON_STATUS_PERMANENT_FAILURE;

  CLog::Log(LOGDEBUG, "ADDON: Dll Initializing - %s", Name().c_str());
  m_initialized = false;

  if (!LoadDll())
    return ADDON_STATUS_PERMANENT_FAILURE;

  /* Check requested instance version on add-on */
  if (!CheckAPIVersion(type))
    return ADDON_STATUS_PERMANENT_FAILURE;

  /* Check versions about global parts on add-on (parts used on all types) */
  for (unsigned int id = ADDON_GLOBAL_MAIN; id <= ADDON_GLOBAL_MAX; ++id)
  {
    if (!CheckAPIVersion(id))
      return ADDON_STATUS_PERMANENT_FAILURE;
  }

  /* Load add-on function table (written by add-on itself) */
  m_pDll->GetAddon(funcTable);

  /* Allocate the helper function class to allow crosstalk over
     helper libraries */
  m_pHelpers = new CAddonInterfaces(this);

  /* Call Create to make connections, initializing data or whatever is
     needed to become the AddOn running */
  ADDON_STATUS status = m_pDll->Create(m_pHelpers->GetCallbacks(), info);
  if (status == ADDON_STATUS_OK)
  {
    m_initialized = true;
  }
  else if (status == ADDON_STATUS_NEED_SETTINGS)
  {
    status = TransferSettings();
    if (status == ADDON_STATUS_OK)
      m_initialized = true;
    else
      new CAddonStatusHandler(ID(), status, "", false);
  }
  else
  { // Addon failed initialization
    CLog::Log(LOGERROR, "ADDON: Dll %s - Client returned bad status (%i) from Create and is not usable", Name().c_str(), status);

    std::string heading = StringUtils::Format("%s: %s", CAddonInfo::TranslateType(Type(), true).c_str(), Name().c_str());
    HELPERS::ShowOKDialogLines(CVariant{ heading }, CVariant{ 24070 }, CVariant{ 24071 });
  }

  return status;
}

ADDON_STATUS CAddonDll::Create(KODI_HANDLE firstKodiInstance)
{
  CLog::Log(LOGDEBUG, "ADDON: Dll Initializing - %s", Name().c_str());
  m_initialized = false;

  if (!LoadDll())
  {
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  /* Check versions about global parts on add-on (parts used on all types) */
  for (unsigned int id = ADDON_GLOBAL_MAIN; id <= ADDON_GLOBAL_MAX; ++id)
  {
    if (!CheckAPIVersion(id))
      return ADDON_STATUS_PERMANENT_FAILURE;
  }

  /* Allocate the helper function class to allow crosstalk over
     helper add-on headers */
  if (!InitInterface(firstKodiInstance))
    return ADDON_STATUS_PERMANENT_FAILURE;

  /* Call Create to make connections, initializing data or whatever is
     needed to become the AddOn running */
  ADDON_STATUS status = m_pDll->Create(&m_interface, nullptr);
  if (status == ADDON_STATUS_OK)
  {
    m_initialized = true;
  }
  else if (status == ADDON_STATUS_NEED_SETTINGS)
  {
    if ((status = TransferSettings()) == ADDON_STATUS_OK)
      m_initialized = true;
    else
      new CAddonStatusHandler(ID(), status, "", false);
  }
  else
  { // Addon failed initialization
    CLog::Log(LOGERROR, "ADDON: Dll %s - Client returned bad status (%i) from Create and is not usable", Name().c_str(), status);

    // @todo currently a copy and paste from other function and becomes improved.
    std::string heading = StringUtils::Format("%s: %s", CAddonInfo::TranslateType(Type(), true).c_str(), Name().c_str());
    HELPERS::ShowOKDialogLines(CVariant{ heading }, CVariant{ 24070 }, CVariant{ 24071 });
  }

  return status;
}

void CAddonDll::Destroy()
{
  /* Unload library file */
  if (m_pDll)
  {
    m_pDll->Destroy();
    m_pDll->Unload();
  }

  DeInitInterface();

  delete m_pHelpers;
  m_pHelpers = NULL;
  if (m_pDll)
  {
    delete m_pDll;
    m_pDll = NULL;
    CLog::Log(LOGINFO, "ADDON: Dll Destroyed - %s", Name().c_str());
  }
  m_initialized = false;
}

ADDON_STATUS CAddonDll::CreateInstance(ADDON_TYPE instanceType, const std::string& instanceID, KODI_HANDLE instance, KODI_HANDLE parentInstance)
{
  ADDON_STATUS status = ADDON_STATUS_OK;

  if (!m_initialized)
    status = Create(instance);
  if (status != ADDON_STATUS_OK)
    return status;

  /* Check version of requested instance type */
  if (!CheckAPIVersion(instanceType))
    return ADDON_STATUS_PERMANENT_FAILURE;

  KODI_HANDLE addonInstance;
  status = m_interface.toAddon->create_instance(instanceType, instanceID.c_str(), instance, &addonInstance, parentInstance);
  if (status == ADDON_STATUS_OK)
  {
    m_usedInstances[instanceID] = std::make_pair(instanceType, addonInstance);
  }

  return status;
}

void CAddonDll::DestroyInstance(const std::string& instanceID)
{
  auto it = m_usedInstances.find(instanceID);
  if (it != m_usedInstances.end())
  {
    m_interface.toAddon->destroy_instance(it->second.first, it->second.second);
    m_usedInstances.erase(it);
  }

  if (m_usedInstances.empty())
    Destroy();
}

AddonPtr CAddonDll::GetRunningInstance() const
{
  return CServiceBroker::GetBinaryAddonManager().GetRunningAddon(ID());
}

bool CAddonDll::DllLoaded(void) const
{
  return m_pDll != NULL;
}

ADDON_STATUS CAddonDll::GetStatus()
{
  return m_pDll->GetStatus();
}

void CAddonDll::SaveSettings()
{
  // must save first, as TransferSettings() reloads saved settings!
  CAddon::SaveSettings();
  if (m_initialized)
    TransferSettings();
}

std::string CAddonDll::GetSetting(const std::string& key)
{
  return CAddon::GetSetting(key);
}

ADDON_STATUS CAddonDll::TransferSettings()
{
  bool restart = false;
  ADDON_STATUS reportStatus = ADDON_STATUS_OK;

  CLog::Log(LOGDEBUG, "Calling TransferSettings for: %s", Name().c_str());

  LoadSettings(false);

  auto settings = GetSettings();
  if (settings != nullptr)
  {
    for (auto section : settings->GetSections())
    {
      for (auto category : section->GetCategories())
      {
        for (auto group : category->GetGroups())
        {
          for (auto setting : group->GetSettings())
          {
            ADDON_STATUS status = ADDON_STATUS_OK;
            const char* id = setting->GetId().c_str();
            switch (setting->GetType())
            {
              case SettingType::Boolean:
              {
                bool tmp = std::static_pointer_cast<CSettingBool>(setting)->GetValue();
                status = m_pDll->SetSetting(id, &tmp);
                break;
              }

              case SettingType::Integer:
              {
                int tmp = std::static_pointer_cast<CSettingInt>(setting)->GetValue();
                status = m_pDll->SetSetting(id, &tmp);
                break;
              }

              case SettingType::Number:
              {
                float tmpf = static_cast<float>(std::static_pointer_cast<CSettingNumber>(setting)->GetValue());
                status = m_pDll->SetSetting(id, &tmpf);
                break;
              }

              case SettingType::String:
                status = m_pDll->SetSetting(id, std::static_pointer_cast<CSettingString>(setting)->GetValue().c_str());
                break;

              default:
                // log unknowns as an error, but go ahead and transfer the string
                CLog::Log(LOGERROR, "Unknown setting type of '%s' for %s", id, Name().c_str());
                status = m_pDll->SetSetting(id, setting->ToString().c_str());
                break;
            }

            if (status == ADDON_STATUS_NEED_RESTART)
              restart = true;
            else if (status != ADDON_STATUS_OK)
              reportStatus = status;
          }
        }
      }
    }
  }

  if (restart || reportStatus != ADDON_STATUS_OK)
  {
    new CAddonStatusHandler(ID(), restart ? ADDON_STATUS_NEED_RESTART : reportStatus, "", true);
  }

  return ADDON_STATUS_OK;
}

bool CAddonDll::CheckAPIVersion(int type)
{
  /* check the API version */
  AddonVersion kodiMinVersion(kodi::addon::GetTypeMinVersion(type));
  AddonVersion addonVersion(m_pDll->GetAddonTypeVersion(type));

  /* Check the global usage from addon
   * if not used from addon becomes "0.0.0" returned
   */
  if (type <= ADDON_GLOBAL_MAX && addonVersion == AddonVersion("0.0.0"))
    return true;

  /* If a instance (not global) version becomes checked must be the version
   * present.
   */
  if (kodiMinVersion > addonVersion || 
      addonVersion > AddonVersion(kodi::addon::GetTypeVersion(type)))
  {
    CLog::Log(LOGERROR, "Add-on '%s' is using an incompatible API version for type '%s'. Kodi API min version = '%s', add-on API version '%s'",
                            Name().c_str(),
                            kodi::addon::GetTypeName(type),
                            kodiMinVersion.asString().c_str(),
                            addonVersion.asString().c_str());

    CEventLog::GetInstance().AddWithNotification(EventPtr(new CNotificationEvent(Name(), 24152, EventLevel::Error)));

    return false;
  }

  return true;
}

bool CAddonDll::UpdateSettingInActiveDialog(const char* id, const std::string& value)
{
  if (!g_windowManager.IsWindowActive(WINDOW_DIALOG_ADDON_SETTINGS))
    return false;

  CGUIDialogAddonSettings* dialog = g_windowManager.GetWindow<CGUIDialogAddonSettings>(WINDOW_DIALOG_ADDON_SETTINGS);
  if (dialog->GetCurrentAddonID() != m_addonInfo.ID())
    return false;

  CGUIMessage message(GUI_MSG_SETTING_UPDATED, 0, 0);
  std::vector<std::string> params;
  params.push_back(id);
  params.push_back(value);
  message.SetStringParams(params);
  g_windowManager.SendThreadMessage(message, WINDOW_DIALOG_ADDON_SETTINGS);

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

bool CAddonDll::InitInterface(KODI_HANDLE firstKodiInstance)
{
  m_interface = {0};

  m_interface.libBasePath = strdup(CSpecialProtocol::TranslatePath("special://xbmcbinaddons").c_str());
  m_interface.addonBase = nullptr;
  m_interface.globalSingleInstance = nullptr;
  m_interface.firstKodiInstance = firstKodiInstance;

  // Create function list from kodi to addon, generated with malloc to have
  // compatible with other versions
  m_interface.toKodi = (AddonToKodiFuncTable_Addon*) malloc(sizeof(AddonToKodiFuncTable_Addon));
  m_interface.toKodi->kodiBase = this;
  m_interface.toKodi->get_addon_path = get_addon_path;
  m_interface.toKodi->get_base_user_path = get_base_user_path;
  m_interface.toKodi->addon_log_msg = addon_log_msg;
  m_interface.toKodi->get_setting_bool = get_setting_bool;
  m_interface.toKodi->get_setting_int = get_setting_int;
  m_interface.toKodi->get_setting_float = get_setting_float;
  m_interface.toKodi->get_setting_string = get_setting_string;
  m_interface.toKodi->set_setting_bool = set_setting_bool;
  m_interface.toKodi->set_setting_int = set_setting_int;
  m_interface.toKodi->set_setting_float = set_setting_float;
  m_interface.toKodi->set_setting_string = set_setting_string;
  m_interface.toKodi->free_string = free_string;
  m_interface.toKodi->free_string_array = free_string_array;

  // Create function list from addon to kodi, generated with calloc to have
  // compatible with other versions and everything with "0"
  // Related parts becomes set from addon headers.
  m_interface.toAddon = (KodiToAddonFuncTable_Addon*) calloc(1, sizeof(KodiToAddonFuncTable_Addon));

  Interface_General::Init(&m_interface);
  Interface_AudioEngine::Init(&m_interface);
  Interface_Filesystem::Init(&m_interface);
  Interface_Network::Init(&m_interface);
  Interface_GUIGeneral::Init(&m_interface);

  return true;
}

void CAddonDll::DeInitInterface()
{
  Interface_GUIGeneral::DeInit(&m_interface);
  Interface_Network::DeInit(&m_interface);
  Interface_Filesystem::DeInit(&m_interface);
  Interface_AudioEngine::DeInit(&m_interface);
  Interface_General::DeInit(&m_interface);

  if (m_interface.libBasePath)
    free((char*)m_interface.libBasePath);
  if (m_interface.toKodi)
    free((char*)m_interface.toKodi);
  if (m_interface.toAddon)
    free((char*)m_interface.toAddon);
  m_interface = {0};
}

char* CAddonDll::get_addon_path(void* kodiBase)
{ 
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr)
  {
    CLog::Log(LOGERROR, "get_addon_path(...) called with empty kodi instance pointer");
    return nullptr;
  }

  return strdup(CSpecialProtocol::TranslatePath(addon->Path()).c_str());
}

char* CAddonDll::get_base_user_path(void* kodiBase)
{ 
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr)
  {
    CLog::Log(LOGERROR, "get_base_user_path(...) called with empty kodi instance pointer");
    return nullptr;
  }

  return strdup(CSpecialProtocol::TranslatePath(addon->Profile()).c_str());
}

void CAddonDll::addon_log_msg(void* kodiBase, const int addonLogLevel, const char* strMessage)
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

  CLog::Log(logLevel, "AddOnLog: %s: %s", addon->Name().c_str(), strMessage);
}

bool CAddonDll::get_setting_bool(void* kodiBase, const char* id, bool* value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || id == nullptr || value == nullptr)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - invalid data (addon='%p', id='%p', value='%p')",
                                        __FUNCTION__, addon, id, value);

    return false;
  }

  if (!addon->ReloadSettings())
  {
    CLog::Log(LOGERROR, "kodi::General::%s - could't get settings for add-on '%s'", __FUNCTION__, addon->Name().c_str());
    return false;
  }

  auto setting = addon->GetSettings()->GetSetting(id);
  if (setting == nullptr)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - can't find setting '%s' in '%s'", __FUNCTION__, id, addon->Name().c_str());
    return false;
  }

  if (setting->GetType() != SettingType::Boolean)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - setting '%s' is not a boolean in '%s'", __FUNCTION__, id, addon->Name().c_str());
    return false;
  }

  *value = std::static_pointer_cast<CSettingBool>(setting)->GetValue();
  return true;
}

bool CAddonDll::get_setting_int(void* kodiBase, const char* id, int* value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || id == nullptr || value == nullptr)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - invalid data (addon='%p', id='%p', value='%p')",
                                        __FUNCTION__, addon, id, value);

    return false;
  }

  if (!addon->ReloadSettings())
  {
    CLog::Log(LOGERROR, "kodi::General::%s - could't get settings for add-on '%s'", __FUNCTION__, addon->Name().c_str());
    return false;
  }

  auto setting = addon->GetSettings()->GetSetting(id);
  if (setting == nullptr)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - can't find setting '%s' in '%s'", __FUNCTION__, id, addon->Name().c_str());
    return false;
  }

  if (setting->GetType() != SettingType::Integer)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - setting '%s' is not a integer in '%s'", __FUNCTION__, id, addon->Name().c_str());
    return false;
  }

  *value = std::static_pointer_cast<CSettingInt>(setting)->GetValue();
  return true;
}

bool CAddonDll::get_setting_float(void* kodiBase, const char* id, float* value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || id == nullptr || value == nullptr)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - invalid data (addon='%p', id='%p', value='%p')",
                                        __FUNCTION__, addon, id, value);

    return false;
  }

  if (!addon->ReloadSettings())
  {
    CLog::Log(LOGERROR, "kodi::General::%s - could't get settings for add-on '%s'", __FUNCTION__, addon->Name().c_str());
    return false;
  }

  auto setting = addon->GetSettings()->GetSetting(id);
  if (setting == nullptr)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - can't find setting '%s' in '%s'", __FUNCTION__, id, addon->Name().c_str());
    return false;
  }

  if (setting->GetType() != SettingType::Number)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - setting '%s' is not a number in '%s'", __FUNCTION__, id, addon->Name().c_str());
    return false;
  }

  *value = static_cast<float>(std::static_pointer_cast<CSettingNumber>(setting)->GetValue());
  return true;
}

bool CAddonDll::get_setting_string(void* kodiBase, const char* id, char** value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || id == nullptr || value == nullptr)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - invalid data (addon='%p', id='%p', value='%p')",
                                        __FUNCTION__, addon, id, value);

    return false;
  }

  if (!addon->ReloadSettings())
  {
    CLog::Log(LOGERROR, "kodi::General::%s - could't get settings for add-on '%s'", __FUNCTION__, addon->Name().c_str());
    return false;
  }

  auto setting = addon->GetSettings()->GetSetting(id);
  if (setting == nullptr)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - can't find setting '%s' in '%s'", __FUNCTION__, id, addon->Name().c_str());
    return false;
  }

  if (setting->GetType() != SettingType::String)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - setting '%s' is not a string in '%s'", __FUNCTION__, id, addon->Name().c_str());
    return false;
  }

  *value = strdup(std::static_pointer_cast<CSettingString>(setting)->GetValue().c_str());
  return true;
}

bool CAddonDll::set_setting_bool(void* kodiBase, const char* id, bool value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || id == nullptr)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - invalid data (addon='%p', id='%p')",
                                        __FUNCTION__, addon, id);

    return false;
  }

  if (addon->UpdateSettingInActiveDialog(id, value ? "true" : "false"))
    return true;

  if (!addon->UpdateSettingBool(id, value))
  {
    CLog::Log(LOGERROR, "kodi::General::%s - invalid setting type", __FUNCTION__);
    return false;
  }

  addon->SaveSettings();

  return true;
}

bool CAddonDll::set_setting_int(void* kodiBase, const char* id, int value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || id == nullptr)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - invalid data (addon='%p', id='%p')",
                                        __FUNCTION__, addon, id);

    return false;
  }

  if (addon->UpdateSettingInActiveDialog(id, StringUtils::Format("%d", value)))
    return true;

  if (!addon->UpdateSettingInt(id, value))
  {
    CLog::Log(LOGERROR, "kodi::General::%s - invalid setting type", __FUNCTION__);
    return false;
  }

  addon->SaveSettings();

  return true;
}

bool CAddonDll::set_setting_float(void* kodiBase, const char* id, float value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || id == nullptr)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - invalid data (addon='%p', id='%p')",
                                        __FUNCTION__, addon, id);

    return false;
  }

  if (addon->UpdateSettingInActiveDialog(id, StringUtils::Format("%f", value)))
    return true;

  if (!addon->UpdateSettingNumber(id, value))
  {
    CLog::Log(LOGERROR, "kodi::General::%s - invalid setting type", __FUNCTION__);
    return false;
  }

  addon->SaveSettings();

  return true;
}

bool CAddonDll::set_setting_string(void* kodiBase, const char* id, const char* value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || id == nullptr || value == nullptr)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - invalid data (addon='%p', id='%p', value='%p')",
                                        __FUNCTION__, addon, id, value);

    return false;
  }

  if (addon->UpdateSettingInActiveDialog(id, value))
    return true;

  if (!addon->UpdateSettingString(id, value))
  {
    CLog::Log(LOGERROR, "kodi::General::%s - invalid setting type", __FUNCTION__);
    return false;
  }

  addon->SaveSettings();

  return true;
}

void CAddonDll::free_string(void* kodiBase, char* str)
{
  if (str)
    free(str);
}

void CAddonDll::free_string_array(void* kodiBase, char** arr, int numElements)
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


//@}

}; /* namespace ADDON */

