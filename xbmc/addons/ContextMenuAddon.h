/*
 *      Copyright (C) 2013-2015 Team XBMC
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
