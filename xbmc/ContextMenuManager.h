/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ContextMenuItem.h"
#include "threads/CriticalSection.h"

#include <memory>
#include <utility>
#include <vector>

namespace ADDON
{
struct AddonEvent;
class CAddonMgr;
} // namespace ADDON

namespace PVR
{
  struct PVRContextMenuEvent;
}

using ContextMenuView = std::vector<std::shared_ptr<const IContextMenuItem>>;

class CContextMenuManager
{
public:
  static const CContextMenuItem MAIN;
  static const CContextMenuItem MANAGE;

  explicit CContextMenuManager(ADDON::CAddonMgr& addonMgr);
  ~CContextMenuManager();

  void Init();
  void Deinit();

  /*! \brief Checks whether context menu items are available for a file item.
   \param fileItem the file item
   \param root the root context menu item
   \return true if any items are present, false otherwise
   */
  bool HasItems(const CFileItem& fileItem, const CContextMenuItem& root) const;

  /*! \brief Gets the context menu items available for a file item.
   \param fileItem the file item
   \param root the root context menu item
   \return the items
   \sa ContextMenuView
   */
  ContextMenuView GetItems(const CFileItem& fileItem, const CContextMenuItem& root) const;

  /*! \brief Checks whether addon context menu items are available for a file item.
   \param fileItem the file item
   \param root the root context menu item
   \return true if any items are present, false otherwise
   */
  bool HasAddonItems(const CFileItem& fileItem, const CContextMenuItem& root) const;

  /*! \brief Gets the addon context menu items available for a file item.
   \param fileItem the file item
   \param root the root context menu item
   \return the items
   \sa ContextMenuView
   */
  ContextMenuView GetAddonItems(const CFileItem& fileItem, const CContextMenuItem& root) const;

private:
  CContextMenuManager(const CContextMenuManager&) = delete;
  CContextMenuManager& operator=(CContextMenuManager const&) = delete;

  bool IsVisible(
    const CContextMenuItem& menuItem,
    const CContextMenuItem& root,
    const CFileItem& fileItem) const;

  void ReloadAddonItems();
  void OnEvent(const ADDON::AddonEvent& event);

  void OnPVREvent(const PVR::PVRContextMenuEvent& event);

  ADDON::CAddonMgr& m_addonMgr;

  mutable CCriticalSection m_criticalSection;
  std::vector<CContextMenuItem> m_addonItems;
  std::vector<std::shared_ptr<IContextMenuItem>> m_items;
};

namespace CONTEXTMENU
{
/*! \brief Checks whether any context menu items are available for a file item.
 \param fileItem the file item
 \param root the root context menu item
 \return true if any items are present, false otherwise
 */
bool HasAnyMenuItemsFor(const std::shared_ptr<CFileItem>& fileItem, const CContextMenuItem& root);

/*! \brief Starts the context menu loop for a file item.
 \param fileItem the file item
 \param root the root context menu item
 \return true on success, false otherwise
 */
bool ShowFor(const std::shared_ptr<CFileItem>& fileItem, const CContextMenuItem& root);

/*! \brief Shortcut for continuing the context menu loop from an existing menu item.
 \param menu the menu item
 \param fileItem the file item
 \return true on success, false otherwise
 */
bool LoopFrom(const IContextMenuItem& menu, const std::shared_ptr<CFileItem>& fileItem);
}
