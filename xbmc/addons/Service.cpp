/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "Service.h"

#include "AddonManager.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "utils/StringUtils.h"
#include "utils/log.h"


namespace ADDON
{

CServiceAddonManager::CServiceAddonManager(CAddonMgr& addonMgr) :
    m_addonMgr(addonMgr)
{
}

CServiceAddonManager::~CServiceAddonManager()
{
  m_addonMgr.Events().Unsubscribe(this);
}

void CServiceAddonManager::OnEvent(const ADDON::AddonEvent& event)
{
  if (typeid(event) == typeid(ADDON::AddonEvents::Enabled))
  {
    Start(event.id);
  }
  else if (typeid(event) == typeid(ADDON::AddonEvents::ReInstalled))
  {
    Stop(event.id);
    Start(event.id);
  }
  else if (typeid(event) == typeid(ADDON::AddonEvents::Disabled) ||
           typeid(event) == typeid(ADDON::AddonEvents::UnInstalled))
  {
    Stop(event.id);
  }
}

void CServiceAddonManager::Start()
{
  m_addonMgr.Events().Subscribe(this, &CServiceAddonManager::OnEvent);
  VECADDONS addons;
  if (m_addonMgr.GetAddons(addons, ADDON_SERVICE))
  {
    for (const auto& addon : addons)
    {
      Start(addon);
    }
  }
}

void CServiceAddonManager::Start(const std::string& addonId)
{
  AddonPtr addon;
  if (m_addonMgr.GetAddon(addonId, addon, ADDON_SERVICE))
  {
    Start(addon);
  }
}

void CServiceAddonManager::Start(const AddonPtr& addon)
{
  CSingleLock lock(m_criticalSection);
  if (m_services.find(addon->ID()) != m_services.end())
  {
    CLog::Log(LOGDEBUG, "CServiceAddonManager: %s already started.", addon->ID().c_str());
    return;
  }

  if (StringUtils::EndsWith(addon->LibPath(), ".py"))
  {
    CLog::Log(LOGDEBUG, "CServiceAddonManager: starting %s", addon->ID().c_str());
    auto handle = CScriptInvocationManager::GetInstance().ExecuteAsync(addon->LibPath(), addon);
    if (handle == -1)
    {
      CLog::Log(LOGERROR, "CServiceAddonManager: %s failed to start", addon->ID().c_str());
      return;
    }
    m_services[addon->ID()] = handle;
  }
}

void CServiceAddonManager::Stop()
{
  m_addonMgr.Events().Unsubscribe(this);
  CSingleLock lock(m_criticalSection);
  for (const auto& service : m_services)
  {
    Stop(service);
  }
  m_services.clear();
}

void CServiceAddonManager::Stop(const std::string& addonId)
{
  CSingleLock lock(m_criticalSection);
  auto it = m_services.find(addonId);
  if (it != m_services.end())
  {
    Stop(*it);
    m_services.erase(it);
  }
}

void CServiceAddonManager::Stop(std::map<std::string, int>::value_type service)
{
  CLog::Log(LOGDEBUG, "CServiceAddonManager: stopping %s.", service.first.c_str());
  if (!CScriptInvocationManager::GetInstance().Stop(service.second))
  {
    CLog::Log(LOGINFO, "CServiceAddonManager: failed to stop %s (may have ended)", service.first.c_str());
  }
}
}
