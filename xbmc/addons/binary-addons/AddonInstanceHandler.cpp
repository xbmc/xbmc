/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonInstanceHandler.h"

#include "ServiceBroker.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

namespace ADDON
{

CCriticalSection IAddonInstanceHandler::m_cdSec;

IAddonInstanceHandler::IAddonInstanceHandler(ADDON_TYPE type,
                                             const AddonInfoPtr& addonInfo,
                                             KODI_HANDLE parentInstance /* = nullptr*/,
                                             const std::string& instanceID /* = ""*/)
  : m_type(type), m_parentInstance(parentInstance), m_addonInfo(addonInfo)
{
  // if no special instance ID is given generate one from class pointer (is
  // faster as unique id and also safe enough for them).
  m_instanceId = !instanceID.empty() ? instanceID : StringUtils::Format("{}", fmt::ptr(this));
  m_addonBase = CServiceBroker::GetBinaryAddonManager().GetAddonBase(addonInfo, this, m_addon);

  KODI_ADDON_INSTANCE_INFO* info = new KODI_ADDON_INSTANCE_INFO();
  info->number = 0;
  info->id = m_instanceId.c_str();
  info->version = kodi::addon::GetTypeVersion(m_type);
  info->type = m_type;
  info->kodi = this;
  info->parent = m_parentInstance;
  info->first_instance = m_addon && !m_addon->Initialized();
  m_ifc.info = info;
}

IAddonInstanceHandler::~IAddonInstanceHandler()
{
  CServiceBroker::GetBinaryAddonManager().ReleaseAddonBase(m_addonBase, this);

  delete m_ifc.info;
}

std::string IAddonInstanceHandler::ID() const
{
  return m_addon ? m_addon->ID() : "";
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

AddonVersion IAddonInstanceHandler::Version() const
{
  return m_addon ? m_addon->Version() : AddonVersion();
}

ADDON_STATUS IAddonInstanceHandler::CreateInstance()
{
  if (!m_addon)
    return ADDON_STATUS_UNKNOWN;

  CSingleLock lock(m_cdSec);

  ADDON_STATUS status = m_addon->CreateInstance(&m_ifc);
  if (status != ADDON_STATUS_OK)
  {
    CLog::Log(LOGERROR,
              "IAddonInstanceHandler::{}: {} returned bad status \"{}\" during instance creation",
              __FUNCTION__, m_addon->ID(), kodi::TranslateAddonStatus(status));
  }
  return status;
}

void IAddonInstanceHandler::DestroyInstance()
{
  CSingleLock lock(m_cdSec);
  if (m_addon)
    m_addon->DestroyInstance(&m_ifc);
}

} /* namespace ADDON */
