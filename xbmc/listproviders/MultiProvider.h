/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <vector>
#include <map>
#include "IListProvider.h"
#include "threads/CriticalSection.h"

typedef std::shared_ptr<IListProvider> IListProviderPtr;

/*!
 \ingroup listproviders
 \brief A listprovider that handles multiple individual providers.
 */
class CMultiProvider : public IListProvider
{
public:
  CMultiProvider(const TiXmlNode *first, int parentID);

  bool Update(bool forceRefresh) override;
  void Fetch(std::vector<CGUIListItemPtr> &items) override;
  bool IsUpdating() const override;
  void Reset() override;
  bool OnClick(const CGUIListItemPtr &item) override;
  bool OnInfo(const CGUIListItemPtr &item) override;
  bool OnContextMenu(const CGUIListItemPtr &item) override;

protected:
  typedef size_t item_key_type;
  static item_key_type GetItemKey(CGUIListItemPtr const &item);
  std::vector<IListProviderPtr> m_providers;
  std::map<item_key_type, IListProviderPtr> m_itemMap;
  CCriticalSection m_section; // protects m_itemMap
};
