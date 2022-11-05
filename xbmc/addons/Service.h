/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/Addon.h"
#include "addons/AddonEvents.h"
#include "threads/CriticalSection.h"

namespace ADDON
{
  class CService: public CAddon
  {
  public:
    explicit CService(const AddonInfoPtr& addonInfo);
  };

  class CServiceAddonManager
  {
  public:
    explicit CServiceAddonManager(CAddonMgr& addonMgr);
    ~CServiceAddonManager();

    /**
     * Start all services.
     */
    void Start();

    /**
     * Start service by add-on id.
     */
    void Start(const AddonPtr& addon);
    void Start(const std::string& addonId);

    /**
     * Stop all services.
     */
    void Stop();

    /**
     * Stop service by add-on id.
     */
    void Stop(const std::string& addonId);

  private:
    void OnEvent(const AddonEvent& event);

    void Stop(const std::map<std::string, int>::value_type& service);

    CAddonMgr& m_addonMgr;
    CCriticalSection m_criticalSection;
    /** add-on id -> script id */
    std::map<std::string, int> m_services;
  };
}
