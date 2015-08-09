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
#include "Addon.h"

class CContextMenuItem;
typedef struct cp_cfg_element_t cp_cfg_element_t;


namespace ADDON
{
  class CContextMenuAddon : public CAddon
  {
  public:
    CContextMenuAddon(const cp_extension_t *ext);
    CContextMenuAddon(const AddonProps &props);
    virtual ~CContextMenuAddon();

    std::vector<CContextMenuItem> GetItems();

  private:
    void ParseMenu(cp_cfg_element_t* elem, const std::string& parent, int& anonGroupCount);
    std::vector<CContextMenuItem> m_items;
  };

  typedef std::shared_ptr<CContextMenuAddon> ContextItemAddonPtr;
}
