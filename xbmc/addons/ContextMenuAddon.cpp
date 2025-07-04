/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContextMenuAddon.h"

#include "ContextMenuItem.h"
#include "ContextMenuManager.h"
#include "addons/addoninfo/AddonType.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <sstream>

namespace ADDON
{

CContextMenuAddon::CContextMenuAddon(const AddonInfoPtr& addonInfo)
  : CAddon(addonInfo, AddonType::CONTEXTMENU_ITEM)
{
  const CAddonExtensions* menu = Type(AddonType::CONTEXTMENU_ITEM)->GetElement("menu");
  if (menu)
  {
    int tmp = 0;
    ParseMenu(menu, "", tmp);
  }
  else
  {
    //backwards compatibility. add first item definition
    const CAddonExtensions* elem = Type(AddonType::CONTEXTMENU_ITEM)->GetElement("item");
    if (elem)
    {
      std::string visCondition = elem->GetValue("visible").asString();
      if (visCondition.empty())
        visCondition = "false";

      std::string parent = elem->GetValue("parent").asString() == "kodi.core.manage"
          ? CContextMenuManager::MANAGE.m_groupId : CContextMenuManager::MAIN.m_groupId;

      std::string label = elem->GetValue("label").asString();
      if (StringUtils::IsNaturalNumber(label))
        label = g_localizeStrings.GetAddonString(ID(), atoi(label.c_str()));

      CContextMenuItem menuItem = CContextMenuItem::CreateItem(
          label, parent,
          URIUtils::AddFileToFolder(Path(), Type(AddonType::CONTEXTMENU_ITEM)->LibName()),
          visCondition, ID());

      m_items.push_back(std::move(menuItem));
    }
  }
}

CContextMenuAddon::~CContextMenuAddon() = default;

void CContextMenuAddon::ParseMenu(
    const CAddonExtensions* elem,
    const std::string& parent,
    int& anonGroupCount)
{
  std::string menuId = elem->GetValue("@id").asString();
  std::string menuLabel = elem->GetValue("label").asString();
  if (StringUtils::IsNaturalNumber(menuLabel))
    menuLabel = g_localizeStrings.GetAddonString(ID(), std::stoi(menuLabel));

  if (menuId.empty())
  {
    //anonymous group. create a new unique internal id.
    anonGroupCount++;
    std::stringstream ss;
    ss << ID() << anonGroupCount;
    menuId = ss.str();
  }

  m_items.emplace_back(CContextMenuItem::CreateGroup(menuLabel, parent, menuId, ID()));

  for (const auto& [_, addonExtensions] : elem->GetElements("menu"))
    ParseMenu(&addonExtensions, menuId, anonGroupCount);

  for (const auto& [_, addonExtensions] : elem->GetElements("item"))
  {
    const std::string visCondition = addonExtensions.GetValue("visible").asString();
    const std::string library = addonExtensions.GetValue("@library").asString();
    std::string label = addonExtensions.GetValue("label").asString();
    if (StringUtils::IsNaturalNumber(label))
      label = g_localizeStrings.GetAddonString(ID(), std::atoi(label.c_str()));

    std::vector<std::string> args;
    args.emplace_back(ID());

    const std::string arg = addonExtensions.GetValue("@args").asString();
    if (!arg.empty())
      args.emplace_back(arg);

    if (!label.empty() && !library.empty() && !visCondition.empty())
    {
      CContextMenuItem menu = CContextMenuItem::CreateItem(
          label, menuId, URIUtils::AddFileToFolder(Path(), library), visCondition, ID(), args);
      m_items.emplace_back(std::move(menu));
    }
  }
}

} // namespace ADDON
