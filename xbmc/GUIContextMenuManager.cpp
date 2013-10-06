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
  ADDON::VECADDONS categories;
  ADDON::CAddonMgr::Get().GetAddons(ADDON::ADDON_CONTEXT_CATEGORY, categories);
  ADDON::VECADDONS::iterator i;
  ADDON::VECADDONS::iterator end = categories.end();
  for (i = categories.begin(); i != end; ++i)
    Register(boost::static_pointer_cast<IContextItem>(*i));

  ADDON::VECADDONS items;
  ADDON::CAddonMgr::Get().GetAddons(ADDON::ADDON_CONTEXT_ITEM, items);
  end = items.end();
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
  std::string parent = contextAddon->GetParent();
  if (parent.empty())
    RegisterContextItem(contextAddon);
  else if (parent == MANAGE_CATEGORY_NAME)
    CGUIDialogVideoInfo::manageContextAddonsMgr.RegisterContextItem(contextAddon);
  else
  {
    AddonPtr parentPtr = BaseContextMenuManager::Get().GetContextItemByID(parent);
    if (parentPtr)
    {
      boost::shared_ptr<CContextCategoryAddon> parentCat = boost::static_pointer_cast<CContextCategoryAddon>(parentPtr);
      parentCat->RegisterContextItem(contextAddon);
    }
    else
    { //fallback... add it to the root menu!
      CLog::Log(LOGWARNING, "ADDON: %s - specified parent category '%s' not found. Context item will show up in root.", contextAddon->ID().c_str(), parent.c_str());
      BaseContextMenuManager::Get().RegisterContextItem(contextAddon);
    }
  }
}

void BaseContextMenuManager::Unregister(ADDON::ContextAddonPtr contextAddon)
{
  //always try to unregister from main category, because thats our fallback.
  UnregisterContextItem(contextAddon);

  std::string parent = contextAddon->GetParent();
  if (parent == MANAGE_CATEGORY_NAME)
    CGUIDialogVideoInfo::manageContextAddonsMgr.UnregisterContextItem(contextAddon);
  else
  {
    AddonPtr parentPtr;
    CAddonMgr::Get().GetAddon(parent, parentPtr, ADDON_CONTEXT_CATEGORY);
    if (parentPtr)
    {
      boost::shared_ptr<CContextCategoryAddon> parentCat = boost::static_pointer_cast<CContextCategoryAddon>(parentPtr);
      if (parentCat)
      {
        parentCat->UnregisterContextItem(contextAddon);
      }
    }
  }
}
