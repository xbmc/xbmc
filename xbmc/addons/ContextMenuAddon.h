#pragma once
/*
 *      Copyright (C) 2013-2015 Team XBMC
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
    static std::unique_ptr<CContextMenuAddon> FromExtension(AddonProps props, const cp_extension_t* ext);

    explicit CContextMenuAddon(AddonProps props) : CAddon(std::move(props)) {}
    CContextMenuAddon(AddonProps props, std::vector<CContextMenuItem> items);

    std::vector<CContextMenuItem> GetItems() const;

  private:
    static void ParseMenu(const AddonProps& props, cp_cfg_element_t* elem, const std::string& parent,
        int& anonGroupCount, std::vector<CContextMenuItem>& items);

    std::vector<CContextMenuItem> m_items;
  };

  typedef std::shared_ptr<const CContextMenuAddon> ContextItemAddonPtr;
}
