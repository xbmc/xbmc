/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContextMenuAddon.h"

#include "AddonManager.h"
#include "ContextMenuItem.h"
#include "ContextMenuManager.h"
#include "ServiceBroker.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <sstream>

namespace ADDON
{

CContextMenuAddon::CContextMenuAddon(const AddonInfoPtr& addonInfo)
    : CAddon(addonInfo, ADDON_CONTEXT_ITEM)
{
  std::vector<CContextMenuItem> items;

  const CAddonExtensions* menu = Type(ADDON_CONTEXT_ITEM)->GetElement("menu");
  if (menu)
  {
    int tmp = 0;
    ParseMenu(menu, "", tmp);
  }
  else
  {
    //backwards compatibility. add first item definition
    const CAddonExtensions* elem = Type(ADDON_CONTEXT_ITEM)->GetElement("item");
    if (elem)
    {
      std::string visCondition = elem->GetValue("visible").asString();
      if (visCondition.empty())
        visCondition = "false";

      std::string parent = elem->GetValue("parent").asString() == "kodi.core.manage"
          ? CContextMenuManager::MANAGE.m_groupId : CContextMenuManager::MAIN.m_groupId;

      auto label = elem->GetValue("label").asString();
      if (StringUtils::IsNaturalNumber(label))
        label = g_localizeStrings.GetAddonString(ID(), atoi(label.c_str()));

      CContextMenuItem menuItem = CContextMenuItem::CreateItem(label, parent,
          URIUtils::AddFileToFolder(Path(), Type(ADDON_CONTEXT_ITEM)->LibName()), visCondition, ID());

      m_items.push_back(menuItem);
    }
  }
}

void CContextMenuAddon::ParseMenu(
    const CAddonExtensions* elem,
    const std::string& parent,
    int& anonGroupCount)
{
  auto menuId = elem->GetValue("@id").asString();
  auto menuLabel = elem->GetValue("label").asString();
  if (StringUtils::IsNaturalNumber(menuLabel))
    menuLabel = g_localizeStrings.GetAddonString(ID(), std::stoi(menuLabel));

  if (menuId.empty())
  {
    //anonymous group. create a new unique internal id.
    std::stringstream ss;
    ss << ID() << ++anonGroupCount;
    menuId = ss.str();
  }

  m_items.push_back(CContextMenuItem::CreateGroup(menuLabel, parent, menuId, ID()));

  for (const auto subMenu : elem->GetElements("menu"))
    ParseMenu(&subMenu.second, menuId, anonGroupCount);

  for (const auto element : elem->GetElements("item"))
  {
    std::string visCondition = element.second.GetValue("visible").asString();
    std::string library = element.second.GetValue("@library").asString();
    std::string label = element.second.GetValue("label").asString();
    if (StringUtils::IsNaturalNumber(label))
      label = g_localizeStrings.GetAddonString(ID(), atoi(label.c_str()));

    if (!label.empty() && !library.empty() && !visCondition.empty())
    {
      auto menu = CContextMenuItem::CreateItem(label, menuId,
          URIUtils::AddFileToFolder(Path(), library), visCondition, ID());
      m_items.push_back(menu);
    }
  }
}

}
