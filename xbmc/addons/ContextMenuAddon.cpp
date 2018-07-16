/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContextMenuAddon.h"

#include "AddonManager.h"
#include "ContextMenuManager.h"
#include "ContextMenuItem.h"
#include "ServiceBroker.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <sstream>

namespace ADDON
{

void CContextMenuAddon::ParseMenu(
    const CAddonInfo& addonInfo,
    cp_cfg_element_t* elem,
    const std::string& parent,
    int& anonGroupCount,
    std::vector<CContextMenuItem>& items)
{
  auto menuId = CServiceBroker::GetAddonMgr().GetExtValue(elem, "@id");
  auto menuLabel = CServiceBroker::GetAddonMgr().GetExtValue(elem, "label");
  if (StringUtils::IsNaturalNumber(menuLabel))
    menuLabel = g_localizeStrings.GetAddonString(addonInfo.ID(), std::stoi(menuLabel));

  if (menuId.empty())
  {
    //anonymous group. create a new unique internal id.
    std::stringstream ss;
    ss << addonInfo.ID() << ++anonGroupCount;
    menuId = ss.str();
  }

  items.push_back(CContextMenuItem::CreateGroup(menuLabel, parent, menuId, addonInfo.ID()));

  for (unsigned int i = 0; i < elem->num_children; i++)
  {
    cp_cfg_element_t& subElem = elem->children[i];
    const std::string elementName = subElem.name;
    if (elementName == "menu")
      ParseMenu(addonInfo, &subElem, menuId, anonGroupCount, items);

    else if (elementName == "item")
    {
      const auto visCondition = CServiceBroker::GetAddonMgr().GetExtValue(&subElem, "visible");
      const auto library = CServiceBroker::GetAddonMgr().GetExtValue(&subElem, "@library");
      auto label = CServiceBroker::GetAddonMgr().GetExtValue(&subElem, "label");
      if (StringUtils::IsNaturalNumber(label))
        label = g_localizeStrings.GetAddonString(addonInfo.ID(), std::stoi(label));

      if (!label.empty() && !library.empty() && !visCondition.empty())
      {
        auto menu = CContextMenuItem::CreateItem(
            label, menuId, URIUtils::AddFileToFolder(addonInfo.Path(), library), visCondition,
            addonInfo.ID());
        items.push_back(menu);
      }
    }
  }
}

std::unique_ptr<CContextMenuAddon> CContextMenuAddon::FromExtension(CAddonInfo addonInfo, const cp_extension_t* ext)
{
  std::vector<CContextMenuItem> items;

  cp_cfg_element_t* menu = CServiceBroker::GetAddonMgr().GetExtElement(ext->configuration, "menu");
  if (menu)
  {
    int tmp = 0;
    ParseMenu(addonInfo, menu, "", tmp, items);
  }
  else
  {
    //backwards compatibility. add first item definition
    ELEMENTS elems;
    if (CServiceBroker::GetAddonMgr().GetExtElements(ext->configuration, "item", elems))
    {
      cp_cfg_element_t *elem = elems[0];

      std::string visCondition = CServiceBroker::GetAddonMgr().GetExtValue(elem, "visible");
      if (visCondition.empty())
        visCondition = "false";

      std::string parent = CServiceBroker::GetAddonMgr().GetExtValue(elem, "parent") == "kodi.core.manage"
          ? CContextMenuManager::MANAGE.m_groupId : CContextMenuManager::MAIN.m_groupId;

      auto label = CServiceBroker::GetAddonMgr().GetExtValue(elem, "label");
      if (StringUtils::IsNaturalNumber(label))
        label = g_localizeStrings.GetAddonString(addonInfo.ID(), atoi(label.c_str()));

      CContextMenuItem menuItem = CContextMenuItem::CreateItem(label, parent,
          URIUtils::AddFileToFolder(addonInfo.Path(), addonInfo.LibName()), visCondition, addonInfo.ID());

      items.push_back(menuItem);
    }
  }

  return std::unique_ptr<CContextMenuAddon>(new CContextMenuAddon(std::move(addonInfo), std::move(items)));
}

CContextMenuAddon::CContextMenuAddon(CAddonInfo addonInfo, std::vector<CContextMenuItem> items)
    : CAddon(std::move(addonInfo)), m_items(std::move(items))
{
}

}
