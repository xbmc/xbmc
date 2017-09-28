/*
 *      Copyright (C) 2005-2017 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "BinaryAddonManager.h"
#include "BinaryAddonBase.h"

#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/Directory.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

using namespace ADDON;

CBinaryAddonManager::CBinaryAddonManager()
  : m_tempAddonBasePath("special://temp/binary-addons")
{
}

CBinaryAddonManager::~CBinaryAddonManager()
{
  DeInit();
}

bool CBinaryAddonManager::Init()
{
  CServiceBroker::GetAddonMgr().Events().Subscribe(this, &CBinaryAddonManager::OnEvent);
  CServiceBroker::GetAddonMgr().UnloadEvents().Subscribe(this, &CBinaryAddonManager::OnEvent);

  BINARY_ADDON_LIST binaryAddonList;
  if (!CServiceBroker::GetAddonMgr().GetInstalledBinaryAddons(binaryAddonList))
  {
    CLog::Log(LOGNOTICE, "CBinaryAddonManager::%s: No binary addons present and related manager, init not necessary", __FUNCTION__);
    return true;
  }

  CSingleLock lock(m_critSection);

  for (auto addon : binaryAddonList)
    AddAddonBaseEntry(addon);

  return true;
}

void CBinaryAddonManager::DeInit()
{
  /* If temporary directory was used from addon delete them */
  if (XFILE::CDirectory::Exists(m_tempAddonBasePath))
    XFILE::CDirectory::RemoveRecursive(CSpecialProtocol::TranslatePath(m_tempAddonBasePath));

  CServiceBroker::GetAddonMgr().UnloadEvents().Unsubscribe(this);
  CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);
}

bool CBinaryAddonManager::HasInstalledAddons(const TYPE &type) const
{
  CSingleLock lock(m_critSection);
  for (auto info : m_installedAddons)
  {
    if (info.second->IsType(type))
      return true;
  }
  return false;
}

bool CBinaryAddonManager::HasEnabledAddons(const TYPE &type) const
{
  CSingleLock lock(m_critSection);
  for (auto info : m_enabledAddons)
  {
    if (info.second->IsType(type))
      return true;
  }
  return false;
}

bool CBinaryAddonManager::IsAddonInstalled(const std::string& addonId, const TYPE &type/* = ADDON_UNKNOWN*/)
{
  CSingleLock lock(m_critSection);
  return (m_installedAddons.find(addonId) != m_installedAddons.end());
}

bool CBinaryAddonManager::IsAddonEnabled(const std::string& addonId, const TYPE &type/* = ADDON_UNKNOWN*/)
{
  CSingleLock lock(m_critSection);
  return (m_enabledAddons.find(addonId) != m_enabledAddons.end());
}

void CBinaryAddonManager::GetAddonInfos(BinaryAddonBaseList& addonInfos, bool enabledOnly, const TYPE &type) const
{
  CSingleLock lock(m_critSection);

  const BinaryAddonMgrBaseList* addons;
  if (enabledOnly)
    addons = &m_enabledAddons;
  else
    addons = &m_installedAddons;

  for (auto info : *addons)
  {
    if (type == ADDON_UNKNOWN || info.second->IsType(type))
    {
      addonInfos.push_back(info.second);
    }
  }
}

void CBinaryAddonManager::GetDisabledAddonInfos(BinaryAddonBaseList& addonInfos, const TYPE& type)
{
  CSingleLock lock(m_critSection);

  for (auto info : m_installedAddons)
  {
    if (type == ADDON_UNKNOWN || info.second->IsType(type))
    {
      if (!IsAddonEnabled(info.second->ID(), type))
        addonInfos.push_back(info.second);
    }
  }
}

