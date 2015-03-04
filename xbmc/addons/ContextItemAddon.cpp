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

#include "ContextItemAddon.h"
#include "AddonManager.h"
#include "ContextMenuManager.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "GUIInfoManager.h"
#include "interfaces/info/InfoBool.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include <boost/lexical_cast.hpp>

using namespace std;

namespace ADDON
{

CContextItemAddon::CContextItemAddon(const AddonProps &props)
  : CAddon(props)
{ }

CContextItemAddon::~CContextItemAddon()
{ }

CContextItemAddon::CContextItemAddon(const cp_extension_t *ext)
  : CAddon(ext)
{
  ELEMENTS items;
  if (CAddonMgr::Get().GetExtElements(ext->configuration, "item", items))
  {
    cp_cfg_element_t *item = items[0];

    m_label = CAddonMgr::Get().GetExtValue(item, "label");
    if (StringUtils::IsNaturalNumber(m_label))
      m_label = GetString(boost::lexical_cast<int>(m_label.c_str()));

    m_parent = CAddonMgr::Get().GetExtValue(item, "parent");

    string visible = CAddonMgr::Get().GetExtValue(item, "visible");
    if (visible.empty())
      visible = "false";

    m_visCondition = g_infoManager.Register(visible, 0);
  }
}

bool CContextItemAddon::OnPreInstall()
{
  return CContextMenuManager::Get().Unregister(std::dynamic_pointer_cast<CContextItemAddon>(shared_from_this()));
}

void CContextItemAddon::OnPostInstall(bool restart, bool update, bool modal)
{
  if (restart)
  {
    // need to grab the local addon so we have the correct library path to run
    AddonPtr localAddon;
    if (CAddonMgr::Get().GetAddon(ID(), localAddon, ADDON_CONTEXT_ITEM))
    {
      ContextItemAddonPtr contextItem = std::dynamic_pointer_cast<CContextItemAddon>(localAddon);
      if (contextItem)
        CContextMenuManager::Get().Register(contextItem);
    }
  }
}

void CContextItemAddon::OnPreUnInstall()
{
  CContextMenuManager::Get().Unregister(std::dynamic_pointer_cast<CContextItemAddon>(shared_from_this()));
}

void CContextItemAddon::OnDisabled()
{
  CContextMenuManager::Get().Unregister(std::dynamic_pointer_cast<CContextItemAddon>(shared_from_this()));
}
void CContextItemAddon::OnEnabled()
{
  CContextMenuManager::Get().Register(std::dynamic_pointer_cast<CContextItemAddon>(shared_from_this()));
}

bool CContextItemAddon::IsVisible(const CFileItemPtr& item) const
{
  return item && m_visCondition->Get(item.get());
}

}
