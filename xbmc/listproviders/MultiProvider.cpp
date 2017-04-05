/*
 *      Copyright (C) 2013-2017 Team Kodi
 *      http://kodi.tv
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

#include "MultiProvider.h"
#include "utils/XBMCTinyXML.h"
#include "threads/SingleLock.h"

CMultiProvider::CMultiProvider(const TiXmlNode *first, int parentID)
 : IListProvider(parentID)
{
  for (const TiXmlNode *content = first; content; content = content->NextSiblingElement("content"))
  {
    IListProviderPtr sub(IListProvider::CreateSingle(content, parentID));
    if (sub)
      m_providers.push_back(sub);
  }
}

bool CMultiProvider::Update(bool forceRefresh)
{
  bool result = false;
  for (auto& provider : m_providers)
    result |= provider->Update(forceRefresh);
  return result;
}

void CMultiProvider::Fetch(std::vector<CGUIListItemPtr> &items)
{
  CSingleLock lock(m_section);
  std::vector<CGUIListItemPtr> subItems;
  items.clear();
  m_itemMap.clear();
  for (auto const& provider : m_providers)
  {
    provider->Fetch(subItems);
    for (auto& item : subItems)
    {
      auto key = GetItemKey(item);
      m_itemMap[key] = provider;
      items.push_back(item);
    }
    subItems.clear();
  }
}

bool CMultiProvider::IsUpdating() const
{
  bool result = false;
  for (auto const& provider : m_providers)
    result |= provider->IsUpdating();
  return result;
}

void CMultiProvider::Reset()
{
  {
    CSingleLock lock(m_section);
    m_itemMap.clear();
  }

  for (auto const& provider : m_providers)
    provider->Reset();
}

bool CMultiProvider::OnClick(const CGUIListItemPtr &item)
{
  CSingleLock lock(m_section);
  auto key = GetItemKey(item);
  auto it = m_itemMap.find(key);
  if (it != m_itemMap.end())
    return it->second->OnClick(item);
  else
    return false;
}

bool CMultiProvider::OnInfo(const CGUIListItemPtr &item)
{
  CSingleLock lock(m_section);
  auto key = GetItemKey(item);
  auto it = m_itemMap.find(key);
  if (it != m_itemMap.end())
    return it->second->OnInfo(item);
  else
    return false;
}

bool CMultiProvider::OnContextMenu(const CGUIListItemPtr &item)
{
  CSingleLock lock(m_section);
  auto key = GetItemKey(item);
  auto it = m_itemMap.find(key);
  if (it != m_itemMap.end())
    return it->second->OnContextMenu(item);
  else
    return false;
}

CMultiProvider::item_key_type CMultiProvider::GetItemKey(CGUIListItemPtr const &item)
{
  return reinterpret_cast<item_key_type>(item.get());
}
