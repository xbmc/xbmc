/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContextMenuManager.h"

#include "ContextMenuItem.h"
#include "ContextMenus.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "addons/Addon.h"
#include "addons/AddonEvents.h"
#include "addons/AddonManager.h"
#include "addons/ContextMenuAddon.h"
#include "addons/ContextMenus.h"
#include "addons/IAddon.h"
#include "addons/addoninfo/AddonType.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "favourites/ContextMenus.h"
#include "messaging/ApplicationMessenger.h"
#include "music/ContextMenus.h"
#include "pvr/PVRContextMenus.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "video/ContextMenus.h"

#include <algorithm>
#include <iterator>
#include <mutex>

using namespace ADDON;
using namespace PVR;

const CContextMenuItem CContextMenuManager::MAIN = CContextMenuItem::CreateGroup("", "", "kodi.core.main", "");
const CContextMenuItem CContextMenuManager::MANAGE = CContextMenuItem::CreateGroup("", "", "kodi.core.manage", "");


CContextMenuManager::CContextMenuManager(CAddonMgr& addonMgr)
  : m_addonMgr(addonMgr) {}

CContextMenuManager::~CContextMenuManager()
{
  Deinit();
}

void CContextMenuManager::Deinit()
{
  CPVRContextMenuManager::GetInstance().Events().Unsubscribe(this);
  m_addonMgr.Events().Unsubscribe(this);
  m_items.clear();
}

void CContextMenuManager::Init()
{
  m_addonMgr.Events().Subscribe(this, &CContextMenuManager::OnEvent);
  CPVRContextMenuManager::GetInstance().Events().Subscribe(this, &CContextMenuManager::OnPVREvent);

  std::unique_lock<CCriticalSection> lock(m_criticalSection);
  m_items = {
      std::make_shared<CONTEXTMENU::CVideoBrowse>(),
      std::make_shared<CONTEXTMENU::CVideoChooseVersion>(),
      std::make_shared<CONTEXTMENU::CVideoPlayVersionUsing>(),
      std::make_shared<CONTEXTMENU::CVideoResume>(),
      std::make_shared<CONTEXTMENU::CVideoPlay>(),
      std::make_shared<CONTEXTMENU::CVideoPlayUsing>(),
      std::make_shared<CONTEXTMENU::CVideoPlayAndQueue>(),
      std::make_shared<CONTEXTMENU::CVideoPlayNext>(),
      std::make_shared<CONTEXTMENU::CVideoQueue>(),
      std::make_shared<CONTEXTMENU::CMusicBrowse>(),
      std::make_shared<CONTEXTMENU::CMusicPlay>(),
      std::make_shared<CONTEXTMENU::CMusicPlayUsing>(),
      std::make_shared<CONTEXTMENU::CMusicPlayNext>(),
      std::make_shared<CONTEXTMENU::CMusicQueue>(),
      std::make_shared<CONTEXTMENU::CAddonInfo>(),
      std::make_shared<CONTEXTMENU::CEnableAddon>(),
      std::make_shared<CONTEXTMENU::CDisableAddon>(),
      std::make_shared<CONTEXTMENU::CAddonSettings>(),
      std::make_shared<CONTEXTMENU::CCheckForUpdates>(),
      std::make_shared<CONTEXTMENU::CEpisodeInfo>(),
      std::make_shared<CONTEXTMENU::CMovieInfo>(),
      std::make_shared<CONTEXTMENU::CMovieSetInfo>(),
      std::make_shared<CONTEXTMENU::CMusicVideoInfo>(),
      std::make_shared<CONTEXTMENU::CTVShowInfo>(),
      std::make_shared<CONTEXTMENU::CSeasonInfo>(),
      std::make_shared<CONTEXTMENU::CAlbumInfo>(),
      std::make_shared<CONTEXTMENU::CArtistInfo>(),
      std::make_shared<CONTEXTMENU::CSongInfo>(),
      std::make_shared<CONTEXTMENU::CVideoMarkWatched>(),
      std::make_shared<CONTEXTMENU::CVideoMarkUnWatched>(),
      std::make_shared<CONTEXTMENU::CVideoRemoveResumePoint>(),
      std::make_shared<CONTEXTMENU::CEjectDisk>(),
      std::make_shared<CONTEXTMENU::CEjectDrive>(),
      std::make_shared<CONTEXTMENU::CFavouritesTargetBrowse>(),
      std::make_shared<CONTEXTMENU::CFavouritesTargetResume>(),
      std::make_shared<CONTEXTMENU::CFavouritesTargetPlay>(),
      std::make_shared<CONTEXTMENU::CFavouritesTargetInfo>(),
      std::make_shared<CONTEXTMENU::CMoveUpFavourite>(),
      std::make_shared<CONTEXTMENU::CMoveDownFavourite>(),
      std::make_shared<CONTEXTMENU::CChooseThumbnailForFavourite>(),
      std::make_shared<CONTEXTMENU::CRenameFavourite>(),
      std::make_shared<CONTEXTMENU::CRemoveFavourite>(),
      std::make_shared<CONTEXTMENU::CAddRemoveFavourite>(),
      std::make_shared<CONTEXTMENU::CFavouritesTargetContextMenu>(),
  };

  ReloadAddonItems();

  const std::vector<std::shared_ptr<IContextMenuItem>> pvrItems(CPVRContextMenuManager::GetInstance().GetMenuItems());
  for (const auto &item : pvrItems)
    m_items.emplace_back(item);
}

