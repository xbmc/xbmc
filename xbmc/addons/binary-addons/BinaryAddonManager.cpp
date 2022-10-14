/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BinaryAddonManager.h"

#include "addons/addoninfo/AddonInfo.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/binary-addons/BinaryAddonBase.h"
#include "utils/log.h"

#include <mutex>

using namespace ADDON;

BinaryAddonBasePtr CBinaryAddonManager::GetAddonBase(const AddonInfoPtr& addonInfo,
                                                     IAddonInstanceHandler* handler,
                                                     AddonDllPtr& addon)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

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
    CLog::Log(LOGFATAL, "CBinaryAddonManager::{}: Tried to get add-on '{}' who not available!",
              __func__, addonInfo->ID());
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

BinaryAddonBasePtr CBinaryAddonManager::GetRunningAddonBase(const std::string& addonId) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  const auto& addonInstances = m_runningAddons.find(addonId);
  if (addonInstances != m_runningAddons.end())
    return addonInstances->second;

  return nullptr;
}

AddonPtr CBinaryAddonManager::GetRunningAddon(const std::string& addonId) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  const BinaryAddonBasePtr base = GetRunningAddonBase(addonId);
  if (base)
    return base->GetActiveAddon();

  return nullptr;
}
