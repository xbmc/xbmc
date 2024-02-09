/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MultiProvider.h"

#include "utils/XBMCTinyXML.h"

#include <mutex>

CMultiProvider::CMultiProvider(const TiXmlNode *first, int parentID)
 : IListProvider(parentID)
{
  for (const TiXmlNode *content = first; content; content = content->NextSiblingElement("content"))
  {
    IListProviderPtr sub(IListProvider::CreateSingle(content, parentID));
    if (sub)
      m_providers.push_back(std::move(sub));
  }
}

CMultiProvider::CMultiProvider(const CMultiProvider& other) : IListProvider(other.m_parentID)
{
  for (const auto& provider : other.m_providers)
  {
    std::unique_ptr<IListProvider> newProvider = provider->Clone();
    if (newProvider)
      m_providers.emplace_back(std::move(newProvider));
  }
}

std::unique_ptr<IListProvider> CMultiProvider::Clone()
{
  return std::make_unique<CMultiProvider>(*this);
}

bool CMultiProvider::Update(bool forceRefresh)
{
  bool result = false;
  for (auto& provider : m_providers)
    result |= provider->Update(forceRefresh);
  return result;
}

void CMultiProvider::Fetch(std::vector<std::shared_ptr<CGUIListItem>>& items)
{
  std::unique_lock<CCriticalSection> lock(m_section);
  std::vector<std::shared_ptr<CGUIListItem>> subItems;
  items.clear();
  m_itemMap.clear();
  for (auto const& provider : m_providers)
  {
    provider->Fetch(subItems);
    for (auto& item : subItems)
    {
      auto key = GetItemKey(item);
      m_itemMap[key] = provider.get();
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
    std::unique_lock<CCriticalSection> lock(m_section);
    m_itemMap.clear();
  }

  for (auto const& provider : m_providers)
    provider->Reset();
}

bool CMultiProvider::OnClick(const std::shared_ptr<CGUIListItem>& item)
{
  std::unique_lock<CCriticalSection> lock(m_section);
  auto key = GetItemKey(item);
  auto it = m_itemMap.find(key);
  if (it != m_itemMap.end())
    return it->second->OnClick(item);
  else
    return false;
}

bool CMultiProvider::OnInfo(const std::shared_ptr<CGUIListItem>& item)
{
  std::unique_lock<CCriticalSection> lock(m_section);
  auto key = GetItemKey(item);
  auto it = m_itemMap.find(key);
  if (it != m_itemMap.end())
    return it->second->OnInfo(item);
  else
    return false;
}

bool CMultiProvider::OnContextMenu(const std::shared_ptr<CGUIListItem>& item)
{
  std::unique_lock<CCriticalSection> lock(m_section);
  auto key = GetItemKey(item);
  auto it = m_itemMap.find(key);
  if (it != m_itemMap.end())
    return it->second->OnContextMenu(item);
  else
    return false;
}

CMultiProvider::item_key_type CMultiProvider::GetItemKey(std::shared_ptr<CGUIListItem> const& item)
{
  return reinterpret_cast<item_key_type>(item.get());
}
