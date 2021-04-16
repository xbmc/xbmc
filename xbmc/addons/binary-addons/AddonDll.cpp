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
#include "addons/binary-addons/BinaryAddonBase.h"
#include "addons/binary-addons/BinaryAddonManager.h"
#include "addons/settings/AddonSettings.h"
#include "events/EventLog.h"
#include "events/NotificationEvent.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <utility>

using namespace KODI::MESSAGING;

namespace ADDON
{

CAddonDll::CAddonDll(const AddonInfoPtr& addonInfo, BinaryAddonBasePtr addonBase)
  : CAddon(addonInfo, addonInfo->MainType()), m_binaryAddonBase(std::move(addonBase))
{
}

CAddonDll::CAddonDll(const AddonInfoPtr& addonInfo, TYPE addonType)
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
      CLog::Log(LOGDEBUG, "ADDON: caching %s to %s", strFileName.c_str(), dstfile.c_str());
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
    m_pDll = nullptr;

    std::string heading = StringUtils::Format("%s: %s", CAddonInfo::TranslateType(Type(), true).c_str(), Name().c_str());
    HELPERS::ShowOKDialogLines(CVariant{heading}, CVariant{24070}, CVariant{24071});

    return false;
  }

  return true;
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
  if (!Interface_Base::InitInterface(this, m_interface, firstKodiInstance))
    return ADDON_STATUS_PERMANENT_FAILURE;

  /* Call Create to make connections, initializing data or whatever is
     needed to become the AddOn running */
  ADDON_STATUS status =
      m_pDll->Create(&m_interface, kodi::addon::GetTypeVersion(ADDON_GLOBAL_MAIN), nullptr);

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
    if (m_interface.toAddon->destroy)
      m_interface.toAddon->destroy();
    m_pDll->Unload();
  }

  Interface_Base::DeInitInterface(m_interface);

  if (m_pDll)
  {
    delete m_pDll;
    m_pDll = nullptr;
    CLog::Log(LOGINFO, "ADDON: Dll Destroyed - %s", Name().c_str());
  }

  ResetSettings();

  m_initialized = false;
}

ADDON_STATUS CAddonDll::CreateInstance(ADDON_TYPE instanceType,
                                       ADDON_INSTANCE_HANDLER instanceClass,
                                       const std::string& instanceID,
                                       KODI_HANDLE instance,
                                       KODI_HANDLE parentInstance)
{
  ADDON_STATUS status = ADDON_STATUS_OK;

  if (!m_initialized)
    status = Create(instance);
  if (status != ADDON_STATUS_OK)
    return status;

  /* Check version of requested instance type */
  if (!CheckAPIVersion(instanceType))
    return ADDON_STATUS_PERMANENT_FAILURE;

  KODI_HANDLE addonInstance = nullptr;
  status = m_interface.toAddon->create_instance(instanceType, instanceID.c_str(), instance,
                                                kodi::addon::GetTypeVersion(instanceType),
                                                &addonInstance, parentInstance);

  if (addonInstance)
  {
    m_usedInstances[instanceClass] = std::make_pair(instanceType, addonInstance);
  }

  return status;
}

void CAddonDll::DestroyInstance(ADDON_INSTANCE_HANDLER instanceClass)
{
  if (m_usedInstances.empty())
    return;

  auto it = m_usedInstances.find(instanceClass);
  if (it != m_usedInstances.end())
  {
    m_interface.toAddon->destroy_instance(it->second.first, it->second.second);
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
  if (CServiceBroker::IsBinaryAddonCacheUp())
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

AddonVersion CAddonDll::GetTypeVersionDll(int type) const
{
  return AddonVersion(m_pDll ? m_pDll->GetAddonTypeVersion(type) : nullptr);
}

AddonVersion CAddonDll::GetTypeMinVersionDll(int type) const
{
  return AddonVersion(m_pDll ? m_pDll->GetAddonTypeMinVersion(type) : nullptr);
}

void CAddonDll::SaveSettings()
{
  // must save first, as TransferSettings() reloads saved settings!
  CAddon::SaveSettings();
  if (m_initialized)
    TransferSettings();
}

ADDON_STATUS CAddonDll::TransferSettings()
{
  if (!m_interface.toAddon->set_setting)
    return ADDON_STATUS_NOT_IMPLEMENTED;

  bool restart = false;
  ADDON_STATUS reportStatus = ADDON_STATUS_OK;

  CLog::Log(LOGDEBUG, "Calling TransferSettings for: %s", Name().c_str());

  LoadSettings(false);

  auto settings = GetSettings();
  if (settings != nullptr)
  {
    for (const auto& section : settings->GetSections())
    {
      for (const auto& category : section->GetCategories())
      {
        for (const auto& group : category->GetGroups())
        {
          for (const auto& setting : group->GetSettings())
          {
            ADDON_STATUS status = ADDON_STATUS_OK;
            const char* id = setting->GetId().c_str();
            switch (setting->GetType())
            {
              case SettingType::Boolean:
              {
                bool tmp = std::static_pointer_cast<CSettingBool>(setting)->GetValue();
                status = m_interface.toAddon->set_setting(id, &tmp);
                break;
              }

              case SettingType::Integer:
              {
                int tmp = std::static_pointer_cast<CSettingInt>(setting)->GetValue();
                status = m_interface.toAddon->set_setting(id, &tmp);
                break;
              }

              case SettingType::Number:
              {
                float tmpf = static_cast<float>(std::static_pointer_cast<CSettingNumber>(setting)->GetValue());
                status = m_interface.toAddon->set_setting(id, &tmpf);
                break;
              }

              case SettingType::String:
                status = m_interface.toAddon->set_setting(
                    id, std::static_pointer_cast<CSettingString>(setting)->GetValue().c_str());
                break;

              default:
                // log unknowns as an error, but go ahead and transfer the string
                CLog::Log(LOGERROR, "Unknown setting type of '%s' for %s", id, Name().c_str());
                status = m_interface.toAddon->set_setting(id, setting->ToString().c_str());
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
  AddonVersion addonMinVersion = m_pDll->GetAddonTypeMinVersion_available()
    ? AddonVersion(m_pDll->GetAddonTypeMinVersion(type))
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
    addonMinVersion > AddonVersion(kodi::addon::GetTypeVersion(type)))
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
      CEventLog &eventLog = CServiceBroker::GetEventLog();
      eventLog.AddWithNotification(EventPtr(new CNotificationEvent(Name(), 24152, EventLevel::Error)));
    }

    return false;
  }

  return true;
}

} /* namespace ADDON */
