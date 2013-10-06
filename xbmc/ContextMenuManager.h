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
#include "addons/ContextItemAddon.h"
#include "dialogs/GUIDialogContextMenu.h"

#define CONTEXT_MENU_GROUP_MANAGE "kodi.core.manage"

class CContextMenuManager
{
public:
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
   * \param parent - the ID of the context menu. Empty string if the root menu.
   * CONTEXT_MENU_GROUP_MANAGE if the 'manage' submenu.
   */
  void AddVisibleItems(const CFileItemPtr& item, CContextButtons& list, const std::string& parent = "");

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

  /*!
   * \brief Get a context menu item by its assigned id.
   * \param id - the button id of the context item.
   * \return the addon or NULL if no item with given id is registered.
   */
  ADDON::ContextItemAddonPtr GetContextItemByID(const unsigned int id);

  std::map<unsigned int, ADDON::ContextItemAddonPtr> m_contextAddons;
  unsigned int m_iCurrentContextId;
};
