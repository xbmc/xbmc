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
#include "GUIInfoManager.h"
#include "interfaces/info/InfoBool.h"
#include "utils/StringUtils.h"
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
    m_parent = CAddonMgr::Get().GetExtValue(item, "parent");

    string visible = CAddonMgr::Get().GetExtValue(item, "visible");
    if (visible.empty())
      visible = "false";

    m_visCondition = g_infoManager.Register(visible, 0);
  }
}

std::string CContextItemAddon::GetLabel()
{
  if (StringUtils::IsNaturalNumber(m_label))
    return GetString(boost::lexical_cast<int>(m_label.c_str()));
  return m_label;
}

bool CContextItemAddon::IsVisible(const CFileItemPtr& item) const
{
  return item && m_visCondition->Get(item.get());
}

}
