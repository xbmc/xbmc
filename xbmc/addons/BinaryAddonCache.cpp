/*
 *      Copyright (C) 2016 Team Kodi
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
  if (typeid(event) == typeid(AddonEvents::ReInstalled) ||
      typeid(event) == typeid(AddonEvents::UnInstalled))
  {
    Update();
  }
  else if (typeid(event) == typeid(AddonEvents::Enabled) ||
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
