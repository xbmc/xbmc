/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "Service.h"

#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <mutex>


namespace ADDON
{

CService::CService(const AddonInfoPtr& addonInfo) : CAddon(addonInfo, AddonType::SERVICE)
{
}

CServiceAddonManager::CServiceAddonManager(CAddonMgr& addonMgr) :
    m_addonMgr(addonMgr)
{
}

CServiceAddonManager::~CServiceAddonManager()
{
  m_addonMgr.Events().Unsubscribe(this);
  m_addonMgr.UnloadEvents().Unsubscribe(this);
}

void CServiceAddonManager::OnEvent(const ADDON::AddonEvent& event)
{
  if (typeid(event) == typeid(ADDON::AddonEvents::Enabled))
  {
    Start(event.addonId);
  }
  else if (typeid(event) == typeid(ADDON::AddonEvents::ReInstalled))
  {
    Stop(event.addonId);
    Start(event.addonId);
  }
  else if (typeid(event) == typeid(ADDON::AddonEvents::Disabled) ||
           typeid(event) == typeid(ADDON::AddonEvents::Unload))
  {
    Stop(event.addonId);
  }
}

void CServiceAddonManager::Start()
{
  m_addonMgr.Events().Subscribe(this, &CServiceAddonManager::OnEvent);
  m_addonMgr.UnloadEvents().Subscribe(this, &CServiceAddonManager::OnEvent);
  VECADDONS addons;
  if (m_addonMgr.GetAddons(addons, AddonType::SERVICE))
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
  if (m_addonMgr.GetAddon(addonId, addon, AddonType::SERVICE, OnlyEnabled::CHOICE_YES))
  {
    Start(addon);
  }
}

void CServiceAddonManager::Start(const AddonPtr& addon)
{
  std::unique_lock<CCriticalSection> lock(m_criticalSection);
  if (m_services.find(addon->ID()) != m_services.end())
  {
    CLog::Log(LOGDEBUG, "CServiceAddonManager: {} already started.", addon->ID());
    return;
  }

  if (StringUtils::EndsWith(addon->LibPath(), ".py"))
  {
    CLog::Log(LOGDEBUG, "CServiceAddonManager: starting {}", addon->ID());
    auto handle = CScriptInvocationManager::GetInstance().ExecuteAsync(addon->LibPath(), addon);
    if (handle == -1)
    {
      CLog::Log(LOGERROR, "CServiceAddonManager: {} failed to start", addon->ID());
      return;
    }
    m_services[addon->ID()] = handle;
  }
}

void CServiceAddonManager::Stop()
{
  m_addonMgr.Events().Unsubscribe(this);
  m_addonMgr.UnloadEvents().Unsubscribe(this);
  std::unique_lock<CCriticalSection> lock(m_criticalSection);
  for (const auto& service : m_services)
  {
    Stop(service);
  }
  m_services.clear();
}

void CServiceAddonManager::Stop(const std::string& addonId)
{
  std::unique_lock<CCriticalSection> lock(m_criticalSection);
  auto it = m_services.find(addonId);
  if (it != m_services.end())
  {
    Stop(*it);
    m_services.erase(it);
  }
}

void CServiceAddonManager::Stop(const std::map<std::string, int>::value_type& service)
{
  CLog::Log(LOGDEBUG, "CServiceAddonManager: stopping {}.", service.first);
  if (!CScriptInvocationManager::GetInstance().Stop(service.second))
  {
    CLog::Log(LOGINFO, "CServiceAddonManager: failed to stop {} (may have ended)", service.first);
  }
}
}
