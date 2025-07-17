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

const std::vector<AddonType> ADDONS_TO_CACHE = {AddonType::GAMEDLL, AddonType::SHADERDLL};

CBinaryAddonCache::~CBinaryAddonCache()
{
  Deinit();
}

void CBinaryAddonCache::Init()
{
  CServiceBroker::GetAddonMgr().Events().Subscribe(
      this,
      [this](const AddonEvent& event)
      {
        if (typeid(event) == typeid(AddonEvents::Enabled) ||
            typeid(event) == typeid(AddonEvents::Disabled) ||
            typeid(event) == typeid(AddonEvents::ReInstalled))
        {
          for (auto& type : ADDONS_TO_CACHE)
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
      });

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
  std::unique_lock lock(m_critSection);
  auto it = m_addons.find(type);
  if (it != m_addons.end())
    addons = it->second;
}

AddonPtr CBinaryAddonCache::GetAddonInstance(const std::string& strId, AddonType type)
{
  AddonPtr addon;

  std::unique_lock lock(m_critSection);

  auto it = m_addons.find(type);
  if (it != m_addons.end())
  {
    const VECADDONS& addons = it->second;
    const auto itAddon =
        std::ranges::find_if(addons, [&strId](const AddonPtr& a) { return a->ID() == strId; });

    if (itAddon != addons.end())
      addon = *itAddon;
  }

  return addon;
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
    std::unique_lock lock(m_critSection);
    m_addons = std::move(addonmap);
  }
}

}
