/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BinaryAddonManager.h"

#include "BinaryAddonBase.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

using namespace ADDON;

BinaryAddonBasePtr CBinaryAddonManager::GetAddonBase(const AddonInfoPtr& addonInfo,
                                                     IAddonInstanceHandler* handler,
                                                     AddonDllPtr& addon)
{
  CSingleLock lock(m_critSection);

  BinaryAddonBasePtr addonBase;

  const auto& addonInstances = m_runningAddons.find(addonInfo->ID());
  if (addonInstances != m_runningAddons.end())
  {
    addonBase = addonInstances->second;
  }
  else
  {
    addonBase = std::make_shared<CBinaryAddonBase>(addonInfo);

    m_runningAddons.emplace(addonInfo->ID(), addonBase);
  }

  if (addonBase)
  {
    addon = addonBase->GetAddon(handler);
  }
  if (!addon)
  {
    CLog::Log(LOGFATAL, "CBinaryAddonManager::%s: Tried to get add-on '%s' who not available!",
              __func__, addonInfo->ID().c_str());
  }

  return addonBase;
}

void CBinaryAddonManager::ReleaseAddonBase(const BinaryAddonBasePtr& addonBase,
                                           IAddonInstanceHandler* handler)
{
  const auto& addon = m_runningAddons.find(addonBase->ID());
  if (addon == m_runningAddons.end())
    return;

  addonBase->ReleaseAddon(handler);

  if (addonBase->UsedInstanceCount() > 0)
    return;

  m_runningAddons.erase(addon);
}

AddonPtr CBinaryAddonManager::GetRunningAddon(const std::string& addonId) const
{
  CSingleLock lock(m_critSection);

  auto addon = m_runningAddons.find(addonId);
  if (addon != m_runningAddons.end())
    return addon->second->GetActiveAddon();

  return nullptr;
}
