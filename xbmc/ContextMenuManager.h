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

#include <utility>
#include <vector>

#include "addons/ContextMenuAddon.h"
#include "ContextMenuItem.h"
#include "dialogs/GUIDialogContextMenu.h"


using ContextMenuView = std::vector<std::shared_ptr<const IContextMenuItem>>;

class CContextMenuManager
{
public:
  static const CContextMenuItem MAIN;
  static const CContextMenuItem MANAGE;

  CContextMenuManager(ADDON::CAddonMgr& addonMgr);
  ~CContextMenuManager();
  static CContextMenuManager& GetInstance();

  void Init();

  ContextMenuView GetItems(const CFileItem& item, const CContextMenuItem& root = MAIN) const;

  ContextMenuView GetAddonItems(const CFileItem& item, const CContextMenuItem& root = MAIN) const;

  bool Unload(const ADDON::CContextMenuAddon& addon);

private:
  CContextMenuManager(const CContextMenuManager&);
  CContextMenuManager const& operator=(CContextMenuManager const&);

  bool IsVisible(
    const CContextMenuItem& menuItem,
    const CContextMenuItem& root,
    const CFileItem& fileItem) const;

  void ReloadAddonItems();
  void OnEvent(const ADDON::AddonEvent& event);

  ADDON::CAddonMgr& m_addonMgr;

  CCriticalSection m_criticalSection;
  std::vector<CContextMenuItem> m_addonItems;
  std::vector<std::shared_ptr<IContextMenuItem>> m_items;
};

namespace CONTEXTMENU
{
  /*!
   * Starts the context menu loop for a file item.
   * */
  bool ShowFor(const CFileItemPtr& fileItem, const CContextMenuItem& root=CContextMenuManager::MAIN);

  /*!
   * Shortcut for continuing the context menu loop from an exisiting menu item.
   */
  bool LoopFrom(const IContextMenuItem& menu, const CFileItemPtr& fileItem);
}