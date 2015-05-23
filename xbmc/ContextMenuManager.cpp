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
#include "addons/Addon.h"
#include "addons/AddonManager.h"
#include "addons/ContextItemAddon.h"
#include "addons/IAddon.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "interfaces/python/ContextItemAddonInvoker.h"
#include "interfaces/python/XBPython.h"

using namespace ADDON;

typedef std::map<unsigned int, ContextItemAddonPtr>::value_type ValueType;


CContextMenuManager::CContextMenuManager()
  : m_iCurrentContextId(CONTEXT_BUTTON_FIRST_ADDON)
{
  Init();
}

CContextMenuManager& CContextMenuManager::Get()
{
  static CContextMenuManager mgr;
  return mgr;
}

void CContextMenuManager::Init()
{
  //Make sure we load all context items on first usage...
  VECADDONS addons;
  if (CAddonMgr::Get().GetAddons(ADDON_CONTEXT_ITEM, addons))
  {
    for (const auto& addon : addons)
      Register(std::static_pointer_cast<CContextItemAddon>(addon));
  }
}

void CContextMenuManager::Register(const ContextItemAddonPtr& cm)
{
  if (!cm)
    return;
  m_contextAddons[m_iCurrentContextId++] = cm;
}

bool CContextMenuManager::Unregister(const ContextItemAddonPtr& cm)
{
  if (!cm)
    return false;

  auto it = std::find_if(m_contextAddons.begin(), m_contextAddons.end(),
      [&](const ValueType& value){ return value.second->ID() == cm->ID(); });

  if (it != m_contextAddons.end())
  {
    m_contextAddons.erase(it);
    return true;
  }
  return false;
}

ContextItemAddonPtr CContextMenuManager::GetContextItemByID(unsigned int id)
{
  auto it = m_contextAddons.find(id);
  if (it != m_contextAddons.end())
    return it->second;
  return ContextItemAddonPtr();
}

void CContextMenuManager::AddVisibleItems(const CFileItemPtr& item, CContextButtons& list, const std::string& parent /* = "" */)
{
  if (!item)
    return;

  for (const auto& kv : m_contextAddons)
  {
    if (kv.second->GetParent() == parent && kv.second->IsVisible(item))
      list.push_back(std::make_pair(kv.first, kv.second->GetLabel()));
  }
}

bool CContextMenuManager::Execute(unsigned int id, const CFileItemPtr& item)
{
  if (!item)
    return false;

  const ContextItemAddonPtr addon = GetContextItemByID(id);
  if (!addon || !addon->IsVisible(item))
    return false;

  LanguageInvokerPtr invoker(new CContextItemAddonInvoker(&g_pythonParser, item));
  return (CScriptInvocationManager::Get().ExecuteAsync(addon->LibPath(), invoker, addon) != -1);
}

