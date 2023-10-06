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

  bool HasItems(const CFileItem& fileItem) const;
  ContextMenuView GetItems(const CFileItem& item, const CContextMenuItem& root = MAIN) const;

  bool HasAddonItems(const CFileItem& fileItem) const;
  ContextMenuView GetAddonItems(const CFileItem& item, const CContextMenuItem& root = MAIN) const;

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
/*!
 * Checks whether any context menu items are available for a file item.
 * */
bool HasAnyMenuItemsFor(const std::shared_ptr<CFileItem>& fileItem);

/*!
   * Starts the context menu loop for a file item.
   * */
bool ShowFor(const std::shared_ptr<CFileItem>& fileItem,
             const CContextMenuItem& root = CContextMenuManager::MAIN);

/*!
   * Shortcut for continuing the context menu loop from an existing menu item.
   */
bool LoopFrom(const IContextMenuItem& menu, const std::shared_ptr<CFileItem>& fileItem);
}
