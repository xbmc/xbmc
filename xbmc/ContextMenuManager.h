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

#include <map>
#include "ContextMenuItem.h"
#include "addons/ContextItemAddon.h"
#include "dialogs/GUIDialogContextMenu.h"


class CContextMenuManager
{
public:
  static const CContextMenuItem MAIN;
  static const CContextMenuItem MANAGE;

  static CContextMenuManager& Get();

  /*!
   * \brief Executes a context menu item.
   * \param id - id of the context button to execute.
   * \param item - the currently selected item.
   * \return true if executed successfully, false otherwise
   */
  bool Execute(unsigned int id, const CFileItemPtr& item);

  /*!
   * \brief Adds all registered context item to the list.
   * \param item - the currently selected item.
   * \param list - the context menu.
   * \param root - the context menu responsible for this call.
   */
  void AddVisibleItems(
    const CFileItemPtr& item,
    CContextButtons& list,
    const CContextMenuItem& root = MAIN);

  /*!
   * \brief Adds a context item to this manager.
   * NOTE: only 'enabled' context addons should be added.
   */
  void Register(const ADDON::ContextItemAddonPtr& cm);

  /*!
   * \brief Removes a context addon from this manager.
   */
  bool Unregister(const ADDON::ContextItemAddonPtr& cm);

private:
  CContextMenuManager();
  CContextMenuManager(const CContextMenuManager&);
  CContextMenuManager const& operator=(CContextMenuManager const&);
  virtual ~CContextMenuManager() {}

  void Init();
  bool IsVisible(
    const CContextMenuItem& menuItem,
    const CContextMenuItem& root,
    const CFileItemPtr& fileItem);

  std::map<unsigned int, CContextMenuItem> m_items;
  unsigned int m_iCurrentContextId;
};
