/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonInstanceHandler.h"
#include "BinaryAddonType.h"

#include "addons/AddonInfo.h"
#include "threads/CriticalSection.h"

#include <memory>
#include <string>
#include <unordered_set>

class TiXmlElement;

namespace ADDON
{

  class IAddonInstanceHandler;

  class CAddonDll;
  typedef std::shared_ptr<CAddonDll> AddonDllPtr;

  class CBinaryAddonBase : public std::enable_shared_from_this<CBinaryAddonBase>
  {
  public:
    explicit CBinaryAddonBase(const CAddonInfo& addonInfo) : m_addonInfo(addonInfo) { }

    bool Create();

    const std::string& ID() const { return m_addonInfo.ID(); }
    const std::string& Path() const { return m_addonInfo.Path(); }

    TYPE MainType() const { return m_addonInfo.MainType(); }
    const std::string& MainLibName() const { return m_addonInfo.LibName(); }

    bool IsType(TYPE type) const;
    const std::vector<CBinaryAddonType>& Types() const { return m_types; }
    const CBinaryAddonType* Type(TYPE type) const;

    const AddonVersion& Version() const { return m_addonInfo.Version(); }
    const AddonVersion& MinVersion() const { return m_addonInfo.MinVersion(); }
    const std::string& Name() const { return m_addonInfo.Name(); }
    const std::string& Summary() const { return m_addonInfo.Summary(); }
    const std::string& Description() const { return m_addonInfo.Description(); }
    const std::string& Author() const { return m_addonInfo.Author(); }
    const std::string& ChangeLog() const { return m_addonInfo.ChangeLog(); }
    const std::string& Icon() const { return m_addonInfo.Icon(); }
    const ArtMap& Art() const { return m_addonInfo.Art(); }
    const std::string& Disclaimer() const { return m_addonInfo.Disclaimer(); }

    bool ProvidesSubContent(const TYPE& content, const TYPE& mainType = ADDON_UNKNOWN) const;
    bool ProvidesSeveralSubContents() const;

    bool MeetsVersion(const AddonVersion &version) const;

    AddonDllPtr GetAddon(const IAddonInstanceHandler* handler);
    void ReleaseAddon(const IAddonInstanceHandler* handler);

    AddonDllPtr GetActiveAddon();

  private:
    bool LoadAddonXML(const TiXmlElement* element, const std::string& addonPath);

    CAddonInfo m_addonInfo;
    std::vector<CBinaryAddonType> m_types;

    CCriticalSection m_critSection;
    AddonDllPtr m_activeAddon;
    std::unordered_set<const IAddonInstanceHandler*> m_activeAddonHandlers;
  };

} /* namespace ADDON */
