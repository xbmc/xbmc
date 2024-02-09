/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <vector>

class TiXmlNode;
class CGUIListItem;

/*!
 \ingroup listproviders
 \brief An interface for providing lists to UI containers.
 */
class IListProvider
{
public:
  explicit IListProvider(int parentID) : m_parentID(parentID) {}
  explicit IListProvider(const IListProvider& other) = default;
  virtual ~IListProvider() = default;

  /*! \brief Factory to create list providers.
   \param parent a parent TiXmlNode for the container.
   \param parentID id of parent window for context.
   \return the list provider, empty pointer if none.
   */
  static std::unique_ptr<IListProvider> Create(const TiXmlNode* parent, int parentID);

  /*! \brief Factory to create list providers.  Cannot create a multi-provider.
   \param content the TiXmlNode for the content to create.
   \param parentID id of parent window for context.
   \return the list provider, empty pointer if none.
   */
  static std::unique_ptr<IListProvider> CreateSingle(const TiXmlNode* content, int parentID);

  /*! \brief Create an instance of the derived class. Allows for polymorphic copies.
   */
  virtual std::unique_ptr<IListProvider> Clone() = 0;

  /*! \brief Update the list content
   \return true if the content has changed, false otherwise.
   */
  virtual bool Update(bool forceRefresh)=0;

  /*! \brief Fetch the current list of items.
   \param items [out] the list to be filled.
   */
  virtual void Fetch(std::vector<std::shared_ptr<CGUIListItem>>& items) = 0;

  /*! \brief Check whether the list provider is updating content.
   \return true if in the processing of updating, false otherwise.
   */
  virtual bool IsUpdating() const { return false; }

  /*! \brief Reset the current list of items.
   Derived classes may choose to ignore this.
   */
  virtual void Reset() {}

  /*! \brief Free all GUI resources allocated by the items.
   \param immediately true to free resources immediately, free resources async later otherwise.
   */
  virtual void FreeResources(bool immediately) {}

  /*! \brief Click event on an item.
   \param item the item that was clicked.
   \return true if the click was handled, false otherwise.
   */
  virtual bool OnClick(const std::shared_ptr<CGUIListItem>& item) = 0;

  /*! \brief Play event on an item.
   \param item the item to play.
   \return true if the event was handled, false otherwise.
   */
  virtual bool OnPlay(const std::shared_ptr<CGUIListItem>& item) { return false; }

  /*! \brief Open the info dialog for an item provided by this IListProvider.
   \param item the item that was clicked.
   \return true if the dialog was shown, false otherwise.
   */
  virtual bool OnInfo(const std::shared_ptr<CGUIListItem>& item) = 0;

  /*! \brief Open the context menu for an item provided by this IListProvider.
   \param item the item that was clicked.
   \return true if the click was handled, false otherwise.
   */
  virtual bool OnContextMenu(const std::shared_ptr<CGUIListItem>& item) = 0;

  /*! \brief Set the default item to focus. For backwards compatibility.
   \param item the item to focus.
   \param always whether this item should always be used on first focus.
   \sa GetDefaultItem, AlwaysFocusDefaultItem
   */
  virtual void SetDefaultItem(int item, bool always) {}

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
