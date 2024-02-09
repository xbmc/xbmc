/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IListProvider.h"
#include "guilib/GUIStaticItem.h"

#include <vector>

class CStaticListProvider : public IListProvider
{
public:
  CStaticListProvider(const TiXmlElement *element, int parentID);
  explicit CStaticListProvider(const std::vector<CGUIStaticItemPtr> &items); // for python
  explicit CStaticListProvider(const CStaticListProvider& other);
  ~CStaticListProvider() override;

  // Implementation of IListProvider
  std::unique_ptr<IListProvider> Clone() override;
  bool Update(bool forceRefresh) override;
  void Fetch(std::vector<std::shared_ptr<CGUIListItem>>& items) override;
  bool OnClick(const std::shared_ptr<CGUIListItem>& item) override;
  bool OnInfo(const std::shared_ptr<CGUIListItem>& item) override { return false; }
  bool OnContextMenu(const std::shared_ptr<CGUIListItem>& item) override { return false; }
  void SetDefaultItem(int item, bool always) override;
  int GetDefaultItem() const override;
  bool AlwaysFocusDefaultItem() const override;
private:
  int m_defaultItem;
  bool m_defaultAlways;
  unsigned int m_updateTime;
  std::vector<CGUIStaticItemPtr> m_items;
};
