/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "StaticProvider.h"
#include "utils/XMLUtils.h"
#include "utils/TimeUtils.h"

CStaticListProvider::CStaticListProvider(const TiXmlElement *element, int parentID)
: IListProvider(parentID),
  m_defaultItem(-1),
  m_defaultAlways(false),
  m_updateTime(0)
{
  assert(element);

  const TiXmlElement *item = element->FirstChildElement("item");
  while (item)
  {
    if (item->FirstChild())
    {
      CGUIStaticItemPtr newItem(new CGUIStaticItem(item, parentID));
      m_items.push_back(newItem);
    }
    item = item->NextSiblingElement("item");
  }

  if (XMLUtils::GetInt(element, "default", m_defaultItem))
  {
    const char *always = element->FirstChildElement("default")->Attribute("always");
    if (always && strnicmp(always, "true", 4) == 0)
      m_defaultAlways = true;
  }
}

CStaticListProvider::CStaticListProvider(const std::vector<CGUIStaticItemPtr> &items)
: IListProvider(0),
  m_defaultItem(-1),
  m_defaultAlways(false),
  m_updateTime(0),
  m_items(items)
{
}

CStaticListProvider::~CStaticListProvider() = default;

bool CStaticListProvider::Update(bool forceRefresh)
{
  bool changed = forceRefresh;
  if (!m_updateTime)
    m_updateTime = CTimeUtils::GetFrameTime();
  else if (CTimeUtils::GetFrameTime() - m_updateTime > 1000)
  {
    m_updateTime = CTimeUtils::GetFrameTime();
    for (std::vector<CGUIStaticItemPtr>::iterator i = m_items.begin(); i != m_items.end(); ++i)
      (*i)->UpdateProperties(m_parentID);
  }
  for (std::vector<CGUIStaticItemPtr>::iterator i = m_items.begin(); i != m_items.end(); ++i)
    changed |= (*i)->UpdateVisibility(m_parentID);
  return changed; //! @todo Also returned changed if properties are changed (if so, need to update scroll to letter).
}

void CStaticListProvider::Fetch(std::vector<CGUIListItemPtr> &items)
{
  items.clear();
  for (std::vector<CGUIStaticItemPtr>::const_iterator i = m_items.begin(); i != m_items.end(); ++i)
  {
    if ((*i)->IsVisible())
      items.push_back(*i);
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
    for (std::vector<CGUIStaticItemPtr>::const_iterator i = m_items.begin(); i != m_items.end(); ++i)
    {
      if ((*i)->IsVisible())
      {
        if ((*i)->m_iprogramCount == m_defaultItem)
          return offset;
        offset++;
      }
    }
  }
  return -1;
}

bool CStaticListProvider::AlwaysFocusDefaultItem() const
{
  return m_defaultAlways;
}

bool CStaticListProvider::OnClick(const CGUIListItemPtr &item)
{
  CGUIStaticItem *staticItem = static_cast<CGUIStaticItem*>(item.get());
  return staticItem->GetClickActions().ExecuteActions(0, m_parentID);
}
