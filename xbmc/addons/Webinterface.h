/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/Addon.h"

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
    explicit CWebinterface(const AddonInfoPtr& addonInfo);

    WebinterfaceType GetType() const { return m_type; }
    const std::string& EntryPoint() const { return m_entryPoint; }

    std::string GetEntryPoint(const std::string &path) const;
    std::string GetBaseLocation() const;

  private:
    WebinterfaceType m_type = WebinterfaceTypeStatic;
    std::string m_entryPoint;
  };
}
