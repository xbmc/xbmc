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

namespace ADDON
{

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

    static std::unique_ptr<CService> FromExtension(CAddonInfo addonInfo, const cp_extension_t* ext);

    explicit CService(CAddonInfo addonInfo) : CAddon(std::move(addonInfo)), m_type(UNKNOWN), m_startOption(LOGIN) {}
    CService(CAddonInfo addonInfo, TYPE type, START_OPTION startOption);

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
