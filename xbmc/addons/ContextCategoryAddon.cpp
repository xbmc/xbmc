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

#include "ContextCategoryAddon.h"
#include "dialogs/GUIDialogContextMenu.h"

using namespace std;


namespace ADDON
{

CContextCategoryAddon::CContextCategoryAddon(const cp_extension_t *ext)
  : CContextItemAddon(ext)
{ }


CContextCategoryAddon::CContextCategoryAddon(const AddonProps &props)
  : CContextItemAddon(props)
{ }

CContextCategoryAddon::~CContextCategoryAddon()
{ }

bool CContextCategoryAddon::IsVisible(const CFileItemPtr item) const
{
  if (!Enabled())
    return false;

  typedef std::vector<ADDON::ContextAddonPtr>::const_iterator iter;

  iter end = m_vecContextMenus.end();
  for (iter i = m_vecContextMenus.begin(); i != end; ++i)
  {
    if ((*i)->IsVisible(item))
      return true;
  }
  return false;
}

void CContextCategoryAddon::AddIfVisible(const CFileItemPtr item, CContextButtons &visible)
{
  if (m_vecContextMenus.size()==1)
    m_vecContextMenus[0]->AddIfVisible(item, visible);
  else if (IsVisible(item))
    visible.push_back(ToNative());
}

ADDON::ContextAddonPtr CContextCategoryAddon::GetChildWithID(const std::string& strID)
{
  if (ID()==strID)
    return boost::static_pointer_cast<IContextItem>(shared_from_this());
  return GetContextItemByID(strID);
}

bool CContextCategoryAddon::Execute(const CFileItemPtr item)
{
  CContextButtons choices;

  AppendVisibleContextItems(item, choices);
  if(choices.empty())
    return false;

  int button = CGUIDialogContextMenu::ShowAndGetChoice(choices);

  ADDON::ContextAddonPtr context_item = GetContextItemByID(button);
  if (context_item!=0)
    return context_item->Execute(item); //execute our context item logic
  return false;
}


}
