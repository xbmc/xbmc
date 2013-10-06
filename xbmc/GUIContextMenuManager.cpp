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

#include "GUIContextMenuManager.h"
#include "addons/Addon.h"
#include "addons/AddonManager.h"
#include "addons/ContextItemAddon.h"
#include "addons/ContextCategoryAddon.h"
#include "Util.h"
#include "addons/IAddon.h"
#include "addons/ContextCategoryAddon.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "utils/log.h"

using namespace ADDON;

void ContextMenuManager::RegisterContextItem(ContextAddonPtr cm)
{
  contextIter it = GetContextItemIterator(cm->GetMsgID());

  if (it != m_vecContextMenus.end())
    *it = cm; //Item might be updated, so replace!
  else
    m_vecContextMenus.push_back(cm);
}

bool ContextMenuManager::UnregisterContextItem(ContextAddonPtr cm)
{
  contextIter it = GetContextItemIterator(cm->GetMsgID());

  if (it != m_vecContextMenus.end())
  {
    m_vecContextMenus.erase(it, m_vecContextMenus.end());
    return true;
  }
  return false;
}

ContextMenuManager::contextIter ContextMenuManager::GetContextItemIterator(const unsigned int ID)
{
  return find_if (m_vecContextMenus.begin(),
                  m_vecContextMenus.end(),
                  std::bind2nd(IDFinder(), ID)
                 );
}

ContextAddonPtr ContextMenuManager::GetContextItemByID(const unsigned int ID)
{
  contextIter it = GetContextItemIterator(ID);
  if (it == m_vecContextMenus.end())
    return ContextAddonPtr();
  return *it;
}

ContextAddonPtr ContextMenuManager::GetContextItemByID(const std::string& strID)
{
  contextIter it = m_vecContextMenus.begin();
  contextIter end = m_vecContextMenus.end();

  for (; it!=end; ++it)
  {
    ContextAddonPtr addon = (*it)->GetChildWithID(strID);
    if (addon)
      return addon;
  }
  return ContextAddonPtr();
}

void ContextMenuManager::AppendVisibleContextItems(const CFileItemPtr item, CContextButtons& list)
{
  contextIter end = m_vecContextMenus.end();
  for (contextIter i=m_vecContextMenus.begin(); i!=end; ++i)
    (*i)->AddIfVisible(item, list);
}

void BaseContextMenuManager::Init()
{
  //Make sure we load all context items on first usage...
  VECADDONS items;
  CAddonMgr::Get().GetAddons(ADDON_CONTEXT_ITEM, items);
  ADDON::VECADDONS::iterator end = items.end();
  for (i = items.begin(); i != end; ++i)
    Register(boost::static_pointer_cast<IContextItem>(*i));
}


bool ContextMenuManager::IDFinder::operator()(const ContextAddonPtr& item, unsigned int id) const
{
  return item->GetMsgID()==id;
}

BaseContextMenuManager *contextManager;

BaseContextMenuManager& BaseContextMenuManager::Get()
{
  if (contextManager==NULL)
  {
    contextManager = new BaseContextMenuManager();
    contextManager->Init();
  }
  return *contextManager;
}

void BaseContextMenuManager::Register(ContextAddonPtr contextAddon)
{
  RegisterContextItem(contextAddon);
}

void BaseContextMenuManager::Unregister(ADDON::ContextAddonPtr contextAddon)
{
  UnregisterContextItem(contextAddon);
}
