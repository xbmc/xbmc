/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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
#include "Service.h"
#include "AddonManager.h"
#include "ServiceBroker.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "utils/log.h"
#include "utils/StringUtils.h"


namespace ADDON
{

std::unique_ptr<CService> CService::FromExtension(CAddonInfo addonInfo, const cp_extension_t* ext)
{
  return std::unique_ptr<CService>(new CService(std::move(addonInfo)));
}

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
