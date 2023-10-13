/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonDll.h"

#include "ServiceBroker.h"
#include "addons/AddonStatusHandler.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/binary-addons/BinaryAddonBase.h"
#include "addons/binary-addons/BinaryAddonManager.h"
#include "addons/binary-addons/DllAddon.h"
#include "addons/interfaces/AddonBase.h"
#include "addons/kodi-dev-kit/include/kodi/versions.h"
#include "addons/settings/AddonSettings.h"
#include "events/EventLog.h"
#include "events/NotificationEvent.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "settings/lib/SettingSection.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <algorithm>
#include <utility>

using namespace KODI::MESSAGING;

namespace ADDON
{

CAddonDll::CAddonDll(const AddonInfoPtr& addonInfo, BinaryAddonBasePtr addonBase)
  : CAddon(addonInfo, addonInfo->MainType()), m_binaryAddonBase(std::move(addonBase))
{
}

CAddonDll::CAddonDll(const AddonInfoPtr& addonInfo, AddonType addonType)
  : CAddon(addonInfo, addonType),
    m_binaryAddonBase(CServiceBroker::GetBinaryAddonManager().GetRunningAddonBase(addonInfo->ID()))
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
  if (XFILE::CFile::Exists(strFileName))
  {
    bool doCopy = true;
    std::string dstfile = URIUtils::AddFileToFolder(CSpecialProtocol::TranslatePath("special://xbmcaltbinaddons/"), strLibName);

    struct __stat64 dstFileStat;
    if (XFILE::CFile::Stat(dstfile, &dstFileStat) == 0)
    {
      struct __stat64 srcFileStat;
      if (XFILE::CFile::Stat(strFileName, &srcFileStat) == 0)
      {
        if (dstFileStat.st_size == srcFileStat.st_size && dstFileStat.st_mtime > srcFileStat.st_mtime)
          doCopy = false;
      }
    }

    if (doCopy)
    {
      CLog::Log(LOGDEBUG, "ADDON: caching {} to {}", strFileName, dstfile);
      XFILE::CFile::Copy(strFileName, dstfile);
    }

    strFileName = dstfile;
  }
  if (!XFILE::CFile::Exists(strFileName))
  {
    std::string tempbin = getenv("KODI_ANDROID_LIBS");
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
      CLog::Log(LOGDEBUG, "ADDON: Trying to load {}", strAltFileName);
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
        CLog::Log(LOGERROR, "ADDON: Could not locate {}", strLibName);
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
    m_pDll = nullptr;

    std::string heading =
        StringUtils::Format("{}: {}", CAddonInfo::TranslateType(Type(), true), Name());
    HELPERS::ShowOKDialogLines(CVariant{heading}, CVariant{24070}, CVariant{24071});

    return false;
  }

  return true;
}

ADDON_STATUS CAddonDll::Create(KODI_ADDON_INSTANCE_STRUCT* firstKodiInstance)
{
  CLog::Log(LOGDEBUG, "ADDON: Dll Initializing - {}", Name());
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
  if (!Interface_Base::InitInterface(this, m_interface, firstKodiInstance))
    return ADDON_STATUS_PERMANENT_FAILURE;

  /* Call Create to make connections, initializing data or whatever is
     needed to become the AddOn running */
  ADDON_STATUS status = m_pDll->Create(&m_interface);

  // "C" ABI related call, if on add-on used.
  if (status == ADDON_STATUS_OK && m_interface.toAddon->create)
    status = m_interface.toAddon->create(m_interface.firstKodiInstance, &m_interface.addonBase);

  if (status == ADDON_STATUS_OK)
  {
    m_initialized = true;
  }
  else if (status == ADDON_STATUS_NEED_SETTINGS)
  {
    if ((status = TransferSettings(ADDON_SETTINGS_ID)) == ADDON_STATUS_OK)
      m_initialized = true;
    else
      new CAddonStatusHandler(ID(), ADDON_SETTINGS_ID, status, false);
  }
  else
  { // Addon failed initialization
    CLog::Log(LOGERROR,
              "ADDON: Dll {} - Client returned bad status ({}) from Create and is not usable",
              Name(), status);

    // @todo currently a copy and paste from other function and becomes improved.
    std::string heading =
        StringUtils::Format("{}: {}", CAddonInfo::TranslateType(Type(), true), Name());
    HELPERS::ShowOKDialogLines(CVariant{ heading }, CVariant{ 24070 }, CVariant{ 24071 });
  }

  return status;
}

void CAddonDll::Destroy()
{
  /* Unload library file */
  if (m_pDll)
  {
    if (m_interface.toAddon->destroy)
      m_interface.toAddon->destroy(m_interface.addonBase);
    m_pDll->Unload();
  }

  Interface_Base::DeInitInterface(m_interface);

  if (m_pDll)
  {
    delete m_pDll;
    m_pDll = nullptr;
    CLog::Log(LOGINFO, "ADDON: Dll Destroyed - {}", Name());
  }

  ResetSettings(ADDON_SETTINGS_ID);

  m_initialized = false;
}

