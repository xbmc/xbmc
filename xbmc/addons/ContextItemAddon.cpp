/*
 *      Copyright (C) 2013-2014 Team XBMC
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
#include "GUIInfoManager.h"
#include "utils/log.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "interfaces/info/InfoBool.h"
#include "utils/StringUtils.h"
#include "GUIContextMenuManager.h"
#include "addons/ContextCategoryAddon.h"
#include "dialogs/GUIDialogContextMenu.h"
#include <boost/lexical_cast.hpp>

using namespace std;

namespace ADDON
{

IContextItem::IContextItem(const AddonProps &props)
  : CAddon(props)
{ }

IContextItem::~IContextItem()
{ }

IContextItem::IContextItem(const cp_extension_t *ext)
  : CAddon(ext)
{
  //generate a unique ID for every context item entry
  m_id = CAddonMgr::Get().GetMsgIdForContextAddon(ID());

  CStdString labelStr = CAddonMgr::Get().GetExtValue(ext->configuration, "@label");
  if (labelStr.empty())
  {
    m_label = Name();
    CLog::Log(LOGDEBUG, "ADDON: %s - failed to load label attribute, falling back to addon name %s.", ID().c_str(), Name().c_str());
  }
  else
  {
    if (StringUtils::IsNaturalNumber(labelStr))
    {
      int id = boost::lexical_cast<int>( labelStr.c_str() );
      m_label = GetString(id);
      ClearStrings();
      if (m_label.empty())
      {
        CLog::Log(LOGDEBUG, "ADDON: %s - label id %i not found, using addon name %s", ID().c_str(), id, Name().c_str());
        m_label = Name();
      }
    }
    else
      m_label = labelStr;
  }

  m_parent = CAddonMgr::Get().GetExtValue(ext->configuration, "@parent");
}

bool IContextItem::OnPreInstall()
{
  //during an update the context item could get a new parent...
  //so it is safer to remove it now from the old parent
  //and add it to the correct one in PostInstall()
  BaseContextMenuManager::Get().Unregister(boost::dynamic_pointer_cast<IContextItem>(shared_from_this()));
  return true;
}

void IContextItem::OnPostInstall(bool restart, bool update)
{
  BaseContextMenuManager::Get().Register(boost::dynamic_pointer_cast<IContextItem>(shared_from_this()));
}

void IContextItem::OnPreUnInstall()
{
  BaseContextMenuManager::Get().Unregister(boost::dynamic_pointer_cast<IContextItem>(shared_from_this()));
}

void IContextItem::OnDisabled()
{
  BaseContextMenuManager::Get().Unregister(boost::dynamic_pointer_cast<IContextItem>(shared_from_this()));
}
void IContextItem::OnEnabled()
{
  BaseContextMenuManager::Get().Register(boost::dynamic_pointer_cast<IContextItem>(shared_from_this()));
}

std::pair<unsigned int, std::string> IContextItem::ToNative()
{
  return make_pair<unsigned int, std::string>(GetMsgID(), GetLabel());
}

CContextItemAddon::CContextItemAddon(const cp_extension_t *ext)
  : IContextItem(ext)
{
  CStdString visible = CAddonMgr::Get().GetExtValue(ext->configuration, "@visible");
  m_VisibleId = g_infoManager.Register(visible, 0);
  if (!m_VisibleId)
    CLog::Log(LOGDEBUG, "ADDON: %s - No visibility expression given, or failed to load it: '%s'. Context item will always be visible", ID().c_str(), visible.c_str());
}

CContextItemAddon::CContextItemAddon(const AddonProps &props)
  : IContextItem(props)
{ }

CContextItemAddon::~CContextItemAddon()
{ }

bool CContextItemAddon::Execute(const CFileItemPtr item)
{
  if (!IsVisible(item))
    return false;
  return (CScriptInvocationManager::Get().Execute(LibPath(), this->shared_from_this(), std::vector<string>(), item) != -1);
}

bool CContextItemAddon::IsVisible(const CFileItemPtr item) const
{
  if(!m_VisibleId)
    return true;
  return m_VisibleId->Get(item.get());
}

void CContextItemAddon::AddIfVisible(const CFileItemPtr item, CContextButtons &visible)
{
  if (IsVisible(item))
    visible.push_back(ToNative());
}

}
