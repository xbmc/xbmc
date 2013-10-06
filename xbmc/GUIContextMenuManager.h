#pragma once
/*
 *      Copyright (C) 2013-2014 Team XBMC
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

#include <boost/shared_ptr.hpp>
#include <list>
#include <boost/noncopyable.hpp>
#include "addons/ContextItemAddon.h"

class CContextButtons;

class ContextMenuManager
{
public:
  virtual ~ContextMenuManager() {};
  /*! Get a context menu item (or a category) by its addon id.
    Goes trough all subcategories recursively
    \param string - the addons string id to find
    \return the addon or NULL
  */
  ADDON::ContextAddonPtr GetContextItemByID(const std::string& strID);
  /*! Get a context menu item (or a category) by its assigned message id.
     NOTE: does not go recursively through subcategories
   \param unsigned int - msg id of the context item
   \return the addon or NULL
   */
  ADDON::ContextAddonPtr GetContextItemByID(const unsigned int ID);
  /*!
   Adds all registered context item to the list
   \param context - the current window id
   \param item - the currently selected item
   \param out visible - appends all visible menu items to this list
   */
  void AppendVisibleContextItems(const CFileItemPtr item, CContextButtons& list);

  /*!
   \brief Adds a context item to this manager.
   NOTE: if a context item has changed, just register it again and it will overwrite the old one
   NOTE: only 'enabled' context addons should be added
   \param the context item to add
   \sa UnegisterContextItem
   */
  void RegisterContextItem(ADDON::ContextAddonPtr cm);
  /*!
   \brief Removes a context addon from this manager.
   \param the context item to remove
   \sa RegisterContextItem
   */
  bool UnregisterContextItem(ADDON::ContextAddonPtr cm);
protected:
  struct IDFinder : std::binary_function<ADDON::ContextAddonPtr, unsigned int, bool>
  {
    bool operator()(const ADDON::ContextAddonPtr& item, unsigned int id) const;
  };

  typedef std::vector<ADDON::ContextAddonPtr>::iterator contextIter;
  std::vector<ADDON::ContextAddonPtr> m_vecContextMenus;

  contextIter GetContextItemIterator(const unsigned int ID);
};

class BaseContextMenuManager : boost::noncopyable, public ContextMenuManager
{
public:
  /*!
   Returns the ContextMenuManager responsible for the root level of the context menu
   */
  static BaseContextMenuManager& Get();

  /*!
   \brief Unregisters the given Context Item from the ContextMenuManager where it is currently registered
   \param the context item to unregister
   \sa ContextMenuManager::UnregisterContextItem
   \sa Register
   */
  void Unregister(ADDON::ContextAddonPtr contextAddon);

  /*!
   \brief Finds out where the given Context Item belongs to and registers it to the appropriate ContextMenuManager.
   NOTE: if a context item has changed, just register it again and it will overwrite the old one
   NOTE: only 'enabled' context addons should be registered
   \param the context item to register
   \sa ContextMenuManager::RegisterContextItem
   \sa Unregister
   */
  void Register(ADDON::ContextAddonPtr contextAddon);
protected:
  void Init();
};