ADDON_STATUS CAddonDll::CreateInstance(KODI_ADDON_INSTANCE_STRUCT* instance)
{
  assert(instance != nullptr);
  assert(instance->functions != nullptr);
  assert(instance->info != nullptr);
  assert(instance->info->functions != nullptr);

  ADDON_STATUS status = ADDON_STATUS_OK;

  if (!m_initialized)
    status = Create(instance);
  if (status != ADDON_STATUS_OK)
    return status;

  /* Check version of requested instance type */
  if (!CheckAPIVersion(instance->info->type))
    return ADDON_STATUS_PERMANENT_FAILURE;

  status = m_interface.toAddon->create_instance(m_interface.addonBase, instance);

  if (instance->info)
  {
    m_usedInstances[instance->info->kodi] = instance;
  }

  return status;
}

void CAddonDll::DestroyInstance(KODI_ADDON_INSTANCE_STRUCT* instance)
{
  if (m_usedInstances.empty())
    return;

  auto it = m_usedInstances.find(instance->info->kodi);
  if (it != m_usedInstances.end())
  {
    m_interface.toAddon->destroy_instance(m_interface.addonBase, it->second);
    m_usedInstances.erase(it);
  }

  if (m_usedInstances.empty())
    Destroy();
}

bool CAddonDll::IsInUse() const
{
  if (m_informer)
    return m_informer->IsInUse(ID());
  return false;
}

void CAddonDll::RegisterInformer(CAddonDllInformer* informer)
{
  m_informer = informer;
}

AddonPtr CAddonDll::GetRunningInstance() const
{
  if (CServiceBroker::IsAddonInterfaceUp())
    return CServiceBroker::GetBinaryAddonManager().GetRunningAddon(ID());

  return AddonPtr();
}

void CAddonDll::OnPreInstall()
{
  if (m_binaryAddonBase)
    m_binaryAddonBase->OnPreInstall();
}

void CAddonDll::OnPostInstall(bool update, bool modal)
{
  if (m_binaryAddonBase)
    m_binaryAddonBase->OnPostInstall(update, modal);
}

void CAddonDll::OnPreUnInstall()
{
  if (m_binaryAddonBase)
    m_binaryAddonBase->OnPreUnInstall();
}

void CAddonDll::OnPostUnInstall()
{
  if (m_binaryAddonBase)
    m_binaryAddonBase->OnPostUnInstall();
}

bool CAddonDll::DllLoaded(void) const
{
  return m_pDll != nullptr;
}

CAddonVersion CAddonDll::GetTypeVersionDll(int type) const
{
  return CAddonVersion(m_pDll ? m_pDll->GetAddonTypeVersion(type) : nullptr);
}

CAddonVersion CAddonDll::GetTypeMinVersionDll(int type) const
{
  return CAddonVersion(m_pDll ? m_pDll->GetAddonTypeMinVersion(type) : nullptr);
}

bool CAddonDll::SaveSettings(AddonInstanceId id /* = ADDON_SETTINGS_ID */)
{
  // must save first, as TransferSettings() reloads saved settings!
  bool success = CAddon::SaveSettings(id);
  if (m_initialized)
    TransferSettings(id);
  return success;
}

