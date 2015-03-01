#pragma once
/*
 *      Copyright (C) 2015 Team XBMC
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

#define WEBINTERFACE_DEFAULT_ENTRY_POINT  "index.html"

namespace ADDON
{
  typedef enum WebinterfaceType
  {
    WebinterfaceTypeStatic = 0,
    WebinterfaceTypeWsgi
  } WebinterfaceType;

  class CWebinterface : public CAddon
  {
  public:
    explicit CWebinterface(const ADDON::AddonProps &props, WebinterfaceType type = WebinterfaceTypeStatic, const std::string &entryPoint = WEBINTERFACE_DEFAULT_ENTRY_POINT);
    explicit CWebinterface(const cp_extension_t *ext);
    virtual ~CWebinterface();

    WebinterfaceType GetType() const { return m_type; }
    const std::string& EntryPoint() const { return m_entryPoint; }

    std::string GetEntryPoint(const std::string &path) const;
    std::string GetBaseLocation() const;

    // specializations of CAddon
    virtual AddonPtr Clone() const;

  private:
    WebinterfaceType m_type;
    std::string m_entryPoint;
  };
}
