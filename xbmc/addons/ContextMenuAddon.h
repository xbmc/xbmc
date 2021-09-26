/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Addon.h"
#include "ContextMenuItem.h"

#include <list>
#include <memory>
#include <string>
#include <vector>

namespace ADDON
{
  class CContextMenuAddon : public CAddon
  {
  public:
    explicit CContextMenuAddon(const AddonInfoPtr& addonInfo);

    const std::vector<CContextMenuItem>& GetItems() const { return m_items; }

  private:
    void ParseMenu(const CAddonExtensions* elem, const std::string& parent, int& anonGroupCount);
    std::vector<CContextMenuItem> m_items;
  };
}
