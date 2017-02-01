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

#include "ContextMenuAddon.h"
#include "AddonManager.h"
#include "ContextMenuManager.h"
#include "ContextMenuItem.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include <sstream>


namespace ADDON
{

void CContextMenuAddon::ParseMenu(
    const AddonProps& props,
    cp_cfg_element_t* elem,
    const std::string& parent,
    int& anonGroupCount,
    std::vector<CContextMenuItem>& items)
{
  auto menuId = CAddonMgr::GetInstance().GetExtValue(elem, "@id");
  auto menuLabel = CAddonMgr::GetInstance().GetExtValue(elem, "label");
  if (StringUtils::IsNaturalNumber(menuLabel))
    menuLabel = g_localizeStrings.GetAddonString(props.id, atoi(menuLabel.c_str()));

  if (menuId.empty())
  {
    //anonymous group. create a new unique internal id.
    std::stringstream ss;
    ss << props.id << ++anonGroupCount;
    menuId = ss.str();
  }

  items.push_back(CContextMenuItem::CreateGroup(menuLabel, parent, menuId, props.id));

  ELEMENTS subMenus;
  if (CAddonMgr::GetInstance().GetExtElements(elem, "menu", subMenus))
    for (const auto& subMenu : subMenus)
      ParseMenu(props, subMenu, menuId, anonGroupCount, items);

  ELEMENTS elems;
  if (CAddonMgr::GetInstance().GetExtElements(elem, "item", elems))
  {
    for (const auto& elem : elems)
    {
      auto visCondition = CAddonMgr::GetInstance().GetExtValue(elem, "visible");
      auto library = CAddonMgr::GetInstance().GetExtValue(elem, "@library");
      auto label = CAddonMgr::GetInstance().GetExtValue(elem, "label");
      if (StringUtils::IsNaturalNumber(label))
        label = g_localizeStrings.GetAddonString(props.id, atoi(label.c_str()));

      if (!label.empty() && !library.empty() && !visCondition.empty())
      {
        auto menu = CContextMenuItem::CreateItem(label, menuId,
            URIUtils::AddFileToFolder(props.path, library), visCondition, props.id);
        items.push_back(menu);
      }
    }
  }
}

std::unique_ptr<CContextMenuAddon> CContextMenuAddon::FromExtension(AddonProps props, const cp_extension_t* ext)
{
  std::vector<CContextMenuItem> items;

  cp_cfg_element_t* menu = CAddonMgr::GetInstance().GetExtElement(ext->configuration, "menu");
  if (menu)
  {
    int tmp = 0;
    ParseMenu(props, menu, "", tmp, items);
  }
  else
  {
    //backwards compatibility. add first item definition
    ELEMENTS elems;
    if (CAddonMgr::GetInstance().GetExtElements(ext->configuration, "item", elems))
    {
      cp_cfg_element_t *elem = elems[0];

      std::string visCondition = CAddonMgr::GetInstance().GetExtValue(elem, "visible");
      if (visCondition.empty())
        visCondition = "false";

      std::string parent = CAddonMgr::GetInstance().GetExtValue(elem, "parent") == "kodi.core.manage"
          ? CContextMenuManager::MANAGE.m_groupId : CContextMenuManager::MAIN.m_groupId;

      auto label = CAddonMgr::GetInstance().GetExtValue(elem, "label");
      if (StringUtils::IsNaturalNumber(label))
        label = g_localizeStrings.GetAddonString(props.id, atoi(label.c_str()));

      CContextMenuItem menuItem = CContextMenuItem::CreateItem(label, parent,
          URIUtils::AddFileToFolder(props.path, props.libname), visCondition, props.id);

      items.push_back(menuItem);
    }
  }

  return std::unique_ptr<CContextMenuAddon>(new CContextMenuAddon(std::move(props), std::move(items)));
}

CContextMenuAddon::CContextMenuAddon(AddonProps props, std::vector<CContextMenuItem> items)
    : CAddon(std::move(props)), m_items(std::move(items))
{
}

}
