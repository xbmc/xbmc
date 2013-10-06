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

#include "Addon.h"
#include <list>

#define MANAGE_CATEGORY_NAME "xbmc.manage"

class CContextButtons;

class CFileItem;
typedef boost::shared_ptr<CFileItem> CFileItemPtr;

namespace INFO
{
  class InfoBool;
  typedef boost::shared_ptr<InfoBool> InfoPtr;
}

namespace ADDON
{
  class IContextItem;
  typedef boost::shared_ptr<IContextItem> ContextAddonPtr;

  class IContextItem : public CAddon
  {
  public:
    IContextItem(const cp_extension_t *ext);
    IContextItem(const AddonProps &props);
    virtual ~IContextItem();

    /*! \brief Returns the unique ID for this context item addons.
     This ID is unique even if you delete and recreate the same item. (Might change after restart)
     \return the unique ID
     */
    unsigned int GetMsgID() const { return m_id; }
    /*! \brief returns the label to show for this context item
     \return the label
     */
    const std::string& GetLabel() const { return m_label; }
    /*! \brief returns the parent category of this context item
        \return empty string if at root level or MANAGE_CATEGORY_NAME when it should be in the 'manage' submenu
             or the id of a ContextCategoryItem
     */
    const std::string& GetParent() const { return m_parent; }
    /*! \brief returns the item with the given addon id
     \param strID - the addons ID we are looking for
     \return either this, a child context item (only on ContextCategoryAddon) or NULL if not found
     */
    virtual ContextAddonPtr GetChildWithID(const std::string& strID) = 0;
    /*! \brief returns true if this item should be visible.
     NOTE:  defaults to true, if no visibility expression was set.
     \return true if this item should be visible
     */
    virtual bool IsVisible(const CFileItemPtr item) const = 0;
    /*! \brief Adds this element to the list, if this element is visible
     \param item - the currently selected item
     \param out visible - may be changed by adding this or a child element.
     */
    virtual void AddIfVisible(const CFileItemPtr item, CContextButtons &visible) = 0;
    /*! \brief executes the context menu item
     \param item - the currently selected item
     \return false if execution failed, aborted or isVisible() returned false
     */
    virtual bool Execute(const CFileItemPtr itemPath) = 0;

    /*! \brief Converts this item into a native context button
     \return a native context button
     */
    std::pair<unsigned int, std::string> ToNative();

    virtual bool OnPreInstall();
    virtual void OnPostInstall(bool restart, bool update);
    virtual void OnPreUnInstall();
    virtual void OnDisabled();
    virtual void OnEnabled();
  protected:
    std::string m_label;
    std::string m_parent;
    unsigned int m_id;
  };

  class CContextItemAddon : public IContextItem
  {
  public:
    CContextItemAddon(const cp_extension_t *ext);
    CContextItemAddon(const AddonProps &props);
    virtual ~CContextItemAddon();

    bool IsVisible(const CFileItemPtr item) const;
    virtual void AddIfVisible(const CFileItemPtr item, CContextButtons &visible);
    virtual bool Execute(const CFileItemPtr itemPath);
    virtual ADDON::ContextAddonPtr GetChildWithID(const std::string& strID) { return ADDON::ContextAddonPtr(); }
  protected:
    INFO::InfoPtr m_VisibleId;
  };

}
