/*
 *      Copyright (C) 2015 Team XBMC
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

#include "ContextMenuItem.h"
#include "addons/Addon.h"
#include "addons/AddonManager.h"
#include "addons/ContextMenuAddon.h"
#include "addons/IAddon.h"
#include "GUIInfoManager.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "interfaces/python/ContextItemAddonInvoker.h"
#include "interfaces/python/XBPython.h"
#include "utils/StringUtils.h"


bool CContextMenuItem::IsVisible(const CFileItem& item) const
{
  if (!m_infoBoolRegistered)
  {
    m_infoBool = g_infoManager.Register(m_visibilityCondition, 0);
    m_infoBoolRegistered = true;
  }
  return IsGroup() || (m_infoBool && m_infoBool->Get(&item));
}

bool CContextMenuItem::IsParentOf(const CContextMenuItem& other) const
{
  return !m_groupId.empty() && (m_groupId == other.m_parent);
}

bool CContextMenuItem::IsGroup() const
{
  return !m_groupId.empty();
}

bool CContextMenuItem::Execute(const CFileItemPtr& item) const
{
  if (!item || m_library.empty() || IsGroup())
    return false;

  ADDON::AddonPtr addon;
  if (!ADDON::CAddonMgr::GetInstance().GetAddon(m_addonId, addon))
    return false;

  LanguageInvokerPtr invoker(new CContextItemAddonInvoker(&g_pythonParser, item));
  return (CScriptInvocationManager::GetInstance().ExecuteAsync(m_library, invoker, addon) != -1);
}

bool CContextMenuItem::operator==(const CContextMenuItem& other) const
{
  if (IsGroup() && other.IsGroup())
    return (m_groupId == other.m_groupId && m_parent == other.m_parent);

  return (IsGroup() == other.IsGroup())
      && (m_parent == other.m_parent)
      && (m_library == other.m_library)
      && (m_addonId == other.m_addonId);
}

std::string CContextMenuItem::ToString() const
{
  if (IsGroup())
    return StringUtils::Format("CContextMenuItem[group, id=%s, parent=%s, addon=%s]",
        m_groupId.c_str(), m_parent.c_str(), m_addonId.c_str());
  else
    return StringUtils::Format("CContextMenuItem[item, parent=%s, library=%s, addon=%s]",
        m_parent.c_str(), m_library.c_str(), m_addonId.c_str());
}

CContextMenuItem CContextMenuItem::CreateGroup(const std::string& label, const std::string& parent,
    const std::string& groupId, const std::string& addonId)
{
  CContextMenuItem menuItem;
  menuItem.m_label = label;
  menuItem.m_parent = parent;
  menuItem.m_groupId = groupId;
  menuItem.m_addonId = addonId;
  return menuItem;
}

CContextMenuItem CContextMenuItem::CreateItem(const std::string& label, const std::string& parent,
    const std::string& library, const std::string& condition, const std::string& addonId)
{
  CContextMenuItem menuItem;
  menuItem.m_label = label;
  menuItem.m_parent = parent;
  menuItem.m_library = library;
  menuItem.m_visibilityCondition = condition;
  menuItem.m_addonId = addonId;
  return menuItem;
}
