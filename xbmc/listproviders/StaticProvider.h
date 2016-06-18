/*
 *      Copyright (C) 2013 Team XBMC
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

#pragma once

#include <vector>

#include "IListProvider.h"
#include "guilib/GUIStaticItem.h"

class CStaticListProvider : public IListProvider
{
public:
  CStaticListProvider(const TiXmlElement *element, int parentID);
  CStaticListProvider(const std::vector<CGUIStaticItemPtr> &items); // for python
  virtual ~CStaticListProvider();

  virtual bool Update(bool forceRefresh) override;
  virtual void Fetch(std::vector<CGUIListItemPtr> &items) const override;
  virtual bool OnClick(const CGUIListItemPtr &item) override;
  bool OnInfo(const CGUIListItemPtr &item) override { return false; }
  bool OnContextMenu(const CGUIListItemPtr &item) override { return false; }
  virtual void SetDefaultItem(int item, bool always) override;
  virtual int  GetDefaultItem() const override;
  virtual bool AlwaysFocusDefaultItem() const override;
private:
  int                            m_defaultItem;
  bool                           m_defaultAlways;
  unsigned int                   m_updateTime;
  std::vector<CGUIStaticItemPtr> m_items;
};
