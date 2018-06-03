#pragma once
/*
 *      Copyright (C) 2013-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "Addon.h"
#include "ContextMenuItem.h"

typedef struct cp_cfg_element_t cp_cfg_element_t;


namespace ADDON
{
  class CContextMenuAddon : public CAddon
  {
  public:
    static std::unique_ptr<CContextMenuAddon> FromExtension(CAddonInfo addonInfo, const cp_extension_t* ext);

    explicit CContextMenuAddon(CAddonInfo addonInfo) : CAddon(std::move(addonInfo)) {}
    CContextMenuAddon(CAddonInfo addonInfo, std::vector<CContextMenuItem> items);

    const std::vector<CContextMenuItem>& GetItems() const { return m_items; };

  private:
    static void ParseMenu(const CAddonInfo& addonInfo, cp_cfg_element_t* elem, const std::string& parent,
        int& anonGroupCount, std::vector<CContextMenuItem>& items);

    std::vector<CContextMenuItem> m_items;
  };
}
