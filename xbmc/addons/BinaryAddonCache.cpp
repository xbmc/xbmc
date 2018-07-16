/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BinaryAddonCache.h"
#include "AddonManager.h"
#include "ServiceBroker.h"
#include "threads/SingleLock.h"

namespace ADDON
{

const std::vector<TYPE> ADDONS_TO_CACHE = { ADDON_PVRDLL, ADDON_GAMEDLL };

CBinaryAddonCache::~CBinaryAddonCache()
{
  Deinit();
}

void CBinaryAddonCache::Init()
{
  CServiceBroker::GetAddonMgr().Events().Subscribe(this, &CBinaryAddonCache::OnEvent);
  Update();
}

void CBinaryAddonCache::Deinit()
{
  CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);
}

void CBinaryAddonCache::GetAddons(VECADDONS& addons, const TYPE& type)
{
  VECADDONS myAddons;
  GetInstalledAddons(myAddons, type);

  for (auto &addon : myAddons)
  {
    if (!CServiceBroker::GetAddonMgr().IsAddonDisabled(addon->ID()))
      addons.emplace_back(std::move(addon));
  }
}

void CBinaryAddonCache::GetDisabledAddons(VECADDONS& addons, const TYPE& type)
{
  VECADDONS myAddons;
  GetInstalledAddons(myAddons, type);

  for (auto &addon : myAddons)
  {
    if (CServiceBroker::GetAddonMgr().IsAddonDisabled(addon->ID()))
      addons.emplace_back(std::move(addon));
  }
}

void CBinaryAddonCache::GetInstalledAddons(VECADDONS& addons, const TYPE& type)
{
  CSingleLock lock(m_critSection);
  auto it = m_addons.find(type);
  if (it != m_addons.end())
    addons = it->second;
}

AddonPtr CBinaryAddonCache::GetAddonInstance(const std::string& strId, TYPE type)
{
  AddonPtr addon;

  CSingleLock lock(m_critSection);

  auto it = m_addons.find(type);
  if (it != m_addons.end())
  {
    VECADDONS& addons = it->second;
    auto itAddon = std::find_if(addons.begin(), addons.end(),
      [&strId](const AddonPtr& addon)
      {
        return addon->ID() == strId;
      });

    if (itAddon != addons.end())
      addon = *itAddon;
  }

  return addon;
}

void CBinaryAddonCache::OnEvent(const AddonEvent& event)
{
  if (typeid(event) == typeid(AddonEvents::Enabled) ||
      typeid(event) == typeid(AddonEvents::Disabled) ||
      typeid(event) == typeid(AddonEvents::ReInstalled))
  {
    for (auto &type : ADDONS_TO_CACHE)
    {
      if (CServiceBroker::GetAddonMgr().HasType(event.id, type))
      {
        Update();
        break;
      }
    }
  }
  else if (typeid(event) == typeid(AddonEvents::UnInstalled))
  {
    Update();
  }
}

void CBinaryAddonCache::Update()
{
  using AddonMap = std::multimap<TYPE, VECADDONS>;
  AddonMap addonmap;

  for (auto &addonType : ADDONS_TO_CACHE)
  {
    VECADDONS addons;
    CServiceBroker::GetAddonMgr().GetInstalledAddons(addons, addonType);
    addonmap.insert(AddonMap::value_type(addonType, addons));
  }

  {
    CSingleLock lock(m_critSection);
    m_addons = std::move(addonmap);
  }
}

}
