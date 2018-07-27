/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
    static std::unique_ptr<CWebinterface> FromExtension(CAddonInfo addonInfo, const cp_extension_t* ext);

    explicit CWebinterface(CAddonInfo addonInfo)
        : CAddon(std::move(addonInfo)),
          m_type(WebinterfaceTypeStatic),
          m_entryPoint(WEBINTERFACE_DEFAULT_ENTRY_POINT) {}
    CWebinterface(ADDON::CAddonInfo addonInfo, WebinterfaceType type, const std::string &entryPoint);

    WebinterfaceType GetType() const { return m_type; }
    const std::string& EntryPoint() const { return m_entryPoint; }

    std::string GetEntryPoint(const std::string &path) const;
    std::string GetBaseLocation() const;

  private:
    WebinterfaceType m_type;
    std::string m_entryPoint;
  };
}
