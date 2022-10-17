/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BinaryAddonCache.h"

#include "ServiceBroker.h"
#include "addons/AddonEvents.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"

#include <mutex>

namespace ADDON
{

const std::vector<AddonType> ADDONS_TO_CACHE = {AddonType::GAMEDLL};

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

void CBinaryAddonCache::GetAddons(VECADDONS& addons, AddonType type)
{
  VECADDONS myAddons;
  GetInstalledAddons(myAddons, type);

  for (auto &addon : myAddons)
  {
    if (!CServiceBroker::GetAddonMgr().IsAddonDisabled(addon->ID()))
      addons.emplace_back(std::move(addon));
  }
}

void CBinaryAddonCache::GetDisabledAddons(VECADDONS& addons, AddonType type)
{
  VECADDONS myAddons;
  GetInstalledAddons(myAddons, type);

  for (auto &addon : myAddons)
  {
    if (CServiceBroker::GetAddonMgr().IsAddonDisabled(addon->ID()))
      addons.emplace_back(std::move(addon));
  }
}

void CBinaryAddonCache::GetInstalledAddons(VECADDONS& addons, AddonType type)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  auto it = m_addons.find(type);
  if (it != m_addons.end())
    addons = it->second;
}

AddonPtr CBinaryAddonCache::GetAddonInstance(const std::string& strId, AddonType type)
{
  AddonPtr addon;

  std::unique_lock<CCriticalSection> lock(m_critSection);

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
      if (CServiceBroker::GetAddonMgr().HasType(event.addonId, type))
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
  using AddonMap = std::multimap<AddonType, VECADDONS>;
  AddonMap addonmap;

  for (auto &addonType : ADDONS_TO_CACHE)
  {
    VECADDONS addons;
    CServiceBroker::GetAddonMgr().GetInstalledAddons(addons, addonType);
    addonmap.insert(AddonMap::value_type(addonType, addons));
  }

  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_addons = std::move(addonmap);
  }
}

}
