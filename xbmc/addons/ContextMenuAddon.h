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

typedef struct cp_cfg_element_t cp_cfg_element_t;


namespace ADDON
{
  class CContextMenuAddon : public CAddon
  {
  public:
    static std::unique_ptr<CContextMenuAddon> FromExtension(const AddonInfoPtr& addonInfo, const cp_extension_t* ext);

    explicit CContextMenuAddon(const AddonInfoPtr& addonInfo) : CAddon(addonInfo) {}
    CContextMenuAddon(const AddonInfoPtr& addonInfo, std::vector<CContextMenuItem> items);

    const std::vector<CContextMenuItem>& GetItems() const { return m_items; };

  private:
    static void ParseMenu(const AddonInfoPtr& addonInfo, cp_cfg_element_t* elem, const std::string& parent,
        int& anonGroupCount, std::vector<CContextMenuItem>& items);

    std::vector<CContextMenuItem> m_items;
  };
}
