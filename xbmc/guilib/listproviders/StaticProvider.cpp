/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "StaticProvider.h"

#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"
#include "utils/XMLUtils.h"

CStaticListProvider::CStaticListProvider(const TiXmlElement* element, int parentID)
  : IListProvider(parentID)
{
  assert(element);

  const TiXmlElement* item = element->FirstChildElement("item");
  while (item)
  {
    if (item->FirstChild())
      m_items.emplace_back(std::make_shared<CGUIStaticItem>(item, parentID));

    item = item->NextSiblingElement("item");
  }

  if (XMLUtils::GetInt(element, "default", m_defaultItem))
  {
    const char* always = element->FirstChildElement("default")->Attribute("always");
    if (always && StringUtils::CompareNoCase(always, "true", 4) == 0)
      m_defaultAlways = true;
  }
}

CStaticListProvider::CStaticListProvider(const std::vector<CGUIStaticItemPtr>& items)
  : IListProvider(0),
    m_items(items)
{
}

CStaticListProvider::CStaticListProvider(const CStaticListProvider& other)
  : IListProvider(other),
    m_defaultItem(other.m_defaultItem),
    m_defaultAlways(other.m_defaultAlways),
    m_updateTime(other.m_updateTime)
{
  for (const auto& item : other.m_items)
  {
    const std::shared_ptr<CGUIListItem> control(item->Clone());
    if (!control)
      continue;

    auto newItem{std::dynamic_pointer_cast<CGUIStaticItem>(control)};
    if (!newItem)
      continue;

    m_items.emplace_back(std::move(newItem));
  }
}

CStaticListProvider::~CStaticListProvider() = default;

std::unique_ptr<IListProvider> CStaticListProvider::Clone()
{
  return std::make_unique<CStaticListProvider>(*this);
}

bool CStaticListProvider::Update(bool forceRefresh)
{
  bool changed = forceRefresh;
  bool updatedProperties{false};

  if (!m_updateTime)
  {
    m_updateTime = CTimeUtils::GetFrameTime();
  }
  else if (CTimeUtils::GetFrameTime() - m_updateTime > 1000)
  {
    m_updateTime = CTimeUtils::GetFrameTime();
    for (const auto& i : m_items)
      i->UpdateProperties(GetParentId());
    updatedProperties = true;
  }

  bool visibilityChanged{false};
  for (const auto& i : m_items)
  {
    if (i->UpdateVisibility(GetParentId()))
    {
      visibilityChanged = true;
      if (!updatedProperties)
        i->UpdateProperties(GetParentId());
    }
  }

  if (visibilityChanged)
    m_updateTime = CTimeUtils::GetFrameTime();

  changed |= visibilityChanged;
  return changed; //! @todo Also returned changed if properties are changed (if so, need to update scroll to letter).
}

void CStaticListProvider::Fetch(std::vector<std::shared_ptr<CGUIListItem>>& items)
{
  items.clear();
  for (const auto& i : m_items)
  {
    if (i->IsVisible())
      items.push_back(i);
  }
}

void CStaticListProvider::SetDefaultItem(int item, bool always)
{
  m_defaultItem = item;
  m_defaultAlways = always;
}

int CStaticListProvider::GetDefaultItem() const
{
  if (m_defaultItem >= 0)
  {
    unsigned int offset = 0;
    for (const auto& i : m_items)
    {
      if (!i->IsVisible())
        continue;

      if (i->GetProgramCount() == m_defaultItem)
        return offset;

      offset++;
    }
  }
  return -1;
}

bool CStaticListProvider::AlwaysFocusDefaultItem() const
{
  return m_defaultAlways;
}

bool CStaticListProvider::OnClick(const std::shared_ptr<CGUIListItem>& item)
{
  const auto* staticItem{static_cast<CGUIStaticItem*>(item.get())};
  return staticItem->GetClickActions().ExecuteActions(0, GetParentId());
}
