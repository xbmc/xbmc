/*
 *      Copyright (C) 2013 Team XBMC
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

#pragma once

#include <vector>
#include <memory>

class TiXmlNode;
class CGUIListItem;
typedef std::shared_ptr<CGUIListItem> CGUIListItemPtr;

/*!
 \ingroup listproviders
 \brief An interface for providing lists to UI containers.
 */
class IListProvider
{
public:
  IListProvider(int parentID) : m_parentID(parentID) {}
  virtual ~IListProvider() {}

  /*! \brief Factory to create list providers.
   \param node a TiXmlNode to create.
   \param parentID id of parent window for context.
   \return the list provider, NULL if none.
   */
  static IListProvider *Create(const TiXmlNode *node, int parentID);

  /*! \brief Update the list content
   \return true if the content has changed, false otherwise.
   */
  virtual bool Update(bool forceRefresh)=0;

  /*! \brief Fetch the current list of items.
   \param items [out] the list to be filled.
   */
  virtual void Fetch(std::vector<CGUIListItemPtr> &items) const=0;

  /*! \brief Check whether the list provider is updating content.
   \return true if in the processing of updating, false otherwise.
   */
  virtual bool IsUpdating() const { return false; }

  /*! \brief Reset the current list of items.
   Derived classes may choose to ignore this.
   \param immediately whether the content of the provider should be cleared.
   */
  virtual void Reset(bool immediately = false) {};

  /*! \brief Click event on an item.
   \param item the item that was clicked.
   \return true if the click was handled, false otherwise.
   */
  virtual bool OnClick(const CGUIListItemPtr &item)=0;

  /*! \brief Set the default item to focus. For backwards compatibility.
   \param item the item to focus.
   \param always whether this item should always be used on first focus.
   \sa GetDefaultItem, AlwaysFocusDefaultItem
   */
  virtual void SetDefaultItem(int item, bool always) {};

  /*! \brief The default item to focus.
   \return the item to focus by default. -1 for none.
   \sa SetDefaultItem, AlwaysFocusDefaultItem
   */
  virtual int GetDefaultItem() const { return -1; }

  /*! \brief Whether to always focus the default item.
   \return true if the default item should always be the one to receive focus.
   \sa GetDefaultItem, SetDefaultItem
   */
  virtual bool AlwaysFocusDefaultItem() const { return false; }
protected:
  int m_parentID;
};
