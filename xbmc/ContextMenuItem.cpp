/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContextMenuItem.h"

#include "FileItem.h"
#include "GUIInfoManager.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "guilib/GUIComponent.h"
#include "guilib/LocalizeStrings.h"
#ifdef HAS_PYTHON
#include "interfaces/generic/ScriptInvocationManager.h"
#include "interfaces/python/ContextItemAddonInvoker.h"
#include "interfaces/python/XBPython.h"
#endif
#include "ServiceBroker.h"
#include "utils/StringUtils.h"

std::string CStaticContextMenuAction::GetLabel(const CFileItem& item) const
{
  return g_localizeStrings.Get(m_label);
}

CContextMenuItem::CContextMenuItem(CGroup groupData)
  : m_label(std::move(groupData.label)),
    m_parent(std::move(groupData.parent)),
    m_groupId(std::move(groupData.groupId)),
    m_addonId(std::move(groupData.addonId))
{
}

CContextMenuItem::CContextMenuItem(CItem itemData)
  : m_label(std::move(itemData.label)),
    m_parent(std::move(itemData.parent)),
    m_library(std::move(itemData.library)),
    m_addonId(std::move(itemData.addonId)),
    m_args(std::move(itemData.args)),
    m_visibilityCondition(std::move(itemData.condition))
{
}

bool CContextMenuItem::IsVisible(const CFileItem& item) const
{
  if (!m_infoBoolRegistered)
  {
    m_infoBool = CServiceBroker::GetGUI()->GetInfoManager().Register(m_visibilityCondition, 0);
    m_infoBoolRegistered = true;
  }
  return IsGroup() || (m_infoBool && m_infoBool->Get(INFO::DEFAULT_CONTEXT, &item));
}

bool CContextMenuItem::IsParentOf(const CContextMenuItem& other) const
{
  return !m_groupId.empty() && (m_groupId == other.m_parent);
}

bool CContextMenuItem::IsGroup() const
{
  return !m_groupId.empty();
}

bool CContextMenuItem::HasParent() const
{
  return !m_parent.empty();
}

bool CContextMenuItem::Execute(const std::shared_ptr<CFileItem>& item) const
{
  if (!item || m_library.empty() || IsGroup())
    return false;

  ADDON::AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(m_addonId, addon, ADDON::OnlyEnabled::CHOICE_YES))
    return false;

  bool reuseLanguageInvoker = false;
  if (addon->ExtraInfo().contains("reuselanguageinvoker"))
    reuseLanguageInvoker = addon->ExtraInfo().at("reuselanguageinvoker") == "true";

#ifdef HAS_PYTHON
  auto invoker = std::make_shared<CContextItemAddonInvoker>(&CServiceBroker::GetXBPython(), item);
  return (CScriptInvocationManager::GetInstance().ExecuteAsync(m_library, invoker, addon, m_args, reuseLanguageInvoker) != -1);
#else
  return false;
#endif
}

bool CContextMenuItem::operator==(const CContextMenuItem& other) const
{
  if (IsGroup() && other.IsGroup())
    return (m_groupId == other.m_groupId && m_parent == other.m_parent);

  return (IsGroup() == other.IsGroup())
      && (m_parent == other.m_parent)
      && (m_library == other.m_library)
      && (m_addonId == other.m_addonId)
      && (m_args == other.m_args);
}

std::string CContextMenuItem::ToString() const
{
  if (IsGroup())
    return StringUtils::Format("CContextMenuItem[group, id={}, parent={}, addon={}]", m_groupId,
                               m_parent, m_addonId);
  else
    return StringUtils::Format("CContextMenuItem[item, parent={}, library={}, addon={}]", m_parent,
                               m_library, m_addonId);
}
