#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#include "Addon.h"
#include "AddonEvents.h"


namespace ADDON
{

  enum class START_OPTION
  {
    STARTUP,
    LOGIN
  };

  class CService: public CAddon
  {
  public:
    static std::unique_ptr<CService> FromExtension(CAddonInfo addonInfo, const cp_extension_t* ext);

    explicit CService(CAddonInfo addonInfo) : CAddon(std::move(addonInfo)),
        m_startOption(START_OPTION::LOGIN) {}

    CService(CAddonInfo addonInfo, START_OPTION startOption);

    START_OPTION GetStartOption() { return m_startOption; }

  private:
    START_OPTION m_startOption;
  };

  class CServiceAddonManager
  {
  public:
    CServiceAddonManager(CAddonMgr& addonMgr);
    ~CServiceAddonManager();

    /**
     * Start all services.
     */
    void Start();

    /**
     * Start services that have start option 'startup'.
     */
    void StartBeforeLogin();

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

    void Stop(std::map<std::string, int>::value_type service);

    CAddonMgr& m_addonMgr;
    CCriticalSection m_criticalSection;
    /** add-on id -> script id */
    std::map<std::string, int> m_services;
  };
}
