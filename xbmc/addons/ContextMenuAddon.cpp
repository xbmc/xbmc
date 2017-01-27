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
    const CAddonExtensions* elem,
    const std::string& parent,
    int& anonGroupCount)
{
  std::string menuId = elem->GetValue("@id").asString();
  std::string menuLabel = elem->GetValue("label").asString();
  if (StringUtils::IsNaturalNumber(menuLabel))
    menuLabel = g_localizeStrings.GetAddonString(AddonInfo()->ID(), atoi(menuLabel.c_str()));

  if (menuId.empty())
  {
    //anonymous group. create a new unique internal id.
    std::stringstream ss;
    ss << AddonInfo()->ID() << ++anonGroupCount;
    menuId = ss.str();
  }

  m_items.push_back(CContextMenuItem::CreateGroup(menuLabel, parent, menuId, AddonInfo()->ID()));

  for (const auto subMenu : elem->GetElements("menu"))
    ParseMenu(&subMenu.second, menuId, anonGroupCount);

  for (const auto element : elem->GetElements("item"))
  {
    std::string visCondition = element.second.GetValue("visible").asString();
    std::string library = element.second.GetValue("@library").asString();
    std::string label = element.second.GetValue("label").asString();
    if (StringUtils::IsNaturalNumber(label))
      label = g_localizeStrings.GetAddonString(AddonInfo()->ID(), atoi(label.c_str()));

    if (!label.empty() && !library.empty() && !visCondition.empty())
    {
      auto menu = CContextMenuItem::CreateItem(label, menuId,
          URIUtils::AddFileToFolder(AddonInfo()->Path(), library), visCondition, AddonInfo()->ID());
      m_items.push_back(menu);
    }
  }
}

CContextMenuAddon::CContextMenuAddon(CAddonInfo addonInfo)
  : CAddon(std::move(addonInfo))
{
  const CAddonExtensions* menu = AddonInfo()->GetElement("menu");
  if (menu)
  {
    int tmp = 0;
    ParseMenu(menu, "", tmp);
  }
  else
  {
    //backwards compatibility. add first item definition
    const CAddonExtensions* elem = AddonInfo()->GetElement("item");
    if (elem)
    {
      std::string visCondition = elem->GetValue("visible").asString();
      if (visCondition.empty())
        visCondition = "false";

      std::string parent = elem->GetValue("parent").asString() == "kodi.core.manage"
          ? CContextMenuManager::MANAGE.m_groupId : CContextMenuManager::MAIN.m_groupId;

      auto label = elem->GetValue("label").asString();
      if (StringUtils::IsNaturalNumber(label))
        label = g_localizeStrings.GetAddonString(AddonInfo()->ID(), atoi(label.c_str()));

      CContextMenuItem menuItem = CContextMenuItem::CreateItem(label, parent,
          URIUtils::AddFileToFolder(addonInfo.m_path, addonInfo.m_libname), visCondition, AddonInfo()->ID());

      m_items.push_back(menuItem);
    }
  }
}

}
