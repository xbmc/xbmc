#pragma once
/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
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

    CService(const cp_extension_t *ext);
    CService(const AddonProps &props);

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