ADDON_STATUS CAddonDll::TransferSettings(AddonInstanceId instanceId)
{
  bool restart = false;
  ADDON_STATUS reportStatus = ADDON_STATUS_OK;

  CLog::Log(LOGDEBUG, "Calling TransferSettings for: {}", Name());

  LoadSettings(false, true, instanceId);

  auto settings = GetSettings(instanceId);
  if (settings != nullptr)
  {
    KODI_ADDON_INSTANCE_FUNC* instanceTarget{nullptr};
    KODI_ADDON_INSTANCE_HDL instanceHandle{nullptr};
    if (instanceId != ADDON_SETTINGS_ID)
    {
      const auto it = std::find_if(
          m_usedInstances.begin(), m_usedInstances.end(),
          [instanceId](const auto& data) { return data.second->info->number == instanceId; });
      if (it == m_usedInstances.end())
        return ADDON_STATUS_UNKNOWN;

      instanceTarget = it->second->functions;
      instanceHandle = it->second->hdl;
    }

    for (const auto& section : settings->GetSections())
    {
      for (const auto& category : section->GetCategories())
      {
        for (const auto& group : category->GetGroups())
        {
          for (const auto& setting : group->GetSettings())
          {
            if (StringUtils::StartsWith(setting->GetId(), ADDON_SETTING_INSTANCE_GROUP))
              continue; // skip internal settings

            ADDON_STATUS status = ADDON_STATUS_OK;
            const char* id = setting->GetId().c_str();

            switch (setting->GetType())
            {
              case SettingType::Boolean:
              {
                bool tmp = std::static_pointer_cast<CSettingBool>(setting)->GetValue();
                if (instanceId == ADDON_SETTINGS_ID)
                {
                  if (m_interface.toAddon->setting_change_boolean)
                    status =
                        m_interface.toAddon->setting_change_boolean(m_interface.addonBase, id, tmp);
                }
                else if (instanceTarget && instanceHandle)
                {
                  if (instanceTarget->instance_setting_change_boolean)
                    status =
                        instanceTarget->instance_setting_change_boolean(instanceHandle, id, tmp);
                }
                break;
              }

              case SettingType::Integer:
              {
                int tmp = std::static_pointer_cast<CSettingInt>(setting)->GetValue();
                if (instanceId == ADDON_SETTINGS_ID)
                {
                  if (m_interface.toAddon->setting_change_integer)
                    status =
                        m_interface.toAddon->setting_change_integer(m_interface.addonBase, id, tmp);
                }
                else if (instanceTarget && instanceHandle)
                {
                  if (instanceTarget->instance_setting_change_integer)
                    status =
                        instanceTarget->instance_setting_change_integer(instanceHandle, id, tmp);
                }
                break;
              }

              case SettingType::Number:
              {
                float tmpf = static_cast<float>(std::static_pointer_cast<CSettingNumber>(setting)->GetValue());
                if (instanceId == ADDON_SETTINGS_ID)
                {
                  if (m_interface.toAddon->setting_change_float)
                    status =
                        m_interface.toAddon->setting_change_float(m_interface.addonBase, id, tmpf);
                }
                else if (instanceTarget && instanceHandle)
                {
                  if (instanceTarget->instance_setting_change_float)
                    status =
                        instanceTarget->instance_setting_change_float(instanceHandle, id, tmpf);
                }
                break;
              }

              case SettingType::String:
              {
                if (instanceId == ADDON_SETTINGS_ID)
                {
                  if (m_interface.toAddon->setting_change_string)
                    status = m_interface.toAddon->setting_change_string(
                        m_interface.addonBase, id,
                        std::static_pointer_cast<CSettingString>(setting)->GetValue().c_str());
                }
                else if (instanceTarget && instanceHandle)
                {
                  if (instanceTarget->instance_setting_change_string)
                    status = instanceTarget->instance_setting_change_string(
                        instanceHandle, id,
                        std::static_pointer_cast<CSettingString>(setting)->GetValue().c_str());
                }
                break;
              }

              default:
              {
                // log unknowns as an error, but go ahead and transfer the string
                CLog::Log(LOGERROR, "Unknown setting type of '{}' for {}", id, Name());
                if (instanceId == ADDON_SETTINGS_ID)
                {
                  if (m_interface.toAddon->setting_change_string)
                    status = m_interface.toAddon->setting_change_string(
                        m_interface.addonBase, id, setting->ToString().c_str());
                }
                else if (instanceTarget && instanceHandle)
                {
                  if (instanceTarget->instance_setting_change_string)
                    status = instanceTarget->instance_setting_change_string(
                        instanceHandle, id, setting->ToString().c_str());
                }
                break;
              }
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
    new CAddonStatusHandler(ID(), instanceId, restart ? ADDON_STATUS_NEED_RESTART : reportStatus,
                            true);
  }

  return ADDON_STATUS_OK;
}

bool CAddonDll::CheckAPIVersion(int type)
{
  /* check the API version */
  CAddonVersion kodiMinVersion(kodi::addon::GetTypeMinVersion(type));
  CAddonVersion addonVersion(m_pDll->GetAddonTypeVersion(type));
  CAddonVersion addonMinVersion = m_pDll->GetAddonTypeMinVersion_available()
                                      ? CAddonVersion(m_pDll->GetAddonTypeMinVersion(type))
                                      : addonVersion;

  /* Check the global usage from addon
   * if not used from addon, empty version is returned
   */
  if (type <= ADDON_GLOBAL_MAX && addonVersion.empty())
    return true;

  /* If a instance (not global) version becomes checked must be the version
   * present.
   */
  if (kodiMinVersion > addonVersion ||
      addonMinVersion > CAddonVersion(kodi::addon::GetTypeVersion(type)))
  {
    CLog::Log(LOGERROR, "Add-on '{}' is using an incompatible API version for type '{}'. Kodi API min version = '{}/{}', add-on API version '{}/{}'",
      Name(),
      kodi::addon::GetTypeName(type),
      kodi::addon::GetTypeVersion(type),
      kodiMinVersion.asString(),
      addonMinVersion.asString(),
      addonVersion.asString());

    if (CServiceBroker::GetGUI())
    {
      CEventLog* eventLog = CServiceBroker::GetEventLog();
      if (eventLog)
        eventLog->AddWithNotification(
            EventPtr(new CNotificationEvent(Name(), 24152, EventLevel::Error)));
    }

    return false;
  }

  return true;
}

} /* namespace ADDON */
