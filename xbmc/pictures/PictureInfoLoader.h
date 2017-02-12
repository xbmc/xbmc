#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "BackgroundInfoLoader.h"
#include <string>

class CPictureInfoLoader : public CBackgroundInfoLoader
{
public:
  CPictureInfoLoader();
  virtual ~CPictureInfoLoader();

  void UseCacheOnHD(const std::string& strFileName);
  virtual bool LoadItem(CFileItem* pItem);
  virtual bool LoadItemCached(CFileItem* pItem);
  virtual bool LoadItemLookup(CFileItem* pItem);

protected:
  virtual void OnLoaderStart();
  virtual void OnLoaderFinish();

  CFileItemList* m_mapFileItems;
  unsigned int m_tagReads;
  bool m_loadTags;
};