void CContextMenuManager::ReloadAddonItems()
{
  VECADDONS addons;
  m_addonMgr.GetAddons(addons, AddonType::CONTEXTMENU_ITEM);

  std::vector<CContextMenuItem> addonItems;
  for (const auto& addon : addons)
  {
    auto items = std::static_pointer_cast<CContextMenuAddon>(addon)->GetItems();
    for (auto& item : items)
    {
      auto it = std::find(addonItems.begin(), addonItems.end(), item);
      if (it == addonItems.end())
        addonItems.push_back(item);
    }
  }

  std::unique_lock<CCriticalSection> lock(m_criticalSection);
  m_addonItems = std::move(addonItems);

  CLog::Log(LOGDEBUG, "ContextMenuManager: addon menus reloaded.");
}

void CContextMenuManager::OnEvent(const ADDON::AddonEvent& event)
{
  if (typeid(event) == typeid(AddonEvents::ReInstalled) ||
      typeid(event) == typeid(AddonEvents::UnInstalled))
  {
    ReloadAddonItems();
  }
  else if (typeid(event) == typeid(AddonEvents::Enabled))
  {
    AddonPtr addon;
    if (m_addonMgr.GetAddon(event.addonId, addon, AddonType::CONTEXTMENU_ITEM,
                            OnlyEnabled::CHOICE_YES))
    {
      std::unique_lock<CCriticalSection> lock(m_criticalSection);
      auto items = std::static_pointer_cast<CContextMenuAddon>(addon)->GetItems();
      for (auto& item : items)
      {
        auto it = std::find(m_addonItems.begin(), m_addonItems.end(), item);
        if (it == m_addonItems.end())
          m_addonItems.push_back(item);
      }
      CLog::Log(LOGDEBUG, "ContextMenuManager: loaded {}.", event.addonId);
    }
  }
  else if (typeid(event) == typeid(AddonEvents::Disabled))
  {
    if (m_addonMgr.HasType(event.addonId, AddonType::CONTEXTMENU_ITEM))
    {
      ReloadAddonItems();
    }
  }
}

void CContextMenuManager::OnPVREvent(const PVRContextMenuEvent& event)
{
  switch (event.action)
  {
    case PVRContextMenuEventAction::ADD_ITEM:
    {
      std::unique_lock<CCriticalSection> lock(m_criticalSection);
      m_items.emplace_back(event.item);
      break;
    }
    case PVRContextMenuEventAction::REMOVE_ITEM:
    {
      std::unique_lock<CCriticalSection> lock(m_criticalSection);
      auto it = std::find(m_items.begin(), m_items.end(), event.item);
      if (it != m_items.end())
        m_items.erase(it);
      break;
    }

    default:
      break;
  }
}

bool CContextMenuManager::IsVisible(
  const CContextMenuItem& menuItem, const CContextMenuItem& root, const CFileItem& fileItem) const
{
  if (menuItem.GetLabel(fileItem).empty() || !root.IsParentOf(menuItem))
    return false;

  if (menuItem.IsGroup())
  {
    std::unique_lock<CCriticalSection> lock(m_criticalSection);
    return std::any_of(m_addonItems.begin(), m_addonItems.end(),
        [&](const CContextMenuItem& other){ return menuItem.IsParentOf(other) && other.IsVisible(fileItem); });
  }

  return menuItem.IsVisible(fileItem);
}

bool CContextMenuManager::HasItems(const CFileItem& fileItem, const CContextMenuItem& root) const
{
  //! @todo implement group support
  if (&root == &CContextMenuManager::MAIN)
  {
    std::unique_lock<CCriticalSection> lock(m_criticalSection);
    return std::any_of(m_items.cbegin(), m_items.cend(),
                       [&fileItem](const std::shared_ptr<const IContextMenuItem>& menu) {
                         return menu->IsVisible(fileItem);
                       });
  }
  return false;
}

