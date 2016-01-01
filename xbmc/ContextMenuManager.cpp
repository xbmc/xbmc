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

#include "ContextMenuManager.h"
#include "ContextMenuItem.h"
#include "addons/Addon.h"
#include "addons/AddonManager.h"
#include "addons/ContextMenuAddon.h"
#include "addons/IAddon.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "interfaces/python/ContextItemAddonInvoker.h"
#include "interfaces/python/XBPython.h"
#include "utils/log.h"

using namespace ADDON;

typedef std::pair<unsigned int, CContextMenuItem> Item;

const CContextMenuItem CContextMenuManager::MAIN = CContextMenuItem::CreateGroup("", "", "kodi.core.main");
const CContextMenuItem CContextMenuManager::MANAGE = CContextMenuItem::CreateGroup("", "", "kodi.core.manage");


CContextMenuManager::CContextMenuManager()
  : m_nextButtonId(CONTEXT_BUTTON_FIRST_ADDON)
{
  Init();
}

CContextMenuManager& CContextMenuManager::GetInstance()
{
  static CContextMenuManager mgr;
  return mgr;
}

void CContextMenuManager::Init()
{
  VECADDONS addons;
  if (CAddonMgr::GetInstance().GetAddons(ADDON_CONTEXT_ITEM, addons))
    for (const auto& addon : addons)
      Register(std::static_pointer_cast<CContextMenuAddon>(addon));
}

void CContextMenuManager::Register(const ContextItemAddonPtr& cm)
{
  if (!cm)
    return;

  for (const auto& menuItem : cm->GetItems())
  {
    auto existing = std::find_if(m_items.begin(), m_items.end(),
        [&](const Item& kv){ return kv.second == menuItem; });
    if (existing != m_items.end())
    {
      if (!menuItem.GetLabel().empty())
        existing->second = menuItem;
    }
    else
    {
      m_items.push_back(std::make_pair(m_nextButtonId, menuItem));
      ++m_nextButtonId;
    }
  }
}

bool CContextMenuManager::Unregister(const ContextItemAddonPtr& cm)
{
  if (!cm)
    return false;

  const auto menuItems = cm->GetItems();

  auto it = std::remove_if(m_items.begin(), m_items.end(),
    [&](const Item& kv)
    {
      if (kv.second.IsGroup())
        return false; //keep in case other items use them
      return std::find(menuItems.begin(), menuItems.end(), kv.second) != menuItems.end();
    }
  );
  m_items.erase(it, m_items.end());
  return true;
}

bool CContextMenuManager::IsVisible(
  const CContextMenuItem& menuItem, const CContextMenuItem& root, const CFileItemPtr& fileItem)
{
  if (menuItem.GetLabel().empty() || !root.IsParentOf(menuItem))
    return false;

  if (menuItem.IsGroup())
    return std::any_of(m_items.begin(), m_items.end(),
        [&](const Item& kv){ return menuItem.IsParentOf(kv.second) && kv.second.IsVisible(fileItem); });

  return menuItem.IsVisible(fileItem);
}

void CContextMenuManager::AddVisibleItems(
  const CFileItemPtr& item, CContextButtons& list, const CContextMenuItem& root /* = CContextMenuItem::MAIN */)
{
  if (!item)
    return;

  const int initialSize = list.size();

  for (const auto& kv : m_items)
    if (IsVisible(kv.second, root, item))
      list.push_back(std::make_pair(kv.first, kv.second.GetLabel()));

  if (root == MAIN || root == MANAGE)
  {
    std::stable_sort(list.begin() + initialSize, list.end(),
      [](const std::pair<int, std::string>& lhs, const std::pair<int, std::string>& rhs)
      {
        return lhs.second < rhs.second;
      }
    );
  }
}


bool CContextMenuManager::OnClick(unsigned int id, const CFileItemPtr& item)
{
  if (!item)
    return false;

  auto it = std::find_if(m_items.begin(), m_items.end(),
      [id](const Item& kv){ return kv.first == id; });
  if (it == m_items.end())
  {
    CLog::Log(LOGERROR, "CContextMenuManager: unknown button id '%u'", id);
    return false;
  }

  CContextMenuItem menuItem = it->second;
  if (menuItem.IsGroup())
  {
    CLog::Log(LOGDEBUG, "CContextMenuManager: showing group '%s'", menuItem.ToString().c_str());
    CContextButtons choices;
    AddVisibleItems(item, choices, menuItem);
    if (choices.empty())
    {
      CLog::Log(LOGERROR, "CContextMenuManager: no items in group '%s'", menuItem.ToString().c_str());
      return false;
    }
    int choice = CGUIDialogContextMenu::ShowAndGetChoice(choices);
    if (choice == -1)
      return false;

    return OnClick(choice, item);
  }

  return menuItem.Execute(item);
}

