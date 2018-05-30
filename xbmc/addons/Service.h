/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#pragma once

#include "Addon.h"
#include "AddonEvents.h"
#include "threads/CriticalSection.h"

namespace ADDON
{
  class CService: public CAddon
  {
  public:
    static std::unique_ptr<CService> FromExtension(CAddonInfo addonInfo, const cp_extension_t* ext);

    explicit CService(CAddonInfo addonInfo) : CAddon(std::move(addonInfo)) {}
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

    void Stop(std::map<std::string, int>::value_type service);

    CAddonMgr& m_addonMgr;
    CCriticalSection m_criticalSection;
    /** add-on id -> script id */
    std::map<std::string, int> m_services;
  };
}
