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
#include "GUIInfoManager.h"
#include "interfaces/info/InfoBool.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include <sstream>


namespace ADDON
{

CContextMenuAddon::CContextMenuAddon(const AddonProps &props)
  : CAddon(props)
{ }

CContextMenuAddon::~CContextMenuAddon()
{ }

CContextMenuAddon::CContextMenuAddon(const cp_extension_t *ext)
  : CAddon(ext)
{
  cp_cfg_element_t* menu = CAddonMgr::GetInstance().GetExtElement(ext->configuration, "menu");
  if (menu)
  {
    int tmp = 0;
    ParseMenu(menu, "", tmp);
  }
  else
  {
    //backwards compatibility. add first item definition
    ELEMENTS items;
    if (CAddonMgr::GetInstance().GetExtElements(ext->configuration, "item", items))
    {
      cp_cfg_element_t *item = items[0];

      std::string visCondition = CAddonMgr::GetInstance().GetExtValue(item, "visible");
      if (visCondition.empty())
        visCondition = "false";

      std::string parent = CAddonMgr::GetInstance().GetExtValue(item, "parent") == "kodi.core.manage"
          ? CContextMenuManager::MANAGE.m_groupId : CContextMenuManager::MAIN.m_groupId;

      CContextMenuItem menuItem = CContextMenuItem::CreateItem(
        CAddonMgr::GetInstance().GetExtValue(item, "label"),
        parent,
        LibPath(),
        g_infoManager.Register(visCondition, 0));

      m_items.push_back(menuItem);
    }
  }
}

void CContextMenuAddon::ParseMenu(cp_cfg_element_t* elem, const std::string& parent, int& anonGroupCount)
{
  auto menuLabel = CAddonMgr::GetInstance().GetExtValue(elem, "label");
  auto menuId = CAddonMgr::GetInstance().GetExtValue(elem, "@id");

  if (menuId.empty())
  {
    //anonymous group. create a new unique internal id.
    std::stringstream ss;
    ss << ID() << ++anonGroupCount;
    menuId = ss.str();
  }

  m_items.push_back(CContextMenuItem::CreateGroup(menuLabel, parent, menuId));

  ELEMENTS subMenus;
  if (CAddonMgr::GetInstance().GetExtElements(elem, "menu", subMenus))
    for (const auto& subMenu : subMenus)
      ParseMenu(subMenu, menuId, anonGroupCount);

  ELEMENTS items;
  if (CAddonMgr::GetInstance().GetExtElements(elem, "item", items))
  {
    for (const auto& item : items)
    {
      auto label = CAddonMgr::GetInstance().GetExtValue(item, "label");
      auto visCondition = CAddonMgr::GetInstance().GetExtValue(item, "visible");
      auto library = CAddonMgr::GetInstance().GetExtValue(item, "@library");

      if (!label.empty() && !library.empty() && !visCondition.empty())
      {
        auto menu = CContextMenuItem::CreateItem(
          label,
          menuId,
          URIUtils::AddFileToFolder(Path(), library),
          g_infoManager.Register(visCondition, 0));

        m_items.push_back(menu);
      }
    }
  }
}

std::vector<CContextMenuItem> CContextMenuAddon::GetItems()
{
  //Return a copy which owns `this`
  std::vector<CContextMenuItem> ret = m_items;
  for (CContextMenuItem& menuItem : ret)
    menuItem.m_addon = this->shared_from_this();
  return ret;
}

}
