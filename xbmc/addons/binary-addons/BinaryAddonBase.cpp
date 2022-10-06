/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BinaryAddonBase.h"

#include "addons/addoninfo/AddonInfo.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/binary-addons/AddonInstanceHandler.h"
#include "utils/log.h"

#include <mutex>

using namespace ADDON;

const std::string& CBinaryAddonBase::ID() const
{
  return m_addonInfo->ID();
}

AddonDllPtr CBinaryAddonBase::GetAddon(IAddonInstanceHandler* handler)
{
  if (handler == nullptr)
  {
    CLog::Log(LOGERROR, "CBinaryAddonBase::{}: for Id '{}' called with empty instance handler",
              __FUNCTION__, ID());
    return nullptr;
  }

  std::unique_lock<CCriticalSection> lock(m_critSection);

  // If no 'm_activeAddon' is defined create it new.
  if (m_activeAddon == nullptr)
    m_activeAddon = std::make_shared<CAddonDll>(m_addonInfo, shared_from_this());

  // add the instance handler to the info to know used amount on addon
  m_activeAddonHandlers.insert(handler);

  return m_activeAddon;
}

void CBinaryAddonBase::ReleaseAddon(IAddonInstanceHandler* handler)
{
  if (handler == nullptr)
  {
    CLog::Log(LOGERROR, "CBinaryAddonBase::{}: for Id '{}' called with empty instance handler",
              __FUNCTION__, ID());
    return;
  }

  std::unique_lock<CCriticalSection> lock(m_critSection);

  auto presentHandler = m_activeAddonHandlers.find(handler);
  if (presentHandler == m_activeAddonHandlers.end())
    return;

  m_activeAddonHandlers.erase(presentHandler);

  // if no handler is present anymore reset and delete the add-on class on information
  if (m_activeAddonHandlers.empty())
  {
    m_activeAddon.reset();
  }
}

size_t CBinaryAddonBase::UsedInstanceCount() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_activeAddonHandlers.size();
}

AddonDllPtr CBinaryAddonBase::GetActiveAddon()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_activeAddon;
}

void CBinaryAddonBase::OnPreInstall()
{
  const std::unordered_set<IAddonInstanceHandler*> activeAddonHandlers = m_activeAddonHandlers;
  for (const auto& instance : activeAddonHandlers)
    instance->OnPreInstall();
}

void CBinaryAddonBase::OnPostInstall(bool update, bool modal)
{
  const std::unordered_set<IAddonInstanceHandler*> activeAddonHandlers = m_activeAddonHandlers;
  for (const auto& instance : activeAddonHandlers)
    instance->OnPostInstall(update, modal);
}

void CBinaryAddonBase::OnPreUnInstall()
{
  const std::unordered_set<IAddonInstanceHandler*> activeAddonHandlers = m_activeAddonHandlers;
  for (const auto& instance : activeAddonHandlers)
    instance->OnPreUnInstall();
}

void CBinaryAddonBase::OnPostUnInstall()
{
  const std::unordered_set<IAddonInstanceHandler*> activeAddonHandlers = m_activeAddonHandlers;
  for (const auto& instance : activeAddonHandlers)
    instance->OnPostUnInstall();
}