const BinaryAddonBasePtr CBinaryAddonManager::GetInstalledAddonInfo(const std::string& addonId, const TYPE &type/* = ADDON_UNKNOWN*/) const
{
  CSingleLock lock(m_critSection);

  auto addon = m_installedAddons.find(addonId);
  if (addon != m_installedAddons.end() && (type == ADDON_UNKNOWN || addon->second->IsType(type)))
    return addon->second;

  CLog::Log(LOGERROR, "CBinaryAddonManager::%s: Requested addon '%s' unknown as binary", __FUNCTION__, addonId.c_str());
  return nullptr;
}

AddonPtr CBinaryAddonManager::GetRunningAddon(const std::string& addonId) const
{
  CSingleLock lock(m_critSection);

  auto addon = m_installedAddons.find(addonId);
  if (addon != m_installedAddons.end())
    return addon->second->GetActiveAddon();

  return nullptr;
}

bool CBinaryAddonManager::AddAddonBaseEntry(BINARY_ADDON_LIST_ENTRY& entry)
{
  BinaryAddonBasePtr base = std::make_shared<CBinaryAddonBase>(entry.second);
  if (!base->Create())
  {
    CLog::Log(LOGERROR, "CBinaryAddonManager::%s: Failed to create base for '%s' and addon not usable", __FUNCTION__, base->ID().c_str());
    return false;
  }

  m_installedAddons[base->ID()] = base;
  if (entry.first)
    m_enabledAddons[base->ID()] = base;
  return true;
}

void CBinaryAddonManager::OnEvent(const AddonEvent& event)
{
  if (typeid(event) == typeid(AddonEvents::Enabled))
  {
    EnableEvent(event.id);
  }
  else if (typeid(event) == typeid(AddonEvents::Disabled))
  {
    DisableEvent(event.id);
  }
  else if (typeid(event) == typeid(AddonEvents::Load) ||
           typeid(event) == typeid(AddonEvents::Unload))
  {
    InstalledChangeEvent();
  }
}

void CBinaryAddonManager::EnableEvent(const std::string& addonId)
{
  CSingleLock lock(m_critSection);

  BinaryAddonBasePtr base;
  auto addon = m_installedAddons.find(addonId);
  if (addon != m_installedAddons.end())
    base = addon->second;
  else
    return;

  CLog::Log(LOGDEBUG, "CBinaryAddonManager::%s: Enable addon '%s' on binary addon manager", __FUNCTION__, base->ID().c_str());
  m_enabledAddons[base->ID()] = base;
}

void CBinaryAddonManager::DisableEvent(const std::string& addonId)
{
  CSingleLock lock(m_critSection);

  BinaryAddonBasePtr base;
  auto addon = m_installedAddons.find(addonId);
  if (addon != m_installedAddons.end())
    base = addon->second;
  else
    return;

  CLog::Log(LOGDEBUG, "CBinaryAddonManager::%s: Disable addon '%s' on binary addon manager", __FUNCTION__, base->ID().c_str());
  m_enabledAddons.erase(base->ID());
}

void CBinaryAddonManager::InstalledChangeEvent()
{
  BINARY_ADDON_LIST binaryAddonList;
  CServiceBroker::GetAddonMgr().GetInstalledBinaryAddons(binaryAddonList);

  CSingleLock lock(m_critSection);

  BinaryAddonMgrBaseList deletedAddons = m_installedAddons;
  for (auto addon : binaryAddonList)
  {
    auto knownAddon = m_installedAddons.find(addon.second.ID());
    if (knownAddon == m_installedAddons.end())
    {
      CLog::Log(LOGDEBUG, "CBinaryAddonManager::%s: Adding new binary addon '%s'", __FUNCTION__, addon.second.ID().c_str());

      if (!AddAddonBaseEntry(addon))
        continue;
    }
    else
    {
      deletedAddons.erase(addon.second.ID());
    }
  }

  for (auto addon : deletedAddons)
  {
    CLog::Log(LOGDEBUG, "CBinaryAddonManager::%s: Removing binary addon '%s'", __FUNCTION__, addon.first.c_str());

    m_installedAddons.erase(addon.first);
    m_enabledAddons.erase(addon.first);
  }
}
