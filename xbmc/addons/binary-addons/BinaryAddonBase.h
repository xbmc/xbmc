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

    const std::string& ID() const { return m_addonInfo->ID(); }
    const std::string& Path() const { return m_addonInfo->Path(); }

    TYPE MainType() const { return m_addonInfo->MainType(); }
    const std::string& MainLibName() const { return m_addonInfo->LibName(); }

    bool IsType(TYPE type) const { return m_addonInfo->IsType(type); }
    const std::vector<CAddonType>& Types() const { return m_addonInfo->Types(); }
    const CAddonType* Type(TYPE type) const { return m_addonInfo->Type(type); }

    const AddonVersion& Version() const { return m_addonInfo->Version(); }
    const AddonVersion& MinVersion() const { return m_addonInfo->MinVersion(); }
    const std::string& Name() const { return m_addonInfo->Name(); }
    const std::string& Summary() const { return m_addonInfo->Summary(); }
    const std::string& Description() const { return m_addonInfo->Description(); }
    const std::string& Author() const { return m_addonInfo->Author(); }
    const std::string& ChangeLog() const { return m_addonInfo->ChangeLog(); }
    const std::string& Icon() const { return m_addonInfo->Icon(); }
    const ArtMap& Art() const { return m_addonInfo->Art(); }
    const std::string& Disclaimer() const { return m_addonInfo->Disclaimer(); }

    bool MeetsVersion(const AddonVersion& version) const { return m_addonInfo->MeetsVersion(version); }

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
