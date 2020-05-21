/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonInstanceHandler.h"
#include "addons/addoninfo/AddonInfo.h"
#include "threads/CriticalSection.h"

#include <memory>
#include <string>
#include <unordered_set>

namespace ADDON
{

  class IAddonInstanceHandler;

  class CAddonDll;
  typedef std::shared_ptr<CAddonDll> AddonDllPtr;

  class CBinaryAddonBase : public std::enable_shared_from_this<CBinaryAddonBase>
  {
  public:
    explicit CBinaryAddonBase(const AddonInfoPtr& addonInfo) : m_addonInfo(addonInfo) { }

    const std::string& ID() const;
    const std::string& Path() const;

    TYPE MainType() const;
    const std::string& MainLibName() const;

    bool HasType(TYPE type) const;
    const std::vector<CAddonType>& Types() const;
    const CAddonType* Type(TYPE type) const;

    const AddonVersion& Version() const;
    const AddonVersion& MinVersion() const;
    const AddonVersion& DependencyVersion(const std::string& dependencyID) const;
    const std::string& Name() const;
    const std::string& Summary() const;
    const std::string& Description() const;
    const std::string& Author() const;
    const std::string& ChangeLog() const;
    const std::string& Icon() const;
    const ArtMap& Art() const;
    const std::string& Disclaimer() const;

    bool MeetsVersion(const AddonVersion& versionMin, const AddonVersion& version) const;

    AddonDllPtr GetAddon(const IAddonInstanceHandler* handler);
    void ReleaseAddon(const IAddonInstanceHandler* handler);

    AddonDllPtr GetActiveAddon();

  private:
    AddonInfoPtr m_addonInfo;

    CCriticalSection m_critSection;
    AddonDllPtr m_activeAddon;
    std::unordered_set<const IAddonInstanceHandler*> m_activeAddonHandlers;
  };

} /* namespace ADDON */
