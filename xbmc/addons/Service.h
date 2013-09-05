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
#include "AddonDatabase.h"

namespace ADDON
{
  /**
   * Class CServiceManager - lightweight service manager. Responisbilities are
   * to start/stop service add-ons when they are enabled/disabled in the add-on
   * database.
   */
  class CServiceManager: public IAddonDatabaseCallback
  {
  protected:
    CServiceManager();
    virtual ~CServiceManager();

  public:
    static CServiceManager &Get();

    virtual void AddonDisabled(ADDON::AddonPtr addon);
    virtual void AddonEnabled(ADDON::AddonPtr addon, bool bDisabled);
  };

  class CService: public CAddon
  {
  public:

    enum TYPE
    {
      UNKNOWN,
      PYTHON
    };

    enum START_OPTION
    {
      STARTUP,
      LOGIN
    };

    CService(const cp_extension_t *ext);
    CService(const AddonProps &props);
    virtual AddonPtr Clone() const;

    bool Start();
    bool Stop();
    TYPE GetServiceType() { return m_type; }
    START_OPTION GetStartOption() { return m_startOption; }

  protected:
    void BuildServiceType();

  private:
    TYPE m_type;
    START_OPTION m_startOption;
  };
}