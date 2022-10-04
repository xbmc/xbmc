/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <memory>
#include <string>
#include <unordered_set>

namespace ADDON
{

  class IAddonInstanceHandler;

  class CAddonInfo;
  using AddonInfoPtr = std::shared_ptr<CAddonInfo>;

  class CAddonDll;
  typedef std::shared_ptr<CAddonDll> AddonDllPtr;

  class CBinaryAddonBase : public std::enable_shared_from_this<CBinaryAddonBase>
  {
  public:
    explicit CBinaryAddonBase(const AddonInfoPtr& addonInfo) : m_addonInfo(addonInfo) { }

    const std::string& ID() const;

    AddonDllPtr GetAddon(IAddonInstanceHandler* handler);
    void ReleaseAddon(IAddonInstanceHandler* handler);
    size_t UsedInstanceCount() const;

    AddonDllPtr GetActiveAddon();

    void OnPreInstall();
    void OnPostInstall(bool update, bool modal);
    void OnPreUnInstall();
    void OnPostUnInstall();

  private:
    AddonInfoPtr m_addonInfo;

    mutable CCriticalSection m_critSection;
    AddonDllPtr m_activeAddon;
    std::unordered_set<IAddonInstanceHandler*> m_activeAddonHandlers;
  };

} /* namespace ADDON */