ContextMenuView CContextMenuManager::GetItems(const CFileItem& fileItem,
                                              const CContextMenuItem& root) const
{
  ContextMenuView result;
  //! @todo implement group support
  if (&root == &CContextMenuManager::MAIN)
  {
    std::unique_lock<CCriticalSection> lock(m_criticalSection);
    std::copy_if(m_items.begin(), m_items.end(), std::back_inserter(result),
        [&](const std::shared_ptr<IContextMenuItem>& menu){ return menu->IsVisible(fileItem); });
  }
  return result;
}

bool CContextMenuManager::HasAddonItems(const CFileItem& fileItem,
                                        const CContextMenuItem& root) const
{
  std::unique_lock<CCriticalSection> lock(m_criticalSection);
  return std::any_of(m_addonItems.cbegin(), m_addonItems.cend(),
                     [this, root, &fileItem](const CContextMenuItem& menu) {
                       return IsVisible(menu, root, fileItem);
                     });
}

ContextMenuView CContextMenuManager::GetAddonItems(const CFileItem& fileItem,
                                                   const CContextMenuItem& root) const
{
  ContextMenuView result;
  {
    std::unique_lock<CCriticalSection> lock(m_criticalSection);
    for (const auto& menu : m_addonItems)
      if (IsVisible(menu, root, fileItem))
        result.emplace_back(new CContextMenuItem(menu));
  }

  if (&root == &CContextMenuManager::MANAGE)
  {
    std::sort(result.begin(), result.end(),
        [&](const ContextMenuView::value_type& lhs, const ContextMenuView::value_type& rhs)
        {
          return lhs->GetLabel(fileItem) < rhs->GetLabel(fileItem);
        }
    );
  }
  return result;
}

bool CONTEXTMENU::HasAnyMenuItemsFor(const std::shared_ptr<CFileItem>& fileItem,
                                     const CContextMenuItem& root)
{
  if (!fileItem)
    return false;

  if (fileItem->HasProperty("contextmenulabel(0)"))
    return true;

  const CContextMenuManager& contextMenuManager = CServiceBroker::GetContextMenuManager();
  return (contextMenuManager.HasItems(*fileItem, root) ||
          contextMenuManager.HasAddonItems(*fileItem, root));
}

bool CONTEXTMENU::ShowFor(const std::shared_ptr<CFileItem>& fileItem, const CContextMenuItem& root)
{
  if (!fileItem)
    return false;

  const CContextMenuManager &contextMenuManager = CServiceBroker::GetContextMenuManager();

  auto menuItems = contextMenuManager.GetItems(*fileItem, root);
  for (auto&& item : contextMenuManager.GetAddonItems(*fileItem, root))
    menuItems.emplace_back(std::move(item));

  CContextButtons buttons;
  // compute fileitem property-based contextmenu items
  // unless we're browsing a child menu item
  if (!root.HasParent())
  {
    int i = 0;
    while (fileItem->HasProperty(StringUtils::Format("contextmenulabel({})", i)))
    {
      buttons.emplace_back(
          ~buttons.size(),
          fileItem->GetProperty(StringUtils::Format("contextmenulabel({})", i)).asString());
      ++i;
    }
  }
  const int propertyMenuSize = buttons.size();

  if (menuItems.empty() && propertyMenuSize == 0)
    return true;

  buttons.reserve(menuItems.size());
  for (size_t i = 0; i < menuItems.size(); ++i)
    buttons.Add(i, menuItems[i]->GetLabel(*fileItem));

  int selected = CGUIDialogContextMenu::Show(buttons);
  if (selected < 0 || selected >= static_cast<int>(buttons.size()))
    return false;

  if (selected < propertyMenuSize)
  {
    CServiceBroker::GetAppMessenger()->SendMsg(
        TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr,
        fileItem->GetProperty(StringUtils::Format("contextmenuaction({})", selected)).asString());
    return true;
  }

  return menuItems[selected - propertyMenuSize]->IsGroup()
             ? ShowFor(fileItem, static_cast<const CContextMenuItem&>(
                                     *menuItems[selected - propertyMenuSize]))
             : menuItems[selected - propertyMenuSize]->Execute(fileItem);
}

bool CONTEXTMENU::LoopFrom(const IContextMenuItem& menu, const std::shared_ptr<CFileItem>& fileItem)
{
  if (!fileItem)
    return false;
  if (menu.IsGroup())
    return ShowFor(fileItem, static_cast<const CContextMenuItem&>(menu));
  return menu.Execute(fileItem);
}
